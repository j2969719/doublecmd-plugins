/*Лицензия:*/
/*Эта программа является свободным программным обеспечением*/
/*Вы можете распространять и/или изменять*/
/*Его в соответствии с условиями GNU General Public License, опубликованной*/
/*Free Software Foundation, версии 2, либо (По вашему выбору) любой более поздней версии.*/
/*Эта программа распространяется в надежде, что она будет полезна,*/
/*Но БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ*/

// https://doublecmd.sourceforge.io/forum/viewtopic.php?f=8&t=2727

#define _GNU_SOURCE

#include <gtk/gtk.h>
#include <string.h>
#include <enca.h>
#include <dlfcn.h>
#include "wlxplugin.h"

#ifdef WLXSRCVW   
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourceiter.h>
#include <gtksourceview/gtksourcebuffer.h>
#endif

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

#define DC_USERDIR "/.local/share/doublecmd/plugins/wlx/" PLUGDIR
#define MC_EXTD "/.config/mc/ext.d:/usr/lib/mc/ext.d:/usr/libexec/mc/ext.d"
#define MAX_REDIRECTS 42
#define MAX_SECOND_EXT 5

char lang[3];
GKeyFile *cfg = NULL;
gchar *new_path = NULL;
gchar *new_path_mc = NULL;
gchar *cfg_path = NULL;
gchar *custom_font = NULL;
char plug_path[PATH_MAX];
gboolean is_plug_init = FALSE;
gboolean is_cursor = FALSE;
gboolean is_use_enca_global = FALSE;
gint pixels_above, pixels_below;


static char* get_file_ext(char *filename, gboolean is_double_ext)
{
	char *result = NULL;
	char *pos = strrchr(filename, '/');

	if (pos)
	{
		char *basename = pos + 1;
		char *dot = strrchr(basename, '.');

		if (dot)
		{
			if (is_double_ext)
			{
				int i = 0;

				for (char *p = dot - 1; p > basename; p--)
				{
					if (i++ > MAX_SECOND_EXT)
						break;

					if (*p == '.')
					{
						dot = p;
						break;
					}
				}
    			}

			char *ext = dot + 1;

			if (basename != dot && *ext != '\0')
				result = g_ascii_strdown(ext, -1);
		}
	}

	return result;
}

static gchar *get_mimetype(char *filename)
{
	GFile *gfile = g_file_new_for_path(filename);

	if (!gfile)
		return NULL;

	GFileInfo *fileinfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL, NULL);

	if (!fileinfo)
	{
		g_object_unref(gfile);
		return NULL;
	}

	gchar *result = g_strdup(g_file_info_get_content_type(fileinfo));
	g_object_unref(fileinfo);
	g_object_unref(gfile);

	return result;
}

static gchar *resolve_redirect(gchar *group)
{
	if (!group)
		return NULL;

	gint redirect_count = 0;
	gchar *redirect = NULL;

	while ((redirect = g_key_file_get_string(cfg, group, "redirect", NULL)) != NULL)
	{
		if (redirect_count++ > MAX_REDIRECTS)
			break;

		g_free(group);
		group = redirect;
	}

	if (redirect)
	{
		g_free(group);
		g_free(redirect);
		return NULL;
	}

	if (!group)
		return NULL;

	if (!g_key_file_has_key(cfg, group, "mc_script", NULL) &&
	                !g_key_file_has_key(cfg, group, "command", NULL))
	{
		g_free(group);
		return NULL;
	}

	return group;
}

static gchar* get_group(char *filename)
{
	gchar *result = resolve_redirect(get_file_ext(filename, FALSE));

	if (!result)
		result = resolve_redirect(get_mimetype(filename));

	return result;
}

static gchar* get_command(char *group, char *ext, gboolean *is_mc_script)
{
	if (!group)
		return NULL;

	gchar *result = g_key_file_get_string(cfg, group, "command", NULL);

	if (!result)
	{
		gchar *script = g_key_file_get_string(cfg, group, "mc_script", NULL);

		if (script)
		{
			*is_mc_script = TRUE;
			result = g_strdup_printf("%s view %s", script, ext ? ext : "");
			g_free(script);
		}
	}

	return result;
}

