#define _GNU_SOURCE

#include <gtk/gtk.h>
#include <math.h>
#include "wlxplugin.h"

#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

#define ZOOM_IN_FACTOR  1.1
#define ZOOM_OUT_FACTOR  0.9
#define ZOOM_IN_MAX  5.0
#define ZOOM_OUT_MAX 0.1

#define ALLOC_CENTER alloc.width / 2.0, alloc.height / 2.0
#define FROM_TOPLEFT -(data->width / 2.0), -(data->height / 2.0)
#define RECT_SIZE 16
//#define RECT1_COLOR 0.65, 0.65, 0.65
//#define RECT2_COLOR 0.85, 0.85, 0.85
#define RECT1_COLOR 0.3, 0.3, 0.3
#define RECT2_COLOR 0.4, 0.4, 0.4


typedef struct
{
	GtkWidget *canvas;
	GtkWidget *scroll;
	GtkToolItem *tb_play;
	GtkToolItem *tb_stop;
	GtkWidget *info_label;
	GdkPixbufAnimation *anim;
	GdkPixbufAnimationIter *iter;
	cairo_surface_t *surf;
	cairo_pattern_t *shashechki;
	gboolean is_fit_only_large;
	gboolean is_transparent;
	gboolean is_playing;
	gboolean is_static;
	gboolean is_fit;
	guint anim_timeout_id;
	guint width;
	guint height;
	guint widget_width;
	guint widget_height;
	double zoom;
	int angle;
	int hflip;
	int vflip;
} CustomData;

gboolean gHideToolbar = TRUE;

static void clear_data(CustomData *data)
{
	data->is_playing = FALSE;

	if (data->anim_timeout_id > 0)
	{
		g_source_remove(data->anim_timeout_id);
		data->anim_timeout_id = 0;
	}

	if (data->surf)
	{
		cairo_surface_destroy(data->surf);
		data->surf = NULL;
	}

	if (data->iter)
	{
		g_object_unref(data->iter);
		data->iter = NULL;
	}

	if (data->anim)
	{
		g_object_unref(data->anim);
		data->anim = NULL;
	}
}

static cairo_pattern_t* create_shashechki()
{
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
	                           RECT_SIZE * 2, RECT_SIZE * 2);
	cairo_t *cr_p = cairo_create(surface);

	cairo_set_source_rgb(cr_p, RECT1_COLOR);
	cairo_rectangle(cr_p, 0, 0, RECT_SIZE, RECT_SIZE);
	cairo_rectangle(cr_p, RECT_SIZE, RECT_SIZE, RECT_SIZE, RECT_SIZE);
	cairo_fill(cr_p);

	cairo_set_source_rgb(cr_p, RECT2_COLOR);
	cairo_rectangle(cr_p, RECT_SIZE, 0, RECT_SIZE, RECT_SIZE);
	cairo_rectangle(cr_p, 0, RECT_SIZE, RECT_SIZE, RECT_SIZE);
	cairo_fill(cr_p);

	cairo_pattern_t *shashechki = cairo_pattern_create_for_surface(surface);
	cairo_pattern_set_extend(shashechki, CAIRO_EXTEND_REPEAT);

	cairo_destroy(cr_p);
	cairo_surface_destroy(surface);

	return shashechki;
}

