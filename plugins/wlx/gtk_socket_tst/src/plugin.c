#define _GNU_SOURCE
#include <gtk/gtk.h>
#ifdef GTK3PLUG
#include <gtk/gtkx.h>
#endif
#include <gdk/gdkx.h>
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <sys/wait.h>
#include <poll.h>
#include <magic.h>
#include <dlfcn.h>
#include "wlxplugin.h"

#define CMD_CREATE  "?CREATE"
#define CMD_DESTROY "?DESTROY"
#define CMD_LOAD    "?LOAD"
#define CMD_FIND    "?FIND"
#define CMD_COPY    "?COPY"
#define CMD_SELECTA "?SELECTALL"
#define CMD_NEWPARM "?NEWPARAMS"

#define OPT_MUTE "*silent"

#define DC_USERDIR "/.local/share/doublecmd/plugins/wlx/" PLUGDIR
#define READ_TIMEOUT_MIN 1000
#define READ_TIMEOUT_DEF 5000
#define MAX_REDIRECTS 42
#define INFO_FMT "%s, PID %d, X11 Window ID %ld"

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
	gboolean is_debug;
	gboolean is_silent;
	gint read_timeout;
} ScriptItem;

typedef struct
{
	GPid pid;
	gint stderr;
	gboolean is_script;
	gboolean is_destroyed;
	gchar *command;
	GtkWidget *errlabel;
} ExecData;

GKeyFile *cfg = NULL;
gchar *new_path = NULL;
gchar *cfg_path = NULL;
char plug_path[PATH_MAX];
GHashTable *active_scripts = NULL;
gboolean is_plug_init = FALSE;
gboolean is_show_debug_ui = FALSE;
gboolean is_global_debug = FALSE;
gboolean is_global_nospinner = FALSE;
gboolean is_use_libmagic = FALSE;
gint read_timeout = 0;

void script_data_free(gpointer data)
{
	ScriptItem *item = (ScriptItem*)data;

	if (item->pid > 0 && item->is_alive)
	{
		kill(item->pid, SIGTERM);
		g_spawn_close_pid(item->pid);
	}

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

static void show_message(char *message, GtkMessageType type, HWND widget)
{
	GtkWindow *parent = NULL;

	if (widget)
		parent = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(widget)));

	GtkWidget *dialog = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, type, GTK_BUTTONS_OK, message);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void get_global_settings(void)
{
	read_timeout = g_key_file_get_integer(cfg, PLUGNAME, "read_timeout", NULL);

	if (read_timeout < READ_TIMEOUT_MIN)
		read_timeout = READ_TIMEOUT_DEF;

	is_global_debug = g_key_file_get_boolean(cfg, ".", "debug", NULL);
	is_show_debug_ui = g_key_file_get_boolean(cfg, ".", "debug_ui", NULL);

	is_global_nospinner = g_key_file_get_boolean(cfg, ".", "nospinner", NULL);
	is_use_libmagic = g_key_file_get_boolean(cfg, ".", "uselibmagic", NULL);
}

static void get_script_settings(ScriptItem *item, gchar *group)
{
	if (!item)
		return;

	if (is_global_debug)
		item->is_debug = TRUE;
	else
		item->is_debug = g_key_file_get_boolean(cfg, group, "debug", NULL);

	item->read_timeout = g_key_file_get_integer(cfg, group, "read_timeout", NULL);

	if (item->read_timeout < READ_TIMEOUT_MIN)
		item->read_timeout = read_timeout;
}

static void on_plug_added(GtkWidget *widget, gpointer data)
{
	gtk_spinner_stop(GTK_SPINNER(data));
	gtk_widget_hide(GTK_WIDGET(data));
	gtk_widget_show(widget);
}

static void on_plug_removed(GtkWidget *widget, gpointer data)
{
	gtk_widget_show(GTK_WIDGET(data));
	gtk_widget_hide(widget);
}

