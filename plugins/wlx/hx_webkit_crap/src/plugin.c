#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <dlfcn.h>
#include <string.h>
#include "wlxplugin.h"

#define TEMPLATE "_dc-hx.XXXXXX"
#define LIBS_DIR "/redist/"
#define EXE_NAME "exsimple"
#define CFG_NAME "default.cfg"

gchar gPlugInit = FALSE;
gchar gDataPath[PATH_MAX] = "";
gchar gWorkdir[PATH_MAX] = "";

static void remove_target(gchar *path)
{
	if (path)
	{
		gchar *quoted = g_shell_quote(path);
		g_free(path);
		gchar *command = g_strdup_printf("rm -r %s", quoted);
		g_free(quoted);
		system(command);
		g_free(command);
	}
}

static gchar* export_html(gchar *tmpdir, gchar *filename)
{
	gchar *result = NULL;

	gchar *output = g_strdup_printf("%s/output.html", tmpdir);

	char *argv[] = {"sh", "-c", "./" EXE_NAME " \"$FILE\" \"$HTML\" \"$OIT_CFG_NAME\"", NULL};
	GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
	gchar **envp = g_environ_setenv(g_get_environ(), "HTML", output, TRUE);
	envp = g_environ_setenv(envp, "FILE", filename, TRUE);
	envp = g_environ_setenv(envp, "OIT_CFG_NAME", CFG_NAME, FALSE);

	if (gDataPath[0] != '\0')
		envp = g_environ_setenv(envp, "OIT_DATA_PATH", gDataPath, FALSE);

	gboolean is_launched = g_spawn_sync(gWorkdir, argv, envp, flags, NULL, NULL, NULL, NULL, NULL, NULL);
	g_strfreev(envp);

	if (!is_launched || !g_file_test(output, G_FILE_TEST_EXISTS))
		remove_target(tmpdir);
	else
		result = g_filename_to_uri(output, NULL, NULL);

	g_free(output);

	return result;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *scroll;
	GtkWidget *webView;

	gchar *tmpdir = g_dir_make_tmp(TEMPLATE, NULL);
	gchar *fileUri = export_html(tmpdir, FileToLoad);

	if (!fileUri)
		return NULL;

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), scroll);
	g_object_set_data(G_OBJECT(scroll), "tmpdir", tmpdir);
	webView = webkit_web_view_new();

	WebKitWebSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webView));
	g_object_set(G_OBJECT(settings), "enable-scripts", FALSE, NULL);
	webkit_web_view_set_settings(WEBKIT_WEB_VIEW(webView), settings);

	// https://doublecmd.sourceforge.io/forum/viewtopic.php?f=8&t=4106&start=72#p22156
	WebKitFaviconDatabase *database = webkit_get_favicon_database();
	webkit_favicon_database_set_path(database, NULL);

	webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webView), fileUri);
	gtk_container_add(GTK_CONTAINER(scroll), webView);
	g_object_set_data(G_OBJECT(scroll), "webkit", WEBKIT_WEB_VIEW(webView));
	gtk_widget_show_all(scroll);

	return scroll;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	gchar *tmpdir = g_object_get_data(G_OBJECT(PluginWin), "tmpdir");

	if (tmpdir)
		remove_target(tmpdir);

	tmpdir = g_dir_make_tmp(TEMPLATE, NULL);
	g_object_set_data(G_OBJECT(PluginWin), "tmpdir", tmpdir);
	gchar *fileUri = export_html(tmpdir, FileToLoad);

	if (!fileUri)
	{
		g_object_set_data(G_OBJECT(PluginWin), "tmpdir", NULL);
		return LISTPLUGIN_ERROR;
	}

	WebKitWebView *webView = (WebKitWebView*)g_object_get_data(G_OBJECT(PluginWin), "webkit");
	webkit_web_view_load_uri(webView, fileUri);

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gchar *tmpdir = g_object_get_data(G_OBJECT(ListWin), "tmpdir");
	gtk_widget_destroy(GTK_WIDGET(ListWin));
	remove_target(tmpdir);
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	gboolean ss_case = FALSE;
	gboolean ss_forward = TRUE;
	WebKitWebView *webView = (WebKitWebView*)g_object_get_data(G_OBJECT(ListWin), "webkit");

	if (SearchParameter & lcs_matchcase)
		ss_case = TRUE;

	if (SearchParameter & lcs_backwards)
		ss_forward = FALSE;

	webkit_web_view_search_text(webView, SearchString, ss_case, ss_forward, TRUE);

	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	WebKitWebView *webView = (WebKitWebView*)g_object_get_data(G_OBJECT(ListWin), "webkit");

	switch (Command)
	{
	case lc_copy :
		webkit_web_view_copy_clipboard(webView);
		break;

	case lc_selectall :
		webkit_web_view_select_all(webView);
		break;

	default :
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	if (gPlugInit)
		return;

	GKeyFile *cfg;
	char cfg_path[PATH_MAX];

	g_strlcpy(cfg_path, dps->DefaultIniName, PATH_MAX);

	char *pos = strrchr(cfg_path, '/');

	if (pos)
		strcpy(pos + 1, "j2969719.ini");

	cfg = g_key_file_new();
	g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_NONE, NULL);
	gchar *string = g_key_file_get_string(cfg, PLUGNAME, "OIT_DATA_PATH", NULL);

	if (string)
	{
		g_strlcpy(gDataPath, string, PATH_MAX);
		g_free(string);
	}

	g_key_file_free(cfg);

	Dl_info dlinfo;
	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(gWorkdir, &dlinfo) != 0)
	{
		g_strlcpy(gWorkdir, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(gWorkdir, '/');

		if (pos)
			strcpy(pos, LIBS_DIR);
	}

	gPlugInit = TRUE;
}

int DCPCALL ListPrint(HWND ListWin, char* FileToPrint, char* DefPrinter, int PrintFlags, RECT* Margins)
{
	WebKitWebView *webView = (WebKitWebView*)g_object_get_data(G_OBJECT(ListWin), "webkit");
	WebKitWebFrame *frame = webkit_web_view_get_main_frame(webView);
	webkit_web_frame_print(frame);

	return LISTPLUGIN_OK;
}
