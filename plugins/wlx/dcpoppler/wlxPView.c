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
#include <cairo.h>
#include <poppler.h>
#include <math.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring "EXT=\"PDF\""

GtkWidget *label;
GtkWidget  *canvas;

PopplerPage *ppage;
PopplerDocument *document;
cairo_surface_t *surface;
int current_page = 0;
int total_pages = 0;

void update_numpages()
{
	gchar *str;
	str = g_strdup_printf("%d / %d", current_page + 1, total_pages);

	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);
}

void reset_scroll(GtkScrolledWindow *scrolled_window)
{
	GtkAdjustment *tmp = gtk_scrolled_window_get_vadjustment(scrolled_window);
	gtk_adjustment_set_value(tmp, 0);
	gtk_scrolled_window_set_vadjustment(scrolled_window, tmp);
	tmp = gtk_scrolled_window_get_hadjustment(scrolled_window);
	gtk_adjustment_set_value(tmp, 0);
	gtk_scrolled_window_set_hadjustment(scrolled_window, tmp);
}

static void canvas_expose_event(GtkWidget *widget, GdkEventExpose *event)
{
	cairo_t *cr;

	gdk_window_clear(canvas->window);
	cr = gdk_cairo_create(canvas->window);

	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);
}

static void redraw_callback(void *data)
{
	gtk_widget_queue_draw(canvas);
}

static void view_set_page(int page)
{
	int err;
	int w, h;
	double width, height;
	cairo_t *cr;

	ppage = poppler_document_get_page(document, page);
	poppler_page_get_size(ppage, &width, &height);
	w = (int) ceil(width);
	h = (int) ceil(height);
	cairo_surface_destroy(surface);
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
	cr = cairo_create(surface);
	poppler_page_render(ppage, cr);
	cairo_destroy(cr);
	gtk_widget_set_size_request(canvas, w, h);
	gtk_widget_queue_draw(canvas);
	update_numpages();
}

static void tb_back_clicked(GtkToolItem *tooleditcut, GtkWindow *parentWindow)
{
	if (current_page != 0)
		current_page--;

	view_set_page(current_page);

	reset_scroll(GTK_SCROLLED_WINDOW(parentWindow));
}

static void tb_forward_clicked(GtkToolItem *tooleditcopy, GtkWindow *parentWindow)
{
	if (current_page != (total_pages - 1))
		current_page++;

	view_set_page(current_page);

	reset_scroll(GTK_SCROLLED_WINDOW(parentWindow));
}

static void tb_home_clicked(GtkToolItem *tooleditpaste, GtkWindow *parentWindow)
{
	current_page = 0;
	view_set_page(0);

	reset_scroll(GTK_SCROLLED_WINDOW(parentWindow));
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{

	GtkWidget  *vscroll;
	GtkWidget  *gFix;
	GdkColor  color;
	GtkWidget  *tb1;
	GtkToolItem  *tb_back;
	GtkToolItem  *tb_forward;
	GtkToolItem  *tb_home;
	GtkToolItem  *tb_separator;
	GtkToolItem  *tb_pages;

	gchar* fileUri = g_filename_to_uri(FileToLoad, NULL, NULL);
	document = poppler_document_new_from_file(fileUri, NULL, NULL);

	if (fileUri)
		g_free(fileUri);

	if (document == NULL)
		return NULL;

	current_page = 0;
	gFix = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);

	vscroll = gtk_scrolled_window_new(NULL, NULL);
	tb1 = gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(tb1), TRUE);
	gtk_toolbar_set_style(GTK_TOOLBAR(tb1), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start(GTK_BOX(gFix), tb1, FALSE, FALSE, 2);

	tb_back = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_back, 0);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_back), "Previous Page");
	g_signal_connect(G_OBJECT(tb_back), "clicked", G_CALLBACK(tb_back_clicked), (gpointer)(GtkWidget*)(vscroll));

	tb_forward = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_forward, 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_forward), "Next page");
	g_signal_connect(G_OBJECT(tb_forward), "clicked", G_CALLBACK(tb_forward_clicked), (gpointer)(GtkWidget*)(vscroll));

	tb_home = gtk_tool_button_new_from_stock(GTK_STOCK_HOME);
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_home, 2);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_home), "First page");
	g_signal_connect(G_OBJECT(tb_home), "clicked", G_CALLBACK(tb_home_clicked), (gpointer)(GtkWidget*)(vscroll));

	tb_separator = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_separator, 3);

	tb_pages = gtk_tool_item_new();
	label = gtk_label_new("1 / 1");
	gtk_container_add(GTK_CONTAINER(tb_pages), label);
	gtk_toolbar_insert(GTK_TOOLBAR(tb1), tb_pages, 4);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_pages), "Current page");

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(vscroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(gFix), vscroll, TRUE, TRUE, 2);
	canvas = gtk_drawing_area_new();
	gdk_color_parse("white", &color);
	gtk_widget_modify_bg(canvas, GTK_STATE_NORMAL, &color);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(vscroll), canvas);
	g_signal_connect(G_OBJECT(canvas), "expose_event", G_CALLBACK(canvas_expose_event), (gpointer)(GtkWidget*)(ParentWin));

	total_pages = poppler_document_get_n_pages(document);
	view_set_page(0);

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
	strncpy(DetectString, _detectstring, maxlen);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}
