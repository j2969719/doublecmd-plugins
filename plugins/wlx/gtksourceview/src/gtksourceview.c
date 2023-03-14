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

typedef struct tCustomData
{
	GtkSourceView *sView;
	GtkSourceBuffer *sBuf;
	GtkLabel *lInfo;
	GtkWidget *btnWrap;
	GtkWidget *cEncoding;
	gchar *filename;
	GKeyFile *cfg;
} CustomData;

static char gCfgPath[PATH_MAX];
static GtkSourceStyleSchemeManager *gStyleManager = NULL;
static GtkSourceLanguageManager *gLanguageManager = NULL;
GtkWrapMode gWrapMode;

static gboolean open_file(CustomData *data, const gchar *filename, const gchar *enc);

static void reload_with_enc_cb(GtkComboBoxText *combo_box, CustomData *data)
{
	gchar *info_txt = NULL;
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(data->sBuf), "", 0);
	gchar *encname = gtk_combo_box_text_get_active_text(combo_box);

	if (!open_file(data, data->filename, encname))
		info_txt = g_strdup_printf(_("Failed to load file"));
	else if (g_strcmp0(encname, _("Default")) != 0)
		info_txt = g_strdup_printf("%s\t%s [%s]", gtk_label_get_text(data->lInfo), _("Encoding:"), encname);

	if (info_txt)
	{
		gtk_label_set_text(data->lInfo, info_txt);
		g_free(info_txt);
	}

	if (encname)
		g_free(encname);
}

static void wrap_line_cb(GtkToggleButton *tbutton, CustomData *data)
{
	gboolean value = gtk_toggle_button_get_active(tbutton);

	if (value)
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(data->sView), gWrapMode);
	else
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(data->sView), GTK_WRAP_NONE);
}

static void hcur_line_cb(GtkToggleButton *tbutton, CustomData *data)
{
	gboolean value = gtk_toggle_button_get_active(tbutton);
	g_key_file_set_boolean(data->cfg, "Flags", "HighlightCurLine", value);
	gtk_source_view_set_highlight_current_line(data->sView, value);
}

static void line_num_cb(GtkToggleButton *tbutton, CustomData *data)
{
	gboolean value = gtk_toggle_button_get_active(tbutton);
	g_key_file_set_boolean(data->cfg, "Flags", "LineNumbers", value);
	gtk_source_view_set_show_line_numbers(data->sView, value);
}

static void draw_spaces_cb(GtkToggleButton *tbutton, CustomData *data)
{
	gboolean value = gtk_toggle_button_get_active(tbutton);
	g_key_file_set_boolean(data->cfg, "Flags", "Spaces", value);

	if (value)
		gtk_source_view_set_draw_spaces(data->sView, GTK_SOURCE_DRAW_SPACES_ALL);
	else
		gtk_source_view_set_draw_spaces(data->sView, 0);
}

static void cursor_cb(GtkToggleButton *tbutton, CustomData *data)
{
	gboolean value = gtk_toggle_button_get_active(tbutton);
	g_key_file_set_boolean(data->cfg, "Flags", "NoCursor", !value);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(data->sView), value);
}

static void s_tab_cb(GtkSpinButton *spin_button, CustomData *data)
{
	gint value = gtk_spin_button_get_value_as_int(spin_button);
	g_key_file_set_integer(data->cfg, "Appearance", "TabSize", value);
	gtk_source_view_set_tab_width(data->sView, value);
}

static gchar *config_get_string(GKeyFile *cfg, const gchar *group, const gchar *key, const gchar *default_value)
{
	gchar *result = g_key_file_get_string(cfg, group, key, NULL);

	if (!result)
	{
		g_key_file_set_string(cfg, group, key, default_value);
		result = g_strdup(default_value);
	}

	return result;
}

