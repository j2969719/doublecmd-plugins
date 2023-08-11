#define _GNU_SOURCE

#if PLUGTARGET == 6
#include <wand/MagickWand.h>
#else
#include <MagickWand/MagickWand.h>
#endif

#include <gtkimageview/gtkimagescrollwin.h>
#include <gtkimageview/gtkimageview.h>
#include "wlxplugin.h"

#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

#define _detectstring "(EXT=\"DDS\")|(EXT=\"TGA\")|(EXT=\"PCX\")|(EXT=\"BMP\")|(EXT=\"WEBP\")"

gboolean gHideToolbar = TRUE;

static void tb_zoom_in_clicked(GtkToolItem *item, GtkWidget *view)
{
	gtk_image_view_zoom_in(GTK_IMAGE_VIEW(view));
}

static void tb_zoom_out_clicked(GtkToolItem *item, GtkWidget *view)
{
	gtk_image_view_zoom_out(GTK_IMAGE_VIEW(view));
}

static void tb_orgsize_clicked(GtkToolItem *item, GtkWidget *view)
{
	gtk_image_view_set_zoom(GTK_IMAGE_VIEW(view), 1);
}

static void tb_fit_clicked(GtkToolItem *item, GtkWidget *view)
{
	gtk_image_view_set_fitting(GTK_IMAGE_VIEW(view), TRUE);
}

static void tb_copy_clicked(GtkToolItem *item, GtkWidget *view)
{
	gtk_clipboard_set_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
	                        gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(view)));
}

static void tb_rotare_clicked(GtkToolItem *item, GtkWidget *view)
{
	GdkPixbuf *pixbuf = gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(view));

	if (GDK_IS_PIXBUF(pixbuf))
	{
		GdkPixbuf *new = gdk_pixbuf_rotate_simple(pixbuf, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
		gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), new, TRUE);

		if (GDK_IS_PIXBUF(new))
			g_object_unref(new);
	}
}

static void tb_rotare1_clicked(GtkToolItem *item, GtkWidget *view)
{
	GdkPixbuf *pixbuf = gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(view));

	if (GDK_IS_PIXBUF(pixbuf))
	{
		GdkPixbuf *new = gdk_pixbuf_rotate_simple(pixbuf, GDK_PIXBUF_ROTATE_CLOCKWISE);
		gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), new, TRUE);

		if (GDK_IS_PIXBUF(new))
			g_object_unref(new);
	}
}

static void tb_hflip_clicked(GtkToolItem *item, GtkWidget *view)
{
	GdkPixbuf *pixbuf = gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(view));

	if (GDK_IS_PIXBUF(pixbuf))
	{
		GdkPixbuf *new = gdk_pixbuf_flip(pixbuf, TRUE);
		gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), new, TRUE);

		if (GDK_IS_PIXBUF(new))
			g_object_unref(new);
	}
}

static void tb_vflip_clicked(GtkToolItem *item, GtkWidget *view)
{
	GdkPixbuf *pixbuf = gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(view));

	if (GDK_IS_PIXBUF(pixbuf))
	{
		GdkPixbuf *new = gdk_pixbuf_flip(pixbuf, FALSE);
		gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), new, TRUE);

		if (GDK_IS_PIXBUF(new))
			g_object_unref(new);
	}
}

static void zoom_changed_cb(GtkWidget *view, GtkWidget *label)
{
	gchar *str;
	GdkPixbuf *pixbuf = gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(view));

	if (!GDK_IS_PIXBUF(pixbuf))
	{
		gtk_label_set_text(GTK_LABEL(label), "");
		return;
	}

	int width = gdk_pixbuf_get_width(pixbuf);
	int height = gdk_pixbuf_get_height(pixbuf);
	gdouble zoom = gtk_image_view_get_zoom(GTK_IMAGE_VIEW(view));

	if (zoom == 1)
		str = g_strdup_printf("%dx%d", width, height);
	else
		str = g_strdup_printf("%dx%d (%.0fx%.0f %.0f%%)", width, height, width * zoom, height * zoom, zoom * 100);

	gtk_label_set_text(GTK_LABEL(label), str);
	g_free(str);
}

