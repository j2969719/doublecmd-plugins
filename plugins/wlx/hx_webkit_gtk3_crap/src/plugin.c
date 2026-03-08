#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <dlfcn.h>
#include <string.h>
#include "wlxplugin.h"

#define TEMPLATE "_dc-hx.XXXXXX"
#define EXE_NAME "/redist/exsimple"
#define CFG_NAME "/redist/default.cfg"

gchar gConfigPath[PATH_MAX] = "";
gchar gExecutablePath[PATH_MAX] = "";

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

	gchar *output = g_strdup_printf("%s/output.html", tmpdir, filename);
	gchar *quoted_output = g_shell_quote(output);
	gchar *quoted_file = g_shell_quote(filename);
	gchar *command = g_strdup_printf("%s %s %s %s", gExecutablePath, quoted_file,
	                                 quoted_output, gConfigPath);
	g_free(quoted_output);
	g_free(quoted_file);

	if (system(command) != 0 || !g_file_test(output, G_FILE_TEST_EXISTS))
		remove_target(tmpdir);
	else
		result = g_filename_to_uri(output, NULL, NULL);

	g_free(command);
	g_free(output);

	return result;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	gchar *tmpdir = g_dir_make_tmp(TEMPLATE, NULL);
	gchar *fileUri = export_html(tmpdir, FileToLoad);

	if (!fileUri)
		return NULL;

	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), scroll);
	WebKitSettings *settings = webkit_settings_new();
	webkit_settings_set_default_charset(settings, "utf-8");
	GtkWidget *webView = webkit_web_view_new_with_settings(settings); 
	webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webView), fileUri);
	g_free(fileUri);
	gtk_container_add(GTK_CONTAINER(scroll), webView);
	g_object_set_data(G_OBJECT(scroll), "tmpdir", tmpdir);
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
	g_free(fileUri);

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
	guint32 options = WEBKIT_FIND_OPTIONS_WRAP_AROUND;
	WebKitWebView *webView = (WebKitWebView*)g_object_get_data(G_OBJECT(ListWin), "webkit");

	if (!(SearchParameter & lcs_matchcase))
		options |= WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE;

	if (SearchParameter & lcs_backwards)
		options |= WEBKIT_FIND_OPTIONS_BACKWARDS;

	WebKitFindController *find_controller = webkit_web_view_get_find_controller(webView);
	webkit_find_controller_search(find_controller, SearchString, options, G_MAXUINT);

	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	WebKitWebView *webView = (WebKitWebView*)g_object_get_data(G_OBJECT(ListWin), "webkit");

	WebKitFindController *find_controller = webkit_web_view_get_find_controller(webView);
	webkit_find_controller_search_finish(find_controller);

	switch (Command)
	{
	case lc_copy :
		webkit_web_view_execute_editing_command(webView, WEBKIT_EDITING_COMMAND_COPY);

		break;

	case lc_selectall :
		webkit_web_view_execute_editing_command(webView, WEBKIT_EDITING_COMMAND_SELECT_ALL);
		break;

	default :
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(gExecutablePath, &dlinfo) != 0)
	{
		g_strlcpy(gExecutablePath, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(gExecutablePath, '/');

		if (pos)
			strcpy(pos, EXE_NAME);

		gchar *quoted = g_shell_quote(gExecutablePath);
		g_strlcpy(gExecutablePath, quoted, PATH_MAX);
		g_free(quoted);

		g_strlcpy(gConfigPath, dlinfo.dli_fname, PATH_MAX);
		pos = strrchr(gConfigPath, '/');

		if (pos)
			strcpy(pos, CFG_NAME);

		quoted = g_shell_quote(gConfigPath);
		g_strlcpy(gConfigPath, quoted, PATH_MAX);
		g_free(quoted);
	}
}
