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
	g_print("bye\n");

	gtk_main_quit();
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

	g_print("hi\n");
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	pDocView = lok_doc_view_new(TEST_PATH, NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_window), pDocView);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);


	lok_doc_view_open_document(LOK_DOC_VIEW(pDocView), FileToLoad, "{}", NULL, NULL, NULL);

	if (id > 0)
	{
		plug = gtk_plug_new(id);
		gtk_container_add(GTK_CONTAINER(plug), GTK_WIDGET(scrolled_window));
		gtk_plug_construct(GTK_PLUG(plug), id);
		gtk_widget_show_all(plug);
		gtk_widget_realize(scrolled_window);
	}
	else
		return EXIT_FAILURE;

	g_signal_connect(plug, "destroy", G_CALLBACK(destroy), NULL);
	gtk_main();

	return EXIT_SUCCESS;
}
