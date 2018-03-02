#include <gio/gio.h>
#include <gtkimageview/gtkanimview.h>
#include <gtkimageview/gtkimageview.h>
#include <gtkimageview/gtkimagescrollwin.h>
#include "wlxplugin.h"

const gchar *anims[] =
{
	"image/gif",
	"application/x-navi-animation",
};

GtkWidget* find_child(GtkWidget* parent, const gchar* name)
{
	if (g_ascii_strcasecmp(gtk_widget_get_name((GtkWidget*)parent), (gchar*)name) == 0)
	{
		return parent;
	}

	if (GTK_IS_BIN(parent))
	{
		GtkWidget *child = gtk_bin_get_child(GTK_BIN(parent));
		return find_child(child, name);
	}

	if (GTK_IS_CONTAINER(parent))
	{
		GList *children = gtk_container_get_children(GTK_CONTAINER(parent));

		while ((children = g_list_next(children)) != NULL)
		{
			GtkWidget* widget = find_child(children->data, name);

			if (widget != NULL)
			{
				return widget;
			}
		}
	}

	return NULL;
}

static void tb_zoom_in_clicked(GtkToolItem *tooleditcut, GtkWidget *imgview)
{
	gtk_image_view_zoom_in(GTK_IMAGE_VIEW(imgview));
}

static void tb_zoom_out_clicked(GtkToolItem *tooleditcopy, GtkWidget *imgview)
{
	gtk_image_view_zoom_out(GTK_IMAGE_VIEW(imgview));
}

static void tb_orgsize_clicked(GtkToolItem *tooleditcopy, GtkWidget *imgview)
{
	gtk_image_view_set_zoom(GTK_IMAGE_VIEW(imgview), 1);
}

static void tb_fit_clicked(GtkToolItem *tooleditcopy, GtkWidget *imgview)
{
	gtk_image_view_set_fitting(GTK_IMAGE_VIEW(imgview), TRUE);
}

static void tb_copy_clicked(GtkToolItem *tooleditcopy, GtkWidget *imgview)
{
	gtk_clipboard_set_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
	                        gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(imgview)));
}

static void tb_rotare_clicked(GtkToolItem *tooleditcopy, GtkWidget *imgview)
{
	if (gtk_anim_view_get_is_playing(GTK_ANIM_VIEW(imgview)))
		gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(imgview), FALSE);

	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(imgview),
	                          gdk_pixbuf_rotate_simple(gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(imgview)),
	                                          GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE), TRUE);
}

static void tb_rotare1_clicked(GtkToolItem *tooleditcopy, GtkWidget *imgview)
{
	if (gtk_anim_view_get_is_playing(GTK_ANIM_VIEW(imgview)))
		gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(imgview), FALSE);

	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(imgview),
	                          gdk_pixbuf_rotate_simple(gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(imgview)),
	                                          GDK_PIXBUF_ROTATE_CLOCKWISE), TRUE);
}

static void tb_hflip_clicked(GtkToolItem *tooleditcopy, GtkWidget *imgview)
{
	if (gtk_anim_view_get_is_playing(GTK_ANIM_VIEW(imgview)))
		gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(imgview), FALSE);

	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(imgview),
	                          gdk_pixbuf_flip(gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(imgview)), TRUE), TRUE);
}

static void tb_vflip_clicked(GtkToolItem *tooleditcopy, GtkWidget *imgview)
{
	if (gtk_anim_view_get_is_playing(GTK_ANIM_VIEW(imgview)))
		gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(imgview), FALSE);

	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(imgview),
	                          gdk_pixbuf_flip(gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(imgview)), FALSE), TRUE);
}

static void tb_play_clicked(GtkToolItem *tooleditcopy, GtkWidget *imgview)
{
	if (!gtk_anim_view_get_is_playing(GTK_ANIM_VIEW(imgview)))
		gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(imgview), TRUE);
}

