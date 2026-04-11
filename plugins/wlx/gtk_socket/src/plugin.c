#define _GNU_SOURCE
#include <gtk/gtk.h>
#ifdef GTK3PLUG
#include <gtk/gtkx.h>
#endif
#include <gdk/gdkx.h>
#include <dlfcn.h>
#include <magic.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glib.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "wlxplugin.h"

#ifdef GTK3PLUG
#define GDK_Escape GDK_KEY_Escape
#define KEY_EVENT "key-press-event"
#else
#define KEY_EVENT "key_press_event"
#endif

static char cfg_path[PATH_MAX];
static char plug_path[PATH_MAX];
const char* cfg_file = "settings.ini";

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

static gchar *get_mime_type(const gchar *Filename)
{
	GFile *gfile = g_file_new_for_path(Filename);

	if (!gfile)
		return NULL;

	GFileInfo *fileinfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL, NULL);

	if (!fileinfo)
	{
		g_object_unref(gfile);
		return NULL;
	}

	gchar *content_type = g_strdup(g_file_info_get_content_type(fileinfo));
	g_object_unref(fileinfo);
	g_object_unref(gfile);

	return content_type;
}

static gchar *get_mime_type_magic(const gchar *Filename)
{
	magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE);

	if (!magic_cookie)
		return NULL;

	if (magic_load(magic_cookie, NULL) != 0)
	{
		magic_close(magic_cookie);
		return NULL;
	}

	gchar *content_type = g_strdup(magic_file(magic_cookie, Filename));
	magic_close(magic_cookie);

	return content_type;
}

static gchar *cfg_get_command(GKeyFile *Cfg, const gchar *Group)
{
	gchar *result, *cfg_value, *scr_path = NULL;
	cfg_value = g_key_file_get_string(Cfg, Group, "script", NULL);

	if (!cfg_value)
	{
		result = g_key_file_get_string(Cfg, Group, "command", NULL);

		if ((!result) || (!g_strrstr(result, "$FILE")) || (!g_strrstr(result, "$XID")))
			result = NULL;
	}
	else
	{
		scr_path = g_strdup_printf("%s/scripts/%s", plug_path, cfg_value);

		if (g_file_test(scr_path, G_FILE_TEST_EXISTS))
		{
			gchar *quotedpath = g_shell_quote(scr_path);
			result = g_strdup_printf("%s $XID $FILE", quotedpath);
			g_free(quotedpath);
		}
		else if (g_file_test(cfg_value, G_FILE_TEST_EXISTS))
			result = g_strdup_printf("%s $XID $FILE", cfg_value);
		else
			result = NULL;

		g_free(cfg_value);
	}

	return result;
}

static gchar *str_replace(gchar *text, gchar *str, gchar *repl)
{
	gchar **split = g_strsplit(text, str, -1);
	gchar *result = g_strjoinv(repl, split);
	g_strfreev(split);
	g_free(text);
	g_free(repl);
	return result;
}

static gchar *cfg_chk_redirect(GKeyFile *Cfg, const gchar *Group)
{
	gchar *result, *redirect;
	redirect = g_key_file_get_string(Cfg, Group, "redirect", NULL);

	if (redirect)
		return redirect;
	else
		return g_strdup(Group);
}

static void plug_added(GtkWidget *widget, gpointer data)
{
	gtk_spinner_stop(GTK_SPINNER(data));
	gtk_widget_hide(GTK_WIDGET(data));
	gtk_widget_show(widget);
}

gboolean key_press_crap(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	if (event->keyval == GDK_Escape)
		gtk_grab_remove(widget);

	return FALSE;
}


HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	GKeyFile *cfg;
	GError *err = NULL;
	GtkWidget *gFix;
	GtkWidget *socket;
	GtkWidget *wspin;
	gchar *file_ext, *mime_type;
	gchar *command = NULL;
	gchar *group = NULL;
	gboolean noquote;
	gboolean insensitive;
	gboolean nospinner;
	gboolean crapgrab;
	gint exit_timeout = 0;

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
		g_print("%s (%s): %s\n", PLUGNAME, cfg_path, (err)->message);
	else
	{
		if (g_key_file_get_boolean(cfg, PLUGNAME, "uselibmagic", NULL))
			mime_type = get_mime_type_magic(FileToLoad);
		else
			mime_type = get_mime_type(FileToLoad);

		file_ext = get_file_ext(FileToLoad);

		if (file_ext)
		{
			group = cfg_chk_redirect(cfg, file_ext);
			command = cfg_get_command(cfg, group);
			g_free(file_ext);
		}

		if (!command && mime_type)
		{
			if (group)
				g_free(group);

			group = cfg_chk_redirect(cfg, mime_type);
			command = cfg_get_command(cfg, group);
			g_free(mime_type);
		}

		noquote = g_key_file_get_boolean(cfg, group, "noquote", NULL);
		insensitive = g_key_file_get_boolean(cfg, group, "insensitive", NULL);
		crapgrab = g_key_file_get_boolean(cfg, group, "grab", NULL);
		exit_timeout = g_key_file_get_integer(cfg, group, "exit_timeout", NULL);

		if (exit_timeout < 1)
			exit_timeout = g_key_file_get_integer(cfg, PLUGNAME, "exit_timeout", NULL);

		nospinner = g_key_file_get_boolean(cfg, PLUGNAME, "nospinner", NULL);

		if (!nospinner)
			nospinner = g_key_file_get_boolean(cfg, group, "nospinner", NULL);

	}

	g_key_file_free(cfg);

	if (err)
		g_error_free(err);

	if (group)
		g_free(group);

	if (!command)
		return NULL;

