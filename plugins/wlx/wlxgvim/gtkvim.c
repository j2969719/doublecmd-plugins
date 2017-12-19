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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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


#include "gtkvim.h"
#include <gdk/gdkx.h>
#include <stdarg.h>
#include <stdlib.h>


/* Local data */
static GtkWidgetClass *parent_class = NULL;

/* Args */
enum {
    ARG_0,
    ARG_SERVER_NAME,
};

/* Forward declararations */
static void gtk_vim_class_init      (GtkVimClass  *class);
static void gtk_vim_init            (GtkVim       *vim);
static void gtk_vim_set_arg         (GtkObject    *object,
                                     GtkArg       *arg,
                                     guint         arg_id);
static void gtk_vim_get_arg         (GtkObject    *object,
                                     GtkArg       *arg,
                                     guint         arg_id);
static void gtk_vim_realize         (GtkWidget    *widget);
static void gtk_vim_unrealize       (GtkWidget    *widget);
static gint gtk_vim_delete_event    (GtkWidget    *widget,
                                     GdkEventAny  *event);


GtkType
gtk_vim_get_type (void)
{
    static GtkType vim_type = 0;

    if (!vim_type)
        {
            static const GtkTypeInfo vim_info =
            {
                "GtkVim",
                sizeof (GtkVim),
                sizeof (GtkVimClass),
                (GtkClassInitFunc) gtk_vim_class_init,
                (GtkObjectInitFunc) gtk_vim_init,
                /* reserved_1 */ NULL,
                /* reserved_2 */ NULL,
                (GtkClassInitFunc) NULL,
            };

            vim_type = gtk_type_unique (GTK_TYPE_SOCKET, &vim_info);
        }

    return vim_type;
}


static void
gtk_vim_class_init (GtkVimClass *class)
{
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    object_class = (GtkObjectClass*) class;
    widget_class = (GtkWidgetClass*) class;
    parent_class = gtk_type_class (GTK_TYPE_SOCKET);

    gtk_object_add_arg_type ("GtkVim::server_name", GTK_TYPE_STRING, GTK_ARG_READABLE, ARG_SERVER_NAME);

    object_class->set_arg      = gtk_vim_set_arg;
    object_class->get_arg      = gtk_vim_get_arg;
    widget_class->delete_event = gtk_vim_delete_event;
    widget_class->realize      = gtk_vim_realize;
    widget_class->unrealize    = gtk_vim_unrealize;
}


static void
gtk_vim_init (GtkVim *vim)
{
    vim->server_name = NULL;
    vim->init_cols   = 0;
    vim->init_rows   = 0;
    vim->init_files  = NULL;
}


static void
plug_added (GtkWidget *widget,
	    void *data)
{
    gtk_widget_show (widget);
}

static gboolean
plug_removed (GtkWidget *widget,
	      void *data)
{
    gtk_widget_hide (widget);
    return TRUE;
}


/*
 * Args. are initial character cols/rows size of widget, plus NULL terminated
 * list of filenames to edit and/or gvim arguments.
 * You may of course give everything in one string with space separators.
 * But you still need the NULL.
 */
GtkWidget*
gtk_vim_new (gint   init_cols,
             gint   init_rows,
             gchar *filename,
             ...)
{
    GtkVim *vim;

    vim = gtk_type_new (gtk_vim_get_type ());

    if (init_cols <= 0  &&  init_rows <= 0)
    {
        vim->init_cols = vim->init_rows = 0;
    }
    else
    {
        vim->init_cols   = (init_cols <= 0  ?  80  :  init_cols);
        vim->init_rows   = (init_rows <= 0  ?  24  :  init_rows);
    }

    if (filename)
    {
        va_list args;
        gchar  *arg, *tmp;

        va_start( args, filename );

        vim->init_files = g_strdup(filename);
        for (arg = (gchar *)va_arg( args, gchar * );  arg; )
        {
            tmp = vim->init_files;
            vim->init_files = g_strconcat( tmp, " ", arg, NULL );
            g_free( tmp );
        }

        va_end(args);
    }
    else
    {
        vim->init_files = g_strdup("");
    }

    g_signal_connect (GTK_WIDGET (vim), "plug_added", G_CALLBACK (plug_added), NULL);
    g_signal_connect (GTK_WIDGET (vim), "plug_removed", G_CALLBACK (plug_removed), NULL);

    return GTK_WIDGET (vim);
}


static void
gtk_vim_set_arg (GtkObject    *object,
                 GtkArg       *arg,
                 guint         arg_id)
{
    GtkVim *vim;

    vim = GTK_VIM (object);

    switch (arg_id)
    {
        default:
            break;
    }
}

static void
gtk_vim_get_arg (GtkObject    *object,
                 GtkArg       *arg,
                 guint         arg_id)
{
    GtkVim *vim;

    vim = GTK_VIM (object);

    switch (arg_id)
    {
        case ARG_SERVER_NAME:
            GTK_VALUE_STRING (*arg) = g_strdup (vim->server_name);
            break;
        default:
            arg->type = GTK_TYPE_INVALID;
            break;
    }
}

