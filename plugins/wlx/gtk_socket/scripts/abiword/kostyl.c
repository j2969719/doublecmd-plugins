#include <stdlib.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <libabiword.h>
#include <abiwidget.h>

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
	libabiword_shutdown();
	gtk_main_quit();
}

int main(int   argc, char *argv[])
{
	GtkWidget *abi;
	GtkWidget *plug;
	gchar* fileUri;
	GError *err = NULL;
	GOptionContext *context;

	gtk_init(&argc, &argv);
	libabiword_init_noargs();
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
	abi = abi_widget_new();

	if (FileToLoad)
		fileUri = g_filename_to_uri(FileToLoad, NULL, NULL);

	abi_widget_load_file(ABI_WIDGET(abi), fileUri, "");

	if (id > 0)
	{
		plug = gtk_plug_new(id);
		gtk_container_add(GTK_CONTAINER(plug), abi);
		gtk_plug_construct(GTK_PLUG(plug), id);
		gtk_widget_show_all(plug);
		gtk_widget_realize(abi);
	}
	else
		return EXIT_FAILURE;

	g_signal_connect(plug, "destroy", G_CALLBACK(destroy), NULL);
	gtk_main();

	return EXIT_SUCCESS;
}
