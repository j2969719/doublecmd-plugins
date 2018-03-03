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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <cairo.h>
#include <poppler.h>
#include "wlxplugin.h"

#define _detectstring "EXT=\"PDF\""
#define kostyl 25

void reset_scroll(GtkScrolledWindow *scrolled_window)
{
	gboolean tmp;
	g_signal_emit_by_name(scrolled_window, "scroll-child", GTK_SCROLL_START, FALSE, &tmp);
	g_signal_emit_by_name(scrolled_window, "scroll-child", GTK_SCROLL_START, TRUE, &tmp);
}

static void canvas_expose_event(GtkWidget *widget)
{
	cairo_surface_t *surface = g_object_get_data(G_OBJECT(widget), "surface1");
	gdk_window_clear(widget->window);
	cairo_t *cr = gdk_cairo_create(widget->window);
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);
}

static void view_set_page(GtkWidget *canvas, guint page)
{
	gdouble width, height, scale;
	cairo_t *cr;

	PopplerDocument *document = g_object_get_data(G_OBJECT(canvas), "doc");
	PopplerPage *ppage = poppler_document_get_page(document, page);
	g_object_set_data_full(G_OBJECT(canvas), "page", ppage, (GDestroyNotify)g_object_unref);
	g_object_set_data(G_OBJECT(canvas), "cpage", GUINT_TO_POINTER(page));

	poppler_page_get_size(ppage, &width, &height);

	guint pbox_width = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(canvas), "pwidth"));
	GtkWidget *chkscale = g_object_get_data(G_OBJECT(canvas), "pchkorg");

	if (pbox_width > kostyl)
		pbox_width = pbox_width - kostyl;

	if (width != 0)
		scale = pbox_width / width;

	cairo_surface_t *surface = g_object_get_data(G_OBJECT(canvas), "surface1");
	cairo_surface_destroy(surface);
	gchar *pstr = g_strdup_printf("Scale ~x%.1f", scale);
	gtk_button_set_label(GTK_BUTTON(chkscale), pstr);

	if ((pbox_width > kostyl) && (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkscale))))
	{
		gtk_widget_set_size_request(canvas, pbox_width, (guint)height * scale);
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (guint)pbox_width, (guint)height * scale);
		cr = cairo_create(surface);
		cairo_scale(cr, scale, scale);
	}
	else
	{
		gtk_widget_set_size_request(canvas, (guint)width, (guint)height);
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (guint)width, (guint)height);
		cr = cairo_create(surface);
	}

	poppler_page_render(ppage, cr);
	cairo_destroy(cr);
	gtk_widget_queue_draw(canvas);

	guint total_pages = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(canvas), "tpages"));
	pstr = g_strdup_printf(" / %d : %dx%d  ", total_pages, (guint)width, (guint)height);
	GtkWidget *label = g_object_get_data(G_OBJECT(canvas), "plabel");
	gtk_label_set_text(GTK_LABEL(label), pstr);
	g_free(pstr);
	g_object_set_data(G_OBJECT(canvas), "surface1", surface);
	reset_scroll(GTK_SCROLLED_WINDOW(g_object_get_data(G_OBJECT(canvas), "pscroll")));

}

static void tb_back_clicked(GtkToolItem *toolbtn, GtkWidget *canvas)
{
	guint current_page = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(canvas), "cpage"));
	guint total_pages = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(canvas), "tpages"));

	if (current_page != 0)
		current_page--;

	GtkWidget *spin = g_object_get_data(G_OBJECT(canvas), "pspin");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), (gdouble)current_page + 1);
	view_set_page(canvas, current_page);
	gtk_widget_grab_focus(g_object_get_data(G_OBJECT(canvas), "pscroll"));
}

static void tb_forward_clicked(GtkToolItem *toolbtn, GtkWidget *canvas)
{
	guint current_page = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(canvas), "cpage"));
	guint total_pages = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(canvas), "tpages"));

	if (current_page != (total_pages - 1))
		current_page++;

	GtkWidget *spin = g_object_get_data(G_OBJECT(canvas), "pspin");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), (gdouble)current_page + 1);
	view_set_page(canvas, current_page);
	gtk_widget_grab_focus(g_object_get_data(G_OBJECT(canvas), "pscroll"));
}

