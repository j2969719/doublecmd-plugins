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

#include <enca.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

#define _detectstring "EXT=\"C\"|EXT=\"H\"|EXT=\"LUA\"|EXT=\"CPP\"|EXT=\"HPP\"|EXT=\"PAS\"|\
EXT=\"CSS\"|EXT=\"SH\"|EXT=\"XML\"|EXT=\"INI\"|EXT=\"DIFF\"|EXT=\"PATCH\"|EXT=\"PO\"|EXT=\"PY\"|\
EXT=\"XSL\"|EXT=\"LPR\"|EXT=\"PP\"|EXT=\"LPI\"|EXT=\"LFM\"|EXT=\"LPK\"|EXT=\"DOF\"|EXT=\"DPR\""

GtkWrapMode wrap_mode;
gchar *font, *style, *ext_pascal, *ext_xml, *ext_ini, *enca_lang, *force_charset;
gboolean line_num, hcur_line, draw_spaces, no_cursor, no_filter;
gint s_tab, p_above, p_below;
static gchar **encodings = NULL;


static GtkWidget *getFirstChild(GtkWidget *w)
{
	GList *list = gtk_container_get_children(GTK_CONTAINER(w));
	GtkWidget *result = GTK_WIDGET(list->data);
	g_list_free(list);
	return result;
}

static gboolean open_file(GtkSourceBuffer *sBuf, const gchar *filename, const gchar *enc);


static void reload_with_enc_cb(GtkComboBoxText *combo_box, GtkSourceBuffer *sBuf)
{
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(sBuf), "", 0);
	gchar *encname = gtk_combo_box_text_get_active_text(combo_box);
	open_file(sBuf, g_object_get_data(G_OBJECT(sBuf), "filename"), encname);

	if (encname)
		g_free(encname);
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *pScrollWin;
	GtkWidget *sView;
	GtkSourceLanguageManager *lm;
	GtkSourceStyleSchemeManager *scheme_manager;
	GtkSourceStyleScheme *scheme;
	GtkSourceBuffer *sBuf;

	GtkWidget *hBox;
	GtkWidget *lInfo;
	GtkWidget *cEnc;

	gFix = gtk_vbox_new(FALSE, 0);
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



	hBox = gtk_hbox_new(FALSE, 5);
	lInfo = gtk_label_new(NULL);
	cEnc = gtk_combo_box_text_new_with_entry();

	gchar **p = encodings;
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cEnc), _("Default"));

	while (*p)
	{
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cEnc), *p);
		*p++;
	}

	gtk_editable_set_editable(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(cEnc))), FALSE);
	g_signal_connect(G_OBJECT(cEnc), "changed", G_CALLBACK(reload_with_enc_cb), sBuf);

	g_object_set_data(G_OBJECT(sBuf), "info-label", lInfo);
	g_object_set_data(G_OBJECT(gFix), "combobox", cEnc);
	gtk_box_pack_start(GTK_BOX(gFix), hBox, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hBox), lInfo, TRUE, FALSE, 5);
	gtk_box_pack_end(GTK_BOX(hBox), cEnc, FALSE, FALSE, 5);


	/* Attach the GtkSourceView to the scrolled Window */
	gtk_container_add(GTK_CONTAINER(pScrollWin), GTK_WIDGET(sView));
	gtk_container_add(GTK_CONTAINER(gFix), pScrollWin);

	gtk_widget_grab_focus(pScrollWin);

	if (!open_file(sBuf, FileToLoad, NULL))
	{
		gtk_widget_destroy(GTK_WIDGET(gFix));
		return NULL;
	}

	g_object_set_data(G_OBJECT(sBuf), "filename", g_strdup(FileToLoad));


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

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	GtkSourceBuffer *sBuf = g_object_get_data(G_OBJECT(PluginWin), "srcbuf");
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(sBuf), "", 0);

	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(g_object_get_data(G_OBJECT(PluginWin), "combobox"))), "");

	gchar *filename = g_object_get_data(G_OBJECT(sBuf), "filename");

	if (filename)
		g_free(filename);

	g_object_set_data(G_OBJECT(sBuf), "filename", g_strdup(FileToLoad));


	if (!open_file(sBuf, FileToLoad, NULL))
		return LISTPLUGIN_ERROR;

	return LISTPLUGIN_OK;
}

