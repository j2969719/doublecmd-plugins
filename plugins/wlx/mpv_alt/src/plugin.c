#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdint.h>
#ifdef GTKPLUG
#include <gtk/gtk.h>
#else
#include <QtWidgets>
#endif
#ifdef DRAWAREA
#include <gdk/gdkx.h>
#endif
//#include <mpv/client.h>
#include <dlfcn.h>
#include <limits.h>
#include <locale.h>
#include <string.h>
#include "wlxplugin.h"

#define LIB_PATH "libmpv.so"
#define MPV_ERROR_SUCCESS 0
#define MPV_FORMAT_INT64  4
#define LUA_SCRIPT "plugload.lua"
#define CONF_NAME  "j2969719.ini"
//#define MSG_ERRLIB PLUGNAME ": Failed to open " LIB_PATH "! Check if its installed or the settings in the $DC_CONFIG_PATH/" CONF_NAME " file."
#define MSG_ERRLIB PLUGNAME ": Failed to open " LIB_PATH "! Check if its installed or correct the contents of the libmpv.txt file in the plugin folder."

typedef struct mpv_handle mpv_handle;
typedef mpv_handle* (*mpv_create_t)(void);
typedef void (*mpv_initialize_t)(mpv_handle*);
typedef void (*mpv_request_log_messages_t)(mpv_handle*, const char*);
typedef void (*mpv_set_option_t)(mpv_handle*, const char*, int, int64_t*);
typedef void (*mpv_set_option_string_t)(mpv_handle*, const char*, const char*);
typedef int (*mpv_command_t)(mpv_handle*, const char**);
typedef void (*mpv_destroy_t)(mpv_handle*);

void *gLibHandle = NULL;
static char gScriptPath[PATH_MAX];

#ifdef __cplusplus
extern "C" {
#endif


HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	int64_t wid;
	setlocale(LC_NUMERIC, "C");

	if (gLibHandle == NULL)
	{
		if (ShowFlags & lcp_forceshow)
		{
#ifdef GTKPLUG
			GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ParentWin))),
			                    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, MSG_ERRLIB);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
#else
			QMessageBox::critical((QWidget*)ParentWin, PLUGNAME, MSG_ERRLIB);
#endif
		}
		else
			fprintf(stderr, "%s\n", MSG_ERRLIB);

		return NULL;
	}

	mpv_create_t mpv_create = (mpv_create_t)dlsym(gLibHandle, "mpv_create");
	mpv_initialize_t mpv_initialize = (mpv_initialize_t)dlsym(gLibHandle, "mpv_initialize");
	mpv_request_log_messages_t mpv_request_log_messages = (mpv_request_log_messages_t)dlsym(gLibHandle, "mpv_request_log_messages");
	mpv_set_option_t mpv_set_option = (mpv_set_option_t)dlsym(gLibHandle, "mpv_set_option");
	mpv_set_option_string_t mpv_set_option_string = (mpv_set_option_string_t)dlsym(gLibHandle, "mpv_set_option_string");
	mpv_command_t mpv_command = (mpv_command_t)dlsym(gLibHandle, "mpv_command");
	mpv_destroy_t mpv_destroy = (mpv_destroy_t)dlsym(gLibHandle, "mpv_destroy");

	mpv_handle *mpv = mpv_create();
	mpv_initialize(mpv);
#ifdef GTKPLUG

#ifdef DRAWAREA
	GtkWidget *view = gtk_drawing_area_new();
#else
	GtkWidget *view = gtk_socket_new();
#endif

	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), view);
	gtk_widget_show_all(view);

#ifdef DRAWAREA
	gtk_widget_realize(view);
	wid = (int64_t)GDK_WINDOW_XID(gtk_widget_get_window(view));
#else
	wid = (int64_t)gtk_socket_get_id(GTK_SOCKET(view));
#endif

	g_object_set_data(G_OBJECT(view), "mpv", (gpointer)mpv);
#else
	QLabel *view = new QLabel((QWidget*)ParentWin);
	view->show();
	wid = (int64_t)view->winId();
	QVariant var = QVariant::fromValue((uintptr_t)mpv);
	view->setProperty("mpv", var);
