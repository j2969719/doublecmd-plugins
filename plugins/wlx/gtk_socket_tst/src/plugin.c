#define _GNU_SOURCE
#include <gtk/gtk.h>
#ifdef GTK3PLUG
#include <gtk/gtkx.h>
#endif
#include <gdk/gdkx.h>
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <poll.h>
#include <dlfcn.h>
#include "wlxplugin.h"

#define CMD_CREATE  "?CREATE"
#define CMD_DESTROY "?DESTROY"
#define CMD_LOAD    "?LOAD"
#define CMD_FIND    "?FIND"
#define CMD_COPY    "?COPY"
#define CMD_SELECTA "?SELECTALL"
#define CMD_NEWPARM "?NEWPARAMS"

#define DC_USERDIR "/.local/share/doublecmd/plugins/wlx/" PLUGDIR
#define READ_TIMEOUT 5000

typedef struct
{
	GPid pid;
	gchar *script;
	gint stdin;
	gint stdout;
	GInputStream *in;
	GOutputStream *out;
	GDataInputStream *data_in;
	GDataOutputStream *data_out;
	gboolean is_alive;
} ScriptItem;

GKeyFile *cfg = NULL;
gchar *new_path = NULL;
GHashTable *active_scripts = NULL;
gboolean is_plug_init = FALSE;

void script_data_free(gpointer data)
{
	ScriptItem *item = (ScriptItem*)data;

	if (item->pid > 0 && item->is_alive)
		kill(item->pid, SIGTERM);

	if (item->in)
		g_object_unref(item->in);

	if (item->out)
		g_object_unref(item->out);

	if (item->data_in)
		g_object_unref(item->data_in);

	if (item->data_out)
		g_object_unref(item->data_out);

	g_free(item->script);
	g_free(item);
}

static void on_plug_added(GtkWidget *widget, gpointer data)
{
	gtk_spinner_stop(GTK_SPINNER(data));
	gtk_widget_hide(GTK_WIDGET(data));
	gtk_widget_show(widget);
}

static void on_child_exited(GPid pid, gint status, gpointer data)
{
	ScriptItem *item = (ScriptItem*)data;
	g_print("%s: PID %d exited, status == %d\n", PLUGNAME, pid, status);
	item->is_alive = FALSE;
	g_spawn_close_pid(pid);
}

static char* get_file_ext(char *filename)
{
	char *result = NULL;
	char *pos = strrchr(filename, '/');

	if (pos)
	{
		char *dot = strrchr(pos + 1, '.');

		if (dot)
		{
			char *ext = dot + 1;

			if (pos + 1 != dot && *ext != '\0')
				result = g_ascii_strdown(ext, -1);
		}
	}

	return result;
}

static gchar *get_mimetype(char *filename)
{
	GFile *gfile = g_file_new_for_path(filename);

	if (!gfile)
		return NULL;

	GFileInfo *fileinfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL, NULL);

	if (!fileinfo)
	{
		g_object_unref(gfile);
		return NULL;
	}

	gchar *result = g_strdup(g_file_info_get_content_type(fileinfo));
	g_object_unref(fileinfo);
	g_object_unref(gfile);

	return result;
}

static gchar *resolve_redirect(gchar *group)
{
	if (!group)
		return NULL;

	gchar *result = g_key_file_get_string(cfg, group, "redirect", NULL);

	if (result)
		g_free(group);
	else
		result = group;

	if (!g_key_file_has_key(cfg, result, "script", NULL))
	{
		g_free(result);
		return NULL;
	}

	return result;
}

static gchar* get_group(char *filename)
{
	gchar *result = resolve_redirect(get_file_ext(filename));

	if (!result)
		result = resolve_redirect(get_mimetype(filename));

	return result;
}

