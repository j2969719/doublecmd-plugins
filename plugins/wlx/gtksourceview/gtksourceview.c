/*
   http://www.bravegnu.org/gtktext/x561.html
   Joe Arose updated code to gtk3 and gtksourceview-3.0.
*/

#define _GNU_SOURCE

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourceiter.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcestyleschememanager.h>
#include <dlfcn.h>
#include <limits.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring "EXT=\"C\"|EXT=\"H\"|EXT=\"LUA\"|EXT=\"CPP\"|EXT=\"HPP\"|EXT=\"PAS\"|\
EXT=\"CSS\"|EXT=\"SH\"|EXT=\"XML\"|EXT=\"INI\"|EXT=\"DIFF\"|EXT=\"PATCH\"|EXT=\"PO\"|EXT=\"PY\"|\
EXT=\"XSL\"|EXT=\"LPR\"|EXT=\"PP\"|EXT=\"LPI\"|EXT=\"LFM\"|EXT=\"LPK\"|EXT=\"DOF\"|EXT=\"DPR\""

GtkWrapMode wrap_mode;
gchar *font, *style, *nfstr, *ext_pascal, *ext_xml, *ext_ini;
gboolean line_num, hcur_line, draw_spaces, no_cursor;
gint s_tab, p_above, p_below;


static GtkWidget *getFirstChild(GtkWidget *w)
{
	GList *list = gtk_container_get_children(GTK_CONTAINER(w));
	GtkWidget *result = GTK_WIDGET(list->data);
	g_list_free(list);
	return result;
}

static gboolean open_file(GtkSourceBuffer *sBuf, const gchar *filename);


HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *pScrollWin;
	GtkWidget *sView;
	GtkSourceLanguageManager *lm;
	GtkSourceStyleSchemeManager *scheme_manager;
	GtkSourceStyleScheme *scheme;
	GtkSourceBuffer *sBuf;

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);

	/* Create a Scrolled Window that will contain the GtkSourceView */
	pScrollWin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollWin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	/* Now create a GtkSourceLanguageManager */
	lm = gtk_source_language_manager_new();

	/* and a GtkSourceBuffer to hold text (similar to GtkTextBuffer) */
	sBuf = GTK_SOURCE_BUFFER(gtk_source_buffer_new(NULL));
	g_object_ref(lm);
	g_object_set_data_full(G_OBJECT(sBuf), "languages-manager", lm, (GDestroyNotify)g_object_unref);
	g_object_set_data_full(G_OBJECT(gFix), "srcbuf", sBuf, (GDestroyNotify)g_object_unref);

	/* Create the GtkSourceView and associate it with the buffer */
	sView = gtk_source_view_new_with_buffer(sBuf);
	gtk_widget_modify_font(sView, pango_font_description_from_string(font));
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(sView), line_num);
	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(sView), s_tab);
	gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(sView), hcur_line);

	if (draw_spaces)
		gtk_source_view_set_draw_spaces(GTK_SOURCE_VIEW(sView), GTK_SOURCE_DRAW_SPACES_ALL);

	/* Attach the GtkSourceView to the scrolled Window */
	gtk_container_add(GTK_CONTAINER(pScrollWin), GTK_WIDGET(sView));
	gtk_container_add(GTK_CONTAINER(gFix), pScrollWin);

	if (!open_file(sBuf, FileToLoad))
	{
		gtk_widget_destroy(GTK_WIDGET(gFix));
		return NULL;
	}

	scheme_manager = gtk_source_style_scheme_manager_get_default();
	scheme = gtk_source_style_scheme_manager_get_scheme(scheme_manager, style);
	gtk_source_buffer_set_style_scheme(sBuf, scheme);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(sView), FALSE);

	if (no_cursor)
		gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(sView), FALSE);

	gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(sView), p_above);
	gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(sView), p_below);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(sView), wrap_mode);

	gtk_widget_show_all(gFix);

	return gFix;
}