static gboolean load_file(CustomData **data, const char *FileToLoad)
{
	GError *err = NULL;
	GdkPixbufAnimation *new_anim = gdk_pixbuf_animation_new_from_file(FileToLoad, &err);

	if (!new_anim)
	{
		if (err)
		{
			g_printerr("%s (load_file): %s\n", PLUGNAME, err->message);
			g_error_free(err);
		}

		return FALSE;
	}

	if (!*data)
	{
		*data = g_new0(CustomData, 1);
		(*data)->shashechki = create_shashechki();
	}
	else
	{
		clear_data(*data);
	}

	(*data)->zoom = 1.0;
	(*data)->angle = 0;
	(*data)->hflip = 1;
	(*data)->vflip = 1;
	(*data)->is_fit = TRUE;
	(*data)->is_playing = TRUE;

	(*data)->anim = new_anim;
	(*data)->is_static = gdk_pixbuf_animation_is_static_image(new_anim);
	GdkPixbuf *pixbuf = gdk_pixbuf_animation_get_static_image(new_anim);
	(*data)->surf = gdk_cairo_surface_create_from_pixbuf(pixbuf, 1, NULL);

	if ((*data)->is_static == FALSE)
	{
		(*data)->iter = gdk_pixbuf_animation_get_iter(new_anim, NULL);
		(*data)->is_transparent = FALSE;
	}
	else
		(*data)->is_transparent = gdk_pixbuf_get_has_alpha(pixbuf);

	(*data)->width = gdk_pixbuf_get_width(pixbuf);
	(*data)->height = gdk_pixbuf_get_height(pixbuf);

	return TRUE;
}

static void get_image_size(CustomData *data, GtkAllocation *alloc, gboolean is_zoomed)
{
	alloc->x = 0;
	alloc->y = 0;

	double zoom = is_zoomed ? data->zoom : 1.0;
	gboolean is_swapped = (data->angle % 180 != 0);

	if (!is_swapped)
	{
		alloc->width = data->width * zoom;
		alloc->height = data->height * zoom;
	}
	else
	{
		alloc->width = data->height * zoom;
		alloc->height = data->width * zoom;
	}
}

void refresh_ui(CustomData *data)
{
	GtkAllocation canvas_size;
	get_image_size(data, &canvas_size, TRUE);

	gtk_widget_set_size_request(data->canvas, canvas_size.width, canvas_size.height);

	gchar *text = NULL;

	if (data->zoom != 1.0)
		text = g_strdup_printf("%dx%d (%dx%d %.0f%%)", data->width,
		                       data->height, canvas_size.width,
		                       canvas_size.height, data->zoom * 100);
	else
		text = g_strdup_printf("%dx%d", data->width, data->height);

	gtk_label_set_text(GTK_LABEL(data->info_label), text);
	g_free(text);

	gtk_widget_queue_draw(data->canvas);

	gtk_widget_set_visible(GTK_WIDGET(data->tb_play), !data->is_static);
	gtk_widget_set_visible(GTK_WIDGET(data->tb_stop), !data->is_static);
}

void do_fit(CustomData *data)
{
	GtkAllocation img_size;
	get_image_size(data, &img_size, FALSE);

	if (data->widget_width < 10)
		return;

	data->zoom = fmin((double)data->widget_width / img_size.width,
	                  (double)data->widget_height / img_size.height);

	if (data->is_fit_only_large)
		data->zoom = fmin(data->zoom, 1.0);

	data->is_fit = TRUE;

	refresh_ui(data);
}

void do_zoom_in(CustomData *data)
{
	double new = data->zoom * ZOOM_IN_FACTOR;
	data->zoom = (new <= ZOOM_IN_MAX) ? new : ZOOM_IN_MAX;
	data->is_fit = FALSE;
	refresh_ui(data);
}

void do_zoom_out(CustomData *data)
{
	double new = data->zoom * ZOOM_OUT_FACTOR;
	data->zoom = (new >= ZOOM_OUT_MAX) ? new : ZOOM_OUT_MAX;
	data->is_fit = FALSE;
	refresh_ui(data);
}