static gboolean config_get_boolean(GKeyFile *cfg, const gchar *group, const gchar *key, gboolean default_value)
{
	GError *err = NULL;
	gboolean result = g_key_file_get_boolean(cfg, group, key, &err);

	if (err)
	{
		g_error_free(err);
		g_key_file_set_boolean(cfg, group, key, default_value);
		return default_value;
	}

	return result;
}

static gint config_get_integer(GKeyFile *cfg, const gchar *group, const gchar *key, gint default_value)
{
	GError *err = NULL;
	gint result = g_key_file_get_integer(cfg, group, key, &err);

	if (err)
	{
		g_error_free(err);
		g_key_file_set_integer(cfg, group, key, default_value);
		return default_value;
	}

	return result;
}

static void font_changed_cb(GtkFontButton *font_button, CustomData *data)
{
	const gchar *font = gtk_font_button_get_font_name(font_button);
	g_key_file_set_string(data->cfg, "Appearance", "Font", font);
	PangoFontDescription *desc = pango_font_description_from_string(font);
	gtk_widget_modify_font(GTK_WIDGET(data->sView), desc);
	pango_font_description_free(desc);
}

static void scheme_changed_cb(GtkComboBoxText *combo_box, CustomData *data)
{
	gchar *style = gtk_combo_box_text_get_active_text(combo_box);
	g_key_file_set_string(data->cfg, "Appearance", "Style", style);
	GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(gStyleManager, style);
	gtk_source_buffer_set_style_scheme(data->sBuf, scheme);
	g_free(style);
}

static void pabove_changed_cb(GtkSpinButton *spin_button, CustomData *data)
{
	gint value = gtk_spin_button_get_value_as_int(spin_button);
	g_key_file_set_integer(data->cfg, "Appearance", "PAbove", value);
	gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(data->sView), value);
}

static void pbelow_changed_cb(GtkSpinButton *spin_button, CustomData *data)
{
	gint value = gtk_spin_button_get_value_as_int(spin_button);
	g_key_file_set_integer(data->cfg, "Appearance", "PBelow", value);
	gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(data->sView), value);
}

static void lang_changed_cb(GtkComboBoxText *combo_box, CustomData *data)
{
	gchar *lang = gtk_combo_box_text_get_active_text(combo_box);
	g_key_file_set_string(data->cfg, "Enca", "Lang", lang);
	g_free(lang);
}

