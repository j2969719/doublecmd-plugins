#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

// file associations > view > command = /path/to/sushi_kostyl, parameters = %p

int main(int argc, char **argv)
{
	const gchar *cmd = "dbus-send --print-reply --dest=org.gnome.NautilusPreviewer \
/org/gnome/NautilusPreviewer org.gnome.NautilusPreviewer.ShowFile string:\"%s\" int32:0 boolean:true";
	gchar *fileUri = g_filename_to_uri(argv[1], NULL, NULL);
	system(g_strdup_printf(cmd, fileUri));
	return(EXIT_SUCCESS);
}
