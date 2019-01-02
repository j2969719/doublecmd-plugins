#!/usr/bin/env python3
# https://github.com/mattn/gtkjsonviewer

import sys
import os
import gi
import re
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Gdk
try:
  import json
except:
  import simplejson as json
  pass

raw_data = ''
from_stdin = not(sys.stdin.isatty())

if len(sys.argv) == 3:
  raw_data = open(sys.argv[2]).read().strip()
elif from_stdin:
  raw_data = sys.stdin.read().strip()

if raw_data and raw_data[0] == '(' and raw_data[-1] == ')':
  raw_data = raw_data[1:-1]

is_dark = Gtk.Settings.get_default().get_property("gtk-application-prefer-dark-theme")
if is_dark:
  color_array = 'yellow'
  color_type = 'orange'
  color_string = 'pink'
  color_integer = 'red'
  color_object = 'yellow'
  color_key = 'light green'
else:
  color_array = 'magenta'
  color_type = 'orange'
  color_string = 'purple'
  color_integer = 'red'
  color_object = 'blue'
  color_key = 'dark green'

def add_item(key, data, model, parent = None):
  if isinstance(data, dict):
    if len(key):
      obj = model.append(parent, ['<span foreground="'+color_object+'">'
                                  + str(key) + '</span>' +
                                  ' <span foreground="'+color_type+'"><b>{}</b></span>'])
      walk_tree(data, model, obj)
    else:
      walk_tree(data, model, parent)
  elif isinstance(data, list):
    arr = model.append(parent, ['<span foreground="'+color_array+'">'+ key + '</span> '
                                '<span foreground="'+color_type+'"><b>[]</b></span> ' +
                                '<span foreground="'+color_integer+'">' + str(len(data)) + '</span>'])
    for index in range(0, len(data)):
      add_item('', data[index], model, model.append(arr, ['<b><span foreground="'+color_type+'">'+'['+'</span></b><span foreground="'+color_integer+'">'
                                                          + str(index)
                                                          + '</span><b><span foreground="'+color_type+'">]</span></b>']))
  elif isinstance(data, str):
    if len(data) > 256:
      data = data[0:255] + "..."
      if len(key):
        model.append(parent, ['<span foreground="'+color_key+'">"' + key + '"</span>' +
                            '<b>:</b> <span foreground="'+color_string+'">"' + data + '"</span>'])
      else:
        model.append(parent, ['<span foreground="'+color_string+'">"' + data + '"</span>'])
    else:
      if len(key):
        model.append(parent, ['<span foreground="'+color_key+'">"' + key + '"</span>' +
                            '  <b>:</b> <span foreground="'+color_string+'">"' + data + '"</span>'])
      else:
        model.append(parent, ['<span foreground="'+color_string+'">"' + data + '"</span>'])

  elif isinstance(data, int):
    model.append(parent, ['<span foreground="'+color_key+'">"' + key + '"</span>' +
                          '  <b>:</b> <span foreground="'+color_integer+'">' + str(data) + '</span>'])
  else:
    model.append(parent, [str(data)])

def walk_tree(data, model, parent = None):
  if isinstance(data, list):
    add_item('', data, model, parent)
  elif isinstance(data, dict):
    for key in sorted(data):
      add_item(key, data[key], model, parent)
  else:
    add_item('', data, model, parent)

# Key/property names which match this regex syntax may appear in a
# JSON path in their original unquoted form in dotted notation.
# Otherwise they must use the quoted-bracked notation.
jsonpath_unquoted_property_regex = re.compile(r"^[a-zA-Z][a-zA-Z0-9_]*$")

#return the json query given a path
def to_jq(path, data):
  indices = path.get_indices()
  jq = ''
  is_array_index = False

  #the expression must begins with identity `.`
  #if the first element is not a dict, add a dot
  if not isinstance(data, dict):
    jq += '.'

  for index in indices:
    if isinstance(data, dict):
      key = (list(sorted(data))[index])
      if len(key)==0 or not jsonpath_unquoted_property_regex.match(key):
        jq += '[\'{}\']'.format(key) # bracket notation (no initial dot)
      else:
        jq += '.' + key # dotted notation
      data = data[key]
      if isinstance(data, list):
        jq += '[]'
        is_array_index = True
    elif isinstance(data, list):
      if is_array_index:
        selected_index = index
        jq = jq[:-2]   #remove []
        jq += '[{}]'.format(selected_index)
        data = data[selected_index]
        is_array_index = False
      else:
        jq += '[]'
        is_array_index = True

  return jq

