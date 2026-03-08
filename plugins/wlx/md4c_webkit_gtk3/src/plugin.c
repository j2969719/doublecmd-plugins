#include <gtk/gtk.h>
#include <md4c-html.h> 
#include <webkit2/webkit2.h>
#include "wlxplugin.h"

static void proc_md_cb(const MD_CHAR* text, MD_SIZE size, void *userdata)
{
	gchar *str = g_strndup(text, size);
	g_string_append((GString*)userdata, str);
	g_free(str);
}

static gchar* md_to_html(char* FileToLoad)
{
	gchar *contents = NULL;
	gsize length;

	if (!g_file_get_contents(FileToLoad, &contents, &length, NULL))
		return NULL;

	GString *html = g_string_new(NULL);

	int ret = md_html(contents, length, proc_md_cb, html, MD_DIALECT_GITHUB, MD_HTML_FLAG_SKIP_UTF8_BOM);
	g_free(contents);

	if (ret != 0)
	{
		g_string_free(html, TRUE);
		return NULL;
	}

	return g_string_free(html, FALSE);
}


HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	gchar *html = md_to_html(FileToLoad);

	if (!html)
		return NULL;

	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), scroll);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);

	WebKitSettings *settings = webkit_settings_new();
	webkit_settings_set_default_charset(settings, "utf-8");
	GtkWidget *webView = webkit_web_view_new_with_settings(settings);
	webkit_web_view_load_html(WEBKIT_WEB_VIEW(webView), html, NULL);
	g_free(html);

	gtk_container_add(GTK_CONTAINER(scroll), webView);
	g_object_set_data(G_OBJECT(scroll), "webkit", WEBKIT_WEB_VIEW(webView)); 

	gtk_widget_show_all(scroll);
	return scroll;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	gchar *html = md_to_html(FileToLoad);

	if (!html)
		return LISTPLUGIN_ERROR;

	WebKitWebView *webView = (WebKitWebView*)g_object_get_data(G_OBJECT(PluginWin), "webkit");
	webkit_web_view_load_html(WEBKIT_WEB_VIEW(webView), html, NULL);
	g_free(html);

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, DETECT_STRING, maxlen);
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
