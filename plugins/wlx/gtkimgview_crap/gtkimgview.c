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

gchar *cfgpath = "";

gchar *get_file_ext(const gchar *Filename)
{
	if (g_file_test(Filename, G_FILE_TEST_IS_DIR))
		return NULL;

	gchar *basename, *result, *tmpval;

	basename = g_path_get_basename(Filename);
	result = g_strrstr(basename, ".");

	if (result)
	{
		if (g_strcmp0(result, basename) != 0)
		{
			tmpval = g_strdup_printf("%s", result + 1);
			result = g_ascii_strdown(tmpval, -1);
			g_free(tmpval);
		}
		else
			result = NULL;
	}

	g_free(basename);

	return result;
}

gchar *str_replace(gchar *text, gchar *str, gchar *repl)
{
	gchar **split = g_strsplit(text, str, -1);
	gchar *result = g_strjoinv(repl, split);
	g_strfreev(split);
	return result;
}


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
	GKeyFile *cfg;
	gchar *command;
	gchar *tmpdir;
	gchar *output;
	gchar *fileExt;
	gchar *forcefile = NULL;

	fileExt = get_file_ext(FileToLoad);

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfgpath, G_KEY_FILE_KEEP_COMMENTS, NULL))
		return NULL;
	else
		command = g_key_file_get_string(cfg, fileExt, "command", NULL);

	forcefile = g_key_file_get_string(cfg, fileExt, "forceopen", NULL);

	g_key_file_free(cfg);

	if (!command)
		return NULL;

	tmpdir = g_dir_make_tmp("_dc-imgview.XXXXXX", NULL);
	output = g_strdup_printf("%s/output.png", tmpdir);
	command = str_replace(command, "$FILE", g_shell_quote(FileToLoad));
	command = str_replace(command, "$IMG", g_shell_quote(output));

	if (system(command) != 0)
		return NULL;

	if (forcefile)
		output = g_strdup_printf("%s/%s", tmpdir, forcefile);

	if (!g_file_test(output, G_FILE_TEST_IS_REGULAR))
		return NULL;

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

	pixbuf = gdk_pixbuf_new_from_file(output, NULL);

	if (!pixbuf)
	{
		gtk_widget_destroy(gFix);
		return NULL;
	}

	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), pixbuf, TRUE);

	tb_zoom_in = gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_IN);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_zoom_in, 0);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_zoom_in), _("Zoom In"));
	g_signal_connect(G_OBJECT(tb_zoom_in), "clicked", G_CALLBACK(tb_zoom_in_clicked), (gpointer)(GtkWidget*)(view));

	tb_zoom_out = gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_OUT);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_zoom_out, 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_zoom_out), _("Zoom Out"));
	g_signal_connect(G_OBJECT(tb_zoom_out), "clicked", G_CALLBACK(tb_zoom_out_clicked), (gpointer)(GtkWidget*)(view));

	tb_orgsize = gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_100);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_orgsize, 2);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_orgsize), _("Original Size"));
	g_signal_connect(G_OBJECT(tb_orgsize), "clicked", G_CALLBACK(tb_orgsize_clicked), (gpointer)(GtkWidget*)(view));

	tb_fit = gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_FIT);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_fit, 3);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_fit), _("Fit"));
	g_signal_connect(G_OBJECT(tb_fit), "clicked", G_CALLBACK(tb_fit_clicked), (gpointer)(GtkWidget*)(view));

	tb_separator = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_separator, 4);

	tb_copy = gtk_tool_button_new_from_stock(GTK_STOCK_COPY);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_copy, 5);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_copy), _("Copy to Clipboard"));
	g_signal_connect(G_OBJECT(tb_copy), "clicked", G_CALLBACK(tb_copy_clicked), (gpointer)(GtkWidget*)(view));

	tb_rotare = gtk_tool_button_new(NULL, _("Rotate"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_rotare), "object-rotate-left");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_rotare, 6);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_rotare), _("Rotate"));
	g_signal_connect(G_OBJECT(tb_rotare), "clicked", G_CALLBACK(tb_rotare_clicked), (gpointer)(GtkWidget*)(view));

	tb_rotare1 = gtk_tool_button_new(NULL, _("Rotate Clockwise"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_rotare1), "object-rotate-right");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_rotare1, 7);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_rotare1), _("Rotate Clockwise"));
	g_signal_connect(G_OBJECT(tb_rotare1), "clicked", G_CALLBACK(tb_rotare1_clicked), (gpointer)(GtkWidget*)(view));

	tb_hflip = gtk_tool_button_new(NULL, _("Flip Horizontally"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_hflip), "object-flip-horizontal");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_hflip, 8);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_hflip), _("Flip Horizontally"));
	g_signal_connect(G_OBJECT(tb_hflip), "clicked", G_CALLBACK(tb_hflip_clicked), (gpointer)(GtkWidget*)(view));

	tb_vflip = gtk_tool_button_new(NULL, _("Flip Vertically"));
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tb_vflip), "object-flip-vertical");
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_vflip, 9);
	gtk_widget_set_tooltip_text(GTK_WIDGET(tb_vflip), _("Flip Vertically"));
	g_signal_connect(G_OBJECT(tb_vflip), "clicked", G_CALLBACK(tb_vflip_clicked), (gpointer)(GtkWidget*)(view));

	tb_separator1 = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_separator1, 10);

	tstr = g_strdup_printf("%dx%d", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
	tb_size = gtk_tool_item_new();
	label = gtk_label_new(tstr);
	gtk_container_add(GTK_CONTAINER(tb_size), label);
	g_free(tstr);
	gtk_toolbar_insert(GTK_TOOLBAR(mtb), tb_size, tb_last);

	g_object_set_data(G_OBJECT(gFix), "tmpdir", tmpdir);

	gtk_widget_grab_focus(view);
	gtk_widget_show_all(gFix);

	if (g_strcmp0(gtk_window_get_title(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ParentWin)))), FileToLoad) != 0)
		gtk_widget_hide(mtb);

	return gFix;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gchar *tmpdir = g_object_get_data(G_OBJECT(ListWin), "tmpdir");
	gtk_widget_destroy(GTK_WIDGET(ListWin));

	if (tmpdir)
	{
		system(g_strdup_printf("rm -r %s", g_shell_quote(tmpdir)));
		g_free(tmpdir);
	}
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

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	const gchar* dir_f = "%s/langs";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(dir_f, &dlinfo) != 0)
	{
		setlocale(LC_ALL, "");
		bindtextdomain(GETTEXT_PACKAGE, g_strdup_printf(dir_f,
		                g_path_get_dirname(dlinfo.dli_fname)));
		textdomain(GETTEXT_PACKAGE);
		cfgpath = g_strdup_printf("%s/settings.ini", g_path_get_dirname(dlinfo.dli_fname));
	}
}
