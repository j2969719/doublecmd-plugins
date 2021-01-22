#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <dlfcn.h>
#include <string.h>
#include "wlxplugin.h"

#define _tmpl "_dc-hx.XXXXXX"
#define _bin "%s/redist/exsimple"
#define _cfg "%s/redist/default.cfg"
#define _out "%s/output_%s.html"

gchar *path = "";

static GtkWidget *getFirstChild(GtkWidget *w)
{
	GList *list = gtk_container_get_children(GTK_CONTAINER(w));
	GtkWidget *result = GTK_WIDGET(list->data);
	g_list_free(list);
	return result;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *webView;
	gchar *exsimple;
	gchar *config;
	gchar *tmpdir;
	gchar *command;
	gchar *output;
	gchar *fileUri;

	if (!path || path == "")
		return NULL;

	tmpdir = g_dir_make_tmp(_tmpl, NULL);
	exsimple = g_strdup_printf(_bin, path);
	config = g_strdup_printf(_cfg, path);
	output = g_strdup_printf(_out, tmpdir, g_path_get_basename(FileToLoad));
	fileUri = g_filename_to_uri(output, NULL, NULL);
	command = g_strdup_printf("%s %s %s %s", g_shell_quote(exsimple), g_shell_quote(FileToLoad),
	                          g_shell_quote(output), g_shell_quote(config));
	g_free(exsimple);
	g_free(config);

	if (system(command) != 0)
	{
		system(g_strdup_printf("rm -r %s", g_shell_quote(tmpdir)));
		return NULL;
	}

	if (!g_file_test(output, G_FILE_TEST_EXISTS))
	{
		system(g_strdup_printf("rm -r %s", g_shell_quote(tmpdir)));
		return NULL;
	}

	g_free(output);
	g_free(command);

	gFix = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);
	g_object_set_data(G_OBJECT(gFix), "tmpdir", tmpdir);
	webView = webkit_web_view_new();

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
	gchar *exsimple;
	gchar *config;
	gchar *tmpdir;
	gchar *command;
	gchar *output;
	gchar *fileUri;

	tmpdir = g_object_get_data(G_OBJECT(PluginWin), "tmpdir");

	if (tmpdir)
		system(g_strdup_printf("rm -r %s", g_shell_quote(tmpdir)));

	tmpdir = g_dir_make_tmp(_tmpl, NULL);
	g_object_set_data(G_OBJECT(PluginWin), "tmpdir", tmpdir);
	exsimple = g_strdup_printf(_bin, path);
	config = g_strdup_printf(_cfg, path);
	output = g_strdup_printf(_out, tmpdir, g_path_get_basename(FileToLoad));
	fileUri = g_filename_to_uri(output, NULL, NULL);
	command = g_strdup_printf("%s %s %s %s", g_shell_quote(exsimple), g_shell_quote(FileToLoad),
	                          g_shell_quote(output), g_shell_quote(config));

	g_free(exsimple);
	g_free(config);

	if (system(command) != 0)
	{
		system(g_strdup_printf("rm -r %s", g_shell_quote(tmpdir)));
		return LISTPLUGIN_ERROR;
	}

	if (!g_file_test(output, G_FILE_TEST_EXISTS))
	{
		system(g_strdup_printf("rm -r %s", g_shell_quote(tmpdir)));
		return LISTPLUGIN_ERROR;
	}

	g_free(output);
	g_free(command);

	webkit_web_view_load_uri(WEBKIT_WEB_VIEW(getFirstChild(PluginWin)), fileUri);
	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gchar *tmpdir = g_object_get_data(G_OBJECT(ListWin), "tmpdir");
	gtk_widget_destroy(GTK_WIDGET(ListWin));

	if (tmpdir)
	{
		system(g_strdup_printf("rm -r %s", g_shell_quote(tmpdir)));
		g_free(tmpdir);
	}
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

	if (dladdr(path, &dlinfo) != 0)
		path = g_path_get_dirname(dlinfo.dli_fname);
}

int DCPCALL ListPrint(HWND ListWin, char* FileToPrint, char* DefPrinter, int PrintFlags, RECT* Margins)
{
	WebKitWebFrame *frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(getFirstChild(ListWin)));
	webkit_web_frame_print(frame);

	return LISTPLUGIN_OK;
}
