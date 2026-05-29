#!/usr/bin/env python3

# Some parts of code are written with the help of free Google AI.

import os
import sys
import vobject
#import re   # I know about PEP 8, but see below.
os.environ["GDK_CORE_DEVICE_EVENTS"] = "1"

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GLib

wid = int(sys.argv[1])
path = sys.argv[2]

def fix_vcard_21(raw_text):
    import re
    fixed_lines = []
    for line in raw_text.splitlines():
        if line.startswith('ADR;'):
            for t in 'PREF', 'DOM', 'INTL', 'POSTAL', 'PARCEL', 'HOME', 'WORK':
                line = re.sub(rf';{t}(?=;|:)', f';TYPE={t}', line)
        if line.startswith('TEL;'):
            for t in 'PREF', 'WORK', 'HOME', 'VOICE', 'FAX', 'MSG', 'CELL', 'PAGER', 'BBS', 'MODEM', 'CAR', 'ISDN', 'VIDEO':
                line = re.sub(rf';{t}(?=;|:)', f';TYPE={t}', line)
        if line.startswith('EMAIL;'):
            for t in 'PREF', 'WORK', 'HOME', 'INTERNET':
                line = re.sub(rf';{t}(?=;|:)', f';TYPE={t}', line)
        fixed_lines.append(line)
    return '\n'.join(fixed_lines)

result = '<span size="large">'
count = 1

with open(path, 'r', encoding='utf-8') as f:
    raw_content = f.read()

n = raw_content.find("VERSION:2.1", 0, 39)
if n == -1:
    contacts = vobject.readComponents(raw_content, allowQP = True)
else:
    contacts = vobject.readComponents(fix_vcard_21(raw_content), allowQP = True)

for contact in contacts:
    result = f"{result}Contact: {count}\n"
    if 'fn' in contact.contents:
        result = result + '<b>Name</b>: ' + GLib.markup_escape_text(contact.fn.value) + '\n'
    else:
        result = result + '<b>Name</b>: ' + GLib.markup_escape_text(contact.n.value) + '\n'

    if 'nickname' in contact.contents:
        result = result + '<b>Nickname</b>: ' + GLib.markup_escape_text(contact.nickname.value) + '\n'

    if 'tel' in contact.contents:
        for tel in contact.contents['tel']:
            vtype = tel.params.get('TYPE', [])
            if len(vtype) == 0:
                result = result + '<b>TEL</b>: ' + tel.value + '\n'
            else:
                result = result + '<b>TEL</b>: ' + ', '.join(vtype) + ': ' + tel.value + '\n'

    if 'email' in contact.contents:
        for email in contact.contents['email']:
            vtype = email.params.get('TYPE', [])
            if len(vtype) == 0:
                result = result + '<b>EMAIL</b>: ' + email.value + '\n'
            else:
                result = result + '<b>EMAIL</b>: ' + ', '.join(vtype) + ': ' + email.value + '\n'

    if 'adr' in contact.contents:
        for address in contact.adr_list:
            components = []
            for attr in ['box', 'extended', 'street', 'city', 'region', 'code', 'country']:
                val = getattr(address.value, attr, '')
                if isinstance(val, list):
                    val = " ".join([str(i).strip() for i in val if i])
                components.append(str(val).strip())

            full_address = ", ".join([c for c in components if c])
            addr_params = getattr(address, 'params', {})
            addr_type_list = addr_params.get('TYPE', ['Not specified'])
            if isinstance(addr_type_list, list):
                addr_type = "-".join([str(t).upper() for t in addr_type_list])
            else:
                addr_type = str(addr_type_list).upper()
            if len(addr_type) == 0:
                result = result + '<b>ADR</b>: ' + full_address + '\n'
            else:
                result = result + '<b>ADR</b>: ' + addr_type + ': ' + full_address + '\n'

    if 'url' in contact.contents:
        result = result + '<b>URL</b>: ' + GLib.markup_escape_text(contact.url.value) + '\n'
    if 'org' in contact.contents:
        result = result + '<b>ORG</b>: ' + GLib.markup_escape_text(', '.join(contact.org.value)) + '\n'
    if 'title' in contact.contents:
        result = result + '<b>TITLE</b>: ' + GLib.markup_escape_text(contact.title.value) + '\n'
    if 'role' in contact.contents:
        result = result + '<b>ROLE</b>: ' + GLib.markup_escape_text(contact.role.value) + '\n'
    if 'bday' in contact.contents:
        result = result + '<b>BDAY</b>: ' + GLib.markup_escape_text(contact.bday.value) + '\n'
    if 'photo' in contact.contents:
        result = result + '<b>PHOTO</b>: yes\n')
    else:
        result = result + '<b>PHOTO</b>: no\n')
    if 'categories' in contact.contents:
        result = result + '<b>CATEGORIES: ' + GLib.markup_escape_text(contact.categories.value) + '\n'
    if 'note' in contact.contents:
        result = result + '<b>NOTE</b>: ' + GLib.markup_escape_text(contact.note.value) + '\n'

    result = result + '______________________________\n'
    count += 1

result = result + '</span>'

plug = Gtk.Plug()
plug.construct(wid)
view = Gtk.ScrolledWindow()
plug.add(view)
hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
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
hbox.pack_start(label, False, True, 0)
view.add(hbox)

plug.connect("delete-event", Gtk.main_quit)
plug.show_all()
Gtk.main()
