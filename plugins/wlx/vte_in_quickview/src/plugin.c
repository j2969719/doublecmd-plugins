#define _GNU_SOURCE

#include <gtk/gtk.h>
#include <vte/vte.h>
#include <gdk/gdkkeysyms.h>
#include "wlxplugin.h"

#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

#define DETECT_STRING "EXT=\"*\""

void vte_popup_menu_copy(GtkWidget *menuitem, gpointer userdata)
{
	vte_terminal_copy_clipboard(VTE_TERMINAL(userdata));
}

void vte_popup_menu_paste(GtkWidget *menuitem, gpointer userdata)
{
	vte_terminal_paste_clipboard(VTE_TERMINAL(userdata));
}

void vte_popup_menu_ctrlc(GtkWidget *menuitem, gpointer userdata)
{
	vte_terminal_feed_child(VTE_TERMINAL(userdata), "\003", -1);
}

void vte_popup_menu_ctrlz(GtkWidget *menuitem, gpointer userdata)
{
	vte_terminal_feed_child(VTE_TERMINAL(userdata), "\032", -1);
}

gboolean grab_hack(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
	{
		GtkWidget *menu;
		GtkWidget *mipaste;
		GtkWidget *micopy;
		GtkWidget *mictrlc;
		GtkWidget *mictrlz;

		menu = gtk_menu_new();

		micopy = gtk_menu_item_new_with_label(_("Copy"));
		mipaste = gtk_menu_item_new_with_label(_("Paste"));
		mictrlc = gtk_menu_item_new_with_label(_("Send CTRL+C"));
		mictrlz = gtk_menu_item_new_with_label(_("Send CTRL+Z"));

		g_signal_connect(micopy, "activate", G_CALLBACK(vte_popup_menu_copy), (gpointer)widget);
		g_signal_connect(mipaste, "activate", G_CALLBACK(vte_popup_menu_paste), (gpointer)widget);
		g_signal_connect(mictrlc, "activate", G_CALLBACK(vte_popup_menu_ctrlc), (gpointer)widget);
		g_signal_connect(mictrlz, "activate", G_CALLBACK(vte_popup_menu_ctrlz), (gpointer)widget);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), micopy);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mipaste);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mictrlc);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mictrlz);

		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent*)event));
	}

	gtk_label_set_text(GTK_LABEL(user_data), _("hit ctrl+q or type \"exit\" to remove grab"));
	gtk_grab_add(widget);
	return FALSE;
}

static void grab_hack_remove(GtkWidget *widget, gpointer user_data)
{
	gtk_label_set_text(GTK_LABEL(user_data), NULL);
	gtk_grab_remove(widget);
}

gboolean key_press_hack(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{

	switch (event->keyval)
	{
	case GDK_q:
		if (event -> state & GDK_CONTROL_MASK)
		{
			gtk_grab_remove(widget);
			gtk_label_set_text(GTK_LABEL(user_data), NULL);
		}

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

	hint = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(gFix), hint, FALSE, FALSE, 1);

	gtk_widget_show_all(gFix);

	g_signal_connect(G_OBJECT(vte), "key_press_event", G_CALLBACK(key_press_hack), (gpointer)hint);
	g_signal_connect(G_OBJECT(vte), "child-exited", G_CALLBACK(grab_hack_remove), (gpointer)hint);
	g_signal_connect(G_OBJECT(vte), "button-press-event", G_CALLBACK(grab_hack), (gpointer)hint);

	return gFix;

}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	gtk_widget_destroy(GTK_WIDGET(ListWin));
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, DETECT_STRING, maxlen-1);
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	const gchar* dir_f = "%s/langs";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(dir_f, &dlinfo) != 0)
	{
		setlocale(LC_ALL, "");
		gchar *plugdir = g_path_get_dirname(dlinfo.dli_fname);
		gchar *langdir = g_strdup_printf(dir_f, plugdir);
		g_free(plugdir);
		bindtextdomain(GETTEXT_PACKAGE, langdir);
		g_free(langdir);
		textdomain(GETTEXT_PACKAGE);
	}
}
