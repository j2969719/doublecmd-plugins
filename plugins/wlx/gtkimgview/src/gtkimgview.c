#define _GNU_SOURCE

#include <gio/gio.h>
#include <gtkimageview/gtkanimview.h>
#include <gtkimageview/gtkimageview.h>
#include <gtkimageview/gtkimagescrollwin.h>
#include "wlxplugin.h"

#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

gboolean gHideToolbar = TRUE;

const gchar *anims[] =
{
	"image/gif",
	"application/x-navi-animation",
};

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
	if (gtk_anim_view_get_is_playing(GTK_ANIM_VIEW(view)))
		gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(view), FALSE);

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
	if (gtk_anim_view_get_is_playing(GTK_ANIM_VIEW(view)))
		gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(view), FALSE);

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
	if (gtk_anim_view_get_is_playing(GTK_ANIM_VIEW(view)))
		gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(view), FALSE);

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
	if (gtk_anim_view_get_is_playing(GTK_ANIM_VIEW(view)))
		gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(view), FALSE);

	GdkPixbuf *pixbuf = gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(view));

	if (GDK_IS_PIXBUF(pixbuf))
	{
		GdkPixbuf *new = gdk_pixbuf_flip(pixbuf, FALSE);
		gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), new, TRUE);

		if (GDK_IS_PIXBUF(new))
			g_object_unref(new);
	}
}

static void tb_play_clicked(GtkToolItem *item, GtkWidget *view)
{
	if (!gtk_anim_view_get_is_playing(GTK_ANIM_VIEW(view)))
		gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(view), TRUE);
}

