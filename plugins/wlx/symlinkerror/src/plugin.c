#include <gtk/gtk.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "wlxplugin.h"

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	int errsv;
	ssize_t len;
	struct stat buf;
	char path[PATH_MAX];
	gchar *text = NULL;

	if (lstat(FileToLoad, &buf) != 0 || !S_ISLNK(buf.st_mode))
		return NULL;

	if (access(FileToLoad, F_OK) == -1)
	{
		errsv = errno;
	}
	else
		return NULL;


	if ((len = readlink(FileToLoad, path, sizeof(path) - 1)) != -1)
	{
		path[len] = '\0';
		text = g_strdup_printf("%s -> %s\n%s", FileToLoad, path, strerror(errsv));
	}
	else
		return NULL;

	GtkWidget *plug_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_WIDGET(ParentWin)), plug_vbox);
	GtkWidget *scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(plug_vbox), scroll_win);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	GtkWidget *label = gtk_label_new(NULL);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(label), FALSE);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_widget_modify_font(label, pango_font_description_from_string("momo 13"));
	gtk_label_set_text (GTK_LABEL(label), text);
	g_free(text);
	g_object_set_data(G_OBJECT(plug_vbox), "label", label);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll_win), label);
	gtk_widget_grab_focus(scroll_win);
	gtk_widget_show_all(plug_vbox);
	

	return plug_vbox;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	GtkWidget *view = (GtkWidget*)g_object_get_data(G_OBJECT(ListWin), "label");

	if (Command == lc_copy)
		gtk_signal_emitv_by_name(GTK_OBJECT(view), "copy-clipboard", NULL);

	return LISTPLUGIN_OK;
}
