#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <dlfcn.h>
#include <string.h>
#include "wlxplugin.h"

#define TEMPLATE "_dc-hx.XXXXXX"
#define EXE_NAME "/redist/exsimple"
#define CFG_NAME "/redist/default.cfg"

gchar gConfigPath[PATH_MAX] = "";
gchar gExecutablePath[PATH_MAX] = "";

static GtkWidget *getFirstChild(GtkWidget *w)
{
	GList *list = gtk_container_get_children(GTK_CONTAINER(w));
	GtkWidget *result = GTK_WIDGET(list->data);
	g_list_free(list);

	return result;
}

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
	GtkWidget *gFix;
	GtkWidget *webView;

	gchar *tmpdir = g_dir_make_tmp(TEMPLATE, NULL);
	gchar *fileUri = export_html(tmpdir, FileToLoad);

	if (!fileUri)
		return NULL;

	gFix = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);
	g_object_set_data(G_OBJECT(gFix), "tmpdir", tmpdir);
	webView = webkit_web_view_new();

	WebKitWebSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webView));
	g_object_set(G_OBJECT(settings), "enable-scripts", FALSE, NULL);
	webkit_web_view_set_settings(WEBKIT_WEB_VIEW(webView), settings);

	// https://doublecmd.sourceforge.io/forum/viewtopic.php?f=8&t=4106&start=72#p22156
	WebKitFaviconDatabase *database = webkit_get_favicon_database();
	webkit_favicon_database_set_path(database, NULL);

	webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webView), fileUri);
	gtk_container_add(GTK_CONTAINER(gFix), webView);
	gtk_widget_show_all(gFix);

	return gFix;
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

	webkit_web_view_load_uri(WEBKIT_WEB_VIEW(getFirstChild(PluginWin)), fileUri);

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

	if (SearchParameter & lcs_matchcase)
		ss_case = TRUE;

	if (SearchParameter & lcs_backwards)
		ss_forward = FALSE;

	webkit_web_view_search_text(WEBKIT_WEB_VIEW(getFirstChild(ListWin)),
	                            SearchString, ss_case, ss_forward, TRUE);

	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	switch (Command)
	{
	case lc_copy :
		webkit_web_view_copy_clipboard(WEBKIT_WEB_VIEW(getFirstChild(ListWin)));
		break;

	case lc_selectall :
		webkit_web_view_select_all(WEBKIT_WEB_VIEW(getFirstChild(ListWin)));
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

int DCPCALL ListPrint(HWND ListWin, char* FileToPrint, char* DefPrinter, int PrintFlags, RECT* Margins)
{
	WebKitWebFrame *frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(getFirstChild(ListWin)));
	webkit_web_frame_print(frame);

	return LISTPLUGIN_OK;
}
