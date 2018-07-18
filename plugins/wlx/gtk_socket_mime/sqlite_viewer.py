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
if len(sys.argv) == 3:
    Wid = long(sys.argv[1])

class SqliteViewer:
    def __init__(self, xid, path):

        self.conn = None
        self.cursor = None
        self.store = None

        self.tables = gtk.combo_box_new_text()
        self.query = gtk.Entry()
        self.table = gtk.TreeView()

        hbox = gtk.HBox(homogeneous=False, spacing=5)
        hbox.add(self.tables)
        hbox.add(self.query)

        self.sw = gtk.ScrolledWindow()
        self.sw.add(self.table)

        self.container = gtk.VBox(homogeneous=False, spacing=5)
        self.container.pack_start(hbox, expand=False)
        self.container.add(self.sw)

        self.open_db(path)
        query = "SELECT * FROM {0};".format(self.tables.get_active_text())
        self.query.set_text(query)
        gobject.idle_add(self.execute, query)
        self.plug = gtk.Plug(xid)
        self.plug.add(self.container)
        self.plug.connect("destroy", lambda w: gtk.main_quit())
        self.plug.show_all()

    def open_db(self, path):
        self.conn = sqlite3.connect(path)
        self.cursor = self.conn.cursor()
        self.cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")

        for table in self.cursor.fetchall():
            self.tables.append_text(table[0])
        self.tables.set_active(0)
        self.tables.connect('changed', self.change_table)

    def change_table(self, tables):
        query = "SELECT * FROM {0};".format(tables.get_active_text())
        self.query.set_text(query)
        self.table.destroy()
        self.table = gtk.TreeView()
        self.sw.add(self.table)
        self.sw.show_all()
        gobject.idle_add(self.execute, query)

    def execute(self, query):
        self.cursor.execute(query)
        names = list(map(lambda x: x[0], self.cursor.description))
        self.store = gtk.ListStore(*([str] * len(names)))
        for row in self.cursor.fetchall():
            self.store.append(row)
        self.table.set_model(self.store)
        self.create_collumns(names)

    def create_collumns(self, column_names):
        rendererText = gtk.CellRendererText()
        for index, name in enumerate(column_names):
            column = gtk.TreeViewColumn(name, rendererText, text=index)
            column.set_sort_column_id(1)
            self.table.append_column(column)


SqliteViewer(Wid, sys.argv[2])
gtk.main()