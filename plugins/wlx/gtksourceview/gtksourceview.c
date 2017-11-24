/* 
   http://www.bravegnu.org/gtktext/x561.html
   Joe Arose updated code to gtk3 and gtksourceview-3.0. 
*/

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <string.h>
#include "wlxplugin.h"


#define _detectstring "EXT=\"C\"|EXT=\"H\"|EXT=\"LUA\"|EXT=\"CPP\"|EXT=\"HPP\"|EXT=\"PAS\""


static gboolean open_file (GtkSourceBuffer *sBuf, const gchar *filename);


HWND DCPCALL ListLoad (HWND ParentWin, char* FileToLoad, int ShowFlags)
{ 
	GtkWidget *gFix;
	GtkWidget *pScrollWin;
	GtkWidget *sView;
	GtkSourceLanguageManager *lm;
	GtkSourceBuffer *sBuf;

	gFix = gtk_vbox_new(FALSE , 5);
	gtk_container_add(GTK_CONTAINER (GTK_WIDGET(ParentWin)), gFix);
 
	/* Create a Scrolled Window that will contain the GtkSourceView */
	pScrollWin = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pScrollWin),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	/* Now create a GtkSourceLanguageManager */
	lm = gtk_source_language_manager_new();

	/* and a GtkSourceBuffer to hold text (similar to GtkTextBuffer) */
	sBuf = GTK_SOURCE_BUFFER (gtk_source_buffer_new (NULL));
	g_object_ref (lm);
	g_object_set_data_full ( G_OBJECT (sBuf), "languages-manager",
	lm, (GDestroyNotify) g_object_unref);
	g_object_set_data_full ( G_OBJECT (gFix), "srcbuf", sBuf, (GDestroyNotify) g_object_unref);

	/* Create the GtkSourceView and associate it with the buffer */
	sView = gtk_source_view_new_with_buffer(sBuf);
	gtk_widget_modify_font (sView, pango_font_description_from_string ("monospace 12"));
	gtk_source_view_set_show_line_numbers (GTK_SOURCE_VIEW(sView), TRUE);
	gtk_source_view_set_highlight_current_line (GTK_SOURCE_VIEW(sView), TRUE);
	gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW(sView), GTK_SOURCE_DRAW_SPACES_TAB);

	/* Attach the GtkSourceView to the scrolled Window */
	gtk_container_add (GTK_CONTAINER (pScrollWin), GTK_WIDGET (sView));

	gtk_container_add (GTK_CONTAINER (gFix), pScrollWin);
	if (!open_file (sBuf, FileToLoad))
	{
		gtk_widget_destroy(GTK_WIDGET(gFix));
		return NULL;
	}
	gtk_widget_show_all (gFix);

	return gFix;
}