class JSONViewerWindow(Gtk.Plug):
    def __init__(self):
      #Gtk.Plug.__init__(self, title="JSon Viewer")
      Gtk.Plug.__init__(self)
      Gtk.Plug.construct(self, int(sys.argv[1]))
      #self.set_default_size(600, 400)

      self.clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)

      self.label_info = Gtk.Label()
      self.label_info.set_selectable(True)

      self.data = None

      column_title = ''

      if(raw_data):
        try:
          self.parse_json(raw_data)
        except Exception as e:
          self.label_info.set_text(str(e))

        if from_stdin:
          column_title = '<input stream>'
        else:
          column_title = sys.argv[1]

      else:
        self.label_info.set_text("No data loaded")

      menubar = Gtk.MenuBar()
      menuitem_file = Gtk.MenuItem(label="File")
      menubar.append(menuitem_file)
      menu = Gtk.Menu()
      menuitem_file.set_submenu(menu)
      menuitem_file_open = Gtk.MenuItem(label="Open")
      menuitem_file_open.connect("activate", self.open_callback)
      menu.append(menuitem_file_open)

      self.model = Gtk.TreeStore(str)
      swintree = Gtk.ScrolledWindow()
      swinpath = Gtk.ScrolledWindow()
      self.tree = Gtk.TreeView(self.model)
      self.tree.connect("button-release-event", self.on_treeview_button_press_event)
      cell = Gtk.CellRendererText()


      self.tvcol = Gtk.TreeViewColumn(column_title, cell, markup=0)

      tree_selection = self.tree.get_selection()
      tree_selection.connect("changed", self.on_selection_changed)
      self.tree.append_column(self.tvcol)

      box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
      #box.pack_start(menubar, False, False, 1)
      box.pack_start(swintree, True, True, 1)
      box.pack_start(swinpath, False, False, 1)
      swintree.add(self.tree)
      swinpath.add(self.label_info)
      self.add(box)

      if self.data:
        walk_tree(self.data, self.model)

    def tree_selection_to_jq(self, tree_selection):
      (model, iter_current) = tree_selection.get_selected()
      jq = ''
      if iter_current:
        path = model.get_path(iter_current)
        jq = to_jq(path, self.data)
      return jq

    def copy_path_to_clipboard(self, menuitem):
      tree_selection = self.tree.get_selection()
      jq = self.tree_selection_to_jq(tree_selection)
      self.clipboard.set_text(jq, -1)

    def parse_json(self, data):
      self.data = json.loads(data)

    def open_callback(self, action):
        open_dialog = Gtk.FileChooserDialog("Select a JSON file", self,
                                            Gtk.FileChooserAction.OPEN,
                                            (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
                                             Gtk.STOCK_OPEN, Gtk.ResponseType.ACCEPT))
        open_dialog.set_local_only(False)
        open_dialog.set_modal(True)
        open_dialog.connect("response", self.open_response_cb)
        open_dialog.run()
        open_dialog.destroy()

    def open_response_cb(self, open_dialog, response_id):
      if response_id == Gtk.ResponseType.ACCEPT:
        try:
          file = open_dialog.get_file()
          [success, content, etags] = file.load_contents(None)
          if success:
            self.parse_json(content.decode("utf-8"))
            self.model.clear()
            walk_tree(self.data, self.model)
            self.label_info.set_text('')
            self.tvcol.set_title(open_dialog.get_filename())
          else:
            raise ValueError('Error while opening ' + open_dialog.get_filename())
        except Exception as e:
          self.label_info.set_text(str(e))

    def on_selection_changed(self, tree_selection) :
      jq = self.tree_selection_to_jq(tree_selection)
      self.label_info.set_text(jq)

    def on_treeview_button_press_event(self, treeview, event):
      if event.button == 3:
        x = int(event.x)
        y = int(event.y)
        time = event.time
        pthinfo = treeview.get_path_at_pos(x, y)

        if pthinfo is not None:
          path, col, cellx, celly = pthinfo
          treeview.grab_focus()
          treeview.set_cursor( path, col, 0)

          self.contextual_menu = Gtk.Menu()
          menuitem_copy_path = Gtk.MenuItem(label="Copy path to clipboard")
          menuitem_copy_path.connect("activate", self.copy_path_to_clipboard)
          menuitem_copy_path.show()
          self.contextual_menu.append(menuitem_copy_path)

          self.contextual_menu.popup(None, None, None, event.button, time, 0)
          return True
      return False

win = JSONViewerWindow()
win.connect("delete-event", Gtk.main_quit)
win.show_all()
Gtk.main()
