#!/usr/bin/env python3

import os
import sys
os.environ["GDK_CORE_DEVICE_EVENTS"] = "1"

import zipfile

import xml.etree.ElementTree as ET

import gi
gi.require_version('Gtk', '3.0')
try:
    gi.require_version('WebKit2', '4.0')
except ValueError:
    gi.require_version('WebKit2', '4.1')
from gi.repository import Gtk, GLib
from gi.repository import WebKit2

data = ""

html = '<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN"><html><head><title>View description</title><meta http-equiv="Content-Type" content="text/html; charset='

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
    n3 = 1
    while n3 > 0:
        n1 = s.find("<author>", n3)
        if n1 == -1:
            break
        else:
            n2 = s.find("</author>", n1)
            tmp = s[n1 + 8:n2]
            fn = GetTagValue(tmp, "last-name")
            for t in "first-name", "middle-name":
                t2 = GetTagValue(tmp, t)
                if len(t2) > 0:
                    fn = fn + " " + t2
            l.append(fn)
        n3 = n2
    if len(l) == 1:
        return l[0]
    else:
        return ', '.join(l)

def GetGenres(s):
    l = []
    c = 0
    n3 = 1
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
    r = "<p>Sequence: " + s[n1 + 6:n2]
    n1 = s.find('number="')
    if n1 != -1:
        n2 = s.find('"', n1 + 8)
        r = r + " #" + s[n1 + 8:n2] + "</p>"
    else:
        r = r + "</p>"
    return r

def GetPub(s):
    l = []
    r = ""
    c = 0
    for t in "publisher", "city", "year":
        d = GetTagValue(s, t)
        if len(d) > 0:
            l.append(d)
    if len(l) > 0:
        r = "Publish: " + ', '.join(l)
        c = 1
    id = GetTagValue(s, "isbn")
    if len(id) > 0:
        if c == 1:
            r = r + ". ISBN: " + id + "."
        else:
            r = "ISBN: " + id + "."
    return "<p>" + r + "</p>"


wid = int(sys.argv[1])
path = sys.argv[2]

plug = Gtk.Plug()
plug.construct(wid)
view = WebKit2.WebView()
plug.add(view)

en = ""
l = len(path)
tmp = path[l - 4:l]
e = tmp.lower()
if e == ".fb2" or e == ".fbd":
    hfile = open(path, "rb")
    tmp = hfile.read(96)
    hfile.close()
    en = CheckFile(tmp)
    if en == "none":
        sys.exit(1)
    else:
        if en[1:8] == "windows-":
            tmp = en.replace("windows-", "cp")
            hfile = open(path, "r", encoding=tmp)
        else:
            hfile = open(path, "r", encoding=en)
        data = hfile.read()
        hfile.close()
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
        if e == ".fb2" or e == "fbd":
            hfile = z.open(finfo.filename, "r")
            tmp = hfile.read(96)
            hfile.close()
            en = CheckFile(tmp)
            if en == "none":
                z.close()
                sys.exit(1)
            else:
                hfile = z.open(finfo.filename, "r")
                if en[1:8] == "windows-":
                    tmp = en.replace("windows-", "cp")
                    data = hfile.read().decode(encoding=tmp)
                else:
                    data = hfile.read().decode(encoding=en)
                hfile.close()
            break
    z.close()

html = html + en + '"></head><body><div>'

enddesc = data.find("</description>", 0)

tb = data.find("<title-info>", 0, enddesc)
te = data.find("</title-info>", tb)

n1 = data.find("<book-title>", tb, te)
n2 = data.find("</book-title>", n1)
html = html + "<h2>" + data[n1 + 12:n2] + "</h2>"

tmp = GetAutors(data[tb + 12:te])
html = html + "<h3>" + tmp + "</h3>"

tmp = GetGenres(data[tb + 12:te])
html = html + "<p>Genres: " + tmp + "</p>"

n1 = data.find("<sequence", tb, te)
if n1 != -1:
    n2 = data.find(">", n1, te)
    html = html + GetSeq(data[n1:n2])

n1 = data.find("<publish-info>", 0, enddesc)
if n1 != -1:
    n2 = data.find("</publish-info>", n1)
    html = html + GetPub(data[n1:n2])

n1 = data.find("<annotation>", tb, te)
if n1 != -1:
    n2 = data.find("</annotation>", n1)
    html = html + data[n1 + 12:n2]

# Validation
try:
    root = ET.fromstring(data)
except ET.ParseError:
    html = html + '<p><font color="#c00000"><u>This file is not a valid XML file!</u></font></p>'

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
                html = html + '<div><img alt="Cover" src="data:' + ctype + ';base64,' + data[ne + 1:n2] + '"/>'
                break

html = html + "</div></body></html>"

view.load_html(html)

plug.connect("delete-event", Gtk.main_quit)
plug.show_all()
Gtk.main()
