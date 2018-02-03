#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <limits.h>
#include <dlfcn.h>
#include <magic.h>
#include <string.h>
#include "wlxplugin.h"

#define DETECT_STRING "EXT=\"*\""

GtkWidget *tView; // ...
gchar *nfstr;

/** case insensitive forward search for implementation without
 *  GtkSourceView.
 */
gboolean gtk_text_iter_forward_search_nocase(GtkTextIter *iter,
                const gchar *text,
                GtkTextSearchFlags flags,
                GtkTextIter *mstart,
                GtkTextIter *mend)
{
	gunichar c;
	gchar *lctext = g_strdup(text);    /* copy text */
	gsize textlen = g_utf8_strlen(text, -1);    /* get length */

	lctext = g_utf8_strdown(lctext, -1);      /* convert to lower-case */

	for (;;)                /* iterate over all chars in range */
	{

		gsize len = textlen;               /* get char at iter */
		c = g_unichar_tolower(gtk_text_iter_get_char(iter));

		if (c == (gunichar)lctext[0]) /* compare 1st in lctext */
		{
			*mstart = *iter;      /* set start iter to current */

			for (gsize i = 0; i < len; i++)
			{
				c = g_unichar_tolower(gtk_text_iter_get_char(iter));

				/* compare/advance -- order IS important */
				if (c != (gunichar)lctext[i] ||
				                !gtk_text_iter_forward_char(iter))
					goto next;              /* start next search */
			}

			*mend = *iter;                  /* set end iter */

			if (lctext) g_free(lctext);     /* free lctext  */

			return TRUE;                    /* return true  */
		}

next:;  /* if at end of selecton break */

		if (!gtk_text_iter_forward_char(iter))
			break;
	}

	if (lctext) g_free(lctext);     /* free lctext */

	if (mstart || mend || flags) {}

	return FALSE;
}

/** case insensitive backward search for implementation without
 *  GtkSourceView.
 */
gboolean gtk_text_iter_backward_search_nocase(GtkTextIter *iter,
                const gchar *text,
                GtkTextSearchFlags flags,
                GtkTextIter *mstart,
                GtkTextIter *mend)
{
	gunichar c;
	gchar *lctext = g_strdup(text);    /* copy text */
	gsize textlen = g_utf8_strlen(text, -1);    /* get length */

	lctext = g_utf8_strdown(lctext, -1);      /* convert to lower-case */
	*mend = *iter;        /* initialize end iterator */

	while (gtk_text_iter_backward_char(iter))
	{

		gsize len = textlen - 1;  /* index for last in lctext */
		c = g_unichar_tolower(gtk_text_iter_get_char(iter));

		if (c == (gunichar)lctext[len]) /* initial comparison */
		{
			/* iterate over remaining chars in lctext/compare */
			while (len-- && gtk_text_iter_backward_char(iter))
			{
				c = g_unichar_tolower(gtk_text_iter_get_char(iter));

				if (c != (gunichar)lctext[len])
				{
					/* reset iter to right of char */
					gtk_text_iter_forward_char(iter);
					goto prev;
				}
			}

			*mstart = *iter; /* set start iter before last char */

			if (lctext) g_free(lctext);         /* free lctext */

			return TRUE;                    /* return success */
		}

prev:
		;
		*mend = *iter;   /* set end iter after next search char */
	}

	if (lctext) g_free(lctext);     /* free lctext */

	if (mstart || mend || flags) {}

	return FALSE;   /* no match */
}


HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	GtkWidget *gFix;
	GtkWidget *scroll;
	//GtkWidget *tView;
	GtkTextBuffer *tBuf;
	GtkWrapMode wrap_mode;
	gchar *tmp, *command, *buf1, *src, *font;
	gboolean bval, no_cursor;
	gint p_above, p_below;
	const char* cfg_file = "settings.ini";
	const char *magic_full;
	static char path[PATH_MAX];
	magic_t magic_cookie;
	GKeyFile *cfg;
	GError *err = NULL;
	Dl_info dlinfo;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(path, &dlinfo) != 0)
	{
		g_strlcpy(path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(path, '/');

		if (pos)
			strcpy(pos + 1, cfg_file);
	}

	cfg = g_key_file_new();

	if (!g_key_file_load_from_file(cfg, path, G_KEY_FILE_KEEP_COMMENTS, &err))
	{
		g_print("%s\n", (err)->message);
		g_error_free(err);
		return NULL;
	}
	else
	{
		magic_cookie = magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK);

		if (magic_load(magic_cookie, NULL) != 0)
		{
			magic_close(magic_cookie);
			return NULL;
		}

		magic_full = magic_file(magic_cookie, FileToLoad);

		g_print("%s\n", magic_full);
		src = g_key_file_get_string(cfg, "Scripts", magic_full, NULL);

		if (!src)
		{
			g_key_file_free(cfg);
			magic_close(magic_cookie);
			return NULL;
		}
		else
		{
			char *pos = strrchr(path, '/');

			if (pos)
				strcpy(pos + 1, src);
		}

		font = g_key_file_get_string(cfg, "Appearance", "Font", NULL);

		if (!font)
			font = "monospace 12";

		nfstr = g_key_file_get_string(cfg, "Appearance", "NotFoundStr", NULL);

		if (!nfstr)
			nfstr = "not found";

		if (err)
			err = NULL;

		p_above = g_key_file_get_integer(cfg, "Appearance", "PAbove", &err);

		if (err)
		{
			p_above = 0;
			err = NULL;
		}

		p_below = g_key_file_get_integer(cfg, "Appearance", "PBelow", &err);

		if (err)
		{
			p_below = 0;
			err = NULL;
		}

		bval = g_key_file_get_boolean(cfg, "Flags", "Wrap", NULL);

		if (bval)
			wrap_mode = GTK_WRAP_WORD;
		else
			wrap_mode = GTK_WRAP_NONE;

		bval = g_key_file_get_boolean(cfg, "Flags", "NoCursor", &err);

		if (!bval && !err)
			no_cursor = FALSE;
		else
			no_cursor = TRUE;
	}

	if (err)
		g_error_free(err);

	g_key_file_free(cfg);
	magic_close(magic_cookie);
	command = g_strdup_printf("\"%s\" \"%s\"", path, FileToLoad);

	if (!g_spawn_command_line_sync(command, &buf1, NULL, NULL, NULL))
	{
		g_free(command);
		return NULL;
	}

	g_free(command);

	gFix = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER((GtkWidget*)(ParentWin)), gFix);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	tBuf = gtk_text_buffer_new(NULL);
	g_object_set_data_full(G_OBJECT(gFix), "txtbuf", tBuf, (GDestroyNotify)g_object_unref);
	tmp = g_locale_to_utf8(buf1, -1, NULL, NULL, NULL);
	g_free(buf1);

	if ((tmp == NULL) || (g_strcmp0(tmp, "") == 0))
	{
		gtk_widget_destroy(GTK_WIDGET(gFix));
		return NULL;
	}

	gtk_text_buffer_set_text(tBuf, tmp, -1);

	tView = gtk_text_view_new_with_buffer(tBuf);
	gtk_widget_modify_font(tView, pango_font_description_from_string(font));
	gtk_text_view_set_editable(GTK_TEXT_VIEW(tView), FALSE);

	if (no_cursor)
		gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(tView), FALSE);

	gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(tView), p_above);
	gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(tView), p_below);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tView), wrap_mode);

	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(tView));
	gtk_container_add(GTK_CONTAINER(gFix), scroll);
	gtk_widget_show_all(gFix);

	g_free(tmp);

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

int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter)
{
	GtkTextBuffer *sBuf;
	GtkTextMark *last_pos;
	GtkTextIter iter, mstart, mend;
	gboolean found;

	sBuf = g_object_get_data(G_OBJECT(ListWin), "txtbuf");
	last_pos = gtk_text_buffer_get_mark(GTK_TEXT_BUFFER(sBuf), "last_pos");

	if (last_pos == NULL)
		gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(sBuf), &iter);
	else
		gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(sBuf), &iter, last_pos);

	if ((SearchParameter & lcs_backwards) && (SearchParameter & lcs_matchcase))
		found = gtk_text_iter_backward_search(&iter, SearchString, GTK_TEXT_SEARCH_TEXT_ONLY, &mend, &mstart, NULL);
	else if (SearchParameter & lcs_matchcase)
		found = gtk_text_iter_forward_search(&iter, SearchString, GTK_TEXT_SEARCH_TEXT_ONLY, &mstart, &mend, NULL);
	else if (SearchParameter & lcs_backwards)
		found = gtk_text_iter_backward_search_nocase(&iter, SearchString, GTK_TEXT_SEARCH_TEXT_ONLY, &mend, &mstart);
	else
		found = gtk_text_iter_forward_search_nocase(&iter, SearchString, GTK_TEXT_SEARCH_TEXT_ONLY, &mstart, &mend);

	if (found)
	{
		gtk_text_buffer_select_range(GTK_TEXT_BUFFER(sBuf), &mstart, &mend);
		gtk_text_buffer_create_mark(GTK_TEXT_BUFFER(sBuf), "last_pos", &mend, FALSE);
		//--------------------------------------------------------------------------------
		GtkTextBuffer *tmp = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tView)); // ...
		last_pos = gtk_text_buffer_get_mark(tmp, "last_pos");

		if (last_pos)
			gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(tView), last_pos);

		//--------------------------------------------------------------------------------
	}
	else
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ListWin))),
		                    GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		                    "\"%s\" %s!", SearchString, nfstr);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
}


int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
	GtkTextBuffer *tBuf;
	GtkTextIter p;

	tBuf = g_object_get_data(G_OBJECT(ListWin), "txtbuf");

	switch (Command)
	{
	case lc_copy :
		gtk_text_buffer_copy_clipboard(GTK_TEXT_BUFFER(tBuf), gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
		break;

	case lc_selectall :
		gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(tBuf), &p);
		gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(tBuf), &p);
		gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(tBuf), &p);
		gtk_text_buffer_move_mark_by_name(GTK_TEXT_BUFFER(tBuf), "selection_bound", &p);
		break;

	default :
		return LISTPLUGIN_ERROR;
	}
}
