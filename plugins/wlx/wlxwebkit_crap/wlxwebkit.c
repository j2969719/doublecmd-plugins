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

gchar *replace_tmpls(gchar *orig, gchar *file, gchar *tmpdir, gchar *output, gboolean quote)
{
	gchar *result = g_strdup(orig);
	gchar *basename = g_path_get_basename(file);
	gchar *plugdir = g_path_get_dirname(cfgpath);
	gchar *filedir = g_path_get_dirname(file);
	gchar *basenamenoext = g_strdup(basename);
	gchar *dot = g_strrstr(basenamenoext, ".");

	if (dot)
	{
		int offset = dot - basenamenoext;
		basenamenoext[offset] = '\0';

		if (g_strcmp0("", basenamenoext) == 0)
			basenamenoext = g_strdup(basename);
	}

	result = str_replace(result, "$FILEDIR", quote ? g_shell_quote(filedir) : filedir);
	result = str_replace(result, "$FILE", quote ? g_shell_quote(file) : file);
	result = str_replace(result, "$HTML", quote ? g_shell_quote(output): output);
	result = str_replace(result, "$TMPDIR", quote ? g_shell_quote(tmpdir) : tmpdir);
	result = str_replace(result, "$PLUGDIR", quote ? g_shell_quote(plugdir) : plugdir);
	result = str_replace(result, "$BASENAMENOEXT", basenamenoext);
	result = str_replace(result, "$BASENAME", basename);
	g_free(plugdir);
	g_free(filedir);
	g_free(basename);
	g_free(basenamenoext);
	return result;
}

gboolean isabsolute(const gchar *file)
{
	if (file[0] =='/')
		return TRUE;
	else if (g_strrstr(file, "$TMPDIR") != NULL)
		return TRUE;
	else if (g_strrstr(file, "$FILEDIR") != NULL)
		return TRUE;
	else if (g_strrstr(file, "$PLUGDIR") != NULL)
		return TRUE;

	return FALSE;
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
	gboolean keeptmp;

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
		keeptmp = g_key_file_get_boolean(cfg, fileExt, "keeptmp", NULL);
	}

	g_key_file_free(cfg);

	if (!command)
		return NULL;

	tmpdir = g_dir_make_tmp("_dc-webkit.XXXXXX", NULL);

	if (outfile)
	{
		output = g_strdup(outfile);

		if (!isabsolute(output))
			output = g_strdup_printf("$TMPDIR/%s", outfile);

		output = replace_tmpls(output, FileToLoad, tmpdir, NULL, FALSE);
	}
	else
		output = g_strdup_printf("%s/output.html", tmpdir);

	command = replace_tmpls(command, FileToLoad, tmpdir, output, TRUE);
	g_print("%s\n", command);

	if (system(command) != 0)
	{
		if (!keeptmp)
			system(g_strdup_printf("rm -r %s", g_shell_quote(tmpdir)));

		return NULL;
	}

	if (!g_file_test(output, G_FILE_TEST_EXISTS) && fallbackfile)
	{
		output = g_strdup(fallbackfile);

		if (!isabsolute(output))
			output = g_strdup_printf("$TMPDIR/%s", fallbackfile);

		output = replace_tmpls(output, FileToLoad, tmpdir, NULL, FALSE);
	}

	if (!g_file_test(output, G_FILE_TEST_EXISTS))
	{
		if (!keeptmp)
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
