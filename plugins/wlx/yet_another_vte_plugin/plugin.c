#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <vte/vte.h>
#include <glib.h>
#include <dlfcn.h>
#include <limits.h>
#include <string.h>
#include "wlxplugin.h"

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

#define _plgname "yavteplg.wlx"

static char cfg_path[PATH_MAX];
const char* cfg_file = "settings.ini";

static GtkWidget *getFirstChild(GtkWidget *w)
{
	GList *list = gtk_container_get_children(GTK_CONTAINER(w));
	GtkWidget *result = GTK_WIDGET(list->data);
	g_list_free(list);
	return result;
}

gchar *get_file_ext(const gchar *Filename)
{
	if (g_file_test(Filename, G_FILE_TEST_IS_DIR))
		return NULL;

	gchar *basename, *result, *tmpval;

	basename = g_path_get_basename(Filename);
	result = g_strrstr(basename, ".");

	if (result)
	{
		if (g_strcmp0(result, basename) != 0)
		{
			tmpval = g_strdup_printf("%s", result + 1);
			result = g_ascii_strdown(tmpval, -1);
			g_free(tmpval);
		}
		else
			result = NULL;
	}

	g_free(basename);

	return result;
}

const gchar *get_mime_type(const gchar *Filename)
{
	GFile *gfile = g_file_new_for_path(Filename);

	if (!gfile)
		return NULL;

	GFileInfo *fileinfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL, NULL);

	if (!fileinfo)
		return NULL;

	const gchar *content_type = g_strdup(g_file_info_get_content_type(fileinfo));
	g_object_unref(fileinfo);
	g_object_unref(gfile);

	return content_type;
}

gchar *str_replace(gchar *text, gchar *str, gchar *repl)
{
	gchar **split = g_strsplit(text, str, -1);
	gchar *result = g_strjoinv(repl, split);
	g_strfreev(split);
	return result;
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	GKeyFile *cfg;
	GError *err = NULL;
	const gchar *file_ext, *mime_type;
	gchar *font, *bgimage, *bgcolor, *cmdstr;
	gchar *frmtcmd = NULL;
	gchar **command;
	gdouble saturation;
	gint argcp;

	mime_type = get_mime_type(FileToLoad);
	file_ext = get_file_ext(FileToLoad);

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
		g_print("%s (%s): %s\n", _plgname, cfg_path, (err)->message);
	else
	{
		font = g_key_file_get_string(cfg, "VTE", "Font", NULL);
		bgimage = g_key_file_get_string(cfg, "VTE", "BGImage", NULL);
		bgcolor = g_key_file_get_string(cfg, "VTE", "BGTintColor", NULL);
		saturation = g_key_file_get_double(cfg, "VTE", "BGSaturation", NULL);

		if (file_ext)
			frmtcmd = g_key_file_get_string(cfg, file_ext, "Command", NULL);

		if (mime_type && ((!file_ext) || (!frmtcmd)))
			frmtcmd = g_key_file_get_string(cfg, mime_type, "Command", NULL);

	}

	g_key_file_free(cfg);

	if (!frmtcmd)
		return NULL;

	cmdstr = str_replace(frmtcmd, "$FILE", g_shell_quote(FileToLoad));
	g_shell_parse_argv(cmdstr, &argcp, &command, NULL);

	if (err)
		g_error_free(err);

	if (!command)
		return NULL;


	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *vte;
	VteTerminal *terminal;
	GdkColor color;

	gFix = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);

	vte = vte_terminal_new();
	terminal = VTE_TERMINAL(vte);
	scroll = gtk_scrolled_window_new(NULL, terminal->adjustment);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	if (!vte_terminal_fork_command_full(terminal, VTE_PTY_DEFAULT, NULL, command, NULL, 0, NULL, NULL, NULL, NULL))
		return NULL;

	vte_terminal_set_scrollback_lines(terminal, -1);
	vte_terminal_set_size(terminal, 80, 60);

	if (font)
		vte_terminal_set_font_from_string(terminal, font);

	if (bgimage)
	{
		vte_terminal_set_background_image_file(terminal, bgimage);

		if (bgcolor)
		{
			gdk_color_parse(bgcolor, &color);
			vte_terminal_set_background_tint_color(terminal, &color);
		}

		if (saturation >= 0)
			vte_terminal_set_background_saturation(terminal, saturation);

		vte_terminal_set_background_transparent(terminal, FALSE);
	}

	gtk_box_pack_start(GTK_BOX(gFix), scroll, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(scroll), vte);
	gtk_widget_show_all(gFix);

	return gFix;
}

int DCPCALL ListLoadNext(HWND ParentWin,HWND PluginWin,char* FileToLoad,int ShowFlags)
{
	GKeyFile *cfg;
	GError *err = NULL;
	const gchar *file_ext, *mime_type;
	gchar *cmdstr;
	gchar *frmtcmd = NULL;
	gchar **command;
	gint argcp;

	mime_type = get_mime_type(FileToLoad);
	file_ext = get_file_ext(FileToLoad);

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
		g_print("%s (%s): %s\n", _plgname, cfg_path, (err)->message);
	else
	{
		if (file_ext)
			frmtcmd = g_key_file_get_string(cfg, file_ext, "Command", NULL);

		if (mime_type && ((!file_ext) || (!frmtcmd)))
			frmtcmd = g_key_file_get_string(cfg, mime_type, "Command", NULL);

	}

	g_key_file_free(cfg);

	if (!frmtcmd)
		return LISTPLUGIN_ERROR;

	cmdstr = str_replace(frmtcmd, "$FILE", g_shell_quote(FileToLoad));
	g_shell_parse_argv(cmdstr, &argcp, &command, NULL);

	if (err)
		g_error_free(err);

	if (!command)
		return LISTPLUGIN_ERROR;

	if (!vte_terminal_fork_command_full(VTE_TERMINAL(getFirstChild(getFirstChild(PluginWin))), VTE_PTY_DEFAULT, NULL, command, NULL, 0, NULL, NULL, NULL, NULL))
		return LISTPLUGIN_ERROR;

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, cfg_file);

		setlocale (LC_ALL, "");
		bindtextdomain(GETTEXT_PACKAGE, g_strdup_printf("%s/langs", g_path_get_dirname(dlinfo.dli_fname)));
		textdomain(GETTEXT_PACKAGE);
	}
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	switch (Command)
	{
	case lc_copy :
		vte_terminal_copy_clipboard(VTE_TERMINAL(getFirstChild(getFirstChild(ListWin))));
		break;

	case lc_selectall :
		vte_terminal_select_all(VTE_TERMINAL(getFirstChild(getFirstChild(ListWin))));
		break;

	default :
		return LISTPLUGIN_ERROR;
	}
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	gboolean found;
	GRegexCompileFlags flags;

	if (!(SearchParameter & lcs_matchcase))
		flags |= G_REGEX_CASELESS;

	GRegex *regex =  g_regex_new(SearchString, flags, 0, NULL);
	VteTerminal *terminal = VTE_TERMINAL(getFirstChild(getFirstChild(ListWin)));
	vte_terminal_search_set_gregex(terminal, regex);


	if (SearchParameter & lcs_backwards)
		found = vte_terminal_search_find_previous(terminal);
	else
		found = vte_terminal_search_find_next(terminal);

	if (!found)
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ListWin))),
		                    GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		                    _("\"%s\" not found!"), SearchString);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
}
