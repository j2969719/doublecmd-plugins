/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

/*
 * CAVEAT: only works on a system with a GTK+ enabled gvim 6.0+ in PATH,
 *         and with a system() call that can start it!  (relies upon gvim
 *         to fork itself into a new process).
 *         mailto:gtkvim@fnxweb.com
 */

#ifndef __GTK_VIM_H__
#define __GTK_VIM_H__


#include <gtk/gtksocket.h>

/* gtksocket.h doesn't as yet supply this */
#ifndef GTK_TYPE_SOCKET
#define GTK_TYPE_SOCKET  (gtk_socket_get_type())
#endif


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_VIM		 (gtk_vim_get_type ())
#define GTK_VIM(obj)		 (GTK_CHECK_CAST ((obj), GTK_TYPE_VIM, GtkVim))
#define GTK_VIM_CLASS(klass)	 (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_VIM, GtkVimClass))
#define GTK_IS_VIM(obj)	 (GTK_CHECK_TYPE ((obj), GTK_TYPE_VIM))
#define GTK_IS_VIM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_VIM))


typedef struct _GtkVim	     GtkVim;
typedef struct _GtkVimClass  GtkVimClass;

/*
 * No public entities; use functions below to access data.
 * Widgets args:
 *      GtkVim::server_name  :  STRING  -  Read Only
 */
struct _GtkVim
{
  GtkSocket socket;
  gchar*    server_name;
  gint      init_cols, init_rows;
  gchar*    init_files;
};

struct _GtkVimClass
{
  GtkSocketClass parent_class;
};


/* Standard type ID */
GtkType    gtk_vim_get_type (void);

/*
 * Args. are initial character cols/rows size of widget, plus NULL terminated
 * list of filenames to edit and/or gvim arguments.
 * You may of course give everything in one string with space separators.
 * But you still need the NULL.
 */
GtkWidget* gtk_vim_new	     (gint   init_cols,
                              gint   init_rows,
                              gchar *filename,
                              ...);

/* Get the hopefully unique servername that gvim was started with */
gchar*     gtk_vim_get_server_name (GtkWidget *widget);

/*
 * Pass NULL terminated list of files (may be space separated) to already
 * started (i.e., realized) GtkVim widget for editing.
 */
void       gtk_vim_edit (GtkWidget *widget,
                         gchar     *filename,
                         ...);

/* Send keypresses to vim process - read vim docs. ':help --remote-send' */
void       gtk_vim_remote_send (GtkWidget *widget,
                                gchar     *keys);

/*
 * Set cursor position to line/column in named buffer.
 * Use current buffer if buffername is NULL.
 */
void gtk_vim_goto( GtkWidget *widget,
                   gchar     *buffer,
                   gint       line,
                   gint       column );


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif  /* __GTK_VIM_H__ */