static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, CustomData* data)
{
	GtkAllocation alloc;
	gtk_widget_get_allocation(widget, &alloc);

	GtkStyleContext *context = gtk_widget_get_style_context(widget);
	gtk_render_background(context, cr, 0, 0, alloc.width, alloc.height);

	if (data->is_transparent)
	{
		GtkAllocation zoom_size;
		get_image_size(data, &zoom_size, TRUE);
		cairo_rectangle(cr, (alloc.width - zoom_size.width) / 2.0,
				(alloc.height - zoom_size.height) / 2.0,
				zoom_size.width, zoom_size.height);
		cairo_clip(cr);

		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		cairo_set_source(cr, data->shashechki);
		cairo_paint(cr);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
	}

	cairo_save(cr);
	cairo_translate(cr, ALLOC_CENTER);
	cairo_scale(cr, data->zoom * data->hflip, data->zoom * data->vflip);
	cairo_rotate(cr, data->angle * G_PI / 180.0);

	if (data->is_static)
	{
		if (data->surf)
			cairo_set_source_surface(cr, data->surf, FROM_TOPLEFT);
	}
	else
	{
		GdkPixbuf *pixbuf = gdk_pixbuf_animation_iter_get_pixbuf(data->iter);
		gdk_cairo_set_source_pixbuf(cr, pixbuf, FROM_TOPLEFT);
	}

	//cairo_pattern_set_filter(cairo_get_source(cr), data->zoom < 0.5 ? CAIRO_FILTER_FAST : CAIRO_FILTER_BILINEAR);
	cairo_pattern_set_filter(cairo_get_source(cr), data->zoom < 0.7 ? CAIRO_FILTER_FAST : CAIRO_FILTER_GOOD);

	cairo_paint(cr);
	cairo_restore(cr);

	return FALSE;
}

static gboolean update_anim(CustomData* data)
{
	data->anim_timeout_id = 0;

	if (data->is_static || !data->is_playing)
		return FALSE;

	if (gdk_pixbuf_animation_iter_advance(data->iter, NULL))
	{
		//refresh_ui(data);
		gtk_widget_queue_draw(data->canvas);
	}

	int delay = gdk_pixbuf_animation_iter_get_delay_time(data->iter);

	if (delay < 0)
		delay = 100;

	data->anim_timeout_id = g_timeout_add(delay, (GSourceFunc)update_anim, data);

	return FALSE;
}

static gboolean scroll_cb(GtkWidget *w, GdkEventScroll *e, CustomData* data)
{
	if (e->state & GDK_CONTROL_MASK)
	{
		if (e->direction == GDK_SCROLL_UP)
			do_zoom_in(data);
		else if (e->direction == GDK_SCROLL_DOWN)
			do_zoom_out(data);
		else if (e->direction == GDK_SCROLL_SMOOTH)
		{
			double delta_y;
			gdk_event_get_scroll_deltas((GdkEvent *)e, NULL, &delta_y);

			if (delta_y < 0)
				do_zoom_in(data);
			else
				do_zoom_out(data);
		}

		return TRUE;
	}

	return FALSE;
}

static void resize_cb(GtkWidget *w, GtkAllocation *al, CustomData *data)
{
	if (data->widget_width != al->width || data->widget_height != al->height)
	{
		data->widget_width = al->width;
		data->widget_height = al->height;

		// WHAT THE FLYING FUUUUUUUU
		//if (data->is_fit)
		//	do_fit(data);
	}
}

static void tb_zoom_in_clicked(GtkToolItem *item, CustomData *data)
{
	do_zoom_in(data);
}

static void tb_zoom_out_clicked(GtkToolItem *item, CustomData *data)
{
	do_zoom_out(data);
}

static void tb_orgsize_clicked(GtkToolItem *item, CustomData *data)
{
	data->is_fit = FALSE;
	data->zoom = 1.0;
	refresh_ui(data);
}

static void tb_fit_clicked(GtkToolItem *item, CustomData *data)
{
	do_fit(data);
}

static void tb_copy_clicked(GtkToolItem *item, CustomData *data)
{
	GdkPixbuf *pixbuf = NULL;

	if (!data->is_static)
		pixbuf = gdk_pixbuf_animation_iter_get_pixbuf(data->iter);
	else
		pixbuf = gdk_pixbuf_animation_get_static_image(data->anim);

	gtk_clipboard_set_image(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), pixbuf);
}

static void tb_rotare_clicked(GtkToolItem *item, CustomData *data)
{
	data->angle = (data->angle - 90) % 360;
	refresh_ui(data);
}

