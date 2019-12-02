/**
*      @brief Too simple PDF Viewer
*
*      Copyright 2009 Yasser Nour <yassernour.wordpress.com>
*
*      This program is free software; you can redistribute it and/or modify
*      it under the terms of the GNU General Public License as published by
*      the Free Software Foundation; either version 2 of the License, or
*      (at your option) any later version.
*
*      This program is distributed in the hope that it will be useful,
*      but WITHOUT ANY WARRANTY; without even the implied warranty of
*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*      GNU General Public License for more details.
*
*      You should have received a copy of the GNU General Public License
*      along with this program; if not, write to the Free Software
*      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
*      MA 02110-1301, USA.
*
*      https://yassernour.wordpress.com/2010/04/04/how-hard-to-build-a-pdf-viewer/
*/

#define _GNU_SOURCE

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <cairo.h>
#include <poppler.h>
#include "wlxplugin.h"

#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

#define _detectstring "EXT=\"PDF\""
#define page_padding 25

typedef struct _CustomData
{
	PopplerDocument *document;
	cairo_surface_t *surface;

	GtkWidget *scrolled_window;
	GtkWidget *canvas;
	GtkWidget *lbl_info;
	GtkWidget *btn_scale;
	GtkWidget *btn_spin;

	GtkToolItem *btn_fit;

	guint current_page;
	guint total_pages;
	guint alloc_width;
	guint alloc_height;

} CustomData;

static gboolean canvas_expose_event(GtkWidget *widget, GdkEventExpose *event, CustomData *data)
{
	gdk_window_clear(widget->window);
	cairo_t *cr = gdk_cairo_create(widget->window);
	cairo_set_source_surface(cr, data->surface, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);
	return TRUE;
}

static void view_set_page(guint page_num, CustomData *data)
{
	PopplerPage *page;
	gdouble width, height, new_width, new_height, scale;
	gboolean sig_value, scale_set, fit_page;
	guint pbox_width, pbox_height;
	cairo_t *cr;
	gchar *info;

	page = poppler_document_get_page(data->document, page_num);
	poppler_page_get_size(page, &width, &height);

	pbox_width = data->alloc_width;
	pbox_height = data->alloc_height;
	fit_page = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(data->btn_fit));

	if (pbox_width > page_padding)
		pbox_width = pbox_width - page_padding;

	if (pbox_height > page_padding)
		pbox_height = pbox_height - page_padding;

	if (fit_page)
	{
		if (height != 0)
			scale = pbox_height / height;

		new_width = width * scale;
		new_height = pbox_height;
	}

	if (!fit_page || new_width > pbox_width)
	{
		if (width != 0)
			scale = pbox_width / width;

		new_width = pbox_width;
		new_height = height * scale;
	}

	cairo_surface_destroy(data->surface);
	scale_set = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->btn_scale));

	if (scale_set)
	{
		gtk_widget_set_size_request(data->canvas, (guint)new_width, (guint)new_height);
		data->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
		                (guint)new_width, (guint)new_height);
		cr = cairo_create(data->surface);
		cairo_scale(cr, scale, scale);
	}
	else
	{
		gtk_widget_set_size_request(data->canvas, (guint)width, (guint)height);
		data->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
		                (guint)width, (guint)height);
		cr = cairo_create(data->surface);
	}

	poppler_page_render(page, cr);
	cairo_destroy(cr);
	gtk_widget_queue_draw(data->canvas);

	data->current_page = page_num;

	info = g_strdup_printf(" / %d : %dx%d  ", data->total_pages, (guint)width, (guint)height);
	gtk_label_set_text(GTK_LABEL(data->lbl_info), info);
	g_free(info);
	info = g_strdup_printf("%s%.1f", _("Scale ~x"), scale);
	gtk_button_set_label(GTK_BUTTON(data->btn_scale), info);
	g_free(info);

	g_signal_emit_by_name(G_OBJECT(data->scrolled_window), "scroll-child",
	                      GTK_SCROLL_START, FALSE, &sig_value);
	g_signal_emit_by_name(G_OBJECT(data->scrolled_window), "scroll-child",
	                      GTK_SCROLL_START, TRUE, &sig_value);
	g_object_unref(page);
}

