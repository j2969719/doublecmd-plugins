#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <vte/vte.h>
#include <dlfcn.h>
#include <limits.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring "EXT=\"*\""
#define _plgname "vte_ncdu.wlx"

gchar *font, *bgimage, *bgcolor, *cmdstr;
gdouble saturation;

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	if (!g_file_test(FileToLoad, G_FILE_TEST_IS_DIR))
		return NULL;

	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *vte;
	VteTerminal *terminal;
	GdkColor color;
	gint argcp;
	gchar **command = NULL;
	gchar *tmpcmd;

	if (cmdstr)
	{
		tmpcmd = g_strdup_printf("%s %s", cmdstr, g_shell_quote(FileToLoad));
		g_shell_parse_argv(tmpcmd, &argcp, &command, NULL);
	}

	if (!command)
		return NULL;


	gFix = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);

	vte = vte_terminal_new();
	terminal = VTE_TERMINAL(vte);
	scroll = gtk_scrolled_window_new(NULL, terminal->adjustment);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	if (!vte_terminal_fork_command_full(terminal, VTE_PTY_DEFAULT, FileToLoad, command, NULL, 0, NULL, NULL, NULL, NULL))
		return NULL;

	vte_terminal_set_scrollback_lines(terminal, -1);
	vte_terminal_set_size(terminal, 80, 60);

	if (font)
		vte_terminal_set_font_from_string(terminal, font);

	if (bgimage)
	{
		vte_terminal_set_background_image_file(terminal, bgimage);

		if (bgcolor)
		{
			gdk_color_parse(bgcolor, &color);
			vte_terminal_set_background_tint_color(terminal, &color);
		}

		if (saturation >= 0)
			vte_terminal_set_background_saturation(terminal, saturation);

		vte_terminal_set_background_transparent(terminal, FALSE);
	}

	gtk_box_pack_start(GTK_BOX(gFix), scroll, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(scroll), vte);
	gtk_widget_show_all(gFix);

	return gFix;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen-1);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	static char cfg_path[PATH_MAX];
	const char* cfg_file = "settings.ini";
	GKeyFile *cfg;
	GError *err = NULL;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, cfg_file);
	}

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
		g_print("%s (%s): %s\n", _plgname, cfg_path, (err)->message);
	else
	{
		font = g_key_file_get_string(cfg, "VTE", "Font", NULL);
		bgimage = g_key_file_get_string(cfg, "VTE", "BGImage", NULL);
		bgcolor = g_key_file_get_string(cfg, "VTE", "BGTintColor", NULL);
		saturation = g_key_file_get_double(cfg, "VTE", "BGSaturation", NULL);
		cmdstr = g_key_file_get_string(cfg, _plgname, "Command", NULL);
	}

	g_key_file_free(cfg);

	if (err)
		g_error_free(err);
}
