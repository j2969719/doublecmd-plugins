#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <vte/vte.h>
#include "wlxplugin.h"

#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

#define DC_USERDIR "/.local/share/doublecmd/plugins/wlx/" PLUGDIR
#define MAX_REDIRECTS 42

static GKeyFile *cfg = NULL;
static gchar *new_path = NULL;
static gchar *cfg_path = NULL;
static char plug_path[PATH_MAX];
static gboolean is_plug_init = FALSE;

static char* get_file_ext(char *filename)
{
	char *result = NULL;
	char *pos = strrchr(filename, '/');

	if (pos)
	{
		char *dot = strrchr(pos + 1, '.');

		if (dot)
		{
			char *ext = dot + 1;

			if (pos + 1 != dot && *ext != '\0')
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

	if (!g_key_file_has_key(cfg, group, "command", NULL))
	{
		g_free(group);
		return NULL;
	}

	return group;
}

static gchar* get_group(char *filename)
{
	gchar *result = resolve_redirect(get_file_ext(filename));

	if (!result)
		result = resolve_redirect(get_mimetype(filename));

	return result;
}

static void launch_command(VteTerminal *terminal, gchar *command, char *filename)
{
	char *argv[] = {"sh", "-c", command, NULL};
	GSpawnFlags flags = G_SPAWN_SEARCH_PATH_FROM_ENVP;
	gchar **envp = g_environ_setenv(g_get_environ(), "PATH", new_path, TRUE);
	envp = g_environ_setenv(envp, "FILE", filename, TRUE);
	envp = g_environ_setenv(envp, "PLUG_DIR", plug_path, TRUE);
	vte_terminal_spawn_async(terminal, VTE_PTY_DEFAULT, NULL, argv, envp, flags, NULL, NULL,  NULL, -1, NULL, NULL, NULL);
	g_free(command);
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	gchar *group = get_group(FileToLoad);

	if (!group)
		return NULL;

	gchar *command = g_key_file_get_string(cfg, group, "command", NULL);

	if (!command)
	{
		g_free(group);
		return NULL;
	}

	GtkWidget *vte = vte_terminal_new();
	VteTerminal *terminal = VTE_TERMINAL(vte);
	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), scroll);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	vte_terminal_set_scrollback_lines(terminal, -1);
	vte_terminal_set_size(terminal, 80, 60);

	gchar *string = g_key_file_get_string(cfg, ".", "Font", NULL);

	if (string)
	{
		PangoFontDescription *descr = pango_font_description_from_string(string);
		vte_terminal_set_font(terminal, descr);
		pango_font_description_free(descr);
		g_free(string);
	}

	GdkRGBA color;
	string = g_key_file_get_string(cfg, ".", "BGColor", NULL);

	if (string)
	{
		gdk_rgba_parse(&color, string);
		vte_terminal_set_color_background(terminal, &color);
		g_free(string);
	}

	string = g_key_file_get_string(cfg, ".", "FGColor", NULL);

	if (string)
	{
		gdk_rgba_parse(&color, string);
		vte_terminal_set_color_foreground(terminal, &color);
		g_free(string);
	}

	gtk_container_add(GTK_CONTAINER(scroll), vte);
	gtk_widget_show_all(scroll);
	launch_command(terminal, command, FileToLoad);

	g_object_set_data(G_OBJECT(scroll), "terminal", terminal); 
	g_free(group);

	return scroll;
}

int DCPCALL ListLoadNext(HWND ParentWin,HWND PluginWin,char* FileToLoad,int ShowFlags)
{
	gchar *group = get_group(FileToLoad);

	if (!group)
		return LISTPLUGIN_ERROR;

	gchar *command = g_key_file_get_string(cfg, group, "command", NULL);

	if (!command)
	{
		g_free(group);
		return LISTPLUGIN_ERROR;
	}

	VteTerminal *terminal = (VteTerminal*)g_object_get_data(G_OBJECT(PluginWin), "terminal");
	launch_command(terminal, command, FileToLoad);
	g_free(group);
	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	VteTerminal *terminal = (VteTerminal*)g_object_get_data(G_OBJECT(ListWin), "terminal");

	switch (Command)
	{
	case lc_copy :
		vte_terminal_copy_clipboard_format(terminal, VTE_FORMAT_TEXT);
		break;

	case lc_selectall :
		vte_terminal_select_all(terminal);
		break;

	default :
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	VteTerminal *terminal = (VteTerminal*)g_object_get_data(G_OBJECT(ListWin), "terminal");
	gboolean is_found = FALSE;
	int flags = 0;

	if (!(SearchParameter & lcs_matchcase))
		flags = 8;

	VteRegex *regex = vte_regex_new_for_search(SearchString, -1, flags, NULL);
	vte_terminal_search_set_regex(terminal, regex, 0);


	if (SearchParameter & lcs_backwards)
		is_found = vte_terminal_search_find_previous(terminal);
	else
		is_found = vte_terminal_search_find_next(terminal);

	if (!is_found)
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ListWin))),
		                    GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		                    _("\"%s\" not found!"), SearchString);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}

	return LISTPLUGIN_OK;
}

static void wlxplug_atexit(void)
{
	g_key_file_free(cfg);
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
		}

		gchar *user_path = g_strdup_printf("%s" DC_USERDIR, g_getenv("HOME"));
		const gchar *search_dirs[] = {user_path, plug_path, NULL};
		g_key_file_load_from_dirs(cfg, "settings.ini", search_dirs, &cfg_path, G_KEY_FILE_NONE, NULL);
		g_free(user_path);

		if (pos)
		{
			setlocale(LC_ALL, "");
			strcpy(pos, "/langs");
			bindtextdomain(GETTEXT_PACKAGE, plug_path);
			textdomain(GETTEXT_PACKAGE);
		}
	}
}