static void tb_spin_changed(GtkSpinButton *spin_button, CustomData *data)
{
	view_set_page(gtk_spin_button_get_value_as_int(spin_button) - 1, data);
}

static void tb_first_clicked(GtkToolItem *toolbtn, CustomData *data)
{
	gtk_spin_button_spin(GTK_SPIN_BUTTON(data->btn_spin), GTK_SPIN_HOME, 1);
}

static void tb_last_clicked(GtkToolItem *toolbtn, CustomData *data)
{
	gtk_spin_button_spin(GTK_SPIN_BUTTON(data->btn_spin), GTK_SPIN_END, 1);
}

static void tb_back_clicked(GtkToolItem *toolbtn, CustomData *data)
{
	gtk_spin_button_spin(GTK_SPIN_BUTTON(data->btn_spin), GTK_SPIN_STEP_BACKWARD, 1);
}

static void tb_forward_clicked(GtkToolItem *toolbtn, CustomData *data)
{
	gtk_spin_button_spin(GTK_SPIN_BUTTON(data->btn_spin), GTK_SPIN_STEP_FORWARD, 1);
}

static void tb_fit_clicked(GtkToggleToolButton *toolbtn, CustomData *data)
{
	if (gtk_toggle_tool_button_get_active(toolbtn))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->btn_scale), TRUE);

	view_set_page(data->current_page, data);
}

static void tb_checkbox_changed(GtkWidget *widget, CustomData *data)
{
	view_set_page(data->current_page, data);
}

static void on_size_allocate(GtkWidget *widget, GtkAllocation *allocation, CustomData *data)
{
	data->alloc_width = allocation->width;
	data->alloc_height = allocation->height;
	view_set_page(data->current_page, data);
}

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, CustomData *data)
{
	gboolean value;

	switch (event->keyval)
	{
	case GDK_Up:
		g_signal_emit_by_name(G_OBJECT(data->scrolled_window), "scroll-child",
		                      GTK_SCROLL_STEP_BACKWARD, FALSE, &value);
		return TRUE;

	case GDK_Down:
		g_signal_emit_by_name(G_OBJECT(data->scrolled_window), "scroll-child",
		                      GTK_SCROLL_STEP_FORWARD, FALSE, &value);
		return TRUE;

	case GDK_Right:
		g_signal_emit_by_name(G_OBJECT(data->scrolled_window), "scroll-child",
		                      GTK_SCROLL_STEP_FORWARD, TRUE, &value);
		return TRUE;

	case GDK_Left:
		g_signal_emit_by_name(G_OBJECT(data->scrolled_window), "scroll-child",
		                      GTK_SCROLL_STEP_BACKWARD, TRUE, &value);
		return TRUE;

	case GDK_Home:
		tb_first_clicked(NULL, data);
		return TRUE;

	case GDK_End:
		tb_last_clicked(NULL, data);
		return TRUE;

	case GDK_Page_Up:
		tb_back_clicked(NULL, data);
		return TRUE;

	case GDK_Page_Down:
		tb_forward_clicked(NULL, data);
		return TRUE;

	case GDK_Insert:
		value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->btn_scale));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->btn_scale), !value);
		return TRUE;

	case GDK_backslash:
		value = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(data->btn_fit));
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(data->btn_fit), !value);
		return TRUE;
	}

	return FALSE;
}

static void tb_text_clicked(GtkToolItem *toolbtn, CustomData *data)
{
	GtkWidget *dialog;
	GtkWidget *scroll;
	GtkWidget *tView;
	GtkTextBuffer *tBuf;

	PopplerPage *ppage = poppler_document_get_page(data->document, data->current_page);
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), _("Text"));
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 400);
	gtk_container_border_width(GTK_CONTAINER(dialog), 2);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	tBuf = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text(tBuf, poppler_page_get_text(ppage), -1);
	tView = gtk_text_view_new_with_buffer(tBuf);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(tView), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(tView), FALSE);
	gtk_container_add(GTK_CONTAINER(scroll), tView);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scroll, TRUE, TRUE, 0);
	gtk_widget_show_all(dialog);
	g_object_unref(ppage);
}

