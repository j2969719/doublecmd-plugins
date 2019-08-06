#include <stdlib.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <evince-view.h>
#include <evince-document.h>

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
	ev_shutdown();
	gtk_main_quit();
}

int main(int   argc, char *argv[])
{
	GtkWidget *plug;
	GtkWidget *scrolled_window;
	GtkWidget *view;
	EvDocument *document;
	EvDocumentModel *docmodel;
	gchar* fileUri;
	GError *err = NULL;
	GOptionContext *context;

	gtk_init(&argc, &argv);

	if (!ev_init())
		return EXIT_FAILURE;

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

	if (FileToLoad)
		fileUri = g_filename_to_uri(FileToLoad, NULL, NULL);

	document = EV_DOCUMENT(ev_document_factory_get_document(fileUri, NULL));

	if (EV_IS_DOCUMENT(document))
	{
		docmodel = EV_DOCUMENT_MODEL(ev_document_model_new_with_document(document));
		view = ev_view_new();
		ev_view_set_model(EV_VIEW(view), docmodel);
		g_object_unref(document);
		g_object_unref(docmodel);
		gtk_container_add(GTK_CONTAINER(scrolled_window), view);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		                               GTK_POLICY_AUTOMATIC,
		                               GTK_POLICY_AUTOMATIC);
	}
	else
	{
		gtk_widget_destroy(scrolled_window);
		return EXIT_FAILURE;
	}

	if (id > 0)
	{
		plug = gtk_plug_new(id);
		gtk_container_add(GTK_CONTAINER(plug), scrolled_window);
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