static gboolean open_file(GtkSourceBuffer *sBuf, const gchar *filename)
{
	GtkSourceLanguageManager *lm;
	GtkSourceLanguage *language = NULL;
	GError *err = NULL;
	gboolean reading;
	GtkTextIter iter;
	GIOChannel *io;
	gchar *buffer;
	gchar *ext;
	gboolean result_uncertain;
	gchar *content_type;

	g_return_val_if_fail(sBuf != NULL, FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(GTK_SOURCE_BUFFER(sBuf), FALSE);

	/* get the Language for source mimetype */
	lm = g_object_get_data(G_OBJECT(sBuf), "languages-manager");

	content_type = g_content_type_guess(filename, NULL, 0, &result_uncertain);

	if (result_uncertain)
	{
		g_free(content_type);
		content_type = NULL;
	}

	language = gtk_source_language_manager_guess_language(lm, filename, content_type);

	if (!language)
	{
		ext = g_strrstr(filename, ".");

		if (ext != NULL)
		{
			ext = g_strdup_printf("%s;", ext);
			ext = g_ascii_strdown(ext, -1);

			if ((ext_pascal != NULL) && (g_strrstr(ext_pascal, ext) != NULL))
				language = gtk_source_language_manager_get_language(lm, "pascal");
			else if ((ext_xml != NULL) && (g_strrstr(ext_xml, ext) != NULL))
				language = gtk_source_language_manager_get_language(lm, "xml");
			else if ((ext_ini != NULL) && (g_strrstr(ext_ini, ext) != NULL))
				language = gtk_source_language_manager_get_language(lm, "ini");

			g_free(ext);
		}
	}

	g_free(content_type);

	//g_return_val_if_fail(language != NULL, FALSE);
	if (!language)
		return FALSE;

	gtk_source_buffer_set_language(sBuf, language);
	g_print("Language: [%s]\n", gtk_source_language_get_name(language));


	/* Now load the file from Disk */
	io = g_io_channel_new_file(filename, "r", &err);

	if (!io)
	{
		g_print("gtksourceview.wlx (%s): %s\n", filename, (err)->message);
		return FALSE;
	}

	if (g_io_channel_set_encoding(io, "utf-8", &err) != G_IO_STATUS_NORMAL)
	{
		g_print("gtksourceview.wlx (%s): Failed to set encoding: %s\n", filename, (err)->message);
		return FALSE;
	}

	gtk_source_buffer_begin_not_undoable_action(sBuf);

	//gtk_text_buffer_set_text (GTK_TEXT_BUFFER(sBuf), "", 0);
	buffer = g_malloc(4096);
	reading = TRUE;

	while (reading)
	{
		gsize bytes_read;
		GIOStatus status;

		status = g_io_channel_read_chars(io, buffer, 4096, &bytes_read, &err);

		switch (status)
		{
		case G_IO_STATUS_EOF:
			reading = FALSE;

		case G_IO_STATUS_NORMAL:
			if (bytes_read == 0) continue;

			gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(sBuf), &iter);
			gtk_text_buffer_insert(GTK_TEXT_BUFFER(sBuf), &iter, buffer, bytes_read);
			break;

		case G_IO_STATUS_AGAIN:
			continue;

		case G_IO_STATUS_ERROR:

		default:
			g_print("gtksourceview.wlx (%s): %s\n", filename, (err)->message);
			/* because of error in input we clear already loaded text */
			gtk_text_buffer_set_text(GTK_TEXT_BUFFER(sBuf), "", 0);
			reading = FALSE;
			break;
		}
	}

	g_free(buffer);

	gtk_source_buffer_end_not_undoable_action(sBuf);
	g_io_channel_unref(io);

	if (err)
	{
		g_error_free(err);
		return FALSE;
	}

	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(sBuf), FALSE);

	/* move cursor to the beginning */
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(sBuf), &iter);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(sBuf), &iter);

	g_object_set_data_full(G_OBJECT(sBuf), "filename", g_strdup(filename), (GDestroyNotify)g_free);

	return TRUE;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen-1);
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	GtkSourceBuffer *sBuf;
	GtkTextMark *last_pos;
	GtkTextIter iter, mstart, mend;
	gboolean found;

	sBuf = g_object_get_data(G_OBJECT(ListWin), "srcbuf");
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
	GtkSourceBuffer *sBuf;
	GtkTextIter p;

	sBuf = g_object_get_data(G_OBJECT(ListWin), "srcbuf");

	switch (Command)
	{
	case lc_copy :
		gtk_text_buffer_copy_clipboard(GTK_TEXT_BUFFER(sBuf), gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
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
void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	static char cfg_path[PATH_MAX];
	const char* cfg_file = "settings.ini";
	GKeyFile *cfg;
	GError *err = NULL;
	gboolean bval;

	// Find in plugin directory
	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, cfg_file);
	}

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		g_print("gtksourceview.wlx (%s): %s\n", cfg_path, (err)->message);
		font = "monospace 12";
		style = "classic";
		wrap_mode = GTK_WRAP_NONE;
		line_num = TRUE;
		hcur_line = TRUE;
		draw_spaces = TRUE;
		no_cursor = TRUE;
		nfstr = "not found";
		s_tab = 8;
		p_above = 0;
		p_below = 0;
	}
	else
	{
		font = g_key_file_get_string(cfg, "Appearance", "Font", NULL);

		if (!font)
			font = "monospace 12";

		style = g_key_file_get_string(cfg, "Appearance", "Style", NULL);

		if (!style)
			style = "classic";

		bval = g_key_file_get_boolean(cfg, "Flags", "LineNumbers", &err);

		if (!bval && !err)
			line_num = FALSE;
		else
			line_num = TRUE;

		if (err)
			err = NULL;

		bval = g_key_file_get_boolean(cfg, "Flags", "HighlightCurLine", &err);

		if (!bval && !err)
			hcur_line = FALSE;
		else
			hcur_line = TRUE;

		if (err)
			err = NULL;

		bval = g_key_file_get_boolean(cfg, "Flags", "Spaces", &err);

		if (!bval && !err)
			draw_spaces = FALSE;
		else
			draw_spaces = TRUE;

		if (err)
			err = NULL;

		bval = g_key_file_get_boolean(cfg, "Flags", "NoCursor", &err);

		if (!bval && !err)
			no_cursor = FALSE;
		else
			no_cursor = TRUE;

		nfstr = g_key_file_get_string(cfg, "Appearance", "NotFoundStr", NULL);

		if (!nfstr)
			nfstr = "not found";

		if (err)
			err = NULL;

		s_tab = g_key_file_get_integer(cfg, "Appearance", "TabSize", &err);

		if (err)
		{
			s_tab = 8;
			err = NULL;
		}

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

		ext_pascal = g_key_file_get_string(cfg, "Override", "Pascal", NULL);
		ext_xml = g_key_file_get_string(cfg, "Override", "XML", NULL);
		ext_ini = g_key_file_get_string(cfg, "Override", "INI", NULL);
	}

	g_key_file_free(cfg);

	if (err)
		g_error_free(err);
}
