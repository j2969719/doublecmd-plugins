#!/usr/bin/env python3

import os
import sys
import base64
import zipfile
os.environ["GDK_CORE_DEVICE_EVENTS"] = "1"

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GLib, GdkPixbuf

import xml.etree.ElementTree as ET

#import re   # I know about PEP 8, but see below.

wid = int(sys.argv[1])
path = sys.argv[2]

def CheckFile(d):
    b = 0
    if d[:3] == b"\xef\xbb\xbf":
        d = d[3:]
        b = 1
    s = d.decode("ascii")
    n1 = s.find("<?xml", 0)
    if n1 == -1:
        return "none"
    else:
        if b == 1:
            return "utf-8-sig"
    n2 = s.find("encoding=", n1)
    if n2 == -1:
        return "none"
    if s[n2 + 9:n2 + 10] == "'":
        n1 = s.find("'", n2 + 11)
    elif s[n2 + 9:n2 + 10] == '"':
        n1 = s.find('"', n2 + 11)
    t = s[n2 + 10:n1]
    return t.lower()

def GetType(s):
    n1 = s.find("content-type=")
    n2 = s.find('"', n1 + 14)
    return s[n1 + 14:n2]

def GetTagValue(s, t):
    n1 = s.find("<" + t + ">")
    if n1 != -1:
        n2 = s.find("</" + t + ">", n1)
        return s[n1 + len(t) + 2:n2]
    else:
        return ""

def GetAutors(s):
    l = []
    n3 = 0
    while n3 > -1:
        n1 = s.find("<author>", n3)
        if n1 == -1:
            break
        else:
            n2 = s.find("</author>", n1)
            tmp = s[n1:n2]
            fn = GetTagValue(tmp, "last-name")
            for t in "first-name", "middle-name":
                t2 = GetTagValue(tmp, t)
                if len(t2) > 0:
                    fn = fn + " " + t2
            if fn == "":
                fn = GetTagValue(tmp, "nickname")
            l.append(fn)
        n3 = n2
    if len(l) == 1:
        return l[0]
    else:
        return ', '.join(l)

def GetTranslator(s):
    l = []
    n3 = 0
    while n3 > -1:
        n1 = s.find("<translator>", n3)
        if n1 == -1:
            break
        else:
            n2 = s.find("</translator>", n1)
            tmp = s[n1:n2]
            fn = GetTagValue(tmp, "last-name")
            for t in "first-name", "middle-name":
                t2 = GetTagValue(tmp, t)
                if len(t2) > 0:
                    fn = fn + " " + t2
            if fn == "":
                fn = GetTagValue(tmp, "nickname")
            l.append(fn)
        n3 = n2
    if len(l) == 1:
        return l[0]
    else:
        return ', '.join(l)

def GetGenres(s):
    l = []
    c = 0
    n3 = 0
    while c > -1:
        n1 = s.find("<genre>", n3)
        if n1 == -1:
            break
        else:
            n2 = s.find("</genre>", n1)
            l.append(s[n1 + 7:n2])
            c = c + 1
        n3 = n2
    if c == 0:
        return "no"
    elif c == 1:
        return l[0]
    else:
        return ', '.join(l)

def GetSeq(s):
    n1 = s.find('name="')
    n2 = s.find('"', n1 + 6)
    r = "Sequence: " + s[n1 + 6:n2]
    n1 = s.find('number="')
    if n1 != -1:
        n2 = s.find('"', n1 + 8)
        r = r + " #" + s[n1 + 8:n2]
    else:
        r = r
    return r

def GetPub(s):
    l = []
    r = ""
    c = 0
    for t in "year", "publisher", "city":
        d = GetTagValue(s, t)
        if len(d) > 0:
            l.append(d)
    if len(l) > 0:
        r = "Publish: " + ', '.join(l)
        c = 1
    id = GetTagValue(s, "isbn")
    if len(id) > 0:
        if c == 1:
            r = r + ".\nISBN: " + id
        else:
            r = "ISBN: " + id
    return r

def GetAnnotation(s):
    l = []
    c = 0
    n3 = 0
    while c > -1:
        n1 = s.find("<p>", n3)
        if n1 == -1:
            break
        else:
            n2 = s.find("</p>", n1)
            t = s[n1 + 3:n2]
            n4 = t.find("<image", 0)
            if n4 == -1:
                l.append(t.replace("xlink:href=", "href=").replace("l:href=", "href=").replace("<br/>", "\n").replace("<br />", "\n").replace("<empty-line/>", "\n").replace("emphasis>", "i>").replace("<sub>", "").replace("<sup>", "").replace("</sup>", "").replace("</sub>", "").replace("strong>", "b>").replace("subtitle>", "b>"))
                c = c + 1
        n3 = n2
    if c == 0:
        return "no"
    elif c == 1:
        r = l[0]
    else:
        r = '\n'.join(l)
    n1 = r.find("<style")
    if n1 == -1:
        return r
    else:
        import re
        clean = re.compile("</?style[^>]*?>")
        return re.sub(clean, "", r)