static void tb_rotare1_clicked(GtkToolItem *item, CustomData *data)
{
	data->angle = (data->angle + 90) % 360;
	refresh_ui(data);
}

static void tb_hflip_clicked(GtkToolItem *item, CustomData *data)
{
	data->hflip *= -1;
	refresh_ui(data);
}

static void tb_vflip_clicked(GtkToolItem *item, CustomData *data)
{
	data->vflip *= -1;
	refresh_ui(data);
}

static void tb_play_clicked(GtkToolItem *item, CustomData *data)
{
	data->is_playing = TRUE;

	if (data->anim_timeout_id == 0)
		update_anim(data);

	refresh_ui(data);
}

static void tb_stop_clicked(GtkToolItem *item, CustomData *data)
{
	data->is_playing = FALSE;
	refresh_ui(data);
}

static GtkWidget *create_ui(GtkWidget *ParentWin, CustomData *data)
{
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

	if (!data)
		return NULL;

	GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_add_events(main_box, GDK_SCROLL_MASK);

	GtkWidget *mtb = gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(mtb), TRUE);
	gtk_toolbar_set_style(GTK_TOOLBAR(mtb), GTK_TOOLBAR_ICONS);

	guint tb_pos = 0;

	tb_zoom_in = gtk_tool_button_new(NULL, _("Zoom In"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_zoom_in), "zoom-in");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_zoom_in, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_zoom_in), _("Zoom In"));
	g_signal_connect(G_OBJECT(tb_zoom_in), "clicked", G_CALLBACK(tb_zoom_in_clicked), data);

	tb_zoom_out = gtk_tool_button_new(NULL, _("Zoom Out"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_zoom_out), "zoom-out");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_zoom_out, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_zoom_out), _("Zoom Out"));
	g_signal_connect(G_OBJECT(tb_zoom_out), "clicked", G_CALLBACK(tb_zoom_out_clicked), data);

	tb_orgsize = gtk_tool_button_new(NULL, _("Original Size"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_orgsize), "zoom-original");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_orgsize, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_orgsize), _("Original Size"));
	g_signal_connect(G_OBJECT(tb_orgsize), "clicked", G_CALLBACK(tb_orgsize_clicked), data);

	tb_fit = gtk_tool_button_new(NULL, _("Fit"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_fit), "zoom-fit-best");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_fit, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_fit), _("Fit"));
	g_signal_connect(G_OBJECT(tb_fit), "clicked", G_CALLBACK(tb_fit_clicked), data);

	gtk_toolbar_insert(GTK_TOOLBAR(mtb), gtk_separator_tool_item_new(), tb_pos++);

	tb_copy = gtk_tool_button_new(NULL, _("Copy to Clipboard"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_copy), "edit-copy");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_copy, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_copy), _("Copy to Clipboard"));
	g_signal_connect(G_OBJECT(tb_copy), "clicked", G_CALLBACK(tb_copy_clicked), data);

	tb_rotare = gtk_tool_button_new(NULL, _("Rotate"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_rotare), "object-rotate-left");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_rotare, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_rotare), _("Rotate"));
	g_signal_connect(G_OBJECT(tb_rotare), "clicked", G_CALLBACK(tb_rotare_clicked), data);

	tb_rotare1 = gtk_tool_button_new(NULL, _("Rotate Clockwise"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_rotare1), "object-rotate-right");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_rotare1, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_rotare1), _("Rotate Clockwise"));
	g_signal_connect(G_OBJECT(tb_rotare1), "clicked", G_CALLBACK(tb_rotare1_clicked), data);

	tb_hflip = gtk_tool_button_new(NULL, _("Flip Horizontally"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_hflip), "object-flip-horizontal");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_hflip, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_hflip), _("Flip Horizontally"));
	g_signal_connect(G_OBJECT(tb_hflip), "clicked", G_CALLBACK(tb_hflip_clicked), data);

	tb_vflip = gtk_tool_button_new(NULL, _("Flip Vertically"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_vflip), "object-flip-vertical");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_vflip, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_vflip), _("Flip Vertically"));
	g_signal_connect(G_OBJECT(tb_vflip), "clicked", G_CALLBACK(tb_vflip_clicked), data);

	data->tb_play = gtk_tool_button_new(NULL, _("Play Animation"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(data->tb_play), "media-playback-start");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), data->tb_play, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(data->tb_play), _("Play Animation"));
	g_signal_connect(G_OBJECT(data->tb_play), "clicked", G_CALLBACK(tb_play_clicked), data);

	data->tb_stop = gtk_tool_button_new(NULL, _("Stop Animation"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(data->tb_stop), "media-playback-pause");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), data->tb_stop, tb_pos++);
	gtk_widget_set_tooltip_text(GTK_WIDGET(data->tb_stop), _("Stop Animation"));
	g_signal_connect(G_OBJECT(data->tb_stop), "clicked", G_CALLBACK(tb_stop_clicked), data);

	gtk_toolbar_insert(GTK_TOOLBAR(mtb), gtk_separator_tool_item_new(), tb_pos++);

	data->info_label = gtk_label_new(NULL);
	tb_size = gtk_tool_item_new();
	gtk_container_add(GTK_CONTAINER(tb_size), data->info_label);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_size, tb_pos++);

	data->scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(data->scroll),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	data->canvas = gtk_drawing_area_new();
	//gtk_widget_set_halign(data->canvas, GTK_ALIGN_CENTER);
	//gtk_widget_set_valign(data->canvas, GTK_ALIGN_CENTER);
	gtk_container_add(GTK_CONTAINER(data->scroll), data->canvas);

	gtk_box_pack_start(GTK_BOX(main_box), mtb, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(main_box), data->scroll, TRUE, TRUE, 0);

	g_object_set_data(G_OBJECT(main_box), "custom-data", data);
	gtk_container_add(GTK_CONTAINER(ParentWin), main_box);
	gtk_widget_show_all(main_box);

	g_signal_connect(data->scroll, "size-allocate", G_CALLBACK(resize_cb), data);
	g_signal_connect(data->canvas, "draw", G_CALLBACK(draw_cb), data);
	g_signal_connect(data->scroll, "scroll-event", G_CALLBACK(scroll_cb), data);
	g_signal_connect(main_box, "scroll-event", G_CALLBACK(scroll_cb), data);

	const gchar *role = gtk_window_get_role(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ParentWin))));

	if (gHideToolbar && g_strcmp0(role, "TfrmViewer") != 0)
		gtk_widget_hide(mtb);

	if (!data->is_static)
		update_anim(data);

	//refresh_ui(data);
	g_timeout_add(500, (GSourceFunc)do_fit, data);

	return main_box;
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	CustomData *data = NULL;

	if (!load_file(&data, FileToLoad))
		return NULL;

	data->is_fit_only_large = TRUE;

	return create_ui(ParentWin, data);
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	CustomData *data = (CustomData*)g_object_get_data(G_OBJECT(PluginWin), "custom-data");

	if (!load_file(&data, FileToLoad))
		return LISTPLUGIN_ERROR;

	if (!data->is_static)
		update_anim(data);

	do_fit(data);

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	CustomData *data = (CustomData*)g_object_get_data(G_OBJECT(ListWin), "custom-data");
	gtk_widget_destroy(GTK_WIDGET(ListWin));
	clear_data(data);

	if (data->shashechki)
		cairo_pattern_destroy(data->shashechki);

	g_free(data);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	CustomData *data = (CustomData*)g_object_get_data(G_OBJECT(ListWin), "custom-data");

	if (Command == lc_copy)
	{
		tb_copy_clicked(NULL, data);
	}

	return LISTPLUGIN_OK;
}

static void wlxplug_atexit(void)
{
	g_print("%s atexit\n", PLUGNAME);
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	atexit(wlxplug_atexit);

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
