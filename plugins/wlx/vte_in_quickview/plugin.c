#include <gtk/gtk.h>
#include <vte/vte.h>
#include <gdk/gdkkeysyms.h>
#include "wlxplugin.h"

#define DETECT_STRING "EXT=\"*\""
#define HACK_HINT "hit ctrl+q twice(or type 'exit' and press ctrl+q) to close quick view"

static void grab_hack(GtkWidget *widget, gpointer user_data)
{
	gtk_grab_remove(widget);
}

gboolean key_press_hack(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{

	switch (event->keyval)
	{
	case GDK_q:
		if (event -> state & GDK_CONTROL_MASK)
			gtk_grab_remove(widget);

		break;

	default:
		return FALSE;
	}

	return FALSE;
}

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	if (!g_file_test(FileToLoad, G_FILE_TEST_IS_DIR))
		return NULL;

	GtkWidget *gFix;
	GtkWidget *scroll;
	GtkWidget *vte;
	GtkWidget *hint;
	VteTerminal *terminal;

	gchar **envp = g_get_environ();
	gchar **command = (gchar * [])
	{
		g_strdup(g_environ_getenv(envp, "SHELL")), NULL
	};
	g_strfreev(envp);

	gFix = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);

	vte = vte_terminal_new();
	terminal = VTE_TERMINAL(vte);
	scroll = gtk_scrolled_window_new(NULL, terminal->adjustment);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	vte_terminal_fork_command_full(terminal, VTE_PTY_DEFAULT, FileToLoad, command, NULL, 0, NULL, NULL, NULL, NULL);
	vte_terminal_set_scrollback_lines(terminal, -1);
	vte_terminal_set_background_transparent(terminal, FALSE);

	gtk_box_pack_start(GTK_BOX(gFix), scroll, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(scroll), vte);

	hint = gtk_label_new(HACK_HINT);
	gtk_box_pack_start(GTK_BOX(gFix), hint, FALSE, FALSE, 1);

	gtk_widget_show_all(gFix);

	gtk_grab_add(vte);
	g_signal_connect(G_OBJECT(vte), "key_press_event", G_CALLBACK(key_press_hack), NULL);
	g_signal_connect(G_OBJECT(vte), "child-exited", G_CALLBACK(grab_hack), NULL);

	return gFix;

}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, DETECT_STRING, maxlen);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}