static void tb_stop_clicked(GtkToolItem *item, GtkWidget *view)
{
	if (gtk_anim_view_get_is_playing(GTK_ANIM_VIEW(view)))
		gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(view), FALSE);
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

	if (!g_file_test(FileToLoad, G_FILE_TEST_IS_REGULAR))
		return NULL;

	pixbuf = gdk_pixbuf_new_from_file(FileToLoad, NULL);

	if (!pixbuf)
		return NULL;

	gFix = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	mtb = gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(mtb), TRUE);
	gtk_toolbar_set_style(GTK_TOOLBAR(mtb), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start(GTK_BOX(gFix), mtb, FALSE, FALSE, 2);
	view = gtk_anim_view_new();
	scroll = gtk_image_scroll_win_new(GTK_IMAGE_VIEW(view));
	gtk_container_add(GTK_CONTAINER(gFix), scroll);

	gchar *content_type = g_content_type_guess(FileToLoad, NULL, 0, NULL);
	g_print("%s (%s): content_type = %s\n", PLUGNAME, FileToLoad, content_type);

	label = gtk_label_new(NULL);
	g_signal_connect(G_OBJECT(view), "zoom_changed", G_CALLBACK(zoom_changed_cb), (gpointer)label);
	g_signal_connect(G_OBJECT(view), "pixbuf_changed", G_CALLBACK(zoom_changed_cb), (gpointer)label);

	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), pixbuf, TRUE);

	for (gint i = 0; anims[i] != NULL; i++)
	{
		if (g_strcmp0(content_type, anims[i]) == 0)
		{
			anim = gdk_pixbuf_animation_new_from_file(FileToLoad, NULL);

			if (G_IS_OBJECT(anim))
			{
				gtk_anim_view_set_anim(GTK_ANIM_VIEW(view), anim);
				gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(view), TRUE);
				g_object_unref(anim);
			}
		}
	}

	g_free(content_type);

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

	tb_play = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_play, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_play), _("Play Animation"));
	g_signal_connect(G_OBJECT(tb_play), "clicked", G_CALLBACK(tb_play_clicked), (gpointer)view);

	tb_stop = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PAUSE);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_stop, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_stop), _("Stop Animation"));
	g_signal_connect(G_OBJECT(tb_stop), "clicked", G_CALLBACK(tb_stop_clicked), (gpointer)view);

	gtk_toolbar_insert(GTK_TOOLBAR(mtb), gtk_separator_tool_item_new(), tb_pos++);

	tb_size = gtk_tool_item_new();
	gtk_container_add(GTK_CONTAINER(tb_size), label);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_size, tb_pos++);

	gtk_widget_grab_focus(view);
	gtk_widget_show_all(gFix);

	if (!anim || gdk_pixbuf_animation_is_static_image(anim))
	{
		gtk_widget_hide(GTK_WIDGET(tb_play));
		gtk_widget_hide(GTK_WIDGET(tb_stop));
	}

	const gchar *role = gtk_window_get_role(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ParentWin))));

	if (gHideToolbar && g_strcmp0(role, "TfrmViewer") != 0)
		gtk_widget_hide(mtb);

	g_object_set_data(G_OBJECT(gFix), "imageview", view);
	g_object_set_data(G_OBJECT(gFix), "play", GTK_WIDGET(tb_play));
	g_object_set_data(G_OBJECT(gFix), "stop", GTK_WIDGET(tb_stop));

	if (G_IS_OBJECT(pixbuf))
		g_object_unref(pixbuf);

	return gFix;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	GdkPixbufAnimation *anim = NULL;

	GtkWidget *view = (GtkWidget*)g_object_get_data(G_OBJECT(PluginWin), "imageview");
	gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(view), FALSE);

	if (!g_file_test(FileToLoad, G_FILE_TEST_IS_REGULAR))
		return LISTPLUGIN_ERROR;

	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(FileToLoad, NULL);

	if (!pixbuf)
		return LISTPLUGIN_ERROR;

	gchar *content_type = g_content_type_guess(FileToLoad, NULL, 0, NULL);
	g_print("%s (%s): content_type = %s\n", PLUGNAME, FileToLoad, content_type);

	gtk_anim_view_set_anim(GTK_ANIM_VIEW(view), NULL);
	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), NULL, FALSE);

	for (gint i = 0; anims[i] != NULL; i++)
	{
		if (g_strcmp0(content_type, anims[i]) == 0)
			anim = gdk_pixbuf_animation_new_from_file(FileToLoad, NULL);
	}

	g_free(content_type);

	GtkWidget *tb_play = (GtkWidget*)g_object_get_data(G_OBJECT(PluginWin), "play");
	GtkWidget *tb_stop = (GtkWidget*)g_object_get_data(G_OBJECT(PluginWin), "stop");

	if (!anim || gdk_pixbuf_animation_is_static_image(anim))
	{
		gtk_widget_hide(GTK_WIDGET(tb_play));
		gtk_widget_hide(GTK_WIDGET(tb_stop));
	}
	else
	{
		gtk_widget_show(GTK_WIDGET(tb_play));
		gtk_widget_show(GTK_WIDGET(tb_stop));
	}

	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), pixbuf, TRUE);

	if (G_IS_OBJECT(anim))
	{
		gtk_anim_view_set_anim(GTK_ANIM_VIEW(view), anim);
		gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(view), TRUE);
		g_object_unref(anim);
	}

	if (G_IS_OBJECT(pixbuf))
		g_object_unref(pixbuf);

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	GtkWidget *view = (GtkWidget*)g_object_get_data(G_OBJECT(ListWin), "imageview");
	gtk_anim_view_set_is_playing(GTK_ANIM_VIEW(view), FALSE);

	GdkPixbufAnimation *anim = gtk_anim_view_get_anim(GTK_ANIM_VIEW(view));
	GdkPixbuf *pixbuf = gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(view));
	GdkPixbufAnimationIter *iter = GTK_ANIM_VIEW(view)->iter;

	gtk_anim_view_set_anim(GTK_ANIM_VIEW(view), NULL);
	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), NULL, FALSE);

	gtk_widget_destroy(GTK_WIDGET(ListWin));
/*
	if (G_IS_OBJECT(iter))
		g_object_unref(iter);

	if (G_IS_OBJECT(anim))
		g_object_unref(anim);
*/
	if (G_IS_OBJECT(pixbuf))
		g_object_unref(pixbuf);
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
