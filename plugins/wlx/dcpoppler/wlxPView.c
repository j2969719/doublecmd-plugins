#include <gtk/gtk.h>
#include <cairo.h>
#include <poppler.h>
#include <math.h>
#include <string.h>
#include "wlxplugin.h"


GtkWidget  *tb1;
GtkToolItem  *tb_back;
GtkToolItem  *tb_forward;
GtkToolItem  *tb_home;
GtkToolItem  *tb_separator;
GtkToolItem  *tb_pages;
GtkWidget  *canvas ;

PopplerPage *ppage;
PopplerDocument *document;
cairo_surface_t *surface;
int current_page = 0;
int total_pages = 0;

void update_numpages()
{
	gchar *str;
	str = g_strdup_printf("%d / %d",
	current_page + 1, total_pages);

	gtk_tool_button_set_label (tb_pages, str);
	g_free(str);
}

static void canvas_expose_event( GtkWidget *widget, GdkEventExpose *event )
{
	cairo_t *cr;

	gdk_window_clear (canvas->window);
	cr = gdk_cairo_create (canvas->window);

	cairo_set_source_surface (cr, surface, 0, 0);
	cairo_paint (cr);
	cairo_destroy (cr);
}

static void redraw_callback (void *data)
{
	gtk_widget_queue_draw (canvas);
}

static void view_set_page (int page)
{
	int err;
	int w, h;
	double width, height;
	cairo_t *cr;

	ppage = poppler_document_get_page (document, page);
	poppler_page_get_size (ppage, &width, &height);
	w = (int) ceil(width);
	h = (int) ceil(height);
	cairo_surface_destroy (surface);
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
	cr = cairo_create (surface);
	poppler_page_render (ppage, cr);
	cairo_destroy (cr);
	gtk_widget_set_size_request (canvas, w, h);
	gtk_widget_queue_draw (canvas);
	update_numpages();
}

static void tb_back_clicked (GtkToolItem *tooleditcut, GtkWindow *parentWindow)
{
	if( current_page != 0)
		current_page--;

	view_set_page(current_page);
}

static void tb_forward_clicked (GtkToolItem *tooleditcopy, GtkWindow *parentWindow)
{
	if( current_page != (total_pages - 1))
		current_page++;

	view_set_page(current_page);
}

static void tb_home_clicked (GtkToolItem *tooleditpaste, GtkWindow *parentWindow)
{
	current_page = 0;
	view_set_page(0);
}

HWND DCPCALL ListLoad (HWND ParentWin, char* FileToLoad, int ShowFlags)
{

	GtkWidget  *vscroll;
	GtkWidget  *gFix;
        current_page = 0;
        gchar* fileUri = g_filename_to_uri(FileToLoad, NULL, NULL);
	document = poppler_document_new_from_file (fileUri, NULL, NULL);

	if (document == NULL)
		return -1;

	gFix = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER ((GtkWidget*)(ParentWin)), gFix);


	tb1 = gtk_toolbar_new ();
	gtk_toolbar_set_show_arrow (GTK_TOOLBAR (tb1), TRUE);
	gtk_toolbar_set_style (GTK_TOOLBAR (tb1), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start (GTK_BOX (gFix), tb1, FALSE, FALSE, 2);

	tb_back = gtk_tool_button_new_from_stock (GTK_STOCK_GO_BACK);
	gtk_toolbar_insert (GTK_TOOLBAR (tb1), tb_back, 0);
	gtk_widget_set_tooltip_text (GTK_WIDGET(tb_back), "Previous Page");
	g_signal_connect (G_OBJECT (tb_back), "clicked", G_CALLBACK (tb_back_clicked), (gpointer) (GtkWidget*)(ParentWin));

	tb_forward = gtk_tool_button_new_from_stock (GTK_STOCK_GO_FORWARD);
	gtk_toolbar_insert (GTK_TOOLBAR (tb1), tb_forward, 1);
	gtk_widget_set_tooltip_text (GTK_WIDGET(tb_forward), "Next page");
	g_signal_connect (G_OBJECT (tb_forward), "clicked", G_CALLBACK (tb_forward_clicked), (gpointer) (GtkWidget*)(ParentWin));

	tb_home = gtk_tool_button_new_from_stock (GTK_STOCK_HOME);
	gtk_toolbar_insert (GTK_TOOLBAR (tb1), tb_home, 2);
	gtk_widget_set_tooltip_text (GTK_WIDGET(tb_home), "First page");
	g_signal_connect (G_OBJECT (tb_home), "clicked", G_CALLBACK (tb_home_clicked), (gpointer) (GtkWidget*)(ParentWin));

	tb_separator = gtk_separator_tool_item_new ();
	gtk_toolbar_insert (GTK_TOOLBAR (tb1), tb_separator, 3);

	tb_pages = gtk_tool_button_new (NULL,"1 / 1");
	gtk_toolbar_insert (GTK_TOOLBAR (tb1), tb_pages, 4);
	gtk_widget_set_tooltip_text (GTK_WIDGET(tb_pages), "Current page");

	vscroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start (GTK_BOX (gFix), vscroll, TRUE, TRUE, 2);
	canvas = gtk_drawing_area_new();
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(vscroll), canvas);
	g_signal_connect (G_OBJECT (canvas), "expose_event", G_CALLBACK (canvas_expose_event), (gpointer) (GtkWidget*)(ParentWin));

	total_pages = poppler_document_get_n_pages (document);
	view_set_page(0);

	gtk_widget_show_all (gFix);
	return gFix;
}

void ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(ListWin);
}

void DCPCALL ListGetDetectString(char* DetectString,int maxlen)
{
	strncpy(DetectString, "EXT=\"PDF\"", maxlen);
}