static GdkPixbuf *load_pixbuf(char *filename)
{
	MagickWand *magick_wand = NULL;

	MagickWandGenesis();
	magick_wand = NewMagickWand();

	if (MagickReadImage(magick_wand, filename) == MagickFalse)
	{
		magick_wand = DestroyMagickWand(magick_wand);
		MagickWandTerminus();
		return NULL;
	}

	MagickResetIterator(magick_wand);

	size_t width = MagickGetImageWidth(magick_wand);
	size_t height = MagickGetImageHeight(magick_wand);
	size_t depth = MagickGetImageDepth(magick_wand);
	GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, depth, width, height);
	const guint8 *pixels = gdk_pixbuf_get_pixels(pixbuf);
	int rowstride = gdk_pixbuf_get_rowstride(pixbuf);

	for (size_t row = 0; row < height; row++)
	{
		guchar *data = (guchar*)pixels + row * rowstride;
		MagickExportImagePixels(magick_wand, 0, row, width, 1, "RGBA", CharPixel, data);
	}

	magick_wand = DestroyMagickWand(magick_wand);
	MagickWandTerminus();

	return pixbuf;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *view;
	GtkWidget *mtb;
	GtkWidget *label;
	GdkPixbuf *pixbuf;
	GtkToolItem *tb_zoom_in;
	GtkToolItem *tb_zoom_out;
	GtkToolItem *tb_orgsize;
	GtkToolItem *tb_fit;
	GtkToolItem *tb_copy;
	GtkToolItem *tb_rotare;
	GtkToolItem *tb_rotare1;
	GtkToolItem *tb_hflip;
	GtkToolItem *tb_vflip;
	GtkToolItem *tb_size;

	pixbuf = load_pixbuf(FileToLoad);

	if (!pixbuf)
		return NULL;

	gFix = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	mtb = gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(mtb), TRUE);
	gtk_toolbar_set_style(GTK_TOOLBAR(mtb), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start(GTK_BOX(gFix), mtb, FALSE, FALSE, 2);
	view = gtk_image_view_new();
	scroll = gtk_image_scroll_win_new(GTK_IMAGE_VIEW(view));
	gtk_container_add(GTK_CONTAINER(gFix), scroll);

	label = gtk_label_new(NULL);
	g_signal_connect(G_OBJECT(view), "zoom_changed", G_CALLBACK(zoom_changed_cb), (gpointer)label);
	g_signal_connect(G_OBJECT(view), "pixbuf_changed", G_CALLBACK(zoom_changed_cb), (gpointer)label);

	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), pixbuf, TRUE);

	guint tb_pos = 0;
	tb_zoom_in = gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_IN);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_zoom_in, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_zoom_in), _("Zoom In"));
	g_signal_connect(G_OBJECT(tb_zoom_in), "clicked", G_CALLBACK(tb_zoom_in_clicked), (gpointer)view);

	tb_zoom_out = gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_OUT);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_zoom_out, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_zoom_out), _("Zoom Out"));
	g_signal_connect(G_OBJECT(tb_zoom_out), "clicked", G_CALLBACK(tb_zoom_out_clicked), (gpointer)view);

	tb_orgsize = gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_100);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_orgsize, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_orgsize), _("Original Size"));
	g_signal_connect(G_OBJECT(tb_orgsize), "clicked", G_CALLBACK(tb_orgsize_clicked), (gpointer)view);

	tb_fit = gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_FIT);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_fit, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_fit), _("Fit"));
	g_signal_connect(G_OBJECT(tb_fit), "clicked", G_CALLBACK(tb_fit_clicked), (gpointer)view);

	gtk_toolbar_insert(GTK_TOOLBAR(mtb), gtk_separator_tool_item_new(), tb_pos++);

	tb_copy = gtk_tool_button_new_from_stock(GTK_STOCK_COPY);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_copy, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_copy), _("Copy to Clipboard"));
	g_signal_connect(G_OBJECT(tb_copy), "clicked", G_CALLBACK(tb_copy_clicked), (gpointer)view);

	tb_rotare = gtk_tool_button_new(NULL, _("Rotate"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_rotare), "object-rotate-left");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_rotare, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_rotare), _("Rotate"));
	g_signal_connect(G_OBJECT(tb_rotare), "clicked", G_CALLBACK(tb_rotare_clicked), (gpointer)view);

	tb_rotare1 = gtk_tool_button_new(NULL, _("Rotate Clockwise"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_rotare1), "object-rotate-right");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_rotare1, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_rotare1), _("Rotate Clockwise"));
	g_signal_connect(G_OBJECT(tb_rotare1), "clicked", G_CALLBACK(tb_rotare1_clicked), (gpointer)view);

	tb_hflip = gtk_tool_button_new(NULL, _("Flip Horizontally"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_hflip), "object-flip-horizontal");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_hflip, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_hflip), _("Flip Horizontally"));
	g_signal_connect(G_OBJECT(tb_hflip), "clicked", G_CALLBACK(tb_hflip_clicked), (gpointer)view);

	tb_vflip = gtk_tool_button_new(NULL, _("Flip Vertically"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_vflip), "object-flip-vertical");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_vflip, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_vflip), _("Flip Vertically"));
	g_signal_connect(G_OBJECT(tb_vflip), "clicked", G_CALLBACK(tb_vflip_clicked), (gpointer)view);

	gtk_toolbar_insert(GTK_TOOLBAR(mtb), gtk_separator_tool_item_new(), tb_pos++);

	tb_size = gtk_tool_item_new();
	gtk_container_add(GTK_CONTAINER(tb_size), label);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_size, tb_pos++);

	gtk_widget_grab_focus(view);
	gtk_widget_show_all(gFix);

	const gchar *role = gtk_window_get_role(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ParentWin))));

	if (gHideToolbar && g_strcmp0(role, "TfrmViewer") != 0)
		gtk_widget_hide(mtb);

	g_object_set_data(G_OBJECT(gFix), "imageview", view);


	if (G_IS_OBJECT(pixbuf))
		g_object_unref(pixbuf);

	return gFix;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *view = (GtkWidget*)g_object_get_data(G_OBJECT(PluginWin), "imageview");

	GdkPixbuf *pixbuf = load_pixbuf(FileToLoad);

	if (!pixbuf)
		return LISTPLUGIN_ERROR;

	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), NULL, FALSE);
	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), pixbuf, TRUE);

	if (G_IS_OBJECT(pixbuf))
		g_object_unref(pixbuf);

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	GtkWidget *view = (GtkWidget*)g_object_get_data(G_OBJECT(ListWin), "imageview");
	GdkPixbuf *pixbuf = gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(view));
	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), NULL, FALSE);
	gtk_widget_destroy(GTK_WIDGET(ListWin));

	if (G_IS_OBJECT(pixbuf))
		g_object_unref(pixbuf);
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen - 1);
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	GtkWidget *view = (GtkWidget*)g_object_get_data(G_OBJECT(ListWin), "imageview");

	switch (Command)
	{
	case lc_copy :
		gtk_clipboard_set_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
		                        gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(view)));
		break;

	default :
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
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

	gchar *cfgdir = g_path_get_dirname(dps->DefaultIniName);
	gchar *cfgpath = g_strdup_printf("%s/j2969719.ini", cfgdir);
	g_free(cfgdir);
	GKeyFile *cfg = g_key_file_new();
	g_key_file_load_from_file(cfg, cfgpath, G_KEY_FILE_KEEP_COMMENTS, NULL);

	if (!g_key_file_has_key(cfg, PLUGNAME, "HideToolbar", NULL))
	{
		g_key_file_set_boolean(cfg, PLUGNAME, "HideToolbar", gHideToolbar);
		g_key_file_save_to_file(cfg, cfgpath, NULL);
	}
	else
		gHideToolbar = g_key_file_get_boolean(cfg, PLUGNAME, "HideToolbar", NULL);

	g_key_file_free(cfg);
	g_free(cfgpath);
}