static void tb_info_clicked(GtkToolItem *toolbtn, CustomData *data)
{
	GtkWidget *dialog;
	GtkWidget *scroll;
	GtkWidget *ilabel;
	GtkWidget *mlabel;
	gchar *mtext, *itext;

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), _("Info"));
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 400);
	gtk_container_border_width(GTK_CONTAINER(dialog), 5);
	itext = g_strdup_printf(_("Title: %s\nAuthor: %s\nSubject: %s\nCreator: %s\nProducer: %s\nKeywords: %s\nVersion: %s"),
	                        poppler_document_get_title(data->document), poppler_document_get_author(data->document),
	                        poppler_document_get_subject(data->document), poppler_document_get_creator(data->document),
	                        poppler_document_get_producer(data->document), poppler_document_get_keywords(data->document),
	                        poppler_document_get_pdf_version_string(data->document));

	ilabel = gtk_label_new(itext);
	gtk_misc_set_alignment(GTK_MISC(ilabel), 0, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), ilabel, FALSE, TRUE, 0);
	gtk_widget_show(ilabel);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scroll, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(scroll);
	mtext = poppler_document_get_metadata(data->document);

	if (!mtext)
		mtext = _("metadata not found");

	mlabel = gtk_label_new(mtext);
	gtk_misc_set_alignment(GTK_MISC(mlabel), 0, 0);
	gtk_label_set_selectable(GTK_LABEL(mlabel), TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(mlabel), FALSE);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), mlabel);
	gtk_widget_show(mlabel);
	gtk_widget_grab_focus(scroll);
	gtk_widget_show(dialog);
	g_free(mtext);
	g_free(itext);
}

