/*Лицензия:*/
/*Эта программа является свободным программным обеспечением*/
/*Вы можете распространять и/или изменять*/
/*Его в соответствии с условиями GNU General Public License, опубликованной*/
/*Free Software Foundation, версии 2, либо (По вашему выбору) любой более поздней версии.*/
/*Эта программа распространяется в надежде, что она будет полезна,*/
/*Но БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ*/

// https://doublecmd.sourceforge.io/forum/viewtopic.php?f=8&t=2727

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <limits.h>
#include <dlfcn.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourceiter.h>
#include <gtksourceview/gtksourcebuffer.h>
#include "wlxplugin.h"

#define DETECT_STRING "\
(EXT=\"ISO\")|(EXT=\"TORRENT\")|(EXT=\"SO\")|(EXT=\"MO\")|\
(EXT=\"DEB\")|(EXT=\"TAR\")|(EXT=\"LHA\")|(EXT=\"ARJ\")|\
(EXT=\"CAB\")|(EXT=\"HA\")|(EXT=\"RAR\")|(EXT=\"ALZ\")|\
(EXT=\"CPIO\")|(EXT=\"7Z\")|(EXT=\"ACE\")|(EXT=\"ARC\")|\
(EXT=\"ZIP\")|(EXT=\"ZOO\")|(EXT=\"PS\")|(EXT=\"PDF\")|\
(EXT=\"ODT\")|(EXT=\"DOC\")|(EXT=\"XLS\")|(EXT=\"DVI\")|\
(EXT=\"DJVU\")|(EXT=\"EPUB\")|(EXT=\"HTML\")|(EXT=\"HTM\")|\
(EXT=\"EXE\")|(EXT=\"DLL\")|(EXT=\"GZ\")|(EXT=\"BZ2\")|\
(EXT=\"XZ\")|(EXT=\"MSI\")|(EXT=\"ZPAQ\")|(EXT=\"IMA\")|\
(EXT=\"IMG\")"

static char script_path[PATH_MAX];
const char* script_file = "fileinfo.sh";
GtkWrapMode wrap_mode;
gchar *font, *nfstr;
gboolean no_cursor;
gint p_above, p_below;

static GtkWidget * getFirstChild(GtkWidget *w)
{
	GList *list = gtk_container_get_children(GTK_CONTAINER(w));
	GtkWidget *result = GTK_WIDGET(list->data);
	g_list_free(list);
	return result;
}

void convertbuf1(gchar* encin, gchar* encout, GtkWidget *widget)
{
	gchar *result;
	gsize coverted;
	gchar *buf1 = g_object_get_data(G_OBJECT(widget), "origin");
	GtkSourceBuffer *sBuf = g_object_get_data(G_OBJECT(widget), "txtbuf");

	if (encout)
	{
		gchar *tmp = g_convert(buf1, -1, encin, "UTF-8", NULL, &coverted, NULL);

		if ((tmp) && (g_strcmp0(tmp, "") != 0))
		{
			result = g_convert(tmp, coverted, "UTF-8", encout, NULL, NULL, NULL);
			g_free(tmp);
		}
	}
	else
		result = g_convert(buf1, -1, "UTF-8", encin, NULL, NULL, NULL);

	if (result && g_utf8_validate(result, -1, NULL))
	{
		gtk_text_buffer_set_text(GTK_TEXT_BUFFER(sBuf), result, -1);
		g_free(result);
	}

}

void resetbuf1(GtkWidget *widget)
{
	gchar *buf1 = g_object_get_data(G_OBJECT(widget), "origin");
	GtkSourceBuffer *sBuf = g_object_get_data(G_OBJECT(widget), "txtbuf");
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(sBuf), buf1, -1);
}



gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	GtkTextView *textview;

	switch (event->keyval)
	{
	case GDK_k:
		convertbuf1("CP1251", "KOI-8", data);
		return TRUE;

	case GDK_d:
		convertbuf1("CP1251", "866", data);
		return TRUE;

	case GDK_r:
		resetbuf1(data);
		return TRUE;

	case GDK_w:
		textview = GTK_TEXT_VIEW(getFirstChild(getFirstChild(GTK_WIDGET(data))));

		if (gtk_text_view_get_wrap_mode(textview) == GTK_WRAP_NONE)
			gtk_text_view_set_wrap_mode(textview, GTK_WRAP_WORD);
		else
			gtk_text_view_set_wrap_mode(textview, GTK_WRAP_NONE);

		return TRUE;

	default:
		return FALSE;
	}

	return FALSE;
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *tView;
	GtkSourceBuffer *tBuf;
	gchar *tmp, *command, *buf1;

	command = g_strdup_printf("\"%s\" \"%s\"", script_path, FileToLoad);

	if (!g_spawn_command_line_sync(command, &buf1, NULL, NULL, NULL))
	{
		g_free(command);
		return NULL;
	}

	g_free(command);

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	tBuf = gtk_source_buffer_new(NULL);
	g_object_set_data_full(G_OBJECT(gFix), "txtbuf", tBuf, (GDestroyNotify)g_object_unref);
	tmp = g_locale_to_utf8(buf1, -1, NULL, NULL, NULL);

	if (tmp == NULL)
	{
		tmp = g_convert(buf1, -1, "UTF-8", "CP1251", NULL, NULL, NULL);

		if ((tmp == NULL) || (g_strcmp0(tmp, "") == 0) || (!g_utf8_validate(tmp, -1, NULL)))
		{
			gtk_widget_destroy(GTK_WIDGET(gFix));
			return NULL;
		}
	}

	g_free(buf1);
	g_object_set_data_full(G_OBJECT(gFix), "origin", tmp, (GDestroyNotify)g_free);
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(tBuf), tmp, -1); // utf only

	tView = gtk_source_view_new_with_buffer(tBuf);
	gtk_widget_modify_font(tView, pango_font_description_from_string(font));
	gtk_text_view_set_editable(GTK_TEXT_VIEW(tView), FALSE);
	g_signal_connect(G_OBJECT(tView), "key_press_event", G_CALLBACK(on_key_press), (gpointer)gFix);

	if (no_cursor)
		gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(tView), FALSE);

	gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(tView), p_above);
	gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(tView), p_below);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tView), wrap_mode);

	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(tView));
	gtk_container_add(GTK_CONTAINER(gFix), scroll);
	gtk_widget_show_all(gFix);

	return gFix;

}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}