static gboolean on_focus_in(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	gsize xid = (gsize)gtk_socket_get_id(GTK_SOCKET(widget));
	GtkWidget *main_box = (GtkWidget*)g_object_get_data(G_OBJECT(data), "main_box");
	g_object_set_data(G_OBJECT(main_box), "xid", GSIZE_TO_POINTER(xid));

	if (is_global_debug)
		g_print("%s: gtk_socket (xid %ld) on focus in\n", PLUGNAME, xid);

	return FALSE;
}

static void on_label_visibility_changed(GObject *source, GParamSpec *pspec, gpointer data)
{
	if (gtk_widget_get_visible(GTK_WIDGET(source)))
		gtk_widget_hide(GTK_WIDGET(data));
}

static void on_child_exited(GPid pid, gint status, gpointer data)
{
	ExecData *edata = (ExecData*)data;

	if (edata->is_script)
	{
		ScriptItem *item = g_hash_table_lookup(active_scripts, edata->command);

		if (item)
			item->is_alive = FALSE;
	}
	else if (!edata->is_destroyed)
	{
		GString *errors = g_string_new(NULL);
		char buf[1024];
		ssize_t len;

		while ((len = read(edata->stderr, buf, sizeof(buf) - 1)) > 0)
			g_string_append_len(errors, buf, len);

		if (errors->str && strlen(errors->str) > 1)
			gtk_label_set_text(GTK_LABEL(edata->errlabel), errors->str);

		close(edata->stderr);
		g_string_free(errors, TRUE);
		gtk_widget_show(edata->errlabel);
	}
	else if (edata->stderr > 0)
		close(edata->stderr);

	if (is_global_debug)
	{
		if (WIFEXITED(status))
			g_printerr("%s: %s [PID %d] exited, exit status = %d\n", PLUGNAME, edata->command, pid, WEXITSTATUS(status));
		else if (WIFSIGNALED(status))
			g_printerr("%s: %s [PID %d] %s\n", PLUGNAME, edata->command, pid, strsignal(WTERMSIG(status)));
	}

	g_spawn_close_pid(pid);
	g_free(edata->command);
	g_free(edata);
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

static gchar *get_mimetype_magic(const gchar *filename)
{
	magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE);

	if (!magic_cookie)
		return NULL;

	if (magic_load(magic_cookie, NULL) != 0)
	{
		magic_close(magic_cookie);
		return NULL;
	}

	gchar *result = g_strdup(magic_file(magic_cookie, filename));
	magic_close(magic_cookie);

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

	gint redirect_count = 0;
	gchar *redirect = NULL;

	while ((redirect = g_key_file_get_string(cfg, group, "redirect", NULL)) != NULL)
	{
		if (redirect_count++ > MAX_REDIRECTS)
			break;

		g_free(group);
		group = redirect;
	}

	if (redirect)
	{
		g_free(group);
		g_free(redirect);
		return NULL;
	}

	if (!g_key_file_has_key(cfg, group, "script_ipc", NULL) &&
	                !g_key_file_has_key(cfg, group, "script", NULL) &&
	                !g_key_file_has_key(cfg, group, "command", NULL))
	{
		g_free(group);
		return NULL;
	}

	return group;
}

static gchar* get_group(char *filename)
{
	gchar *result = resolve_redirect(get_file_ext(filename));

	if (!result)
		result = resolve_redirect(is_use_libmagic ? get_mimetype_magic(filename) : get_mimetype(filename));

	return result;
}