static GtkWidget *create_ui(HWND ParentWin, CustomData *data)
{
	GtkWidget *main_vbox;
	GtkWidget *toolbar;
	GtkWidget *page_box;
	GtkWidget *page_frame;
	GtkToolItem *tb_back;
	GtkToolItem *tb_forward;
	GtkToolItem *tb_first;
	GtkToolItem *tb_last;
	GtkToolItem *tb_text;
	GtkToolItem *tb_info;
	GtkToolItem *tb_pages;
	GtkToolItem *tb_selector;
	GtkToolItem *tb_checkbox;
	GdkColor color;

	main_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER((GtkWidget *)(ParentWin)), main_vbox);

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(toolbar), TRUE);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 1);

	data->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(main_vbox), data->scrolled_window, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(data->scrolled_window),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_grab_focus(data->scrolled_window);
	g_signal_connect(G_OBJECT(data->scrolled_window), "size-allocate",
	                 G_CALLBACK(on_size_allocate), data);
	g_signal_connect(G_OBJECT(data->scrolled_window), "key_press_event",
	                 G_CALLBACK(on_key_press), data);

	data->canvas = gtk_drawing_area_new();
	gdk_color_parse("white", &color);
	gtk_widget_modify_bg(data->canvas, GTK_STATE_NORMAL, &color);
	page_box = gtk_vbox_new(TRUE, 0);
	page_frame = gtk_aspect_frame_new(NULL, 0.5, 0.5, 0, TRUE);
	gtk_box_pack_start(GTK_BOX(page_box), page_frame, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(page_frame), data->canvas);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(data->scrolled_window),
	                                      page_box);
	g_signal_connect(G_OBJECT(data->canvas), "expose_event",
	                 G_CALLBACK(canvas_expose_event), data);

	tb_first = gtk_tool_button_new_from_stock(GTK_STOCK_GOTO_FIRST);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_first, 0);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(tb_first), _("First page"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_first), _("First page"));
	g_signal_connect(G_OBJECT(tb_first), "clicked", G_CALLBACK(tb_first_clicked), data);

	tb_back = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_back, 1);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(tb_back), _("Previous page"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_back), _("Previous page"));
	g_signal_connect(G_OBJECT(tb_back), "clicked", G_CALLBACK(tb_back_clicked), data);

	tb_forward = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_forward, 2);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(tb_forward), _("Next page"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_forward), _("Next page"));
	g_signal_connect(G_OBJECT(tb_forward), "clicked", G_CALLBACK(tb_forward_clicked), data);

	tb_last = gtk_tool_button_new_from_stock(GTK_STOCK_GOTO_LAST);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_last, 3);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(tb_last), _("Last page"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_last), _("Last page"));
	g_signal_connect(G_OBJECT(tb_last), "clicked", G_CALLBACK(tb_last_clicked), data);

	tb_selector = gtk_tool_item_new();
	data->btn_spin = gtk_spin_button_new_with_range(1, (gdouble)data->total_pages, 1);
	gtk_editable_set_editable(GTK_EDITABLE(data->btn_spin), FALSE);
	gtk_widget_set_can_focus(data->btn_spin, FALSE);
	gtk_container_add(GTK_CONTAINER(tb_selector), data->btn_spin);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_selector, 4);
	g_signal_connect(G_OBJECT(data->btn_spin), "value-changed",
	                 G_CALLBACK(tb_spin_changed), data);

	tb_pages = gtk_tool_item_new();
	data->lbl_info = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(tb_pages), data->lbl_info);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_pages, 5);

	tb_checkbox = gtk_tool_item_new();
	data->btn_scale = gtk_check_button_new();
	gtk_widget_set_can_focus(data->btn_scale, FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->btn_scale), TRUE);
	gtk_container_add(GTK_CONTAINER(tb_checkbox), data->btn_scale);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_checkbox, 6);
	g_signal_connect(G_OBJECT(data->btn_scale), "toggled",
	                 G_CALLBACK(tb_checkbox_changed), data);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), 7);

	data->btn_fit = gtk_toggle_tool_button_new_from_stock(GTK_STOCK_ZOOM_FIT);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), data->btn_fit, 8);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(data->btn_fit), _("Fit"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(data->btn_fit), _("Fit"));
	g_signal_connect(G_OBJECT(data->btn_fit), "toggled", G_CALLBACK(tb_fit_clicked), data);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), 9);

	tb_text = gtk_tool_button_new_from_stock(GTK_STOCK_SELECT_ALL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_text, 10);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(tb_text), _("Text"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_text), _("Text"));
	g_signal_connect(G_OBJECT(tb_text), "clicked", G_CALLBACK(tb_text_clicked), data);

	tb_info = gtk_tool_button_new_from_stock(GTK_STOCK_INFO);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_info, 11);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(tb_info), _("Info"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_info), _("Info"));
	g_signal_connect(G_OBJECT(tb_info), "clicked", G_CALLBACK(tb_info_clicked), data);

	return main_vbox;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *main_vbox;
	gchar *fileUri;
	CustomData *data;

	data = g_new0(CustomData, 1);
	fileUri = g_filename_to_uri(FileToLoad, NULL, NULL);
	data->document = poppler_document_new_from_file(fileUri, NULL, NULL);
	data->total_pages = poppler_document_get_n_pages(data->document);

	if (fileUri)
		g_free(fileUri);

	if (!data->document)
	{
		g_free(data);
		return NULL;
	}

	main_vbox = create_ui(ParentWin, data);
	g_object_set_data(G_OBJECT(main_vbox), "custom-data", data);

	gtk_widget_show_all(main_vbox);

	view_set_page(0, data);

	return main_vbox;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	gchar *fileUri;
	CustomData *data;

	data = (CustomData*)g_object_get_data(G_OBJECT(PluginWin), "custom-data");
	fileUri = g_filename_to_uri(FileToLoad, NULL, NULL);
	data->document = poppler_document_new_from_file(fileUri, NULL, NULL);
	data->total_pages = poppler_document_get_n_pages(data->document);
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(data->btn_spin), 1, (gdouble)data->total_pages);

	if (fileUri)
		g_free(fileUri);

	if (!data->document)
		return LISTPLUGIN_ERROR;

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->btn_spin), 1);
	view_set_page(0, data);

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	CustomData *data;

	data = (CustomData*)g_object_get_data(G_OBJECT(ListWin), "custom-data");
	gtk_widget_destroy(GTK_WIDGET(ListWin));
	g_object_unref(data->document);
	g_free(data);
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen - 1);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	const gchar* dir_f = "%s/langs";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(dir_f, &dlinfo) != 0)
	{
		setlocale(LC_ALL, "");
		gchar *plugdir = g_path_get_dirname(dlinfo.dli_fname);
		gchar *langdir = g_strdup_printf(dir_f, plugdir);
		g_free(plugdir);
		bindtextdomain(GETTEXT_PACKAGE, langdir);
		g_free(langdir);
		textdomain(GETTEXT_PACKAGE);
	}
}
