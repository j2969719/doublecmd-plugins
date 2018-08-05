#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <vte/vte.h>
#include <glib.h>
#include <magic.h>
#include <dlfcn.h>
#include <limits.h>
#include <string.h>
#include "wlxplugin.h"

#define _detectstring "EXT=\"*\""
#define _plgname "yavteplg.wlx"

static char cfg_path[PATH_MAX];
const char* cfg_file = "settings.ini";

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	GKeyFile *cfg;
	GError *err = NULL;
	gchar *fileext, *tmpval;
	magic_t magic_cookie;
	const gchar *mimetype;
	gchar *font, *bgimage, *bgcolor, *frmtcmd, *cmdstr;
	gchar **command;
	gdouble saturation;
	gint argcp;

	magic_cookie = magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK);
	if (magic_load(magic_cookie, NULL) != 0)
	{
		magic_close(magic_cookie);
		return NULL;
	}

	mimetype = magic_file(magic_cookie, FileToLoad);


	fileext = g_strrstr(FileToLoad, ".");

	if (fileext)
	{
		tmpval = g_strdup_printf("%s", fileext + 1);
		fileext = g_ascii_strdown(tmpval, -1);
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
		if (fileext)
			frmtcmd = g_key_file_get_string(cfg, fileext, "Command", NULL);
		if ((!fileext) || (!frmtcmd))
			frmtcmd = g_key_file_get_string(cfg, mimetype, "Command", NULL);

	}

	g_key_file_free(cfg);
	magic_close(magic_cookie);

	if (!frmtcmd)
		return NULL;

	cmdstr = g_strdup_printf(frmtcmd, g_shell_quote(FileToLoad));
	g_shell_parse_argv(cmdstr, &argcp, &command, NULL);

	if (err)
		g_error_free(err);

	if (!command)
		return NULL;


	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *vte;
	VteTerminal *terminal;
	GdkColor color;

	gFix = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);

	vte = vte_terminal_new();
	terminal = VTE_TERMINAL(vte);
	scroll = gtk_scrolled_window_new(NULL, terminal->adjustment);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	if (!vte_terminal_fork_command_full(terminal, VTE_PTY_DEFAULT, NULL, command, NULL, 0, NULL, NULL, NULL, NULL))
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
	g_strlcpy(DetectString, _detectstring, maxlen);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, cfg_file);
	}
}