static gchar* get_output(char *filename)
{
	gchar *result = NULL;

	gchar *group = get_group(filename);
	gchar *ext = NULL;

	if (g_key_file_has_key(cfg, group, "mc_script", NULL))
	{
		ext = g_key_file_get_string(cfg, group, "mc_filetype", NULL);

		if (!ext)
			ext = get_file_ext(filename, TRUE);
	}

	gboolean is_mc_script = FALSE;
	gchar *command = get_command(group, ext, &is_mc_script);
	g_free(ext);

	if (!command)
		command = g_key_file_get_string(cfg, ".", "fallback_command", NULL);

	if (command)
	{
		char *argv[] = {"sh", "-c", command, NULL};
		gchar **envp = g_environ_setenv(g_get_environ(), "PLUG_DIR", plug_path, TRUE);
		if (!is_mc_script)
		{
			envp = g_environ_setenv(envp, "FILE", filename, TRUE);
			envp = g_environ_setenv(envp, "file", filename, TRUE);
			envp = g_environ_setenv(envp, "PATH", new_path, TRUE);
		}
		else
		{
			envp = g_environ_setenv(envp, "MC_EXT_FILENAME", filename, TRUE);
			envp = g_environ_setenv(envp, "PATH", new_path_mc, TRUE);
		}

		envp = g_environ_setenv(envp, "PLUG_CFGFILE", cfg_path, TRUE);

		g_spawn_sync(NULL, argv, envp, G_SPAWN_SEARCH_PATH_FROM_ENVP, NULL, NULL, &result, NULL, NULL, NULL);
		g_strfreev(envp);
		g_free(command);
	}

	gboolean is_use_enca = (g_key_file_get_boolean(cfg, group, "use_enca", NULL) || is_use_enca_global);

	if (result)
	{
		size_t len = strlen(result);

		if (len < 3)
		{
			g_free(result);
			result = NULL;
		}
		else if (is_use_enca)
		{
			EncaAnalyser analyser = enca_analyser_alloc(lang);

			if (analyser)
			{
				enca_set_threshold(analyser, 1.38);
				enca_set_multibyte(analyser, 1);
				enca_set_ambiguity(analyser, 1);
				enca_set_garbage_test(analyser, 1);
				enca_set_filtering(analyser, 0);

				EncaEncoding encoding = enca_analyse(analyser, (unsigned char*)result, len);

				if (encoding.charset != ENCA_CS_UNKNOWN)
				{
					const char *enc = enca_charset_name(encoding.charset, ENCA_NAME_STYLE_ICONV);

					if (enc && strcmp(enc, "UTF-8") != 0 && strcmp(enc, "ASCII") != 0)
					{
						gchar *converted = g_convert_with_fallback(result, -1, "UTF-8", enc, NULL, NULL, NULL, NULL);
						g_free(result);
						result = converted;
					}
				}

				enca_analyser_free(analyser);
			}
		}

		if (!g_utf8_validate(result, -1, NULL))
		{
			g_free(result);
			result = NULL;
		}
	}

	g_free(group);
	return result;
}

static void set_output(GtkTextBuffer *buffer, gchar *output, int flags)
{
	gtk_text_buffer_set_text(buffer, output, -1);
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	gtk_text_buffer_apply_tag_by_name(buffer, "custom_font", &start, &end);
} 

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	gchar *output = get_output(FileToLoad);

	if (!output)
		return NULL;

#ifndef GTK3PLUG
	GtkWidget *plug_box = gtk_vbox_new(FALSE, 5);
#else
	GtkWidget *plug_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
#endif
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), plug_box);
	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(plug_box), scroll, TRUE, TRUE, 5);

	GtkTextBuffer *buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_create_tag(buffer, "custom_font", "font", custom_font, NULL);
	set_output(GTK_TEXT_BUFFER(buffer), output, ShowFlags);

#ifndef WLXSRCVW 
	GtkWidget *view = gtk_text_view_new_with_buffer(buffer);
#else
	GtkWidget *view = gtk_source_view_new_with_buffer(GTK_SOURCE_BUFFER(buffer));
#endif
	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(view));

	gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), is_cursor);
	gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(view), pixels_above);
	gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(view), pixels_below);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), (ShowFlags & lcp_wraptext) ? GTK_WRAP_WORD_CHAR : GTK_WRAP_NONE);

	g_object_set_data(G_OBJECT(plug_box), "view", view);
	//g_object_set_data_full(G_OBJECT(plug_box), "output", output, (GDestroyNotify)g_free);
	g_free(output);
	g_object_set_data_full(G_OBJECT(plug_box), "txtbuf", buffer, (GDestroyNotify)g_object_unref);

	gtk_widget_show_all(plug_box);
	return plug_box;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	gchar *output = get_output(FileToLoad);

	if (!output)
		return LISTPLUGIN_ERROR;

	GtkTextBuffer *buffer = (GtkTextBuffer*)g_object_get_data(G_OBJECT(PluginWin), "txtbuf");
	set_output(buffer, output, ShowFlags);

	//g_object_set_data(G_OBJECT(PluginWin), "output", NULL);
	//g_object_set_data_full(G_OBJECT(PluginWin), "output", output, (GDestroyNotify)g_free);
	g_free(output);

	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	GtkTextIter p;
	GtkTextBuffer *buffer = (GtkTextBuffer*)g_object_get_data(G_OBJECT(ListWin), "txtbuf");

	switch (Command)
	{
	case lc_copy :
		gtk_text_buffer_copy_clipboard(buffer, gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
		break;

	case lc_selectall :
		gtk_text_buffer_get_start_iter(buffer, &p);
		gtk_text_buffer_place_cursor(buffer, &p);
		gtk_text_buffer_get_end_iter(buffer, &p);
		gtk_text_buffer_move_mark_by_name(buffer, "selection_bound", &p);
		break;

	case lc_newparams :
	{
		GtkWidget *view = (GtkWidget*)g_object_get_data(G_OBJECT(ListWin), "view");
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), (Parameter & lcp_wraptext) ? GTK_WRAP_WORD_CHAR : GTK_WRAP_NONE);
		break;
	}
	default :
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	GtkTextIter iter, start, end;
	gboolean is_found;

	GtkTextBuffer *buffer = (GtkTextBuffer*)g_object_get_data(G_OBJECT(ListWin), "txtbuf");
	GtkTextMark *last_pos = gtk_text_buffer_get_mark(buffer, "last_pos");

	if (last_pos == NULL || SearchParameter & lcs_findfirst)
	{
		if (SearchParameter & lcs_backwards)
			gtk_text_buffer_get_end_iter(buffer, &iter);
		else
			gtk_text_buffer_get_start_iter(buffer, &iter);
	}
	else
		gtk_text_buffer_get_iter_at_mark(buffer, &iter, last_pos);