static void tb_first_clicked(GtkToolItem *toolbtn, GtkWidget *canvas)
{
	GtkWidget *spin = g_object_get_data(G_OBJECT(canvas), "pspin");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), 1);
	view_set_page(canvas, 0);
	gtk_widget_grab_focus(g_object_get_data(G_OBJECT(canvas), "pscroll"));
}

static void tb_last_clicked(GtkToolItem *toolbtn, GtkWidget *canvas)
{
	guint total_pages = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(canvas), "tpages"));
	GtkWidget *spin = g_object_get_data(G_OBJECT(canvas), "pspin");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), total_pages);
	view_set_page(canvas, total_pages - 1);
	gtk_widget_grab_focus(g_object_get_data(G_OBJECT(canvas), "pscroll"));
}

static void tb_spin_changed(GtkSpinButton *spin_button, GtkWidget *canvas)
{
	view_set_page(canvas, gtk_spin_button_get_value_as_int(spin_button) - 1);
	gtk_widget_grab_focus(g_object_get_data(G_OBJECT(canvas), "pscroll"));
}

static void tb_spin_changed_nograb(GtkSpinButton *spin_button, GtkWidget *canvas)
{
	view_set_page(canvas, gtk_spin_button_get_value_as_int(spin_button) - 1);
}

static void tb_checkbox_changed(GtkSpinButton *spin_button, GtkWidget *canvas)
{
	guint current_page = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(canvas), "cpage"));
	view_set_page(canvas, current_page);
	gtk_widget_grab_focus(g_object_get_data(G_OBJECT(canvas), "pscroll"));
}

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, GtkWidget *canvas)
{
	gboolean tmp;
	GtkWidget *chkscale = g_object_get_data(G_OBJECT(canvas), "pchkorg");

	switch (event->keyval)
	{
	case GDK_Up:
		g_signal_emit_by_name(widget, "scroll-child", GTK_SCROLL_STEP_BACKWARD, FALSE, &tmp);
		return TRUE;

	case GDK_Down:
		g_signal_emit_by_name(widget, "scroll-child", GTK_SCROLL_STEP_FORWARD, FALSE, &tmp);
		return TRUE;

	case GDK_Right:
		g_signal_emit_by_name(widget, "scroll-child", GTK_SCROLL_STEP_FORWARD, TRUE, &tmp);
		return TRUE;

	case GDK_Left:
		g_signal_emit_by_name(widget, "scroll-child", GTK_SCROLL_STEP_BACKWARD, TRUE, &tmp);
		return TRUE;

	case GDK_Home:
		tb_first_clicked(NULL, canvas);
		return TRUE;

	case GDK_End:
		tb_last_clicked(NULL, canvas);
		return TRUE;

	case GDK_Page_Up:
		tb_back_clicked(NULL, canvas);
		return TRUE;

	case GDK_Page_Down:
		tb_forward_clicked(NULL, canvas);
		return TRUE;

	case GDK_Insert:
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chkscale)))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkscale), FALSE);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkscale), TRUE);

		return TRUE;

	default:
		return FALSE;
	}

	return FALSE;
}


void p_getwidth(GtkWidget *widget, GtkAllocation *allocation, GtkWidget *canvas)
{
	guint tmp = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(canvas), "pwidth"));

	if (tmp == 1)
	{
		guint current_page = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(canvas), "cpage"));
		g_object_set_data(G_OBJECT(canvas), "pwidth", GUINT_TO_POINTER(allocation->width));
		view_set_page(canvas, current_page);

	}
	else if ((allocation->width > 1) && (tmp == 0))
		g_object_set_data(G_OBJECT(canvas), "pwidth", GUINT_TO_POINTER(1));
}

