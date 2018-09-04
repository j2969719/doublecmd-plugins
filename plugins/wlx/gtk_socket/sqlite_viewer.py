#!/usr/bin/python2

# copypasted from https://github.com/ArseniyK/sqlite_viewer

#TODO: load tables in separate thread
#TODO: pysqlite as backend
#TODO: allow query

import pygtk
pygtk.require('2.0')
import sys

import sqlite3
import gtk, gobject


Wid = 0L
# TREE_VIEW_GRID_LINES_NONE TREE_VIEW_GRID_LINES_BOTH TREE_VIEW_GRID_LINES_VERTICAL
grid_lines = gtk.TREE_VIEW_GRID_LINES_VERTICAL
# cell_width = -1
# cell_height = -1

class SqliteViewer:
    def __init__(self, xid, path):

        self.conn = None
        self.cursor = None
        self.store = None
        self.isopen = False

        self.tables = gtk.combo_box_new_text()
        self.query = gtk.Entry()
        self.table = gtk.TreeView()

        self.dialog = gtk.Dialog()

        self.table.set_enable_search(True)
        # self.query.set_editable(False)
        self.table.set_grid_lines(grid_lines)
        # self.table.set_hover_selection(True)
        self.query.connect("activate", self.entry_activate)

        button = gtk.Button("Query", None, False)
        button.connect("pressed", self.kostyl_dialog)

        hbox = gtk.HBox(homogeneous=False, spacing=5)
        hbox.pack_start(gtk.Label("Tables:"), False)
        hbox.pack_start(self.tables, False)
        hbox.add(self.query)
        hbox.pack_end(button, False)
        hbox.set_border_width(5)

        self.sw = gtk.ScrolledWindow()
        self.sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        self.sw.add(self.table)

        self.container = gtk.VBox(homogeneous=False, spacing=5)
        self.container.pack_start(hbox, expand=False)
        self.container.add(self.sw)

        self.open_db(path)
        query = "SELECT * FROM {0};".format(self.tables.get_active_text())

        self.query.set_text(query)
        if self.isopen:
           gobject.idle_add(self.execute, query)
        self.plug = gtk.Plug(xid)
        self.plug.add(self.container)
        self.plug.connect("destroy", self.exit)
        self.plug.show_all()

    def open_db(self, path):
        self.conn = sqlite3.connect(path)
        self.cursor = self.conn.cursor()
        try:
           self.cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")

           for table in self.cursor.fetchall():
               self.tables.append_text(table[0])
           self.tables.set_active(0)
           self.tables.connect('changed', self.change_table)
           self.isopen = True
        except sqlite3.DatabaseError as err:
           self.show_message(str(err))

    def change_table(self, tables):
        query = "SELECT * FROM {0};".format(tables.get_active_text())
        self.query.set_text(query)
        self.table_refresh(query)

    def execute(self, query):
        if query:
           try:
               self.cursor.execute(query)
               names = list(map(lambda x: x[0], self.cursor.description))
               self.store = gtk.ListStore(*([str] * len(names)))
               for row in self.cursor.fetchall():
                   self.store.append(row)
               self.table.set_model(self.store)
               self.create_collumns(names)
           except sqlite3.DatabaseError as err:
               self.show_message(str(err))

    def create_collumns(self, column_names):
        rendererText = gtk.CellRendererText()
        rendererText.set_property('editable', True)
        # rendererText.set_fixed_size(cell_width, cell_height)
        rendererText.set_fixed_height_from_font(1)
        for index, name in enumerate(column_names):
            column = gtk.TreeViewColumn(name, rendererText, text=index)
            column.props.resizable = True
            column.set_sort_column_id(index)
            self.table.append_column(column)

    def show_message(self, text):
        msg = gtk.MessageDialog(None, 0, gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE, text)
        msg.show_all()
        msg.connect("response", lambda *a: msg.destroy())

    def entry_activate(self, widget):
        query = self.query.get_text()
        self.table_refresh(query)

    def kostyl_dialog(self, widget):
        query = self.query.get_text()
        self.dialog = gtk.Dialog("Query", buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT, gtk.STOCK_OK, gtk.RESPONSE_OK))
        container = gtk.HBox()
        entry = gtk.Entry()
        entry.set_text(query)
        entry.set_activates_default(True)
        entry.set_size_request(500,-1)
        container.pack_end(entry)
        container.set_border_width(5)
        container.show_all()
        self.dialog.vbox.pack_end(container, True, True, 0)
        self.dialog.set_default_response(gtk.RESPONSE_OK)
        response = self.dialog.run()
        query = entry.get_text()
        self.dialog.destroy()

        if response == gtk.RESPONSE_OK:
           self.query.set_text(query)
           self.table_refresh(query)

    def table_refresh(self, query):
        self.table.destroy()
        self.table = gtk.TreeView()
        self.table.set_enable_search(True)
        self.table.set_grid_lines(grid_lines)
        self.sw.add(self.table)
        self.sw.show_all()
        gobject.idle_add(self.execute, query)

    def exit(self, widget):
        self.conn.close()
        self.dialog.destroy()
        gtk.main_quit()


if len(sys.argv) == 3:
    Wid = long(sys.argv[1])
    SqliteViewer(Wid, sys.argv[2])
    gtk.main()