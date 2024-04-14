#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdint.h>
#ifdef GTKPLUG
#include <gtk/gtk.h>
#else
#include <QtWidgets>
#endif
#include <mpv/client.h>
#include <dlfcn.h>
#include <limits.h>
#include <locale.h>
#include <string.h>
#include "wlxplugin.h"

#define LUA_SCRIPT "plugload.lua"
static char gScriptPath[PATH_MAX];

#ifdef __cplusplus
extern "C" {
#endif

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	int64_t wid;
	setlocale(LC_NUMERIC, "C");
	mpv_handle *mpv = mpv_create();
	mpv_initialize(mpv);
#ifdef GTKPLUG
	GtkWidget *view = gtk_socket_new();
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), view);
	gtk_widget_show_all(view);
	wid = (int64_t)gtk_socket_get_id(GTK_SOCKET(view));
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
	mpv_set_option_string(mpv, "loop", "yes");
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

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(gScriptPath, &dlinfo) != 0)
	{
		snprintf(gScriptPath, PATH_MAX, "%s", dlinfo.dli_fname);
		char *pos = strrchr(gScriptPath, '/');

		if (pos)
			strcpy(pos + 1, LUA_SCRIPT);
	}
}

#ifdef __cplusplus
}
#endif