static void tb_text_clicked(GtkToolItem *toolbtn, GtkWidget *canvas)
{
	GtkWidget *dialog;
	GtkWidget *scroll;
	GtkWidget *tView;
	GtkTextBuffer *tBuf;

	PopplerDocument *document = g_object_get_data(G_OBJECT(canvas), "doc");
	guint current_page = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(canvas), "cpage"));
	PopplerPage *ppage = poppler_document_get_page(document, current_page);
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), "Text");
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 400);
	gtk_container_border_width(GTK_CONTAINER(dialog), 2);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	tBuf = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text(tBuf, poppler_page_get_text(ppage), -1);
	tView = gtk_text_view_new_with_buffer(tBuf);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(tView), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(tView), FALSE);
	gtk_container_add(GTK_CONTAINER(scroll), tView);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scroll, TRUE, TRUE, 0);
	gtk_widget_show_all(dialog);
}

static void tb_info_clicked(GtkToolItem *toolbtn, GtkWidget *canvas)
{
	GtkWidget *dialog;
	GtkWidget *scroll;
	GtkWidget *ilabel;
	GtkWidget *mlabel;
	gchar *mtext, *itext;

	PopplerDocument *document = g_object_get_data(G_OBJECT(canvas), "doc");
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), "Info");
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 400);
	gtk_container_border_width(GTK_CONTAINER(dialog), 5);
	itext = g_strdup_printf("Title: %s\nAuthor: %s\nSubject: %s\nCreator: %s\nProducer: %s\nKeywords: %s\nVersion: %s",
	                        poppler_document_get_title(document), poppler_document_get_author(document),
	                        poppler_document_get_subject(document), poppler_document_get_creator(document),
	                        poppler_document_get_producer(document), poppler_document_get_keywords(document),
	                        poppler_document_get_pdf_version_string(document));

	ilabel = gtk_label_new(itext);
	gtk_misc_set_alignment(GTK_MISC(ilabel), 0, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), ilabel, FALSE, TRUE, 0);
	gtk_widget_show(ilabel);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scroll, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(scroll);
	mtext = poppler_document_get_metadata(document);

	if (!mtext)
		mtext = "metadata not found";

	mlabel = gtk_label_new(mtext);
	gtk_misc_set_alignment(GTK_MISC(mlabel), 0, 0);
	gtk_label_set_selectable(GTK_LABEL(mlabel), TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(mlabel), FALSE);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), mlabel);
	gtk_widget_show(mlabel);
	gtk_widget_grab_focus(scroll);
	gtk_widget_show(dialog);
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *pBox;
	GtkWidget *pArea;
	GtkWidget *vscroll;
	GtkWidget *canvas;
	GtkWidget *label;
	GtkWidget *spinbtn;
	GtkWidget *chkscale;
	GtkWidget *tb1;
	GtkToolItem *tb_back;
	GtkToolItem *tb_forward;
	GtkToolItem *tb_first;
	GtkToolItem *tb_last;
	GtkToolItem *tb_text;
	GtkToolItem *tb_info;
	GtkToolItem *tb_separator;
	GtkToolItem *tb_pages;
	GtkToolItem *tb_selector;
	GtkToolItem *tb_checkbox;
	GdkColor color;
	cairo_surface_t *surface;
	guint current_page = 0;
	guint total_pages;

	gchar* fileUri = g_filename_to_uri(FileToLoad, NULL, NULL);
	PopplerDocument *document = poppler_document_new_from_file(fileUri, NULL, NULL);

	if (fileUri)
		g_free(fileUri);

	if (document == NULL)
		return NULL;

	PopplerPage *ppage = poppler_document_get_page(document, current_page);
	total_pages = poppler_document_get_n_pages(document);
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);

	gFix = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);

	tb1 = gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(tb1), TRUE);
	gtk_toolbar_set_style(GTK_TOOLBAR(tb1), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start(GTK_BOX(gFix), tb1, FALSE, FALSE, 1);

	vscroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(gFix), vscroll, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(vscroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	canvas = gtk_drawing_area_new();
	gdk_color_parse("white", &color);
	gtk_widget_modify_bg(canvas, GTK_STATE_NORMAL, &color);
	pBox = gtk_vbox_new(TRUE, 0);
	pArea = gtk_aspect_frame_new(NULL, 0.5, 0.5, 0, TRUE);
	gtk_box_pack_start(GTK_BOX(pBox), pArea, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(pArea), canvas);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(vscroll), pBox);
	g_signal_connect(G_OBJECT(canvas), "expose_event", G_CALLBACK(canvas_expose_event), NULL);

	tb_first = gtk_tool_button_new_from_stock(GTK_STOCK_GOTO_FIRST);
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_first, 0);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_first), "First page");
	g_signal_connect(G_OBJECT(tb_first), "clicked", G_CALLBACK(tb_first_clicked), (gpointer)canvas);

	tb_back = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_back, 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_back), "Previous page");
	g_signal_connect(G_OBJECT(tb_back), "clicked", G_CALLBACK(tb_back_clicked), (gpointer)canvas);

	tb_forward = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_forward, 2);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_forward), "Next page");
	g_signal_connect(G_OBJECT(tb_forward), "clicked", G_CALLBACK(tb_forward_clicked), (gpointer)canvas);

	tb_last = gtk_tool_button_new_from_stock(GTK_STOCK_GOTO_LAST);
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_last, 3);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_last), "Last page");
	g_signal_connect(G_OBJECT(tb_last), "clicked", G_CALLBACK(tb_last_clicked), (gpointer)canvas);

	tb_selector = gtk_tool_item_new();
	spinbtn = gtk_spin_button_new_with_range(1, (gdouble)total_pages, 1);
	gtk_container_add(GTK_CONTAINER(tb_selector), spinbtn);
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_selector, 4);
	//gtk_widget_set_tooltip_text(GTK_WIDGET(tb_selector), "Current page");
	//g_signal_connect(G_OBJECT(spinbtn), "activate", G_CALLBACK(tb_spin_changed), (gpointer)canvas);
	g_signal_connect(G_OBJECT(spinbtn), "value-changed", G_CALLBACK(tb_spin_changed), (gpointer)canvas);

	tb_pages = gtk_tool_item_new();
	label = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(tb_pages), label);
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_pages, 5);
	//gtk_widget_set_tooltip_text(GTK_WIDGET(tb_pages), "Current page");
	g_signal_connect(G_OBJECT(vscroll), "key_press_event", G_CALLBACK(on_key_press), (gpointer)canvas);

	tb_checkbox = gtk_tool_item_new();
	chkscale = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkscale), TRUE);
	gtk_container_add(GTK_CONTAINER(tb_checkbox), chkscale);
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_checkbox, 6);
	g_signal_connect(G_OBJECT(chkscale), "toggled", G_CALLBACK(tb_checkbox_changed), (gpointer)canvas);

	tb_separator = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_separator, 7);

	tb_text = gtk_tool_button_new_from_stock(GTK_STOCK_SELECT_ALL);
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_text, 8);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_text), "Text");
	g_signal_connect(G_OBJECT(tb_text), "clicked", G_CALLBACK(tb_text_clicked), (gpointer)canvas);

	tb_info = gtk_tool_button_new_from_stock(GTK_STOCK_INFO);
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_info, 9);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_info), "Info");
	g_signal_connect(G_OBJECT(tb_info), "clicked", G_CALLBACK(tb_info_clicked), (gpointer)canvas);

	g_signal_connect(G_OBJECT(pBox), "size-allocate", G_CALLBACK(p_getwidth), (gpointer)canvas);

	g_object_set_data_full(G_OBJECT(canvas), "doc", document, (GDestroyNotify)g_object_unref);
	g_object_set_data(G_OBJECT(canvas), "surface1", surface);
	g_object_set_data(G_OBJECT(canvas), "tpages", GUINT_TO_POINTER(total_pages));
	g_object_set_data(G_OBJECT(canvas), "cpage", GUINT_TO_POINTER(current_page));
	g_object_set_data_full(G_OBJECT(canvas), "page", ppage, (GDestroyNotify)g_object_unref);
	g_object_set_data(G_OBJECT(canvas), "plabel", label);
	g_object_set_data(G_OBJECT(canvas), "pspin", spinbtn);
	g_object_set_data(G_OBJECT(canvas), "pchkorg", chkscale);
	g_object_set_data(G_OBJECT(canvas), "pscroll", vscroll);

	gtk_widget_grab_focus(vscroll);
	gtk_widget_show_all(gFix);
	return gFix;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}
