#!/usr/bin/python

# copypasted from https://github.com/ArseniyK/sqlite_viewer

import os
import sys
os.environ["GDK_CORE_DEVICE_EVENTS"] = "1"

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GObject, GLib
import sqlite3

class SqliteViewer(Gtk.Plug):
    def __init__(self):
        Gtk.Plug.__init__(self)
        Gtk.Plug.construct(self, int(sys.argv[1]))
        
        self.conn = None
        self.cursor = None
        self.store = None
        self.isopen = False
        
        self.tables = Gtk.ComboBoxText()
        self.table = Gtk.TreeView()
        self.table.set_grid_lines(Gtk.TreeViewGridLines.BOTH)
        
        self.dialog = Gtk.Dialog()
        
        self.query = Gtk.Label()
        self.query.set_selectable(True)
        button = Gtk.Button.new_with_label("Query")
        button.connect("pressed", self.query_dialog)
        
        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        labeltbl = Gtk.Label()
        labeltbl.set_text("Table:")
        hbox.pack_start(labeltbl, False, True, 1)
        hbox.pack_start(self.tables, False, False, 1)
        hbox.pack_start(self.query, True, True, 2)
        hbox.pack_end(button, False, True, 1)
        hbox.set_border_width(5)
        
        self.sw = Gtk.ScrolledWindow()
        self.sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        self.sw.add(self.table)
        
        self.win = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.win.pack_start(hbox, False, True, 1)
        self.win.pack_end(self.sw, True, True, 1)

        self.open_db(sys.argv[2])
        query = "SELECT * FROM {0};".format(self.tables.get_active_text())

        self.query.set_text(query)
        if self.isopen:
           GLib.idle_add(self.execute, query)
        
        Gtk.Plug.add(self, self.win)
        Gtk.Plug.connect(self, "destroy", self.exit)

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
               self.store = Gtk.ListStore(*([str] * len(names)))
               for row in self.cursor.fetchall():
                   self.store.append(list(map(str,row)))
               self.table.set_model(self.store)
               self.create_collumns(names)
           except sqlite3.DatabaseError as err:
               self.show_message(str(err))

    def create_collumns(self, column_names):
        rendererText = Gtk.CellRendererText()
        rendererText.set_property('editable', True)
        rendererText.set_property("single-paragraph-mode", True)
        rendererText.set_property("max_width_chars", 256)
        for index, name in enumerate(column_names):
            column = Gtk.TreeViewColumn(name, rendererText, text=index)
            column.props.resizable = True
            column.set_sort_column_id(index)
            label = Gtk.Label()
            label.set_text(str(name))
            label.show()
            column.props.widget = label
            self.table.append_column(column)

    def show_message(self, msgtxt):
        self.msg = Gtk.MessageDialog(parent=None, flags=0, message_type=Gtk.MessageType.ERROR, buttons=Gtk.ButtonsType.CLOSE, text=msgtxt)
        self.msg.show_all()
        self.msg.connect("response", lambda *a: self.msg.destroy())

    def entry_activate(self, widget):
        query = self.query.get_text()
        self.table_refresh(query)

    def query_dialog(self, widget):
        query = self.query.get_text()
        self.dialog = Gtk.Dialog("Query")
        self.dialog.add_buttons(Gtk.STOCK_CANCEL, Gtk.ResponseType.REJECT, Gtk.STOCK_OK, Gtk.ResponseType.OK)
        contr = Gtk.HBox()
        entry = Gtk.Entry()
        entry.set_text(query)
        entry.set_activates_default(True)
        entry.set_size_request(500,-1)
        contr.pack_end(entry, True, True, 0)
        contr.set_border_width(5)
        contr.show_all()
        self.dialog.get_content_area().add(contr)
        self.dialog.set_default_response(Gtk.ResponseType.OK)
        response = self.dialog.run()
        query = entry.get_text()
        self.dialog.destroy()

        if response == Gtk.ResponseType.OK:
           self.query.set_text(query)
           self.table_refresh(query)

    def table_refresh(self, query):
        self.table.destroy()
        self.table = Gtk.TreeView()
        self.table.set_grid_lines(Gtk.TreeViewGridLines.BOTH)
        self.sw.add(self.table)
        self.sw.show_all()
        GLib.idle_add(self.execute, query)

    def exit(self, widget):
        self.conn.close()
        self.dialog.destroy()
        Gtk.main_quit()

win = SqliteViewer()
win.show_all()
Gtk.main()