static gchar* get_command(char *group)
{
	gchar *result = g_key_file_get_string(cfg, group, "command", NULL);

	if (!result)
	{
		gchar *script = g_key_file_get_string(cfg, group, "script", NULL);

		if (script)
		{
			result = g_strdup_printf("%s $XID \"$FILE\"", script);
			g_free(script);
		}
	}

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

	if (poll(&pfd, 1, item->read_timeout) == 0)
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
	gchar *script = g_key_file_get_string(cfg, group, "script_ipc", NULL);

	if (!script)
		return NULL;

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
		get_script_settings(item, group);

		char *argv[] = {"sh", "-c", script, NULL};
		GSpawnFlags flags = G_SPAWN_SEARCH_PATH_FROM_ENVP | G_SPAWN_DO_NOT_REAP_CHILD;
		gchar **envp = g_environ_setenv(g_get_environ(), "PATH", new_path, TRUE);
		envp = g_environ_setenv(envp, "GDK_CORE_DEVICE_EVENTS", "1", TRUE);

		if (item->is_debug)
			envp = g_environ_setenv(envp, "SCRIPT_DEBUG", "1", TRUE);

		ExecData *data = g_new0(ExecData, 1);

		item->is_alive = g_spawn_async_with_pipes(NULL, argv, envp, flags, NULL, NULL, &item->pid, &item->stdin, &item->stdout, NULL, &err);
		g_strfreev(envp);

		if (!item->is_alive)
		{
			g_free(data);
			script_data_free(item);
			g_printerr("%s (%s g_spawn_async_with_pipes): %s\n", PLUGNAME, script, err->message);
			g_error_free(err);
			return NULL;
		}

		data->command = g_strdup(script);
		data->is_script = TRUE;
		g_child_watch_add(item->pid, on_child_exited, data);

		item->in = G_INPUT_STREAM(g_unix_input_stream_new(item->stdout, TRUE));
		item->out = G_OUTPUT_STREAM(g_unix_output_stream_new(item->stdin, TRUE));

		item->data_in = g_data_input_stream_new(item->in);
		item->data_out = g_data_output_stream_new(item->out);

		gchar *line = get_line_from_script(item);

		if (!line || !g_str_has_prefix(line, "READY"))
		{
			if (item->is_debug)
				g_printerr("%s (%s): expected \"READY\", got \"%s\" instead\n", PLUGNAME, script, line);

			g_free(line);
			script_data_free(item);
			return NULL;
		}

		item->is_silent = (strstr(line, OPT_MUTE) != NULL);

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

	if (item->is_silent)
	{
		if (item->is_debug)
			g_print("%s (%s): %s\t%ld\t%s\t%d !!!response ignored!!!\n", PLUGNAME, item->script, command, xid, param, flags);

		return item->is_alive;
	}

	char *response = get_line_from_script(item);

	if (item->is_debug)
		g_print("%s (%s): %s\t%ld\t%s\t%d -> %s\n", PLUGNAME, item->script, command, xid, param, flags, response);

	if (!response)
		return FALSE;

	gboolean result = (strcmp(response, "!OK") == 0);
	g_free(response);

	return result;
}

static gboolean execute_command(gchar *command, char *filename, gsize xid, gint exit_timeout, ExecData *data)
{
	if (!command || !g_strrstr(command, "$FILE") || !g_strrstr(command, "$XID"))
		return FALSE;

	char *argv[] = {"sh", "-c", command, NULL};
	GSpawnFlags flags = G_SPAWN_SEARCH_PATH_FROM_ENVP | G_SPAWN_DO_NOT_REAP_CHILD;
	gchar **envp = g_environ_setenv(g_get_environ(), "PATH", new_path, TRUE);
	envp = g_environ_setenv(envp, "FILE", filename, TRUE);
	gchar *string = g_strdup_printf("%ld", xid);
	envp = g_environ_setenv(envp, "XID", string, TRUE);
	envp = g_environ_setenv(envp, "GDK_CORE_DEVICE_EVENTS", "1", TRUE);
	envp = g_environ_setenv(envp, "PLUG_DIR", plug_path, TRUE);
	g_free(string);
	data->command = command;

	gboolean is_launched = g_spawn_async_with_pipes(NULL, argv, envp, flags, NULL, NULL, &data->pid, NULL, NULL, &data->stderr, NULL);
	g_strfreev(envp);

	if (!is_launched)
	{
		g_free(command);
		g_free(data);
		return FALSE;
	}

	g_child_watch_add(data->pid, on_child_exited, data);

	if (exit_timeout > 0)
	{
		int status;
		usleep(exit_timeout * 1000);
		pid_t result = waitpid(data->pid, &status, WNOHANG);

		if (result != 0)
			is_launched = FALSE;
	}

	return is_launched;
}