static gchar* get_line_from_script(ScriptItem *item)
{
	if (!item->is_alive)
		return NULL;

	struct pollfd pfd;
	pfd.fd = item->stdout;
	pfd.events = POLLIN;
	GError *err = NULL;

	if (poll(&pfd, 1, READ_TIMEOUT) == 0)
	{
		g_printerr("%s (%s get_line_from_script): timeout\n", PLUGNAME, item->script);
		return NULL;
	}

	char *line = g_data_input_stream_read_line(item->data_in, NULL, NULL, &err);

	if (err)
	{
		g_printerr("%s (%s g_data_input_stream_read_line): %s", PLUGNAME, item->script, err->message);
		g_error_free(err);
		return NULL;
	}

	return line;
}

static ScriptItem* get_script_item(gchar *group)
{
	GError *err = NULL;
	gchar *script = g_key_file_get_string(cfg, group, "script", NULL);
	ScriptItem *item = g_hash_table_lookup(active_scripts, script);

	if (item && !item->is_alive)
	{
		g_hash_table_remove(active_scripts, script);
		item = NULL;
	}

	if (!item)
	{
		item = g_new0(ScriptItem, 1);
		item->script = script;

		char *argv[] = {"sh", "-c", script, NULL};
		GSpawnFlags flags = G_SPAWN_SEARCH_PATH_FROM_ENVP | G_SPAWN_DO_NOT_REAP_CHILD;
		gchar **envp = g_environ_setenv(g_get_environ(), "PATH", new_path, TRUE);
		item->is_alive = g_spawn_async_with_pipes(NULL, argv, envp, flags, NULL, NULL, &item->pid, &item->stdin, &item->stdout, NULL, &err);
		g_strfreev(envp);

		if (!item->is_alive)
		{
			script_data_free(item);
			g_printerr("%s (%s g_spawn_async_with_pipes): %s", PLUGNAME, script, err->message);
			g_error_free(err);
			return NULL;
		}

		g_child_watch_add(item->pid, on_child_exited, item);

		item->in = G_INPUT_STREAM(g_unix_input_stream_new(item->stdout, TRUE));
		item->out = G_OUTPUT_STREAM(g_unix_output_stream_new(item->stdin, TRUE));

		item->data_in = g_data_input_stream_new(item->in);
		item->data_out = g_data_output_stream_new(item->out);

		gchar *line = get_line_from_script(item);

		if (!line || g_strcmp0(line, "READY") != 0)
		{
			script_data_free(item);
			g_printerr("%s (%s): wheres the \"READY\" lebowski? got \"%s\" instead\n", PLUGNAME, script, line);
			g_free(line);
			return NULL;
		}

		g_free(line);
		g_hash_table_insert(active_scripts, g_strdup(item->script), item);
	}
	else
		g_free(script);

	if (err)
		g_error_free(err);

	return item;
}