void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	static char cfg_path[PATH_MAX];
	const char* cfg_file = "settings.ini";
	GKeyFile *cfg;
	GError *err = NULL;
	gboolean bval;
	gboolean found = FALSE;

	// Find in plugin directory
	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(script_path, &dlinfo) != 0)
	{
		strncpy(script_path, dlinfo.dli_fname, PATH_MAX);
		strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(script_path, '/');

		if (pos)
		{
			strcpy(pos + 1, script_file);
			found = (access(script_path, X_OK) == 0);
		}

		pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, cfg_file);
	}

	// Find in configuration directory
	if (!found)
	{
		strcpy(script_path, dps->DefaultIniName);
		char* pos = strrchr(script_path, '/');

		if (pos != NULL)
		{
			strcpy(pos + 1, script_file);
			found = (access(script_path, X_OK) == 0);
		}
	}

	// Find in $PATH
	if (!found)
		strcpy(script_path, script_file);

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		g_print("fileinfo.wlx (%s): %s\n", cfg_path, (err)->message);
		font = "monospace 12";
		wrap_mode = GTK_WRAP_NONE;
		no_cursor = TRUE;
		nfstr = "not found";
		p_above = 0;
		p_below = 0;
	}
	else
	{
		font = g_key_file_get_string(cfg, "Appearance", "Font", NULL);

		if (!font)
			font = "monospace 12";

		if (err)
			g_error_free(err);

		p_above = g_key_file_get_integer(cfg, "Appearance", "PAbove", &err);

		if (err)
		{
			p_above = 0;
			err = NULL;
		}

		p_below = g_key_file_get_integer(cfg, "Appearance", "PBelow", &err);

		if (err)
		{
			p_below = 0;
			err = NULL;
		}

		bval = g_key_file_get_boolean(cfg, "Flags", "Wrap", NULL);

		if (bval)
			wrap_mode = GTK_WRAP_WORD;
		else
			wrap_mode = GTK_WRAP_NONE;

		bval = g_key_file_get_boolean(cfg, "Flags", "NoCursor", &err);

		if (!bval && !err)
			no_cursor = FALSE;
		else
			no_cursor = TRUE;

		nfstr = g_key_file_get_string(cfg, "Appearance", "NotFoundStr", NULL);

		if (!nfstr)
			nfstr = "not found";
	}

	g_key_file_free(cfg);

	if (err)
		g_error_free(err);
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, DETECT_STRING, maxlen-1);
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	GtkSourceBuffer *sBuf;
	GtkTextMark *last_pos;
	GtkTextIter iter, mstart, mend;
	gboolean found;

	sBuf = g_object_get_data(G_OBJECT(ListWin), "txtbuf");
	last_pos = gtk_text_buffer_get_mark(GTK_TEXT_BUFFER(sBuf), "last_pos");

	if (last_pos == NULL)
		gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(sBuf), &iter);
	else
		gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(sBuf), &iter, last_pos);

	if ((SearchParameter & lcs_backwards) && (SearchParameter & lcs_matchcase))
		found = gtk_source_iter_backward_search(&iter, SearchString, GTK_SOURCE_SEARCH_TEXT_ONLY, &mend, &mstart, NULL);
	else if (SearchParameter & lcs_matchcase)
		found = gtk_source_iter_forward_search(&iter, SearchString, GTK_SOURCE_SEARCH_TEXT_ONLY, &mstart, &mend, NULL);
	else if (SearchParameter & lcs_backwards)
		found = gtk_source_iter_backward_search(&iter, SearchString, GTK_SOURCE_SEARCH_TEXT_ONLY | GTK_SOURCE_SEARCH_CASE_INSENSITIVE,
		                                        &mend, &mstart, NULL);
	else
		found = gtk_source_iter_forward_search(&iter, SearchString, GTK_SOURCE_SEARCH_TEXT_ONLY | GTK_SOURCE_SEARCH_CASE_INSENSITIVE,
		                                       &mstart, &mend, NULL);

	if (found)
	{
		gtk_text_buffer_select_range(GTK_TEXT_BUFFER(sBuf), &mstart, &mend);
		gtk_text_buffer_create_mark(GTK_TEXT_BUFFER(sBuf), "last_pos", &mend, FALSE);
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(getFirstChild(getFirstChild(GTK_WIDGET(ListWin)))),
		                                   gtk_text_buffer_get_mark(GTK_TEXT_BUFFER(sBuf), "last_pos"));

	}
	else
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ListWin))),
		                    GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		                    "\"%s\" %s!", SearchString, nfstr);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	GtkTextBuffer *tBuf;
	GtkTextIter p;

	tBuf = g_object_get_data(G_OBJECT(ListWin), "txtbuf");

	switch (Command)
	{
	case lc_copy :
		gtk_text_buffer_copy_clipboard(GTK_TEXT_BUFFER(tBuf), gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
		break;

	case lc_selectall :
		gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(tBuf), &p);
		gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(tBuf), &p);
		gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(tBuf), &p);
		gtk_text_buffer_move_mark_by_name(GTK_TEXT_BUFFER(tBuf), "selection_bound", &p);
		break;

	default :
		return LISTPLUGIN_ERROR;
	}
}
