#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <dlfcn.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "wlxplugin.h"

#define MPV_PARAMS "--force-window=yes --keep-open=yes --script-opts=osc-visibility=always --cursor-autohide-fs-only"

gboolean is_pkill = FALSE;
static char cfg_path[PATH_MAX];

gchar *get_file_ext(const gchar *filename)
{
	if (g_file_test(filename, G_FILE_TEST_IS_DIR))
		return NULL;

	gchar *basename, *result, *tmpval;

	basename = g_path_get_basename(filename);
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

gchar *cfg_get_value(GKeyFile *cfg, const gchar *key, const gchar *ext, const gchar *content)
{
	gchar *result = NULL, *mediatype;

	if (ext)
		result = g_key_file_get_string(cfg, ext, key, NULL);

	if (!result && content)
	{
		result = g_key_file_get_string(cfg, content, key, NULL);

		if (!result)
		{
			mediatype = g_strdup_printf("%.5s", content);

			if (mediatype)
			{
				result = g_key_file_get_string(cfg, content, key, NULL);
				g_free(mediatype);
			}
		}
	}

	if (!result)
		result = g_key_file_get_string(cfg, "Default", key, NULL);


	return result;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GKeyFile *cfg;
	GError *err = NULL;
	gboolean is_certain = FALSE;
	gboolean is_socket = FALSE;
	gchar *exec = NULL;
	gchar *params = NULL;
	GdkNativeWindow id;
	GtkWidget *gFix;
	GtkWidget *mpv;
	GPid mpv_pid;
	gchar **argv;

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
		g_print("%s (%s): %s\n", PLUGNAME, cfg_path, (err)->message);
	else
	{
		gchar *ext = get_file_ext(FileToLoad);
		gchar *content_type = g_content_type_guess(FileToLoad, NULL, 0, &is_certain);

		if (g_key_file_get_boolean(cfg, "Default", "PrintContentType", NULL))
			g_print("%s (%s): content_type = %s\n", PLUGNAME, FileToLoad, content_type);

		is_socket = g_key_file_get_boolean(cfg, "Default", "GTK_Socket", NULL);
		is_pkill = g_key_file_get_boolean(cfg, "Default", "Use_pkill", NULL);

		exec = cfg_get_value(cfg, "Cmd", ext, content_type);
		params = cfg_get_value(cfg, "Params", ext, content_type);
		g_free(content_type);
		g_free(ext);
	}

	g_key_file_free(cfg);

	if (err)
		g_error_free(err);


	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);

	if (is_socket)
	{
		mpv = gtk_socket_new();
		gtk_container_add(GTK_CONTAINER(gFix), mpv);
		id = gtk_socket_get_id(GTK_SOCKET(mpv));
	}
	else
	{
		mpv = gtk_drawing_area_new();
		gtk_container_add(GTK_CONTAINER(gFix), mpv);
		gtk_widget_realize(mpv);
		id = GDK_WINDOW_XID(gtk_widget_get_window(mpv));

		GdkColor color;
		gdk_color_parse("black", &color);
		gtk_widget_modify_bg(mpv, GTK_STATE_NORMAL, &color);
	}

	gchar *quoted = g_shell_quote(FileToLoad);
	gchar *command = g_strdup_printf("%s %s --wid=%d %s", exec ? exec : "mpv", params ? params : MPV_PARAMS, id, quoted);
	g_free(exec);
	g_free(params);
	g_free(quoted);

	if (!g_shell_parse_argv(command, NULL, &argv, NULL))
	{
		g_free(command);
		gtk_widget_destroy(gFix);
		return NULL;
	}

	g_free(command);

	if ((id == 0) || (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &mpv_pid, NULL)))
	{
		g_strfreev(argv);
		gtk_widget_destroy(gFix);
		return NULL;
	}
	
	g_strfreev(argv);

	g_object_set_data(G_OBJECT(gFix), "pid", GINT_TO_POINTER(mpv_pid));

	gtk_widget_show_all(gFix);

	return gFix;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	if (is_pkill)
	{
		int status;
		pid_t mpv_pid = (pid_t)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(ListWin), "pid"));
		kill(mpv_pid, SIGTERM);
		g_print("ListCloseWindow: mpv_pid=%d, waitpid=%d, exit_status=%d\n", mpv_pid, waitpid(mpv_pid, &status, 0), WEXITSTATUS(status));
	}

	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	GKeyFile *cfg;
	gchar *string = NULL;

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, NULL))
		g_strlcpy(DetectString, DETECT_STRING, maxlen - 1);
	else
	{
		string = g_key_file_get_string(cfg, "Default", "DetectString", NULL);

		if (!string)
			g_strlcpy(DetectString, DETECT_STRING, maxlen - 1);
		else
		{
			g_strlcpy(DetectString, string, maxlen - 1);
			g_free(string);
		}
	}

	g_key_file_free(cfg);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	return LISTPLUGIN_ERROR;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	gchar cfg_name[] = "settings.ini"; 
	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, cfg_name);
	}
}