#ifndef GTK3PLUG
	gFix = gtk_vbox_new(FALSE, 5);
#else
	gFix =gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_widget_set_no_show_all(gFix, TRUE);
#endif
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), gFix);
	socket = gtk_socket_new();

	if (!nospinner)
	{
		wspin = gtk_spinner_new();
		gtk_widget_show(wspin);
		gtk_spinner_start(GTK_SPINNER(wspin));
		gtk_box_pack_start(GTK_BOX(gFix), wspin, TRUE, TRUE, 0);
		g_signal_connect(socket, "plug-added", G_CALLBACK(plug_added), (gpointer)wspin);
	}
	else
		gtk_widget_show(socket);

#ifndef GTK3PLUG
	gtk_container_add(GTK_CONTAINER(gFix), socket);
#else
	gtk_box_pack_end(GTK_BOX(gFix), socket, TRUE, TRUE, 0);

	if (!nospinner)
		gtk_widget_hide(socket);
#endif

	if (crapgrab)
	{
		gtk_grab_add(socket);
		g_signal_connect(G_OBJECT(socket), KEY_EVENT, G_CALLBACK(key_press_crap), NULL);
	}
#ifndef GTK3PLUG
	GdkNativeWindow id = gtk_socket_get_id(GTK_SOCKET(socket));
#else
	Window id = gtk_socket_get_id(GTK_SOCKET(socket));
#endif

	command = str_replace(command, "$FILE", noquote ? g_strdup(FileToLoad) : g_shell_quote(FileToLoad));
	command = str_replace(command, "$XID", g_strdup_printf("%d", id));
	g_print("%s\n", command);
/*
	if (!g_spawn_command_line_async(command, NULL))
	{
		gtk_widget_destroy(gFix);
		return NULL;
	}
*/

	GPid pid;
	int argc;
	gchar** argv;
	GSpawnFlags flags = G_SPAWN_DEFAULT;

	if (!g_shell_parse_argv(command, &argc, &argv, NULL))
	{
		gtk_widget_destroy(gFix);
		return NULL;
	}

	gboolean is_launched = g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, &pid, NULL);
	g_strfreev(argv);

	if (is_launched && exit_timeout > 0)
	{
		int status;
		g_print("%s: exit_timeout = %d usec\n", PLUGNAME, exit_timeout);
		usleep(exit_timeout);
		//g_print("%s: waitpid %d\n", PLUGNAME, pid);
		pid_t result = waitpid(pid, &status, WNOHANG);

		if (result > 0)
		{
			if (WIFEXITED(status))
				g_print("%s: WIFEXITED, WEXITSTATUS(status) == %d\n", PLUGNAME, WEXITSTATUS(status));
			else if (WIFSIGNALED(status))
				g_print("%s: WIFSIGNALED, WTERMSIG(status) == %d\n", PLUGNAME, WTERMSIG(status));

			is_launched = FALSE;
		}
		else if (result != 0)
		{
			//g_print("%s: waitpid != 0\n", PLUGNAME);
			is_launched = FALSE;
		}
	}

	if (!is_launched)
	{
		gtk_widget_destroy(gFix);
		return NULL;
	}

	if (insensitive)
	{
		const gchar *role = gtk_window_get_role(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ParentWin))));

		if (g_strcmp0(role, "TfrmViewer") != 0)
#ifndef GTK3PLUG
			gtk_widget_set_state(socket, GTK_STATE_INSENSITIVE);
#else
			gtk_widget_set_sensitive(socket, FALSE);
#endif
	}

	gtk_widget_show(gFix);
	return gFix;

}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{

	if (Command == lc_copy)
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
		                       gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY)), -1);

	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		g_strlcpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		g_strlcpy(plug_path, g_path_get_dirname(cfg_path), PATH_MAX);
		char *pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, cfg_file);
	}
}
