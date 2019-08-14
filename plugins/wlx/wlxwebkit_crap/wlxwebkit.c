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
#include <dlfcn.h>
#include <string.h>
#include "wlxplugin.h"

gchar *cfgpath = "";

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

gchar *str_replace(gchar *text, gchar *str, gchar *repl)
{
	gchar **split = g_strsplit(text, str, -1);
	gchar *result = g_strjoinv(repl, split);
	g_strfreev(split);
	return result;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GKeyFile *cfg;
	GtkWidget *gFix;
	GtkWidget *webView;
	gchar *command;
	gchar *tmpdir;
	gchar *output;
	gchar *fileExt;
	gchar *fileUri;
	gchar *outfile = NULL;
	gchar *fallbackfile = NULL;

	fileExt = get_file_ext(FileToLoad);

	if (!fileExt)
		return NULL;

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfgpath, G_KEY_FILE_KEEP_COMMENTS, NULL))
		return NULL;
	else
	{
		command = g_key_file_get_string(cfg, fileExt, "command", NULL);
		outfile = g_key_file_get_string(cfg, fileExt, "filename", NULL);
		fallbackfile = g_key_file_get_string(cfg, fileExt, "fallbackopen", NULL);
	}

	g_key_file_free(cfg);

	if (!command)
		return NULL;

	tmpdir = g_dir_make_tmp("_dc-webkit.XXXXXX", NULL);

	if (outfile)
		output = g_strdup_printf("%s/%s", tmpdir, outfile);
	else
		output = g_strdup_printf("%s/output.html", tmpdir);

	command = str_replace(command, "$FILE", g_shell_quote(FileToLoad));
	command = str_replace(command, "$HTML", g_shell_quote(output));
	command = str_replace(command, "$TMPDIR", g_shell_quote(tmpdir));
	g_print("%s\n", command);

	if (system(command) != 0)
	{
		system(g_strdup_printf("rm -r %s", g_shell_quote(tmpdir)));
		return NULL;
	}

	if (!g_file_test(output, G_FILE_TEST_EXISTS) && fallbackfile)
		output = g_strdup_printf("%s/%s", tmpdir, fallbackfile);

	if (!g_file_test(output, G_FILE_TEST_EXISTS))
	{
		system(g_strdup_printf("rm -r %s", g_shell_quote(tmpdir)));
		return NULL;
	}

	fileUri = g_filename_to_uri(output, NULL, NULL);

	gFix = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);
	g_object_set_data(G_OBJECT(gFix), "tmpdir", tmpdir);
	webView = webkit_web_view_new();

	// https://doublecmd.sourceforge.io/forum/viewtopic.php?f=8&t=4106&start=72#p22156
	WebKitFaviconDatabase *database = webkit_get_favicon_database();
	webkit_favicon_database_set_path(database, NULL);

	webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webView), fileUri);

	if (fileUri)
		g_free(fileUri);

	gtk_widget_grab_focus(webView);
	gtk_container_add(GTK_CONTAINER(gFix), webView);
	gtk_widget_show_all(gFix);
	return gFix;
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

	if (dladdr(cfgpath, &dlinfo) != 0)
		cfgpath = g_strdup_printf("%s/settings.ini", g_path_get_dirname(dlinfo.dli_fname));
}

int DCPCALL ListPrint(HWND ListWin, char* FileToPrint, char* DefPrinter, int PrintFlags, RECT* Margins)
{
	WebKitWebFrame *frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(getFirstChild(ListWin)));
	webkit_web_frame_print(frame);

	return LISTPLUGIN_OK;
}