static void on_btn_bumpcfg_clicked(GtkToolItem *button, gpointer data)
{
	if (cfg_path)
	{
		g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_NONE, NULL);
		get_global_settings();
	}
}

static void on_btn_spawn_clicked(GtkToolItem *button, gpointer data)
{
	gchar *filename = (gchar*)g_object_get_data(G_OBJECT(data), "filename");
	gchar *group = get_group(filename);

	if (group)
	{
		GtkWidget *label = GTK_WIDGET(g_object_get_data(G_OBJECT(data), "shruggie"));
		gtk_widget_hide(label);
		GtkWidget *socket = gtk_socket_new();
		GtkWidget *main_box = (GtkWidget*)g_object_get_data(G_OBJECT(data), "main_box");
		gtk_box_pack_start(GTK_BOX(main_box), socket, TRUE, TRUE, 0);
		g_signal_connect(socket, "plug-removed", G_CALLBACK(on_plug_removed), label);
		g_signal_connect(socket, "focus-in-event", G_CALLBACK(on_focus_in), data);
		gtk_widget_show(socket);
		gtk_widget_realize(socket);

		gsize xid = (gsize)gtk_socket_get_id(GTK_SOCKET(socket));
		GArray *pids = (GArray*)g_object_get_data(G_OBJECT(data), "pids");
		ScriptItem *item = get_script_item(group);

		if (item)
		{
			if (!send_command(item, xid, CMD_CREATE, "spawn", 0))
			{
				gtk_widget_destroy(socket);
				show_message(CMD_CREATE " failed!", GTK_MESSAGE_ERROR, button);
				gtk_widget_show(label);
			}
			else if (!send_command(item, xid, CMD_LOAD, filename, 0))
			{
				send_command(item, xid, CMD_DESTROY, "spawn failed", -1);
				gtk_widget_destroy(socket);
				show_message(CMD_LOAD " failed!", GTK_MESSAGE_ERROR, button);
				gtk_widget_show(label);
			}
			else
			{
				GArray *xids = (GArray*)g_object_get_data(G_OBJECT(main_box), "xids");
				g_array_append_val(xids, xid);
				gchar *info = g_strdup_printf(INFO_FMT " (last)", item->script, item->pid, xid);
				gtk_label_set_text(GTK_LABEL(data), info);
				g_free(info);
				g_object_set_data(G_OBJECT(main_box), "script_item", item);
				g_object_set_data(G_OBJECT(main_box), "xid", GSIZE_TO_POINTER(xid));
			}

			if (item->is_alive)
			{
				gboolean is_new = TRUE;

				for (guint i = 0; i < pids->len; i++)
				{
					if (item->pid == g_array_index(pids, GPid, i))
					{
						is_new = FALSE;
						break;
					}
				}

				if (is_new)
					g_array_append_val(pids, item->pid);
			}
		}
		else
		{
			gchar *command = get_command(group);

			if (command)
			{
				gint exit_timeout = g_key_file_get_integer(cfg, group, "exit_timeout", NULL);
				ExecData *edata = g_new0(ExecData, 1);
				edata->errlabel = label;

				if (!execute_command(command, filename, xid, exit_timeout, edata))
				{
					gtk_widget_destroy(socket);
					show_message("execute_command failed!", GTK_MESSAGE_ERROR, button);
					gtk_widget_show(label);
				}
				else
				{
					gchar *info = g_strdup_printf(INFO_FMT " (last)", command, edata->pid, xid);
					gtk_label_set_text(GTK_LABEL(data), info);
					g_free(info);
					g_array_append_val(pids, edata->pid);
					GPtrArray *exec_data = (GPtrArray*)g_object_get_data(G_OBJECT(main_box), "exec_data");
					g_ptr_array_add(exec_data, (gpointer)edata);
				}
			}
			else
			{
				gtk_widget_destroy(socket);
				show_message("could not get either the script or the command!", GTK_MESSAGE_ERROR, button);
				gtk_widget_show(label);
			}
		}

		g_free(group);
	}
}

