#include <gtk/gtk.h>
#include <libabiword.h>
#include <abiwidget.h>
#include "wlxplugin.h"

#define _detectstring "(EXT=\"DOC\")|(EXT=\"RTF\")|(EXT=\"DOT\")|(EXT=\"ABW\")|(EXT=\"AWT\")|(EXT=\"ZABW\")"


HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{

	GtkWidget *abi;
	libabiword_init_noargs();
	abi = abi_widget_new();
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), abi);
	gchar* fileUri = g_filename_to_uri(FileToLoad, NULL, NULL);

	if (abi_widget_load_file(ABI_WIDGET(abi), fileUri, ""))
	{
		if (fileUri)
			g_free(fileUri);

		return NULL;
	}

	if (fileUri)
		g_free(fileUri);

	abi_widget_view_print_layout(ABI_WIDGET(abi));
	gtk_widget_show_all(abi);
	return abi;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen-1);
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
}
