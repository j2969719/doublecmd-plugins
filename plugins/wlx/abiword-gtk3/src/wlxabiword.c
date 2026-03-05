#include <gtk/gtk.h>
#include <libabiword.h>
#include <abiwidget.h>
#include "wlxplugin.h"

gboolean gInit = FALSE;

static void wlxplug_atexit(void)
{
	printf("%s atexit\n", PLUGNAME);

	if (gInit)
	{
		//libabiword_shutdown();
	}
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	if (!gInit)
	{
		gInit = TRUE;
		libabiword_init_noargs();
		atexit(wlxplug_atexit);
	}
}

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
	if (!gInit)
		return NULL;

	GtkWidget *abi = abi_widget_new();
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), abi);

	if (abi_widget_load_file(ABI_WIDGET(abi), FileToLoad, ""))
	{
		gtk_widget_destroy(abi);
		return NULL;
	}

	abi_widget_view_print_layout(ABI_WIDGET(abi));
	gtk_widget_show_all(abi);

	return abi;
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
	if (abi_widget_load_file(ABI_WIDGET(PluginWin), FileToLoad, ""))
		return LISTPLUGIN_ERROR;

	return LISTPLUGIN_OK;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, DETECT_STRING, maxlen);
}

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	abi_widget_set_find_string(ABI_WIDGET(ListWin), SearchString);

	if (SearchParameter & lcs_backwards)
		abi_widget_find_prev(ABI_WIDGET(ListWin));
	else if (SearchParameter & lcs_findfirst)
		abi_widget_find_next(ABI_WIDGET(ListWin), TRUE);
	else
		abi_widget_find_next(ABI_WIDGET(ListWin), FALSE);

	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	switch (Command)
	{
	case lc_copy :
		abi_widget_copy(ABI_WIDGET(ListWin));
		break;

	case lc_selectall :
		abi_widget_select_all(ABI_WIDGET(ListWin));
		break;

	default :
		return LISTPLUGIN_ERROR;
	}

	return LISTPLUGIN_OK;
}