static void send_destroy_to_all(HWND main_box)
{
	ScriptItem *item = (ScriptItem*)g_object_get_data(G_OBJECT(main_box), "script_item");

	if (item)
	{
		GArray *xids = (GArray*)g_object_get_data(G_OBJECT(main_box), "xids");

		for (guint i = 0; i < xids->len; i++)
			send_command(item, g_array_index(xids, gsize, i), CMD_DESTROY, "", -1);

		g_array_set_size(xids, 0);
	}

	GPtrArray *exec_data = (GPtrArray*)g_object_get_data(G_OBJECT(main_box), "exec_data");

	if (exec_data)
	{
		for (guint i = 0; i < exec_data->len; i++)
		{
			ExecData *data = g_ptr_array_index(exec_data, i);
			data->is_destroyed = TRUE;
		}

		g_ptr_array_set_size(exec_data, 0);
	}
}

static void on_btn_kill_clicked(GtkToolItem *button, gpointer data)
{
	GtkWidget *main_box = (GtkWidget*)g_object_get_data(G_OBJECT(data), "main_box");
	send_destroy_to_all(main_box);

	GArray *pids = (GArray*)g_object_get_data(G_OBJECT(data), "pids");

	for (guint i = 0; i < pids->len; i++)
	{
		GPid pid = g_array_index(pids, GPid, i);
		kill(pid, SIGKILL);
	}

	g_array_set_size(pids, 0);

	gchar *info = g_strdup_printf("zed is dead, baby, zed is dead…");
	gtk_label_set_text(GTK_LABEL(data), info);
	g_free(info);
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	gchar *group = get_group(FileToLoad);

	if (!group)
		return NULL;

	ScriptItem *item = get_script_item(group);

	gchar *command = NULL;
	gint exit_timeout = 0;
	gboolean is_spinner = FALSE;
	gboolean is_insensitive = FALSE;

	if (!item)
	{
		command = get_command(group);

		if (!command)
		{
			g_free(group);
			return NULL;
		}

		exit_timeout = g_key_file_get_integer(cfg, group, "exit_timeout", NULL);
		is_spinner = !g_key_file_get_boolean(cfg, group, "nospinner", NULL);
		is_insensitive = g_key_file_get_boolean(cfg, group, "insensitive", NULL);
	}

	g_free(group);

#ifndef GTK3PLUG
	GtkWidget *main_box = gtk_vbox_new(FALSE, 5);
#else
	GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
#endif
	gtk_widget_set_no_show_all(main_box, TRUE);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), main_box);
#ifndef GTK3PLUG
	GtkWidget *debug_box = gtk_hbox_new(FALSE, 5);
#else
	GtkWidget *debug_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#endif
	GtkWidget *socket = gtk_socket_new();
	GtkWidget *label = gtk_label_new("¯\\_(ツ)_/¯");
	gtk_widget_show(socket);
	gtk_box_pack_start(GTK_BOX(main_box), debug_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(main_box), socket, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(main_box), label, TRUE, FALSE, 0);

	if (!is_global_nospinner && (is_spinner || item->is_silent))
	{
		GtkWidget *spinner = gtk_spinner_new();
		gtk_widget_show(spinner);
		gtk_widget_hide(socket);
		gtk_spinner_start(GTK_SPINNER(spinner));
		gtk_box_pack_end(GTK_BOX(main_box), spinner, TRUE, FALSE, 0);
		g_signal_connect(socket, "plug-added", G_CALLBACK(on_plug_added), (gpointer)spinner);
		g_signal_connect(label, "notify::visible", G_CALLBACK(on_label_visibility_changed), spinner);
	}

	g_signal_connect(socket, "plug-removed", G_CALLBACK(on_plug_removed), (gpointer)label);

	gtk_widget_show(main_box);
	gtk_widget_realize(socket);

	if (is_insensitive)
	{
		const gchar *role = gtk_window_get_role(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ParentWin))));

		if (g_strcmp0(role, "TfrmViewer") != 0)