#endif
	mpv_request_log_messages(mpv, "no");
	mpv_set_option(mpv, "wid", MPV_FORMAT_INT64, &wid);
	mpv_set_option_string(mpv, "keep-open", "yes");
	mpv_set_option_string(mpv, "osc", "yes");
	mpv_set_option_string(mpv, "force-window", "yes");
	mpv_set_option_string(mpv, "script-opts", "osc-visibility=always");

	const char *args[] = {"load-script", gScriptPath, NULL};
	mpv_command(mpv, args);

	args[0] = "loadfile";
	args[1] = FileToLoad;

	if (mpv_command(mpv, args) != MPV_ERROR_SUCCESS)
	{
		mpv_destroy(mpv);

#ifdef GTKPLUG
		gtk_widget_destroy(view);
#else
		delete view;
#endif

		return NULL;
	}

	return view;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	mpv_handle *mpv = NULL;
	mpv_command_t mpv_command = (mpv_command_t)dlsym(gLibHandle, "mpv_command");
	mpv_set_option_string_t mpv_set_option_string = (mpv_set_option_string_t)dlsym(gLibHandle, "mpv_set_option_string");
#ifdef GTKPLUG
	mpv = (mpv_handle*)g_object_get_data(G_OBJECT(PluginWin), "mpv");
#else
	QLabel *view = (QLabel*)PluginWin;
	mpv = (mpv_handle*)view->property("mpv").value<uintptr_t>();
#endif

	if (!mpv)
		return LISTPLUGIN_ERROR;

	const char *args[] = {"loadfile", FileToLoad, NULL};

	if (mpv_command(mpv, args) == MPV_ERROR_SUCCESS)
		return LISTPLUGIN_OK;

	return LISTPLUGIN_ERROR;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	mpv_handle *mpv = NULL;
#ifdef GTKPLUG
	mpv = (mpv_handle*)g_object_get_data(G_OBJECT(ListWin), "mpv");
#else
	QLabel *view = (QLabel*)ListWin;
	mpv = (mpv_handle*)view->property("mpv").value<uintptr_t>();
#endif

	mpv_destroy_t mpv_destroy = (mpv_destroy_t)dlsym(gLibHandle, "mpv_destroy");

	if (mpv)
		mpv_destroy(mpv);

#ifdef GTKPLUG
	gtk_widget_destroy(GTK_WIDGET(ListWin));
#else
	delete (QLabel*)ListWin;
#endif
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	snprintf(DetectString, maxlen - 1, "%s", DETECT_STRING);
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
	char libpath[PATH_MAX] = LIB_PATH;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(gScriptPath, &dlinfo) != 0)
	{
		snprintf(gScriptPath, PATH_MAX, "%s", dlinfo.dli_fname);
		char *pos = strrchr(gScriptPath, '/');

		if (pos)
			strcpy(pos + 1, "libmpv.txt");

		FILE *fp;
		char *line = NULL;
		size_t len = 0;
		ssize_t lread;

		if (!gLibHandle && (fp = fopen(gScriptPath, "r")) != NULL)
		{
			while ((lread = getline(&line, &len, fp)) != -1)
			{
				if (lread >= strlen(LIB_PATH) && line[0] != '#')
				{
					if (line[lread - 1] == '\n')
						line[lread - 1] = '\0';

					snprintf(libpath, PATH_MAX, "%s", line);
					break;
				}
			}

			free(line);
			fclose(fp);
		}

		if (pos)
			strcpy(pos + 1, LUA_SCRIPT);
	}
/*
#ifdef GTKPLUG
	char cfg_path[PATH_MAX];
	g_strlcpy(cfg_path, dps->DefaultIniName, PATH_MAX);

	char *pos = strrchr(cfg_path, '/');

	if (pos)
		strcpy(pos + 1, CONF_NAME);

	GKeyFile *cfg = g_key_file_new();

	g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, NULL);
	gchar *path = g_key_file_get_string(cfg, PLUGNAME, "libpath", NULL);

	if (!path)
	{
		g_key_file_set_string(cfg, PLUGNAME, "libpath", LIB_PATH);
		g_key_file_save_to_file(cfg, cfg_path, NULL);
		g_strlcpy(libpath, LIB_PATH, PATH_MAX);
	}
	else
		g_strlcpy(libpath, path, PATH_MAX);

	g_free(path);
	g_key_file_free(cfg);
#else
	QFileInfo defini(QString::fromStdString(dps->DefaultIniName));
	QString cfg = defini.absolutePath() + "/" CONF_NAME;
	QSettings settings(cfg, QSettings::IniFormat);
	QString path = settings.value(PLUGNAME "/libpath").toString();

	if (path.isEmpty())
	{
		snprintf(libpath, PATH_MAX, "%s", LIB_PATH);
		settings.setValue(PLUGNAME "/libpath", LIB_PATH);
	}
	else
		snprintf(libpath, PATH_MAX, "%s", path.toStdString().c_str());

#endif
*/

	if (!gLibHandle)
		gLibHandle = dlopen(libpath, RTLD_LAZY | RTLD_DEEPBIND);
}

#ifdef __cplusplus
}

#endif