static gboolean
open_file (GtkSourceBuffer *sBuf, const gchar *filename)
{
	GtkSourceLanguageManager *lm;
	GtkSourceLanguage *language = NULL;
	GError *err = NULL;
	gboolean reading;
	GtkTextIter iter;
	GIOChannel *io;
	gchar *buffer;
	gboolean result_uncertain;
	gchar *content_type;

	g_return_val_if_fail (sBuf != NULL, FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);
	g_return_val_if_fail (GTK_SOURCE_BUFFER (sBuf), FALSE);

	/* get the Language for source mimetype */
	lm = g_object_get_data (G_OBJECT (sBuf), "languages-manager");

	content_type = g_content_type_guess (filename, NULL, 0, &result_uncertain);
	if (result_uncertain)
	{
		g_free (content_type);
		content_type = NULL;
	}

	language = gtk_source_language_manager_guess_language (lm, filename, content_type);
	gtk_source_buffer_set_language (sBuf, language);
	g_free (content_type);

	g_print("Language: [%s]\n", gtk_source_language_get_name(language));

	/* Now load the file from Disk */
	io = g_io_channel_new_file (filename, "r", &err);
	if (!io)
	{
		g_print("error: %s %s\n", (err)->message, filename);
		return FALSE;
	}

	if (g_io_channel_set_encoding (io, "utf-8", &err) != G_IO_STATUS_NORMAL)
	{
		g_print("err: Failed to set encoding:\n%s\n%s", filename, (err)->message);
		return FALSE;
	}

	gtk_source_buffer_begin_not_undoable_action (sBuf);

	//gtk_text_buffer_set_text (GTK_TEXT_BUFFER (sBuf), "", 0);
	buffer = g_malloc (4096);
	reading = TRUE;
	while (reading)
	{
		gsize bytes_read;
		GIOStatus status;

		status = g_io_channel_read_chars (io, buffer, 4096, &bytes_read, &err);
		switch (status)
		{
			case G_IO_STATUS_EOF: 
				reading = FALSE;

			case G_IO_STATUS_NORMAL:
				if (bytes_read == 0) continue; 
				gtk_text_buffer_get_end_iter ( GTK_TEXT_BUFFER (sBuf), &iter);
				gtk_text_buffer_insert (GTK_TEXT_BUFFER(sBuf),&iter,buffer,bytes_read);
			break;

			case G_IO_STATUS_AGAIN: continue;

			case G_IO_STATUS_ERROR:

			default:
				g_print("err (%s): %s", filename, (err)->message);
				/* because of error in input we clear already loaded text */
				gtk_text_buffer_set_text (GTK_TEXT_BUFFER (sBuf), "", 0);

				reading = FALSE;
				break;
		}
	}
	g_free (buffer);

	gtk_source_buffer_end_not_undoable_action (sBuf);
	g_io_channel_unref (io);

	if (err)
	{
		g_error_free (err);
		return FALSE;
	}

	gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (sBuf), FALSE);

	/* move cursor to the beginning */
	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (sBuf), &iter);
	gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (sBuf), &iter);

	g_object_set_data_full (G_OBJECT (sBuf),"filename", g_strdup (filename),
	(GDestroyNotify) g_free);

	return TRUE;
}

void ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString,int maxlen)
{
	strncpy(DetectString, _detectstring, maxlen);
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString,int SearchParameter)
{
	GtkSourceBuffer *sBuf;
	GtkTextMark *last_pos;
	GtkTextIter iter, mstart, mend;
	gboolean found;
	
	sBuf = g_object_get_data (G_OBJECT (ListWin), "srcbuf");
	last_pos = gtk_text_buffer_get_mark (GTK_TEXT_BUFFER(sBuf), "last_pos");

	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(sBuf), &iter);
	
	if (SearchParameter & lcs_findfirst)
		gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(sBuf), &iter);
	else 
		gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER(sBuf), &iter, last_pos);

	if (SearchParameter & lcs_backwards)
		found = gtk_text_iter_backward_search (&iter, SearchString, GTK_TEXT_SEARCH_TEXT_ONLY, &mend, &mstart, NULL);
	else
		found = gtk_text_iter_forward_search (&iter, SearchString, GTK_TEXT_SEARCH_TEXT_ONLY, &mstart, &mend, NULL);

	if (found)
	{
		gtk_text_buffer_select_range (GTK_TEXT_BUFFER(sBuf), &mstart, &mend);
		gtk_text_buffer_create_mark (GTK_TEXT_BUFFER(sBuf), "last_pos", &mend, FALSE);
	}
	
}

int DCPCALL ListSendCommand(HWND ListWin,int Command,int Parameter)
{
	GtkSourceBuffer *sBuf;
	GtkTextIter p;

	sBuf = g_object_get_data (G_OBJECT (ListWin), "srcbuf");

	switch(Command)
	{
		case lc_copy :
			gtk_text_buffer_copy_clipboard(GTK_TEXT_BUFFER(sBuf),gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
			break;
		case lc_selectall :
			gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(sBuf), &p);
			gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(sBuf), &p);
			gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(sBuf), &p);
			gtk_text_buffer_move_mark_by_name(GTK_TEXT_BUFFER(sBuf), "selection_bound", &p);
			break;
		default :
			return LISTPLUGIN_ERROR;
	}
}

