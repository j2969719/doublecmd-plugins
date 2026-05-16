#!/usr/bin/env python3

# MIME: message/rfc822

import os
import sys

os.environ["GDK_CORE_DEVICE_EVENTS"] = "1"

import email
from email import policy

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GLib

wid = int(sys.argv[1])
path = sys.argv[2]

with open(path, "rb") as f:
    msg = email.message_from_binary_file(f, policy=policy.default)

result = '<span size="large">'

# Header
for h in 'From', 'To', 'Date', 'Subject':
    if h in msg:
        result = result + '<b>' + h + '</b>: ' + GLib.markup_escape_text(msg[h]) + '\n'
    else:
        result = result + '<b>' + h + '</b>:\n'

result = result + '______________________________'

# Body
if msg.is_multipart():
    for part in msg.walk():
        if part.get_content_type() == "text/plain":
            result = result + GLib.markup_escape_text(part.get_body(preferencelist=("plain")).get_content())
            break
else:
    result = result + GLib.markup_escape_text(msg.get_body(preferencelist=("plain")).get_content())

# Attachment(s)
result = result + '\n______________________________\n<b>Attachment(s)</b>:'

for attachment in msg.iter_attachments():
    result = result + '\n' + attachment.get_filename()

result = result + '</span>'

plug = Gtk.Plug()
plug.construct(wid)
view = Gtk.ScrolledWindow()
plug.add(view)
hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
label = Gtk.Label()
label.set_margin_start(5)
label.set_margin_end(5)
label.set_margin_top(5)
label.set_margin_bottom(5)
label.set_markup(result)
label.set_line_wrap(True)
label.set_selectable(True)
label.set_halign(Gtk.Align.START)
label.set_valign(Gtk.Align.START)
vbox.pack_start(label, False, True, 0)
hbox.pack_start(vbox, True, True, 0)
view.add(hbox)

plug.connect("delete-event", Gtk.main_quit)
plug.show_all()
Gtk.main()