static gboolean open_file(GtkSourceBuffer *sBuf, const gchar *filename, const gchar *enc)
{
	GtkSourceLanguageManager *lm;
	GtkSourceLanguage *language = NULL;
	GError *err = NULL;
	gboolean reading;
	GtkTextIter iter;
	GIOChannel *io;
	gchar *buffer;
	gchar *ext;
	const gchar *content_type;

	g_return_val_if_fail(sBuf != NULL, FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(GTK_SOURCE_BUFFER(sBuf), FALSE);

	/* get the Language for source mimetype */
	lm = g_object_get_data(G_OBJECT(sBuf), "languages-manager");

	GFile *gfile = g_file_new_for_path(filename);
	g_return_val_if_fail(gfile != NULL, FALSE);
	GFileInfo *fileinfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL, NULL);
	g_return_val_if_fail(fileinfo != NULL, FALSE);

	content_type = g_file_info_get_content_type(fileinfo);

	language = gtk_source_language_manager_guess_language(lm, filename, content_type);

	g_object_unref(fileinfo);
	g_object_unref(gfile);

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

	//g_return_val_if_fail(language != NULL, FALSE);
	if (!language)
		return FALSE;

	gtk_source_buffer_set_language(sBuf, language);
	//g_print("%s [%s]\n", _("Language:"), gtk_source_language_get_name(language));
	GtkLabel *label = g_object_get_data(G_OBJECT(sBuf), "info-label");
	gchar *info_txt = g_strdup_printf("%s [%s]", _("Language:"), gtk_source_language_get_name(language));
	gtk_label_set_text(label, info_txt);
	g_free(info_txt);


	/* Now load the file from Disk */
/*	io = g_io_channel_new_file(filename, "r", &err);

	if (!io)
	{
		g_print("gtksourceview.wlx (%s): %s\n", filename, (err)->message);
		return FALSE;
	}

	if (g_io_channel_set_encoding(io, "utf-8", &err) != G_IO_STATUS_NORMAL)
	{
		g_print("gtksourceview.wlx (%s): %s: %s\n", filename, _("Failed to set encoding"), (err)->message);
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
/*			gtk_text_buffer_set_text(GTK_TEXT_BUFFER(sBuf), "", 0);
			reading = FALSE;
			break;
		}
	}

	g_free(buffer);

	gtk_source_buffer_end_not_undoable_action(sBuf);
	g_io_channel_unref(io);*/


	gsize bytes_read;
	EncaAnalyser analyser;
	EncaEncoding encoding;

	if (!g_file_get_contents(filename, &buffer, &bytes_read, &err))
	{
		g_print("gtksourceview.wlx (%s): %s\n", filename, (err)->message);
		g_error_free(err);
		return FALSE;
	}
	else
	{
		if (bytes_read == 0)
		{
			g_free(buffer);
			g_print("gtksourceview.wlx (%s): %s\n", filename, _("file seems empty"));
			return FALSE;
		}

		if (!enc || g_strcmp0(enc, _("Default")) == 0)
		{

			analyser = enca_analyser_alloc(enca_lang);

			if (analyser)
			{
				enca_set_threshold(analyser, 1.38);
				enca_set_multibyte(analyser, 1);
				enca_set_ambiguity(analyser, 1);
				enca_set_garbage_test(analyser, 1);

				if (no_filter)
					enca_set_filtering(analyser, 0);

				encoding = enca_analyse(analyser, (unsigned char*)buffer, (size_t)bytes_read);

				//g_print("%s [%s]\n", _("Encoding:"), enca_charset_name(encoding.charset, ENCA_NAME_STYLE_ICONV));
				info_txt = g_strdup_printf("%s\t\t%s [%s]", gtk_label_get_text(label), _("Encoding:"), enca_charset_name(encoding.charset, ENCA_NAME_STYLE_ICONV));
				gtk_label_set_text(label, info_txt);
				g_free(info_txt);

				if (encoding.charset != -1 && encoding.charset != 27)
				{
					gchar *converted = g_convert_with_fallback(buffer, bytes_read, "UTF-8", enca_charset_name(encoding.charset, ENCA_NAME_STYLE_ICONV), NULL, NULL, &bytes_read, &err);

					if (err)
						g_print("gtksourcevi1ew.wlx (%s): %s\n", filename, (err)->message);

					g_free(buffer);
					buffer = converted;
				}
				else if (encoding.charset == -1)
				{
					if (force_charset && force_charset[0] != '\0')
					{
						gchar *converted = g_convert_with_fallback(buffer, bytes_read, "UTF-8", force_charset, NULL, NULL, &bytes_read, &err);

						if (err)
							g_print("gtksourcevi1ew.wlx (%s): %s\n", filename, (err)->message);

						g_free(buffer);
						buffer = converted;
					}
					else
					{
						enca_analyser_free(analyser);
						g_free(buffer);
						return FALSE;
					}
				}

				enca_analyser_free(analyser);
			}
		}
		else
		{
			gchar *converted = g_convert_with_fallback(buffer, bytes_read, "UTF-8", enc, NULL, NULL, &bytes_read, &err);

			if (err)
				g_print("gtksourcevi1ew.wlx (%s): %s\n", filename, (err)->message);

			g_free(buffer);
			buffer = converted;
		}

		if (buffer)
		{
			gtk_text_buffer_set_text(GTK_TEXT_BUFFER(sBuf), buffer, bytes_read);
			g_free(buffer);
		}
		else
			return FALSE;
	}

	if (err)
	{
		g_error_free(err);
		return FALSE;
	}

	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(sBuf), FALSE);

	/* move cursor to the beginning */
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(sBuf), &iter);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(sBuf), &iter);

	return TRUE;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	GtkSourceBuffer *sBuf = g_object_get_data(G_OBJECT(ListWin), "srcbuf");
	gchar *filename = g_object_get_data(G_OBJECT(sBuf), "filename");

	if (filename)
		g_free(filename);

	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen - 1);
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
		                    _("\"%s\" not found!"), SearchString);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}

	return LISTPLUGIN_OK;
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

	return LISTPLUGIN_OK;
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

		setlocale(LC_ALL, "");
		bindtextdomain(GETTEXT_PACKAGE, g_strdup_printf("%s/langs", g_path_get_dirname(dlinfo.dli_fname)));
		textdomain(GETTEXT_PACKAGE);
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

		enca_lang = g_key_file_get_string(cfg, "Enca", "Lang", NULL);

		if (!enca_lang)
			enca_lang = "__";

		no_filter = g_key_file_get_boolean(cfg, "Enca", "NoFilters", NULL);
		force_charset = g_key_file_get_string(cfg, "Enca", "ForceCharSet", NULL);

		encodings = g_key_file_get_string_list(cfg, "Enca", "Encodings", NULL, NULL);

		if (!encodings)
			encodings = g_strsplit("CP1251;866;ISO_8859-1", ";", -1);

	}

	g_key_file_free(cfg);

	if (err)
		g_error_free(err);
}