en = ""
r = False
l = len(path)
tmp = path[l - 4:l]
e = tmp.lower()
if e == ".fb2" or e == ".fbd":
    hfile = open(path, "rb")
    tmp = hfile.read(96)
    hfile.close()
    en = CheckFile(tmp)
    if en != "none":
        if en[1:8] == "windows-":
            tmp = en.replace("windows-", "cp")
            hfile = open(path, "r", encoding=tmp)
        else:
            hfile = open(path, "r", encoding=en)
        data = hfile.read()
        hfile.close()
        r = True
else:
    if e == ".zip":
        tmp = path[l - 8:l - 4]
        if tmp.lower() != ".fb2":
            sys.exit(1)
    elif e !=  ".fbz":
        sys.exit(1)
    z = zipfile.ZipFile(path, "r")
    for finfo in z.infolist():
        l = len(finfo.filename)
        tmp = finfo.filename[l - 4:l]
        e = tmp.lower()
        if e == ".fb2" or e == ".fbd":
            hfile = z.open(finfo.filename, "r")
            tmp = hfile.read(96)
            hfile.close()
            en = CheckFile(tmp)
            if en != "none":
                hfile = z.open(finfo.filename, "r")
                if en[1:8] == "windows-":
                    tmp = en.replace("windows-", "cp")
                    data = hfile.read().decode(encoding=tmp)
                else:
                    data = hfile.read().decode(encoding=en)
                hfile.close()
                r = True
            break
    z.close()

if r == False:
    sys.exit(1)

enddesc = data.find("</description>", 0)
if enddesc == -1:
    sys.exit(1)

tb = data.find("<title-info>", 0, enddesc)
te = data.find("</title-info>", tb)

n1 = data.find("<book-title>", tb, te)
n2 = data.find("</book-title>", n1)
result = '<span size="x-large"><b>' + data[n1 + 12:n2] + '</b></span>\n'

tmp = GetAutors(data[tb:te])
result = result + '\n<span size="large"><b>' + tmp + "</b>\n"

tmp = GetTagValue(data[tb:te], "date")
if tmp != "":
    result = result + "\nDate: " + tmp

n1 = data.find("<translator", tb, te)
if n1 != -1:
    tmp = GetTranslator(data[n1 - 2:te])
    result = result + "\nTranslator(s): " + tmp

tmp = GetGenres(data[tb:te])
result = result + "\nGenres: " + tmp

n1 = data.find("<sequence", tb, te)
if n1 != -1:
    n2 = data.find(">", n1, te)
    result = result + "\n" + GetSeq(data[n1:n2])

n1 = data.find("<publish-info>", 0, enddesc)
if n1 != -1:
    n2 = data.find("</publish-info>", n1)
    result = result + "\n" + GetPub(data[n1:n2])

n1 = data.find("<annotation>", tb, te)
if n1 != -1:
    n2 = data.find("</annotation>", n1)
    result = result + "\n\n" + GetAnnotation(data[n1 + 12:n2])
result = result + "</span>"

# Validation
try:
    root = ET.fromstring(data)
except ET.ParseError:
    result = result + '\n\n<span foreground="darkred"><b><u>This file is not a valid XML file!</u></b></span>'

bp = False
n1 = data.find("<coverpage>", tb, te)
if n1 != -1:
    n2 = data.find(':href="#', n1)
    n1 = data.find('"', n2 + 8)
    cname = data[n2 + 8:n1]
    n1 = enddesc
    while enddesc > 0:
        nb = data.find("<binary", n1)
        if nb == -1:
            break
        else:
            ne = data.find(">", nb)
            nt = data.find('id="' + cname + '"', nb, ne)
            if nt == -1:
                n1 = ne
                continue
            else:
                ctype = GetType(data[nb:ne])
                n2 = data.find("</binary>", nb)
                base64_data = data[ne + 1:n2]
                if len(base64_data) > 0:
                    bp = True
                break

plug = Gtk.Plug()
plug.construct(wid)
view = Gtk.ScrolledWindow()
plug.add(view)
hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=5)
label = Gtk.Label()
label.set_margin_start(5)
#label.set_margin_end(5)
label.set_margin_top(5)
#label.set_margin_bottom(5)
label.set_markup(result)
label.set_line_wrap(True)
label.set_selectable(True)
label.set_halign(Gtk.Align.START)
label.set_valign(Gtk.Align.START)
if bp == True:
    image = Gtk.Image()
    image.set_halign(Gtk.Align.START)
    image.set_valign(Gtk.Align.START)
    loader = GdkPixbuf.PixbufLoader()
    loader.write(base64.b64decode(base64_data))
    loader.close()
    pixbuf = loader.get_pixbuf()
    width = pixbuf.get_width()
    height = pixbuf.get_height()
    #ratio = min(640 / width, 480 / height)
    ratio = min(400 / width, 300 / height)
    #image.set_from_pixbuf(pixbuf)
    image.set_from_pixbuf(pixbuf.scale_simple(width * ratio, height * ratio, GdkPixbuf.InterpType.BILINEAR))
hbox.pack_start(label, False, True, 0)
if bp == True:
    hbox.pack_start(image, False, False, 0)
view.add(hbox)

plug.connect("delete-event", Gtk.main_quit)
plug.show_all()
Gtk.main()
