#define _GNU_SOURCE

#include <gio/gio.h>
#include "gtkimageview/gtkimageview.h"
#include "wlxplugin.h"

#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

//gboolean gHideToolbar = TRUE;

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GdkPixbufAnimation *anim = gdk_pixbuf_animation_new_from_file(FileToLoad, NULL);

	if (!anim)
		return NULL;

	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), scroll);
	GtkWidget *view = gtk_image_view_new();
	gtk_container_add(GTK_CONTAINER (scroll), view);
	gtk_image_view_set_angle(GTK_IMAGE_VIEW (view), 0);
	gtk_image_view_set_fit_allocation(GTK_IMAGE_VIEW(view), TRUE);
	gtk_image_view_set_zoomable(GTK_IMAGE_VIEW(view), TRUE);
	gtk_image_view_set_animation(GTK_IMAGE_VIEW(view), anim, 1.0);
	g_object_set_data(G_OBJECT(scroll), "imageview", view); 
	gtk_widget_show_all(scroll);

	return scroll;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *view = (GtkWidget*)g_object_get_data(G_OBJECT(PluginWin), "imageview");

	GdkPixbufAnimation *anim = gdk_pixbuf_animation_new_from_file(FileToLoad, NULL);

	if (!anim)
		return LISTPLUGIN_ERROR;

	gtk_image_view_set_animation(GTK_IMAGE_VIEW(view), anim, 1.0);

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	GtkWidget *view = (GtkWidget*)g_object_get_data(G_OBJECT(ListWin), "imageview");
	GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);
	gtk_image_view_set_pixbuf(GTK_IMAGE_VIEW(view), pixbuf, 1.0);
	gtk_widget_destroy(GTK_WIDGET(ListWin));
	g_object_unref(pixbuf);
}
/*
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
*/
int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

static void wlxplug_atexit(void)
{
	printf("%s atexit\n", PLUGNAME);
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	atexit(wlxplug_atexit);
/*
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
*/
}