static gint
gtk_vim_delete_event (GtkWidget    *widget,
                      GdkEventAny  *event)
{
    GtkVim *vim;

    vim = GTK_VIM (widget);

    if (!vim->server_name)  return TRUE;

    if (vim->socket.plug_window)
    {
        gchar *cmd;
        cmd = g_strdup_printf(
            "gvim --servername %s --remote-send '<C-\\><C-N>:qa<CR>'",
            vim->server_name );
        system(cmd);
        g_free(cmd);
    }

    /*
     * Eventually, I think this ought to ask vim to quit, and wait for it's
     * 'save changes' dialogue to return ok/no, and then return here
     * accordingly.
     * But we don't have that I/F yet, so just return TRUE ("OK to delete").
     * So for now, don't select 'Cancel' from a vim 'save changes' prompt!
     */
    return TRUE;
}

static void
gtk_vim_realize (GtkWidget *widget)
{
    GtkVim    *vim;
    gchar     *cmd, *xid_str;

    /* Do socket realize */
    parent_class->realize(widget);

    vim = GTK_VIM (widget);

    if (!widget->parent)
    {
        g_error( "gtk_vim_realize(): no parent" );
        return;
    }
    else if (!widget->parent->window)
    {
        g_error( "gtk_vim_realize(): no parent window" );
        return;
    }

    /* Ensure GDK has created our window so we know it's ID */
    gdk_flush();
    if (!widget->window)
    {
        g_error( "gtk_vim_realize(): no window" );
        return;
    }

    /* Now find XID of sub-window we're to use */
    xid_str = g_strdup_printf( "0x%X",
#ifdef GTK1
        GDK_WINDOW_XWINDOW(widget->window)
#else
        gtk_socket_get_id(&(vim->socket))
#endif
    );
    vim->server_name = g_strdup_printf( "GtkVim-%s", xid_str );

    /* Fork off a gvim (it forks itself) */
    if (vim->init_cols == 0  ||  vim->init_rows == 0)
    {
        cmd = g_strdup_printf(
            "gvim --servername %s --socketid %s %s",
            vim->server_name, xid_str,
            vim->init_files );
    }
    else
    {
        cmd = g_strdup_printf(
            "gvim -geom %dx%d --servername %s --socketid %s %s",
            vim->init_cols, vim->init_rows,
            vim->server_name, xid_str,
            vim->init_files );
    }
    system(cmd);

    g_free( xid_str );
    g_free( cmd );
}


static void
gtk_vim_unrealize (GtkWidget *widget)
{
    GtkVim *vim;

    vim = GTK_VIM (widget);

    if (vim->server_name)
    {
        g_free (vim->server_name);
        vim->server_name = NULL;
    }
    if (vim->init_files)
    {
        g_free (vim->init_files);
        vim->init_files = NULL;
    }

    /* Do socket unrealize */
    parent_class->unrealize(widget);
}


gchar*
gtk_vim_get_server_name (GtkWidget *widget)
{
    GtkVim *vim;

    g_return_val_if_fail (widget != NULL,  NULL);
    g_return_val_if_fail (GTK_IS_VIM (widget),  NULL);

    vim = GTK_VIM (widget);

    return g_strdup (vim->server_name);
}


void
gtk_vim_edit (GtkWidget *widget,
              gchar     *filename,
              ...)
{
    GtkVim *vim;
    va_list args;
    gchar  *files, *arg, *tmp, *cmd;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_VIM (widget));
    g_return_if_fail (filename != NULL);

    vim = GTK_VIM (widget);

    g_return_if_fail (vim->server_name != NULL);
    g_return_if_fail (vim->socket.plug_window != NULL);

    va_start( args, filename );

    files = g_strdup(filename);
    for (arg = (gchar *)va_arg( args, gchar * );  arg; )
    {
        tmp = files;
        files = g_strconcat( tmp, " ", arg, NULL );
        g_free( tmp );
    }

    va_end(args);

    cmd = g_strdup_printf(
        "gvim --servername %s --remote-send '<C-\\><C-N>:drop %s<CR>'",
        vim->server_name, files );
    system(cmd);

    g_free(cmd);
    g_free(files);
}


void
gtk_vim_remote_send (GtkWidget *widget,
                     gchar     *keys)
{
    GtkVim *vim;
    gchar  *cmd;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_VIM (widget));
    g_return_if_fail (keys != NULL);

    vim = GTK_VIM (widget);

    g_return_if_fail (vim->server_name != NULL);
    g_return_if_fail (vim->socket.plug_window != NULL);

    cmd = g_strdup_printf(
        "gvim --servername %s --remote-send '%s'",
        vim->server_name, keys );
    system(cmd);

    g_free(cmd);
}


void
gtk_vim_goto( GtkWidget *widget,
              gchar     *buffer,
              gint       line,
              gint       column )
{
    GtkVim *vim;
    gchar  *cmd, *vimcmd;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_VIM (widget));
    g_return_if_fail (line > 0  &&  column >= 0);

    vim = GTK_VIM (widget);

    g_return_if_fail (vim->server_name != NULL);
    g_return_if_fail (vim->socket.plug_window != NULL);

    if (buffer)
    {
        vimcmd = g_strdup_printf(
            "<C-\\><C-N>:buffer %s<cr>%dG%d|",
            buffer, line, column );
    }
    else
    {
        vimcmd = g_strdup_printf(
            "<C-\\><C-N>%dG%d|",
            line, column );
    }

    cmd = g_strdup_printf(
        "gvim --servername %s --remote-send '%s'",
        vim->server_name, vimcmd );
    system(cmd);

    g_free(cmd);
    g_free(vimcmd);
}
