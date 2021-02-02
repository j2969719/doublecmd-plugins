/*
 * Copyright (C) 2006, 2007 Apple Inc.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2011 Lukasz Slachciak
 * Copyright (C) 2011 Bob Murphy
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE

#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include "wlxplugin.h"
#include <md4c-html.h>

#define _detectstring "EXT=\"MD\""

static void proc_md_cb(const MD_CHAR* text, MD_SIZE size, void *userdata)
{
	gchar *str = g_strndup(text, size);
	g_string_append((GString*)userdata, str);
	g_free(str);
}

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
	GtkWidget* webView;
	gchar *contents = NULL;
	gsize length;

	if (!g_file_get_contents(FileToLoad, &contents, &length, NULL))
		return NULL;

	GString *res = g_string_new(NULL);

	if (md_html(contents, length, proc_md_cb, res, MD_DIALECT_GITHUB, MD_HTML_FLAG_SKIP_UTF8_BOM) != 0)
	{
		g_free(contents);
		g_string_free(res, TRUE);
		return NULL;
	}

	gFix = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);
	webView = webkit_web_view_new();

	// https://doublecmd.sourceforge.io/forum/viewtopic.php?f=8&t=4106&start=72#p22156
	WebKitFaviconDatabase *database = webkit_get_favicon_database();
	webkit_favicon_database_set_path(database, NULL);

	gchar *html_str = g_string_free(res, FALSE);
	webkit_web_view_load_html_string(WEBKIT_WEB_VIEW(webView), html_str, NULL);
	g_free(contents);
	g_free(html_str);

	gtk_widget_grab_focus(webView);
	gtk_container_add(GTK_CONTAINER(gFix), webView);

	gtk_widget_show_all(gFix);
	return gFix;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	gchar *contents = NULL;
	gsize length;

	if (!g_file_get_contents(FileToLoad, &contents, &length, NULL))
		return LISTPLUGIN_ERROR;

	GString *res = g_string_new(NULL);

	if (md_html(contents, length, proc_md_cb, res, 0, MD_HTML_FLAG_SKIP_UTF8_BOM) != 0)
	{
		g_free(contents);
		g_string_free(res, TRUE);
		return LISTPLUGIN_ERROR;
	}

	gchar *html_str = g_string_free(res, FALSE);
	webkit_web_view_load_html_string(WEBKIT_WEB_VIEW(getFirstChild(PluginWin)), html_str, NULL);
	g_free(contents);
	g_free(html_str);

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen - 1);
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


int DCPCALL ListPrint(HWND ListWin, char* FileToPrint, char* DefPrinter, int PrintFlags, RECT* Margins)
{
	WebKitWebFrame *frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(getFirstChild(ListWin)));
	webkit_web_frame_print(frame);

	return LISTPLUGIN_OK;
}