static void options_dlg_cb(GtkButton *button, CustomData *data)
{
	GtkWindow *window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(data->sView)));
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Options"), window, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, NULL);


	GtkWidget *vMain = gtk_vbox_new(FALSE, 5);
	GtkWidget *hMain = gtk_hbox_new(FALSE, 5);
	GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	GtkWidget *fFont = gtk_frame_new(_("Font"));
	gtk_frame_set_shadow_type(GTK_FRAME(fFont), GTK_SHADOW_NONE);
	gchar *font = config_get_string(data->cfg, "Appearance", "Font", "monospace 12");
	GtkWidget *bFont = gtk_font_button_new_with_font(font);
	gtk_container_add(GTK_CONTAINER(fFont), bFont);
	g_free(font);
	g_signal_connect(G_OBJECT(bFont), "font-set", G_CALLBACK(font_changed_cb), (gpointer)data);

	GtkWidget *fScheme = gtk_frame_new(_("Style"));
	gtk_frame_set_shadow_type(GTK_FRAME(fScheme), GTK_SHADOW_NONE);
	GtkWidget *cScheme = gtk_combo_box_text_new_with_entry();
	gtk_container_add(GTK_CONTAINER(fScheme), cScheme);
	gchar *style = config_get_string(data->cfg, "Appearance", "Style", "classic");
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(cScheme))), style);
	gtk_editable_set_editable(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(cScheme))), FALSE);
	g_free(style);
	g_signal_connect(G_OBJECT(cScheme), "changed", G_CALLBACK(scheme_changed_cb), (gpointer)data);
	const gchar * const *scheme_ids = gtk_source_style_scheme_manager_get_scheme_ids(gStyleManager);

	while (*scheme_ids)
	{
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cScheme), *scheme_ids);
		*scheme_ids++;
	}

	GtkWidget *fPixels = gtk_frame_new(_("Blank space"));
	gtk_frame_set_shadow_type(GTK_FRAME(fPixels), GTK_SHADOW_NONE);
	GtkWidget *tPixels = gtk_table_new(2, 3, FALSE);
	GtkWidget *sAbove = gtk_spin_button_new_with_range(0, 99, 1);
	GtkWidget *sBelow = gtk_spin_button_new_with_range(0, 99, 1);
	GtkWidget *sTabSize = gtk_spin_button_new_with_range(1, 32, 1);
	GtkWidget *lAbove = gtk_label_new(_("Above paragraphs"));
	GtkWidget *lBelow = gtk_label_new(_("Below paragraphs"));
	GtkWidget *lTabSize = gtk_label_new(_("Tab width"));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(sAbove), (gdouble)config_get_integer(data->cfg, "Appearance", "PAbove", 0));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(sBelow), (gdouble)config_get_integer(data->cfg, "Appearance", "PBelow", 0));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(sTabSize), (gdouble)config_get_integer(data->cfg, "Appearance", "TabSize", 0));
	gtk_misc_set_alignment(GTK_MISC(lAbove), 0, 0.5);
	gtk_misc_set_alignment(GTK_MISC(lBelow), 0, 0.5);
	gtk_misc_set_alignment(GTK_MISC(lTabSize), 0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(tPixels), lAbove, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tPixels), lBelow, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(tPixels), lTabSize, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(tPixels), sAbove, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(tPixels), sBelow, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(tPixels), sTabSize, 1, 2, 2, 3);
	gtk_table_set_row_spacings(GTK_TABLE(tPixels), 2);
	gtk_table_set_col_spacings(GTK_TABLE(tPixels), 5);
	gtk_container_add(GTK_CONTAINER(fPixels), tPixels);
	g_signal_connect(G_OBJECT(sAbove), "changed", G_CALLBACK(pabove_changed_cb), (gpointer)data);
	g_signal_connect(G_OBJECT(sBelow), "changed", G_CALLBACK(pbelow_changed_cb), (gpointer)data);
	g_signal_connect(G_OBJECT(sTabSize), "value-changed", G_CALLBACK(s_tab_cb), (gpointer)data);

	GtkWidget *fEnca = gtk_frame_new(_("Enca Lang"));
	gtk_frame_set_shadow_type(GTK_FRAME(fEnca), GTK_SHADOW_NONE);
	GtkWidget *cEnca = gtk_combo_box_text_new_with_entry();
	gtk_container_add(GTK_CONTAINER(fEnca), cEnca);
	gchar *lang = config_get_string(data->cfg, "Enca", "Lang", "__");
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(cEnca))), lang);
	gtk_editable_set_editable(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(cEnca))), FALSE);
	g_free(lang);
	g_signal_connect(G_OBJECT(cEnca), "changed", G_CALLBACK(lang_changed_cb), (gpointer)data);
	size_t langscount;
	const char **langs = enca_get_languages(&langscount);

	for (size_t i = 0; i < langscount; i++)
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cEnca), langs[i]);

	gtk_box_pack_start(GTK_BOX(vMain), fFont, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vMain), fScheme, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vMain), fPixels, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vMain), fEnca, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hMain), vMain, FALSE, FALSE, 5);
	gtk_container_add(GTK_CONTAINER(content_area), hMain);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_widget_show_all(dialog);
}


