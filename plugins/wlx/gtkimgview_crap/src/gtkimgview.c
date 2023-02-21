#define _GNU_SOURCE

#include <gio/gio.h>
#include <gtkimageview/gtkimageview.h>
#include <gtkimageview/gtkimagescrollwin.h>
#include "wlxplugin.h"

#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

static gchar g_cfgpath[PATH_MAX];

static gchar *get_file_ext(const gchar *Filename)
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

static gchar *str_replace(gchar *text, gchar *str, gchar *repl)
{
	gchar **split = g_strsplit(text, str, -1);
	gchar *result = g_strjoinv(repl, split);
	g_strfreev(split);
	return result;
}

static gchar *replace_tmpls(gchar *orig, gchar *file, gchar *tmpdir, gchar *output, gboolean quote)
{
	gchar *result = g_strdup(orig);
	gchar *basename = g_path_get_basename(file);
	gchar *plugdir = g_path_get_dirname(g_cfgpath);
	gchar *filedir = g_path_get_dirname(file);
	gchar *basenamenoext = g_strdup(basename);
	gchar *dot = g_strrstr(basenamenoext, ".");

	if (dot)
	{
		int offset = dot - basenamenoext;
		basenamenoext[offset] = '\0';

		if (g_strcmp0("", basenamenoext) == 0)
			basenamenoext = g_strdup(basename);
	}

	result = str_replace(result, "$FILEDIR", quote ? g_shell_quote(filedir) : filedir);
	result = str_replace(result, "$FILE", quote ? g_shell_quote(file) : file);
	result = str_replace(result, "$IMG", quote ? g_shell_quote(output) : output);
	result = str_replace(result, "$TMPDIR", quote ? g_shell_quote(tmpdir) : tmpdir);
	result = str_replace(result, "$PLUGDIR", quote ? g_shell_quote(plugdir) : plugdir);
	result = str_replace(result, "$BASENAMENOEXT", basenamenoext);
	result = str_replace(result, "$BASENAME", basename);
	g_free(plugdir);
	g_free(filedir);
	g_free(basename);
	g_free(basenamenoext);
	return result;
}

static gboolean isabsolute(const gchar *file)
{
	if (file[0] == '/')
		return TRUE;
	else if (g_strrstr(file, "$TMPDIR") != NULL)
		return TRUE;
	else if (g_strrstr(file, "$FILEDIR") != NULL)
		return TRUE;
	else if (g_strrstr(file, "$PLUGDIR") != NULL)
		return TRUE;

	return FALSE;
}


static void rm_tmpdir(gchar *tmpdir)
{
	gchar *quoted = g_shell_quote(tmpdir);
	gchar *command = g_strdup_printf("rm -r %s", quoted);
	system(command);
	g_free(command);
	g_free(quoted);
}