#ifndef WLXSRCVW
	GtkTextSearchFlags flags = GTK_TEXT_SEARCH_TEXT_ONLY;

#ifdef GTK3PLUG
	if (!(SearchParameter & lcs_matchcase))
		flags |= GTK_TEXT_SEARCH_CASE_INSENSITIVE;
#endif

#else
	GtkSourceSearchFlags flags = GTK_SOURCE_SEARCH_TEXT_ONLY;

	if (!(SearchParameter & lcs_matchcase))
		flags |= GTK_SOURCE_SEARCH_CASE_INSENSITIVE;
#endif

	if (SearchParameter & lcs_backwards)
		is_found = gtk_text_iter_backward_search(&iter, SearchString, flags, &end, &start, NULL);
	else
		is_found = gtk_text_iter_forward_search(&iter, SearchString, flags, &start, &end, NULL);

	if (is_found)
	{
		gtk_text_buffer_select_range(buffer, &start, &end);
		gtk_text_buffer_create_mark(buffer, "last_pos", &end, FALSE);
		GtkWidget *view = (GtkWidget*)g_object_get_data(G_OBJECT(ListWin), "view");
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(view),
		                                   gtk_text_buffer_get_mark(buffer, "last_pos"));

	}
	else
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ListWin))),
		                    GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		                    _("\"%s\" not found!"), SearchString);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

static void wlxplug_atexit(void)
{
	g_key_file_free(cfg);
	g_free(custom_font);
	g_free(new_path_mc);
	g_free(new_path);
	g_free(cfg_path);
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	if (is_plug_init)
		return;

	Dl_info dlinfo;

	memset(&dlinfo, 0, sizeof(dlinfo));
	atexit(wlxplug_atexit);
	cfg = g_key_file_new();
	is_plug_init = TRUE;

	if (dladdr(&is_plug_init, &dlinfo) != 0)
	{
		g_strlcpy(plug_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(plug_path, '/');

		if (pos)
		{
			*pos = '\0';
			new_path = g_strdup_printf("%s" DC_USERDIR "/scripts:%s/scripts:%s", g_getenv("HOME"), plug_path, g_getenv("PATH"));
			new_path_mc = g_strdup_printf("%s" MC_EXTD ":%s", g_getenv("HOME"), g_getenv("PATH"));
		}

		gchar *user_path = g_strdup_printf("%s" DC_USERDIR, g_getenv("HOME"));
		const gchar *search_dirs[] = {user_path, plug_path, NULL};
		g_key_file_load_from_dirs(cfg, "settings.ini", search_dirs, &cfg_path, G_KEY_FILE_NONE, NULL);
		g_free(user_path);
		custom_font = g_key_file_get_string(cfg, ".", "font", NULL);
		pixels_above = g_key_file_get_integer(cfg, ".", "pixels_above_lines", NULL);
		pixels_below = g_key_file_get_integer(cfg, ".", "pixels_below_lines", NULL);
		is_cursor = g_key_file_get_boolean(cfg, ".", "show_cursor", NULL);
		is_use_enca_global = g_key_file_get_boolean(cfg, ".", "use_enca", NULL);

		snprintf(lang, 3, "%s", setlocale(LC_ALL, ""));
		gchar *lang_dir = g_strdup_printf("%s/langs", plug_path);
		bindtextdomain(GETTEXT_PACKAGE, lang_dir);
		g_free(lang_dir);
		textdomain(GETTEXT_PACKAGE);
	}
}