static void tb_stop_clicked(GtkToolItem *tooleditcopy, GtkWidget *imgview)
{
	if (gtk_anim_view_get_is_playing(GTK_ANIM_VIEW(imgview)))
		gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(imgview), FALSE);
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *view;
	GtkWidget *mtb;
	GtkWidget *label;
	GtkToolItem *tb_zoom_in;
	GtkToolItem *tb_zoom_out;
	GtkToolItem *tb_orgsize;
	GtkToolItem *tb_fit;
	GtkToolItem *tb_separator;
	GtkToolItem *tb_separator1;
	GtkToolItem *tb_copy;
	GtkToolItem *tb_rotare;
	GtkToolItem *tb_rotare1;
	GtkToolItem *tb_hflip;
	GtkToolItem *tb_vflip;
	GtkToolItem *tb_play;
	GtkToolItem *tb_stop;
	GtkToolItem *tb_size;
	GdkPixbuf *pixbuf;
	GdkPixbufAnimation *anim = NULL;
	gchar *tstr;
	gboolean is_certain = FALSE;
	gint tb_last = 11, i;

	gFix = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	mtb = gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(mtb), TRUE);
	gtk_toolbar_set_style(GTK_TOOLBAR(mtb), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start(GTK_BOX(gFix), mtb, FALSE, FALSE, 2);
	view = gtk_anim_view_new();
	gtk_widget_set_name(view, "imageview");
	scroll = gtk_image_scroll_win_new(GTK_IMAGE_VIEW(view));
	gtk_container_add(GTK_CONTAINER(gFix), scroll);

	pixbuf = gdk_pixbuf_new_from_file(FileToLoad, NULL);

	if (!pixbuf)
	{
		gtk_widget_destroy(gFix);
		return NULL;
	}

	gchar *content_type = g_content_type_guess(FileToLoad, NULL, 0, &is_certain);
	g_print("content_type = %s\n", content_type);

	for (i = 0; anims[i] != NULL; i++)
	{
		if (g_strcmp0(content_type, anims[i]) == 0)
		{
			anim = gdk_pixbuf_animation_new_from_file(FileToLoad, NULL);

			if (anim)
			{
				gtk_anim_view_set_anim(GTK_ANIM_VIEW(view), anim);
				gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(view), TRUE);
			}
		}
	}

	g_free(content_type);
	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), pixbuf, TRUE);

	tb_zoom_in = gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_IN);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_zoom_in, 0);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_zoom_in), "Zoom In");
	g_signal_connect(G_OBJECT(tb_zoom_in), "clicked", G_CALLBACK(tb_zoom_in_clicked), (gpointer)(GtkWidget*)(view));

	tb_zoom_out = gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_OUT);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_zoom_out, 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_zoom_out), "Zoom Out");
	g_signal_connect(G_OBJECT(tb_zoom_out), "clicked", G_CALLBACK(tb_zoom_out_clicked), (gpointer)(GtkWidget*)(view));

	tb_orgsize = gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_100);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_orgsize, 2);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_orgsize), "Original Size");
	g_signal_connect(G_OBJECT(tb_orgsize), "clicked", G_CALLBACK(tb_orgsize_clicked), (gpointer)(GtkWidget*)(view));

	tb_fit = gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_FIT);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_fit, 3);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_fit), "Fit");
	g_signal_connect(G_OBJECT(tb_fit), "clicked", G_CALLBACK(tb_fit_clicked), (gpointer)(GtkWidget*)(view));

	tb_separator = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_separator, 4);

	tb_copy = gtk_tool_button_new_from_stock(GTK_STOCK_COPY);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_copy, 5);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_copy), "Copy to Clipboard");
	g_signal_connect(G_OBJECT(tb_copy), "clicked", G_CALLBACK(tb_copy_clicked), (gpointer)(GtkWidget*)(view));

	tb_rotare = gtk_tool_button_new(NULL, "Rotare");
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_rotare), "object-rotate-left");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_rotare, 6);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_rotare), "Rotare");
	g_signal_connect(G_OBJECT(tb_rotare), "clicked", G_CALLBACK(tb_rotare_clicked), (gpointer)(GtkWidget*)(view));

	tb_rotare1 = gtk_tool_button_new(NULL, "Rotare Clockwise");
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_rotare1), "object-rotate-right");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_rotare1, 7);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_rotare1), "Rotare Clockwise");
	g_signal_connect(G_OBJECT(tb_rotare1), "clicked", G_CALLBACK(tb_rotare1_clicked), (gpointer)(GtkWidget*)(view));

	tb_hflip = gtk_tool_button_new(NULL, "Flip Horizontally");
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_hflip), "object-flip-horizontal");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_hflip, 8);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_hflip), "Flip Horizontally");
	g_signal_connect(G_OBJECT(tb_hflip), "clicked", G_CALLBACK(tb_hflip_clicked), (gpointer)(GtkWidget*)(view));

	tb_vflip = gtk_tool_button_new(NULL, "Flip Vertically");
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_vflip), "object-flip-vertical");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_vflip, 9);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_vflip), "Flip Vertically");
	g_signal_connect(G_OBJECT(tb_vflip), "clicked", G_CALLBACK(tb_vflip_clicked), (gpointer)(GtkWidget*)(view));

	if ((anim) && (!gdk_pixbuf_animation_is_static_image(anim)))
	{
		tb_play = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
		gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_play, 10);
		gtk_widget_set_tooltip_text(GTK_WIDGET(tb_play), "Play Animation");
		g_signal_connect(G_OBJECT(tb_play), "clicked", G_CALLBACK(tb_play_clicked), (gpointer)(GtkWidget*)(view));

		tb_stop = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PAUSE);
		gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_stop, 11);
		gtk_widget_set_tooltip_text(GTK_WIDGET(tb_stop), "Stop Animation");
		g_signal_connect(G_OBJECT(tb_stop), "clicked", G_CALLBACK(tb_stop_clicked), (gpointer)(GtkWidget*)(view));

		tb_separator1 = gtk_separator_tool_item_new();
		gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_separator1, 12);
		tb_last = 13;
	}
	else
	{
		tb_separator1 = gtk_separator_tool_item_new();
		gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_separator1, 10);
	}

	tstr = g_strdup_printf("%dx%d", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
	tb_size = gtk_tool_item_new();
	label = gtk_label_new(tstr);
	gtk_container_add(GTK_CONTAINER(tb_size), label);
	g_free(tstr);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_size, tb_last);

	gtk_widget_grab_focus(view);
	gtk_widget_show_all(gFix);

	if (g_strcmp0(gtk_window_get_title(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ParentWin)))), FileToLoad) != 0)
		gtk_widget_hide(mtb);

	return gFix;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	GSList *l, *list;
	gchar *detectstr = "", **ext;
	gint i;

	list = gdk_pixbuf_get_formats();

	for (l = list; l != NULL; l = l->next)
	{
		GdkPixbufFormat *format = (GdkPixbufFormat*)l->data;
		ext = gdk_pixbuf_format_get_extensions(format);

		for (i = 0; ext[i] != NULL; i++)
		{
			if (g_strcmp0(detectstr, "") != 0)
				detectstr = g_strdup_printf("|%s", detectstr);

			detectstr = g_strdup_printf("(EXT=\"%s\")%s", ext[i], detectstr);
		}
	}

	g_strlcpy(DetectString, detectstr, maxlen);
	g_free(ext);
	g_free(detectstr);
	g_slist_free(l);
	g_slist_free(list);

}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	switch (Command)
	{
	case lc_copy :
		gtk_clipboard_set_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
		                        gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(find_child(ListWin, "imageview"))));
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