#ifndef GTK3PLUG
			gtk_widget_set_state(socket, GTK_STATE_INSENSITIVE);

#else
			gtk_widget_set_sensitive(socket, FALSE);
#endif
	}

	gsize xid = (gsize)gtk_socket_get_id(GTK_SOCKET(socket));

	GtkWidget *info_label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(debug_box), info_label, FALSE, TRUE, 10);

	if (is_show_debug_ui)
	{
		gtk_box_pack_start(GTK_BOX(debug_box), gtk_label_new(NULL), TRUE, TRUE, 0);
		GtkToolItem *btn_kill = gtk_tool_button_new(NULL, NULL);
		GtkToolItem *btn_bumpcfg = gtk_tool_button_new(NULL, NULL);
		gtk_widget_set_tooltip_text(GTK_WIDGET(btn_bumpcfg), "reload config");
		gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(btn_bumpcfg), "view-refresh");
		gtk_widget_set_tooltip_text(GTK_WIDGET(btn_kill), "kill");
		gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(btn_kill), "system-shutdown");
		GtkToolItem *btn_spawn = gtk_tool_button_new(NULL, NULL);
		gtk_widget_set_tooltip_text(GTK_WIDGET(btn_spawn), "spawn");
		gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(btn_spawn), "media-playback-start");
		gtk_box_pack_end(GTK_BOX(debug_box), GTK_WIDGET(btn_kill), FALSE, FALSE, 2);
		gtk_box_pack_end(GTK_BOX(debug_box), GTK_WIDGET(btn_spawn), FALSE, FALSE, 2);
		gtk_box_pack_end(GTK_BOX(debug_box), GTK_WIDGET(btn_bumpcfg), FALSE, FALSE, 2);

		g_object_set_data(G_OBJECT(info_label), "shruggie", label);
		g_object_set_data(G_OBJECT(info_label), "socket", socket);
		g_object_set_data(G_OBJECT(info_label), "main_box", main_box);
		g_object_set_data_full(G_OBJECT(info_label), "filename", g_strdup(FileToLoad), (GDestroyNotify)g_free);
		g_signal_connect(G_OBJECT(btn_spawn), "clicked", G_CALLBACK(on_btn_spawn_clicked), info_label);
		g_signal_connect(G_OBJECT(btn_kill), "clicked", G_CALLBACK(on_btn_kill_clicked), info_label);
		g_signal_connect(G_OBJECT(btn_bumpcfg), "clicked", G_CALLBACK(on_btn_bumpcfg_clicked), info_label);
	}

	g_signal_connect(socket, "focus-in-event", G_CALLBACK(on_focus_in), (gpointer)info_label);

	if (!item)
	{
		ExecData *data = g_new0(ExecData, 1);
		data->errlabel = label;

		if (!execute_command(command, FileToLoad, xid, exit_timeout, data))
		{
			gtk_widget_destroy(main_box);
			return NULL;
		}

		if (is_show_debug_ui)
		{
			gchar *info = g_strdup_printf(INFO_FMT, command, data->pid, xid);
			gtk_label_set_text(GTK_LABEL(info_label), info);
			g_free(info);
			GArray *pids = g_array_new(FALSE, FALSE, sizeof(GPid));
			g_array_append_val(pids, data->pid);
			g_object_set_data_full(G_OBJECT(info_label), "pids", pids, (GDestroyNotify)g_array_unref);

			gtk_widget_show_all(debug_box);
		}

		GPtrArray *exec_data = g_ptr_array_new();
		g_ptr_array_add(exec_data, (gpointer)data);
		g_object_set_data_full(G_OBJECT(main_box), "exec_data", exec_data, (GDestroyNotify)g_ptr_array_unref);
		return main_box;
	}

	if (!send_command(item, xid, CMD_CREATE, "", ShowFlags) || !send_command(item, xid, CMD_LOAD, FileToLoad, ShowFlags))
	{
		gtk_widget_destroy(main_box);
		return NULL;
	}

	if (is_show_debug_ui)
	{
		gchar *info = g_strdup_printf(INFO_FMT, item->script, item->pid, xid);
		gtk_label_set_text(GTK_LABEL(info_label), info);
		g_free(info);

		GArray *pids = g_array_new(FALSE, FALSE, sizeof(GPid));
		g_array_append_val(pids, item->pid);
		g_object_set_data_full(G_OBJECT(info_label), "pids", pids, (GDestroyNotify)g_array_unref);

		gtk_widget_show_all(debug_box);
	}

	GArray *xids = g_array_new(FALSE, FALSE, sizeof(gsize));
	g_array_append_val(xids, xid);
	g_object_set_data_full(G_OBJECT(main_box), "xids", xids, (GDestroyNotify)g_array_unref);

	g_object_set_data(G_OBJECT(main_box), "script_item", item);
	g_object_set_data(G_OBJECT(main_box), "info_label", info_label);
	g_object_set_data(G_OBJECT(main_box), "xid", GSIZE_TO_POINTER(xid));

	return main_box;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	ScriptItem *item = (ScriptItem*)g_object_get_data(G_OBJECT(PluginWin), "script_item");

	if (!item)
		return LISTPLUGIN_ERROR;

	gchar *group = get_group(FileToLoad);

	if (!group)
		return LISTPLUGIN_ERROR;

	ScriptItem *newitem = get_script_item(group);
	g_free(group);

	if (!newitem)
		return LISTPLUGIN_ERROR;

	gsize xid = (gsize)GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(PluginWin), "xid"));

	if (item == newitem && send_command(item, xid, CMD_LOAD, FileToLoad, ShowFlags))
	{
		if (is_show_debug_ui)
		{
			GtkWidget *info_label = g_object_get_data(G_OBJECT(PluginWin), "info_label");
			g_object_set_data(G_OBJECT(info_label), "filename", NULL);
			g_object_set_data_full(G_OBJECT(info_label), "filename", g_strdup(FileToLoad), (GDestroyNotify)g_free);
		}

		return LISTPLUGIN_OK;
	}

	return LISTPLUGIN_ERROR;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	send_destroy_to_all(ListWin);
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	gboolean result = FALSE;
	ScriptItem *item = (ScriptItem*)g_object_get_data(G_OBJECT(ListWin), "script_item");

	if (!item)
	{
		if (Command == lc_copy)
		{
			gchar *text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY));
			gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), text, -1);
			g_free(text);
			return LISTPLUGIN_OK;
		}

		return LISTPLUGIN_ERROR;
	}

	if (!item->is_alive)
		return LISTPLUGIN_ERROR;

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

	if (!item || !item->is_alive)
	{
		show_message("Not suppoted!", GTK_MESSAGE_INFO, ListWin);
		return LISTPLUGIN_OK;
	}

	gsize xid = (gsize)GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(ListWin), "xid"));

	if (!send_command(item, xid, CMD_FIND, SearchString, SearchParameter))
		show_message("Not suppoted!", GTK_MESSAGE_INFO, ListWin);

	return LISTPLUGIN_OK;
}

static void wlxplug_atexit(void)
{
	if (is_global_debug)
		g_print("%s atexit\n", PLUGNAME);

	g_hash_table_destroy(active_scripts);
	g_key_file_free(cfg);
	g_free(new_path);
	g_free(cfg_path);
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
		g_strlcpy(plug_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(plug_path, '/');

		if (pos)
		{
			*pos = '\0';
			new_path = g_strdup_printf("%s" DC_USERDIR "/scripts:%s/scripts:%s", g_getenv("HOME"), plug_path, g_getenv("PATH"));
		}

		gchar *user_path = g_strdup_printf("%s" DC_USERDIR, g_getenv("HOME"));
		const gchar *search_dirs[] = {user_path, plug_path, NULL};
		g_key_file_load_from_dirs(cfg, "settings.ini", search_dirs, &cfg_path, G_KEY_FILE_NONE, NULL);
		g_free(user_path);
		get_global_settings();
	}
}
