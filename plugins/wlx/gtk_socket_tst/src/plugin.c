#define _GNU_SOURCE
#include <gtk/gtk.h>
#ifdef GTK3PLUG
#include <gtk/gtkx.h>
#endif
#include <gdk/gdkx.h>
#include <ftw.h>
#include <dlfcn.h>
#include "wlxplugin.h"

#define CMD_CREATE  "?CREATE"
#define CMD_DESTROY "?DESTROY"
#define CMD_LOAD    "?LOAD"
#define CMD_FIND    "?FIND"
#define CMD_COPY    "?COPY"
#define CMD_SELECTA "?SELECTALL"
#define CMD_NEWPARM "?NEWPARAMS"

#define DC_USERDIR "/.local/share/doublecmd/plugins/wlx/gtk_socket_tst"
#define MSG_BUF 16

typedef struct
{
	GPid pid;
	gchar *script;
	GInputStream *in;
	GOutputStream *out;
	GDataInputStream *data_in;
	GDataOutputStream *data_out;
	gchar *socket_path;
	GSocketConnection *sock;
	gsize wlx_count;
} ScriptItem;

GKeyFile *cfg = NULL;
gchar *tmpdir = NULL;
gchar *new_path = NULL;
GHashTable *active_scripts = NULL;
gboolean is_plug_init = FALSE;

void script_data_free(gpointer data)
{
	ScriptItem *item = (ScriptItem*)data;

	if (item->sock)
	{
		g_io_stream_close(G_IO_STREAM(item->sock), NULL, NULL);
		g_object_unref(item->sock);
	}

	unlink(item->socket_path);
	g_free(item->socket_path);
	g_free(item->script);

	if (item->data_in)
		g_object_unref(item->data_in);

	if (item->data_out)
		g_object_unref(item->data_out);

	if (item->pid > 0)
		kill(item->pid, SIGTERM);

	g_free(item);
}

static int nftw_remove_cb(const char *file, const struct stat *st, int tflag, struct FTW *ftwbuf)
{
	remove(file);
	return 0;
}

static void on_plug_added(GtkWidget *widget, gpointer data)
{
	gtk_spinner_stop(GTK_SPINNER(data));
	gtk_widget_hide(GTK_WIDGET(data));
	gtk_widget_show(widget);
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

static ScriptItem* get_script_item(gchar *group)
{
	GError *err = NULL;
	gchar *script = g_key_file_get_string(cfg, group, "script", NULL);
	ScriptItem *item = g_hash_table_lookup(active_scripts, script);

	if (!item)
	{
		item = g_new0(ScriptItem, 1);
		item->script = script;
		item->socket_path = g_strdup_printf("%s/%s.sock", tmpdir, script);
		unlink(item->socket_path);
		GSocketAddress *addr = g_unix_socket_address_new(item->socket_path);
		GSocketService *service = g_socket_service_new();

		if (!g_socket_listener_add_address(G_SOCKET_LISTENER(service), addr, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_DEFAULT, NULL, NULL, &err))
		{
			script_data_free(item);
			g_object_unref(addr);
			g_object_unref(service);
			g_printerr("%s (%s g_socket_listener_add_address): %s", PLUGNAME, script, err->message);
			g_error_free(err);
			return NULL;
		}

		g_object_unref(addr);
		char *argv[] = {"sh", "-c", script, NULL};
		GSpawnFlags flags = G_SPAWN_SEARCH_PATH_FROM_ENVP | G_SPAWN_DO_NOT_REAP_CHILD;
		gchar **envp = g_environ_setenv(g_get_environ(), "PATH", new_path, TRUE);
		envp = g_environ_setenv(envp, "SOCKET", item->socket_path, TRUE);
		gboolean is_launched = g_spawn_async(NULL, argv, envp, flags, NULL, NULL, &item->pid, NULL);
		g_strfreev(envp);

		if (!is_launched)
		{
			script_data_free(item);
			g_object_unref(service);
			return NULL;
		}

		item->sock = g_socket_listener_accept(G_SOCKET_LISTENER(service), NULL, NULL, &err);
		g_object_unref(service);

		if (!item->sock)
		{
			script_data_free(item);
			g_printerr("%s (%s g_socket_listener_accept): %s", PLUGNAME, script, err->message);
			g_error_free(err);
			return NULL;
		}

		item->in = g_io_stream_get_input_stream(G_IO_STREAM(item->sock));
		item->out = g_io_stream_get_output_stream(G_IO_STREAM(item->sock));

		char buffer[MSG_BUF];

		if (kill(item->pid, 0) != 0)
		{
			g_printerr("%s (%s): %d is dead\n", PLUGNAME, item->script, item->pid);
			script_data_free(item);
			return NULL;
		}

		g_input_stream_read(item->in, buffer, sizeof(buffer), NULL, &err);

		if (err || !g_str_has_prefix(buffer, "READY"))
		{
			script_data_free(item);

			if (err)
			{
				g_printerr("%s (%s g_input_stream_read): %s", PLUGNAME, script, err->message);
				g_error_free(err);
			}
			else
				g_printerr("%s (%s): %s", PLUGNAME, script, buffer);

			return NULL;
		}

		item->data_in = g_data_input_stream_new(item->in);
		item->data_out = g_data_output_stream_new(item->out);
		g_hash_table_insert(active_scripts, g_strdup(item->script), item);
	}
	else
	{
		item->wlx_count++;
		g_free(script);
	}

	if (err)
		g_error_free(err);

	return item;
}

static gboolean send_command(ScriptItem *item, gint64 xid, gchar *command, gchar *param, int flags)
{
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

	if (kill(item->pid, 0) != 0)
	{
		g_printerr("%s (%s): %d is dead\n", PLUGNAME, item->script, item->pid);
		return FALSE;
	}

	char *response = g_data_input_stream_read_line(item->data_in, NULL, NULL, &err);

	if (err)
	{
		g_printerr("%s (%s g_data_input_stream_read_line): %s\n", PLUGNAME, item->script, err->message);
		g_error_free(err);
		return FALSE;
	}

	g_print("%s (%s): %s\t%ld\t%s\t%d -> %s\n", PLUGNAME, item->script, command, xid, param, flags, response);
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

int DCPCALL ListLoadNext(HWND ParentWin,HWND PluginWin,char* FileToLoad,int ShowFlags)
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

int DCPCALL ListSearchText(HWND ListWin,char* SearchString,int SearchParameter)
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
	nftw(tmpdir, nftw_remove_cb, 13, FTW_DEPTH | FTW_PHYS);
	g_free(tmpdir);
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

	tmpdir = g_dir_make_tmp("_dc-wlxsocket.XXXXXX", NULL);
	active_scripts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, script_data_free);

	if (dladdr(&is_plug_init, &dlinfo) != 0)
	{
		char plug_path[PATH_MAX];
		g_strlcpy(plug_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(plug_path, '/');

		if (pos)
		{
			strcpy(pos + 1, "scripts");
			new_path = g_strdup_printf("%s:%s:%s" DC_USERDIR "/scripts", g_getenv("PATH"), plug_path, g_getenv("HOME"));
			*pos = '\0';
		}

		gchar *user_path = g_strdup_printf("%s" DC_USERDIR, g_getenv("HOME"));
		const gchar *search_dirs[] = {user_path, plug_path, NULL};
		g_key_file_load_from_dirs(cfg, "settings.ini", search_dirs, NULL, G_KEY_FILE_NONE, NULL);
		g_free(user_path);
	}
}
