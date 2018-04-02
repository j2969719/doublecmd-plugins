#include <stdlib.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
//#include <LibreOfficeKit/LibreOfficeKit.h>
//#include <LibreOfficeKit/LibreOfficeKitInit.h>
#include <LibreOfficeKit/LibreOfficeKitGtk.h>

#define TEST_PATH "/usr/lib/libreoffice/program"

//static GdkNativeWindow id = 0;
static Window id = 0;
static gchar *FileToLoad;
static GOptionEntry entries[] =
{
	{ "xid", 'w', 0, G_OPTION_ARG_INT, &id, "Window ID", "ID" },
	{ "file", 'f', 0, G_OPTION_ARG_FILENAME, &FileToLoad, "File to load", "File" },
	{ NULL }
};

static void destroy(GtkWidget *widget, gpointer data)
{
	//g_print("bye\n");

	gtk_main_quit();
}

static void on_find_response(GtkDialog *dialog, gint response_id, gpointer userdata)
{
	GList *list = gtk_container_get_children(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))));

	if (response_id == GTK_RESPONSE_YES)
	{
		lok_doc_view_find_prev(LOK_DOC_VIEW(userdata), gtk_entry_get_text(GTK_ENTRY(list->data)), FALSE);
	}
	else if (response_id == GTK_RESPONSE_NO)
		lok_doc_view_find_next(LOK_DOC_VIEW(userdata), gtk_entry_get_text(GTK_ENTRY(list->data)), FALSE);

	g_list_free(list);
	//	gtk_widget_destroy(GTK_WIDGET(dialog));
}

void lo_popup_menu_find(GtkWidget *menuitem, gpointer userdata)
{
	GtkWindow *window;
	GtkWidget *dialog;
	GtkWidget *content_area;
	GtkWidget *stext;
	gint response_id;

	window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(userdata))); //GTK_WINDOW_TOPLEVEL
	dialog = gtk_dialog_new_with_buttons("Find", window, GTK_DIALOG_MODAL,
	                                     "Prev", GTK_RESPONSE_YES, "Next", GTK_RESPONSE_NO, NULL);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	stext = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(content_area), stext);
	gtk_widget_show_all(dialog);
	g_signal_connect(GTK_DIALOG(dialog), "response", G_CALLBACK(on_find_response), (gpointer)userdata);
}

void lo_popup_menu_copy(GtkWidget *menuitem, gpointer userdata)
{
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
	                       lok_doc_view_copy_selection(LOK_DOC_VIEW(userdata),
	                                       "text/plain;charset=utf-8", NULL), -1);

}

gboolean view_popup_menu(GtkWidget *widget, GdkEventButton *event, gpointer userdata)
{

	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
	{
		GtkWidget *menu;
		GtkWidget *mifind;
		GtkWidget *micopy;

		menu = gtk_menu_new();

		micopy = gtk_menu_item_new_with_label("Copy");
		mifind = gtk_menu_item_new_with_label("Find");

		g_signal_connect(micopy, "activate", G_CALLBACK(lo_popup_menu_copy), (gpointer)widget);
		g_signal_connect(mifind, "activate", G_CALLBACK(lo_popup_menu_find), (gpointer)widget);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), micopy);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mifind);

		gtk_widget_show_all(menu);
		gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
		return TRUE;
	}

	return FALSE;
}

gboolean view_kostyl_primary(GtkWidget *widget, GdkEventButton *event, gpointer userdata)
{
	if (event->type == GDK_BUTTON_RELEASE  &&  event->button == 1)
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY),
		                       lok_doc_view_copy_selection(LOK_DOC_VIEW(widget),
		                                       "text/plain;charset=utf-8", NULL), -1);

	return FALSE;
}
int main(int   argc, char *argv[])
{
	GtkWidget *pDocView;
	GtkWidget *plug;
	GtkWidget *scrolled_window;
	gchar* fileUri;
	GError *err = NULL;
	GOptionContext *context;

	gtk_init(&argc, &argv);

	context = g_option_context_new("- commandline options");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));

	if (!g_option_context_parse(context, &argc, &argv, &err))
	{
		g_print("option parsing failed: %s\n", err->message);
		exit(1);
	}

	if (err)
		g_error_free(err);

	//g_print("hi\n");
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	pDocView = lok_doc_view_new(TEST_PATH, NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_window), pDocView);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_capture_button_press(GTK_SCROLLED_WINDOW(scrolled_window), TRUE);
	gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scrolled_window), FALSE);


	lok_doc_view_open_document(LOK_DOC_VIEW(pDocView), FileToLoad, "{}", NULL, NULL, NULL);

	if (id > 0)
	{
		plug = gtk_plug_new(id);
		gtk_container_add(GTK_CONTAINER(plug), GTK_WIDGET(scrolled_window));
		gtk_plug_construct(GTK_PLUG(plug), id);
		gtk_widget_show_all(plug);
		gtk_widget_realize(scrolled_window);
		gtk_widget_grab_focus(scrolled_window);
	}
	else
		return EXIT_FAILURE;

	g_signal_connect(plug, "destroy", G_CALLBACK(destroy), NULL);
	gtk_widget_add_events(pDocView, GDK_SCROLL_MASK);
	g_signal_connect(GTK_WIDGET(pDocView), "button_press_event", G_CALLBACK(view_popup_menu), NULL);
	g_signal_connect(GTK_WIDGET(pDocView), "button_release_event", G_CALLBACK(view_kostyl_primary), NULL);

	gtk_main();

	return EXIT_SUCCESS;
}