static gboolean send_command(ScriptItem *item, gint64 xid, gchar *command, gchar *param, int flags)
{
	if (!item->is_alive)
		return FALSE;

	GError *err = NULL;
	gchar *message = g_strdup_printf("%s\t%ld\t%s\t%d\n", command, xid, param, flags);
	g_data_output_stream_put_string(item->data_out, message, NULL, &err);
	g_free(message);

	if (err)
	{
		g_printerr("%s (%s g_data_output_stream_put_string): %s\n", PLUGNAME, item->script, err->message);
		g_error_free(err);
		return FALSE;
	}
	else
		g_output_stream_flush(item->out, NULL, NULL);

	char *response = get_line_from_script(item);

	g_print("%s (%s): %s\t%ld\t%s\t%d -> %s\n", PLUGNAME, item->script, command, xid, param, flags, response);

	if (!response)
		return FALSE;

	gboolean result = (strcmp(response, "!OK") == 0);
	g_free(response);

	return result;
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	gchar *group = get_group(FileToLoad);

	if (!group)
		return NULL;

	ScriptItem *item = get_script_item(group);

	if (!item)
	{
		g_free(group);
		return NULL;
	}

#ifndef GTK3PLUG
	GtkWidget *main_box = gtk_vbox_new(FALSE, 5);
#else
	GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
#endif
	gtk_widget_set_no_show_all(main_box, TRUE);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), main_box);
	GtkWidget *socket = gtk_socket_new();
	GtkWidget *spinner = gtk_spinner_new();
	gtk_widget_show(spinner);
	gtk_spinner_start(GTK_SPINNER(spinner));
	gtk_box_pack_start(GTK_BOX(main_box), socket, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(main_box), spinner, TRUE, FALSE, 0);
	g_signal_connect(socket, "plug-added", G_CALLBACK(on_plug_added), (gpointer)spinner);

	gtk_widget_show(main_box);

	gsize xid = (gsize)gtk_socket_get_id(GTK_SOCKET(socket));

	if (!send_command(item, xid, CMD_CREATE, "", ShowFlags) || !send_command(item, xid, CMD_LOAD, FileToLoad, ShowFlags))
	{
		gtk_widget_destroy(main_box);
		return NULL;
	}

	g_object_set_data(G_OBJECT(main_box), "script_item", item);
	g_object_set_data(G_OBJECT(main_box), "xid", GSIZE_TO_POINTER(xid));
	g_free(group);

	return main_box;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	gchar *group = get_group(FileToLoad);

	if (!group)
		return LISTPLUGIN_ERROR;

	ScriptItem *newitem = get_script_item(group);

	if (!newitem)
	{
		g_free(group);
		return LISTPLUGIN_ERROR;
	}

	ScriptItem *item = (ScriptItem*)g_object_get_data(G_OBJECT(PluginWin), "script_item");
	gsize xid = (gsize)GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(PluginWin), "xid"));
	g_free(group);

	if (item == newitem && send_command(item, xid, CMD_LOAD, FileToLoad, ShowFlags))
		return LISTPLUGIN_OK;

	return LISTPLUGIN_ERROR;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	ScriptItem *item = (ScriptItem*)g_object_get_data(G_OBJECT(ListWin), "script_item");
	gsize xid = (gsize)GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(ListWin), "xid"));
	send_command(item, xid, CMD_DESTROY, "", -1);
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	gboolean result = FALSE;
	ScriptItem *item = (ScriptItem*)g_object_get_data(G_OBJECT(ListWin), "script_item");
	gsize xid = (gsize)GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(ListWin), "xid"));

	if (Command == lc_copy)
		result = send_command(item, xid, CMD_COPY, "", -1);
	else if (Command == lc_selectall)
		result = send_command(item, xid, CMD_SELECTA, "", -1);
	else if (Command == lc_newparams)
		result = send_command(item, xid, CMD_NEWPARM, "", Parameter);

	return (result ? LISTPLUGIN_OK : LISTPLUGIN_ERROR);
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	ScriptItem *item = (ScriptItem*)g_object_get_data(G_OBJECT(ListWin), "script_item");
	gsize xid = (gsize)GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(ListWin), "xid"));

	return (send_command(item, xid, CMD_FIND, SearchString, SearchParameter) ? LISTPLUGIN_OK : LISTPLUGIN_ERROR);
}

static void wlxplug_atexit(void)
{
	g_print("%s atexit\n", PLUGNAME);

	g_hash_table_destroy(active_scripts);
	g_key_file_free(cfg);
	g_free(new_path);
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	if (is_plug_init)
		return;

	Dl_info dlinfo;

	memset(&dlinfo, 0, sizeof(dlinfo));
	atexit(wlxplug_atexit);
	cfg = g_key_file_new();
	is_plug_init = TRUE;

	active_scripts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, script_data_free);

	if (dladdr(&is_plug_init, &dlinfo) != 0)
	{
		char plug_path[PATH_MAX];
		g_strlcpy(plug_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(plug_path, '/');

		if (pos)
		{
			*pos = '\0';
			new_path = g_strdup_printf("%s:%s" DC_USERDIR "/scripts:%s/scripts", g_getenv("PATH"), g_getenv("HOME"), plug_path);
		}

		gchar *user_path = g_strdup_printf("%s" DC_USERDIR, g_getenv("HOME"));
		const gchar *search_dirs[] = {user_path, plug_path, NULL};
		g_key_file_load_from_dirs(cfg, "settings.ini", search_dirs, NULL, G_KEY_FILE_NONE, NULL);
		g_free(user_path);
	}
}