static GdkPixbuf *load_pixbuf(GKeyFile *cfg, gchar *filename, gchar *ext, gchar *tmpdir)
{
	gchar *output = NULL;
	gchar *cmdtmpl = g_key_file_get_string(cfg, ext, "command", NULL);
	gboolean keeptmp = g_key_file_get_boolean(cfg, ext, "keeptmp", NULL);

	if (!cmdtmpl)
		return NULL;

	gchar *outfile = g_key_file_get_string(cfg, ext, "filename", NULL);

	if (outfile)
	{
		if (!isabsolute(outfile))
		{
			output = g_strdup_printf("$TMPDIR/%s", outfile);
			g_free(outfile);
			outfile = output;
		}

		output = replace_tmpls(outfile, filename, tmpdir, NULL, FALSE);
		g_free(outfile);
	}
	else
		output = g_strdup_printf("%s/output.png", tmpdir);


	gchar *command = replace_tmpls(cmdtmpl, filename, tmpdir, output, TRUE);
	g_free(cmdtmpl);
	g_print("%s\n", command);

	if (system(command) != 0)
	{
		g_free(command);
		g_free(output);

		if (!keeptmp)
			rm_tmpdir(tmpdir);

		return NULL;
	}

	g_free(command);

	if (!g_file_test(output, G_FILE_TEST_EXISTS))
	{
		g_free(output);
		outfile = g_key_file_get_string(cfg, ext, "fallbackopen", NULL);

		if (outfile)
		{

			if (!isabsolute(outfile))
			{
				output = g_strdup_printf("$TMPDIR/%s", outfile);
				g_free(outfile);
				outfile = output;
			}

			output = replace_tmpls(outfile, filename, tmpdir, NULL, FALSE);
			g_free(outfile);

			if (!g_file_test(output, G_FILE_TEST_EXISTS))
			{
				g_free(output);

				if (!keeptmp)
					rm_tmpdir(tmpdir);

				return NULL;
			}
		}
	}

	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(output, NULL);
	g_free(output);

	return pixbuf;
}

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
	GKeyFile *cfg;
	gchar *tmpdir;
	gchar *fileExt;
	gchar *bgcolor = NULL;
	gboolean hidetoolbar;
	gboolean keeptmp;
	gboolean quickview = FALSE;
	GdkColor color;

	fileExt = get_file_ext(FileToLoad);

	if (!fileExt)
		return NULL;

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, g_cfgpath, G_KEY_FILE_KEEP_COMMENTS, NULL))
	{
		g_key_file_free(cfg);
		g_free(fileExt);
		return NULL;
	}

	tmpdir = g_dir_make_tmp("_dc-imgview.XXXXXX", NULL);
	pixbuf = load_pixbuf(cfg, FileToLoad, fileExt, tmpdir);
	keeptmp = g_key_file_get_boolean(cfg, fileExt, "keeptmp", NULL);

	if (!pixbuf)
	{
		g_key_file_free(cfg);
		g_free(fileExt);

		if (!keeptmp)
			rm_tmpdir(tmpdir);

		g_free(tmpdir);
		return NULL;
	}

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

	bgcolor = g_key_file_get_string(cfg, fileExt, "bgcolor", NULL);
	hidetoolbar = g_key_file_get_boolean(cfg, fileExt, "hidetoolbar", NULL);

	if (bgcolor)
	{
		gtk_image_view_set_show_frame(GTK_IMAGE_VIEW(view), FALSE);
		gdk_color_parse(bgcolor, &color);
		gtk_widget_modify_bg(view, GTK_STATE_NORMAL, &color);
		g_free(bgcolor);
	}

	gtk_widget_grab_focus(view);
	gtk_widget_show_all(gFix);

	gchar *title = gtk_window_get_title(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ParentWin))));
	if (title[0] != '/' && title[0] != '~')
		quickview = TRUE;

	if (hidetoolbar || quickview)
		gtk_widget_hide(mtb);

	g_object_set_data(G_OBJECT(gFix), "imageview", view);
	g_object_set_data(G_OBJECT(gFix), "tmpdir", tmpdir);
	g_object_set_data(G_OBJECT(gFix), "toolbar", mtb);
	g_object_set_data(G_OBJECT(gFix), "config", cfg);
	g_object_set_data(G_OBJECT(gFix), "quickview", GINT_TO_POINTER((gint)quickview));

	if (G_IS_OBJECT(pixbuf))
		g_object_unref(pixbuf);

	return gFix;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	gchar *fileExt = get_file_ext(FileToLoad);

	if (!fileExt)
		return LISTPLUGIN_ERROR;

	gchar *tmpdir = g_object_get_data(G_OBJECT(PluginWin), "tmpdir");
	GKeyFile *cfg = g_object_get_data(G_OBJECT(PluginWin), "config");
	GtkWidget *view = g_object_get_data(G_OBJECT(PluginWin), "imageview");
	GtkWidget *mtb = g_object_get_data(G_OBJECT(PluginWin), "toolbar");
	GdkPixbuf *pixbuf = load_pixbuf(cfg, FileToLoad, fileExt, tmpdir);
	gboolean quickview = (gboolean)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(PluginWin), "quickview"));
	gboolean keeptmp = g_key_file_get_boolean(cfg, fileExt, "keeptmp", NULL);

	if (!pixbuf)
	{
		g_free(fileExt);

		return LISTPLUGIN_ERROR;
	}

	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), NULL, FALSE);

	gchar *bgcolor = g_key_file_get_string(cfg, fileExt, "bgcolor", NULL);
	gboolean hidetoolbar = g_key_file_get_boolean(cfg, fileExt, "hidetoolbar", NULL);

	if (bgcolor)
	{
		GdkColor color;
		gtk_image_view_set_show_frame(GTK_IMAGE_VIEW(view), FALSE);
		gdk_color_parse(bgcolor, &color);
		gtk_widget_modify_bg(view, GTK_STATE_NORMAL, &color);
		g_free(bgcolor);
	}
	else
		gtk_widget_modify_bg(view, GTK_STATE_NORMAL, NULL);

	if (hidetoolbar || quickview)
		gtk_widget_hide(mtb);
	else
		gtk_widget_show(mtb);

	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), pixbuf, TRUE);

	if (G_IS_OBJECT(pixbuf))
		g_object_unref(pixbuf);

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gchar *tmpdir = g_object_get_data(G_OBJECT(ListWin), "tmpdir");
	GtkWidget *view = (GtkWidget*)g_object_get_data(G_OBJECT(ListWin), "imageview");
	GdkPixbuf *pixbuf = gtk_image_view_get_pixbuf(GTK_IMAGE_VIEW(view));
	GKeyFile *cfg = g_object_get_data(G_OBJECT(ListWin), "config");
	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), NULL, FALSE);
	gtk_widget_destroy(GTK_WIDGET(ListWin));

	if (G_IS_OBJECT(pixbuf))
		g_object_unref(pixbuf);

	g_key_file_free(cfg);

	if (tmpdir)
	{
		rm_tmpdir(tmpdir);
		g_free(tmpdir);
	}
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
		gchar *cfgpath = g_strdup_printf("%s/settings.ini", plugdir);
		g_strlcpy(g_cfgpath, cfgpath, PATH_MAX);
		g_free(cfgpath);
		g_free(plugdir);
		bindtextdomain(GETTEXT_PACKAGE, langdir);
		g_free(langdir);
		textdomain(GETTEXT_PACKAGE);
	}
}
