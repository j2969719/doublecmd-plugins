#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <dlfcn.h>
#include "wlxplugin.h"

#define _fontsize_min 8
#define _fontsize_max 60
#define _detectstring "EXT=\"NFO\"|EXT=\"DIZ\""
#define _plugname "nfoview.wlx"


gchar *fontname, *enc;
gint fontsize;

static void change_font_size(GtkWidget *scale, gpointer widget)
{
	gdouble value = gtk_range_get_value(GTK_RANGE(scale));
	gtk_widget_modify_font((GtkWidget*)widget, pango_font_description_from_string(g_strdup_printf("%s %d", fontname, (gint)value)));
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	gchar *contents, *result;

	if (!g_file_get_contents(FileToLoad, &contents, NULL, NULL))
		return NULL;

	result = g_convert(contents, -1, "UTF-8", enc, NULL, NULL, NULL);

	if ((result == NULL) || (g_strcmp0(result, "") == 0) || (!g_utf8_validate(result, -1, NULL)))
		return NULL;

	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *label;
	GtkWidget *scale;

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(gFix), scroll);
	scale = gtk_hscale_new_with_range(_fontsize_min, _fontsize_max, 1);
	gtk_range_set_value(GTK_RANGE(scale), fontsize);
	gtk_box_pack_start(GTK_BOX(gFix), scale, FALSE, FALSE, 1);
	label = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(label), FALSE);
	gtk_widget_modify_font(label, pango_font_description_from_string(g_strdup_printf("%s %d", fontname, fontsize)));
	gtk_label_set_text(GTK_LABEL(label), result);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), label);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	g_signal_connect(G_OBJECT(scale), "value-changed", G_CALLBACK(change_font_size), (gpointer)(GtkWidget*)label);
	gtk_widget_show_all(gFix);
	return gFix;

}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen - 1);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	gchar *cfg_path;
	GKeyFile *cfg;
	const gchar* cfg_name = "settings.ini";

	GError *err = NULL;

	memset(&dlinfo, 0, sizeof(dlinfo));
	fontname = NULL;
	enc = NULL;
	fontsize = 0;

	if (dladdr(cfg_name, &dlinfo) != 0)
	{
		cfg_path = g_strdup_printf("%s/%s", g_path_get_dirname(dlinfo.dli_fname), cfg_name);
		cfg = g_key_file_new();

		if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
			g_print("%s(%s): %s\n", _plugname, cfg_path, (err)->message);
		else
		{
			fontname = g_key_file_get_string(cfg, _plugname, "Font", NULL);
			enc = g_key_file_get_string(cfg, _plugname, "Enc", NULL);
			fontsize = g_key_file_get_integer(cfg, _plugname, "FontSize", NULL);
		}
		g_key_file_free(cfg);
	}

	if (err)
		g_error_free(err);

	if (!fontname)
		fontname = "monospace";


	if (!enc)
		enc = "437";

	if (fontsize < _fontsize_min || fontsize > _fontsize_max)
		fontsize = 11;
}