static gboolean open_file(CustomData *data, const gchar *filename, const gchar *enc)
{
	GtkSourceLanguage *language = NULL;
	GError *err = NULL;
	GtkTextIter iter;
	gchar *buffer;
	gchar *ext;
	const gchar *content_type;

	GtkSourceBuffer *sBuf = data->sBuf;

	g_return_val_if_fail(sBuf != NULL, FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(GTK_SOURCE_BUFFER(sBuf), FALSE);

	GFile *gfile = g_file_new_for_path(filename);
	g_return_val_if_fail(gfile != NULL, FALSE);
	GFileInfo *fileinfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL, NULL);
	g_return_val_if_fail(fileinfo != NULL, FALSE);

	content_type = g_file_info_get_content_type(fileinfo);

	language = gtk_source_language_manager_guess_language(gLanguageManager, filename, content_type);

	g_object_unref(fileinfo);
	g_object_unref(gfile);

	if (!language)
	{
		ext = g_strrstr(filename, ".");

		if (ext != NULL)
		{
			gchar *ext_lower = g_ascii_strdown(ext + 1, -1);
			gchar *lng = g_key_file_get_string(data->cfg, "Override", ext_lower, NULL);

			if (lng && lng[0] != '\0')
			{
				language = gtk_source_language_manager_get_language(gLanguageManager, lng);
				g_free(lng);
			}

			g_free(ext_lower);
		}
	}

	if (!language)
		return FALSE;

	gtk_source_buffer_set_language(sBuf, language);
	gchar *info_txt = g_strdup_printf("%s [%s]", _("Language:"), gtk_source_language_get_name(language));
	gtk_label_set_text(data->lInfo, info_txt);
	g_free(info_txt);


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
			gchar *enca_lang = config_get_string(data->cfg, "Enca", "Lang", "__");
			analyser = enca_analyser_alloc(enca_lang);

			if (analyser)
			{
				enca_set_threshold(analyser, 1.38);
				enca_set_multibyte(analyser, 1);
				enca_set_ambiguity(analyser, 1);
				enca_set_garbage_test(analyser, 1);

				gboolean no_filter = config_get_boolean(data->cfg, "Enca", "NoFilters", FALSE);

				if (no_filter)
					enca_set_filtering(analyser, 0);

				encoding = enca_analyse(analyser, (unsigned char*)buffer, (size_t)bytes_read);

				info_txt = g_strdup_printf("%s\t%s [%s]", gtk_label_get_text(data->lInfo),
				                           _("Encoding:"), enca_charset_name(encoding.charset, ENCA_NAME_STYLE_ICONV));
				gtk_label_set_text(data->lInfo, info_txt);
				g_free(info_txt);

				if (encoding.charset != -1 && encoding.charset != 27)
				{
					gchar *converted = g_convert_with_fallback(buffer, bytes_read, "UTF-8",
					                   enca_charset_name(encoding.charset, ENCA_NAME_STYLE_ICONV), NULL, NULL, &bytes_read, &err);

					if (err)
						g_print("gtksourceview.wlx (%s): %s\n", filename, (err)->message);

					g_free(buffer);
					buffer = converted;
				}
				else if (encoding.charset == -1)
				{
					gchar *force_charset = config_get_string(data->cfg, "Enca", "ForceCharSet", "");

					if (force_charset && force_charset[0] != '\0')
					{
						gchar *converted = g_convert_with_fallback(buffer, bytes_read, "UTF-8",
						                   force_charset, NULL, NULL, &bytes_read, &err);

						if (err)
							g_print("gtksourceview.wlx (%s): %s\n", filename, (err)->message);

						g_free(buffer);
						buffer = converted;
						g_free(force_charset);
					}
					else
					{
						enca_analyser_free(analyser);
						g_free(buffer);
						return FALSE;
					}
				}

				enca_analyser_free(analyser);
				g_free(enca_lang);
			}
		}
		else
		{
			gchar *converted = g_convert_with_fallback(buffer, bytes_read, "UTF-8", enc, NULL, NULL, &bytes_read, &err);

			if (err)
				g_print("gtksourceview.wlx (%s): %s\n", filename, (err)->message);

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




HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *pScrollWin;
	GtkSourceStyleScheme *scheme;
	CustomData *data;

	data = g_new0(CustomData, 1);
	gFix = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	g_object_set_data(G_OBJECT(gFix), "custom-data", data);

	/* Create a Scrolled Window that will contain the GtkSourceView */
	pScrollWin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollWin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	/* and a GtkSourceBuffer to hold text (similar to GtkTextBuffer) */
	data->sBuf = GTK_SOURCE_BUFFER(gtk_source_buffer_new(NULL));

	/* Create the GtkSourceView and associate it with the buffer */
	data->sView = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(data->sBuf));


	/* Attach the GtkSourceView to the scrolled Window */
	gtk_container_add(GTK_CONTAINER(pScrollWin), GTK_WIDGET(data->sView));
	gtk_container_add(GTK_CONTAINER(gFix), pScrollWin);

	data->cfg = g_key_file_new();
	g_key_file_load_from_file(data->cfg, gCfgPath, G_KEY_FILE_KEEP_COMMENTS, NULL);

	data->lInfo = GTK_LABEL(gtk_label_new(NULL));

	if (!open_file(data, FileToLoad, NULL))
	{
		gtk_widget_destroy(GTK_WIDGET(data->lInfo));
		gtk_widget_destroy(GTK_WIDGET(gFix));
		g_free(data);
		return NULL;
	}

	data->filename = g_strdup(FileToLoad);




	gchar *font = config_get_string(data->cfg, "Appearance", "Font", "monospace 12");
	PangoFontDescription *desc = pango_font_description_from_string(font);
	gtk_widget_modify_font(GTK_WIDGET(data->sView), desc);
	pango_font_description_free(desc);
	g_free(font);

	gchar *style = config_get_string(data->cfg, "Appearance", "Style", "classic");
	scheme = gtk_source_style_scheme_manager_get_scheme(gStyleManager, style);
	g_free(style);
	gtk_source_buffer_set_style_scheme(data->sBuf, scheme);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(data->sView), FALSE);

	gboolean no_cursor = config_get_boolean(data->cfg, "Flags", "NoCursor", TRUE);

	if (no_cursor)
		gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(data->sView), FALSE);

	gboolean line_num = config_get_boolean(data->cfg, "Flags", "LineNumbers", TRUE);
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(data->sView), line_num);
	gint s_tab = config_get_integer(data->cfg, "Appearance", "TabSize", 8);
	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(data->sView), s_tab);

	gboolean hcur_line = config_get_boolean(data->cfg, "Flags", "HighlightCurLine", TRUE);
	gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(data->sView), hcur_line);

	gboolean draw_spaces = config_get_boolean(data->cfg, "Flags", "Spaces", TRUE);

	if (draw_spaces)
		gtk_source_view_set_draw_spaces(GTK_SOURCE_VIEW(data->sView), GTK_SOURCE_DRAW_SPACES_ALL);

	gboolean wrap_line = ShowFlags & lcp_wraptext;


	if (g_key_file_get_boolean(data->cfg, "Flags", "WrapWordChar", NULL))
		gWrapMode = GTK_WRAP_WORD_CHAR;
	else
		gWrapMode = GTK_WRAP_WORD;


	if (wrap_line)
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(data->sView), gWrapMode);

	gint p_above = config_get_integer(data->cfg, "Appearance", "PAbove", 0);
	gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(data->sView), p_above);
	gint p_below = config_get_integer(data->cfg, "Appearance", "PBelow", 0);
	gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(data->sView), p_below);

	GtkWidget *hControlBox = gtk_hbox_new(FALSE, 5);
	GtkWidget *hEncodingBox = gtk_hbox_new(FALSE, 0);
	data->cEncoding = gtk_combo_box_text_new();

	gchar *value = config_get_string(data->cfg, "Enca", "Encodings", "CP1251;866;ISO_8859-1;");
	g_free(value);

	gchar **encodings = g_key_file_get_string_list(data->cfg, "Enca", "Encodings", NULL, NULL);

	gchar **p = encodings;
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->cEncoding), _("Default"));

	while (*p)
	{
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->cEncoding), *p);
		*p++;
	}

	g_free(encodings);

	gtk_widget_set_tooltip_text(data->cEncoding, _("Custom encoding"));
	g_signal_connect(G_OBJECT(data->cEncoding), "changed", G_CALLBACK(reload_with_enc_cb), (gpointer)data);
	gtk_box_pack_start(GTK_BOX(hControlBox), GTK_WIDGET(data->lInfo), FALSE, FALSE, 5);
	gtk_box_pack_end(GTK_BOX(hEncodingBox), data->cEncoding, FALSE, FALSE, 0);

	gboolean quickview = FALSE;

	const gchar *role = gtk_window_get_role(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ParentWin))));

	if (g_strcmp0(role, "TfrmViewer") != 0)
		quickview = TRUE;

	if (!quickview)
	{
		GtkToolItem *btnCfg = gtk_tool_button_new_from_stock(GTK_STOCK_PROPERTIES);
		g_signal_connect(G_OBJECT(btnCfg), "clicked", G_CALLBACK(options_dlg_cb), (gpointer)data);
		gtk_box_pack_end(GTK_BOX(hControlBox), GTK_WIDGET(btnCfg), FALSE, FALSE, 5);
		gtk_widget_set_tooltip_text(GTK_WIDGET(btnCfg), _("Options"));
	}

	gtk_box_pack_end(GTK_BOX(hControlBox), hEncodingBox, FALSE, FALSE, 5);

	if (!quickview)
	{
		GtkWidget *btnSpaces = gtk_toggle_button_new_with_label(_("Draw Spaces"));
		GtkWidget *btnCursor = gtk_toggle_button_new_with_label(_("Text Cursor"));
		GtkWidget *btnLineNum = gtk_toggle_button_new_with_label(_("Line Numbers"));
		GtkWidget *btnHglLine = gtk_toggle_button_new_with_label(_("Highlight Line"));
		data->btnWrap = gtk_toggle_button_new_with_label(_("Wrap Line"));

		gtk_box_pack_end(GTK_BOX(hControlBox), data->btnWrap, FALSE, FALSE, 2);
		gtk_box_pack_end(GTK_BOX(hControlBox), btnHglLine, FALSE, FALSE, 2);
		gtk_box_pack_end(GTK_BOX(hControlBox), btnLineNum, FALSE, FALSE, 2);
		gtk_box_pack_end(GTK_BOX(hControlBox), btnSpaces, FALSE, FALSE, 2);
		gtk_box_pack_end(GTK_BOX(hControlBox), btnCursor, FALSE, FALSE, 2);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->btnWrap), wrap_line);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btnHglLine), hcur_line);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btnLineNum), line_num);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btnSpaces), draw_spaces);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btnCursor), !no_cursor);
		g_signal_connect(G_OBJECT(data->btnWrap), "toggled", G_CALLBACK(wrap_line_cb), (gpointer)data);
		g_signal_connect(G_OBJECT(btnHglLine), "toggled", G_CALLBACK(hcur_line_cb), (gpointer)data);
		g_signal_connect(G_OBJECT(btnLineNum), "toggled", G_CALLBACK(line_num_cb), (gpointer)data);
		g_signal_connect(G_OBJECT(btnSpaces), "toggled", G_CALLBACK(draw_spaces_cb), (gpointer)data);
		g_signal_connect(G_OBJECT(btnCursor), "toggled", G_CALLBACK(cursor_cb), (gpointer)data);
	}

	gtk_box_pack_end(GTK_BOX(gFix), hControlBox, FALSE, FALSE, 5);
	gtk_widget_grab_focus(GTK_WIDGET(data->sView));

	gtk_widget_show_all(gFix);

	gboolean hide_controls = config_get_boolean(data->cfg, "Flags", "HideControls", FALSE);

	if (hide_controls)
		gtk_widget_hide(GTK_WIDGET(hControlBox));

	return gFix;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	CustomData *data = (CustomData*)g_object_get_data(G_OBJECT(PluginWin), "custom-data");

	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(data->sBuf), "", 0);

	gtk_combo_box_set_active(GTK_COMBO_BOX(data->cEncoding), -1);

	g_free(data->filename);
	data->filename = g_strdup(FileToLoad);

	if (!open_file(data, FileToLoad, NULL))
		return LISTPLUGIN_ERROR;

	//~ esh: delete search mark for next/prev doc
	GtkTextMark *last_pos;
	last_pos = gtk_text_buffer_get_mark(GTK_TEXT_BUFFER(data->sBuf), "last_pos");

	if (last_pos != NULL)
		gtk_text_buffer_delete_mark(GTK_TEXT_BUFFER(data->sBuf), last_pos);

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	CustomData *data = (CustomData*)g_object_get_data(G_OBJECT(ListWin), "custom-data");

	gchar *value = config_get_string(data->cfg, "Enca", "ForceCharSet", "");
	g_free(value);

	g_key_file_save_to_file(data->cfg, gCfgPath, NULL);
	g_key_file_free(data->cfg);

	gtk_widget_destroy(GTK_WIDGET(ListWin));
	g_free(data->filename);

	if (G_IS_OBJECT(data->sBuf))
		g_object_unref(data->sBuf);

	g_free(data);
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	GtkSourceBuffer *sBuf;
	GtkTextMark *last_pos;
	GtkTextIter iter, mstart, mend;
	gboolean found;

	CustomData *data = (CustomData*)g_object_get_data(G_OBJECT(ListWin), "custom-data");
	sBuf = data->sBuf;
	last_pos = gtk_text_buffer_get_mark(GTK_TEXT_BUFFER(sBuf), "last_pos");

	if (last_pos == NULL || SearchParameter & lcs_findfirst)
	{
		if (SearchParameter & lcs_backwards)
			gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(sBuf), &iter);
		else
			gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(sBuf), &iter);
	}
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
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(data->sView),
		                                   gtk_text_buffer_get_mark(GTK_TEXT_BUFFER(sBuf), "last_pos"));

	}
	else
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ListWin))),
		                    GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		                    _("\"%s\" not found!"), SearchString);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	GtkSourceBuffer *sBuf;
	GtkTextIter p;

	CustomData *data = (CustomData*)g_object_get_data(G_OBJECT(ListWin), "custom-data");
	sBuf = data->sBuf;

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

	case lc_newparams :
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->btnWrap), Parameter & lcp_wraptext);
		break;

	default :
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	GKeyFile *cfg;
	gchar *oldcfg = NULL, *bakcfg = NULL;
	gchar lcode[3], *loc = NULL;
	gboolean import_oldcfg;

	if (!gLanguageManager || !gStyleManager)
	{
		gLanguageManager = gtk_source_language_manager_get_default();
		gStyleManager = gtk_source_style_scheme_manager_get_default();

		cfg = g_key_file_new();
		memset(&dlinfo, 0, sizeof(dlinfo));

		if (dladdr(gCfgPath, &dlinfo) != 0)
		{
			gchar *plugpath = g_path_get_dirname(dlinfo.dli_fname);
			gchar *langpath = g_strdup_printf("%s/langs", plugpath);
			loc = setlocale(LC_ALL, "");
			bindtextdomain(GETTEXT_PACKAGE, langpath);
			textdomain(GETTEXT_PACKAGE);

			oldcfg = g_strdup_printf("%s/settings.ini", plugpath);
			bakcfg = g_strdup_printf("%s/settings.ini.bak", plugpath);
			import_oldcfg = g_key_file_load_from_file(cfg, oldcfg, G_KEY_FILE_KEEP_COMMENTS, NULL);

			g_free(plugpath);
			g_free(langpath);
		}

		if (gCfgPath[0] == '\0')
		{
			g_strlcpy(gCfgPath, dps->DefaultIniName, PATH_MAX - 1);
			char *pos = strrchr(gCfgPath, '/');

			if (pos)
				strcpy(pos + 1, "wlx_gtksourceview.ini");

			if (import_oldcfg)
			{
				gsize length;
				gchar **keys = g_key_file_get_keys(cfg, "Override", &length, NULL);

				if (keys != NULL)
				{
					for (gsize i = 0; i < length; i++)
					{
						gchar *exts = g_key_file_get_string(cfg, "Override", keys[i], NULL);
						gchar **split = g_strsplit(exts, ";", -1);

						for (gchar **ext = split; *ext != NULL; *ext++)
						{
							if (*ext[0] != '\0')
							{
								gchar *ext_lower = g_ascii_strdown(*ext + 1, -1);
								gchar *lang_lower = g_ascii_strdown(keys[i], -1);
								g_key_file_set_string(cfg, "Override", ext_lower,  lang_lower);
								g_free(ext_lower);
								g_free(lang_lower);
							}
						}

						g_strfreev(split);
						g_free(exts);
						g_key_file_remove_key(cfg, "Override", keys[i], NULL);
					}

					g_strfreev(keys);
				}

				if (bakcfg && oldcfg)
					rename(oldcfg, bakcfg);
			}
			else
			{
				g_key_file_load_from_file(cfg, gCfgPath, G_KEY_FILE_KEEP_COMMENTS, NULL);

				if (!g_key_file_has_key(cfg, "Enca", "Lang", NULL))
				{
					if (loc)
						g_strlcpy(lcode, loc, 3);

					size_t langscount;
					const char **langs = enca_get_languages(&langscount);

					for (size_t i = 0; i < langscount; i++)
						if (g_strcmp0(langs[i], lcode) == 0)
							g_key_file_set_string(cfg, "Enca", "Lang", langs[i]);
				}

				if (g_key_file_has_key(cfg, "Flags", "Wrap", NULL))
				{
					g_key_file_set_boolean(cfg, "Flags", "WrapWordChar", FALSE);
					g_key_file_remove_key(cfg, "Flags", "Wrap", NULL);
				}

				if (!g_key_file_has_group(cfg, "Override") || g_key_file_has_key(cfg, "Override", "Pascal", NULL))
				{
					g_key_file_set_string(cfg, "Override", "lpr", "pascal");
					g_key_file_set_comment(cfg, "Override", "lpr", "ext=language", NULL);
					g_key_file_set_string(cfg, "Override", "pp", "pascal");
					g_key_file_set_string(cfg, "Override", "dpr", "pascal");
					g_key_file_set_string(cfg, "Override", "lpi", "xml");
					g_key_file_set_string(cfg, "Override", "lpk", "xml");
					g_key_file_set_string(cfg, "Override", "hgl", "xml");
					g_key_file_set_string(cfg, "Override", "lfm", "ini");
					g_key_file_set_string(cfg, "Override", "dof", "ini");
					g_key_file_set_string(cfg, "Override", "cfg", "ini");
					g_key_file_remove_key(cfg, "Override", "Pascal", NULL);
					g_key_file_remove_key(cfg, "Override", "XML", NULL);
					g_key_file_remove_key(cfg, "Override", "INI", NULL);

				}
			}

			g_key_file_save_to_file(cfg, gCfgPath, NULL);
		}

		if (oldcfg)
			g_free(oldcfg);

		if (bakcfg)
			g_free(bakcfg);

		g_key_file_free(cfg);
	}
}
