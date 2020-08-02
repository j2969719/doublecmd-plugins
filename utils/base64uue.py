#!/usr/bin/python2
# -*- coding: utf-8 -*-
#
# Base64 & UUEncode: encode/decode
# 2020.08.02
#
# If you want to use Python 3:
#   just replace shebang on "#!/usr/bin/env python3" or "#!/usr/bin/python3"
#
# File extension:
#   b64: Base64
#   uue: UUEncode
# Params:
#   base64uue.py -l %AQ
#     get name of encoded file, date/time (for MultiArc)
#     Output format: yyyy tt dd hh mm ss aaaaa zzzzzzzzzzzz n+
#       For example: 2012-11-23 01:54:17 ....A          664 filename.ext
#   base64uue.py -eb %AQ %FQ
#     encode file in Base64 format (like Total Commander)
#   base64uue.py -eu %AQ %FQ
#     encode file in UUEncode format
#   base64uue.py -d %AQ %FQ
#     decode file (detect by extension)

import base64
import binascii
import os
import sys
import time
import uu

def get_name_and_size_b64(fl):
    j = 1
    f = open(fl, 'r')
    for line in f:
        line = line.strip()
        if j > 4:
            break
        if line[0:13] == 'Content-Type:':
            i = line.find(' name="')
            n1 = line[i + 7:-1]
        elif line[0:20] == 'Content-Disposition:':
            i = line.find(' filename="')
            n2 = line[i + 11:-1]
        j = j + 1
    f.close()
    if len(n1) >= 1:
        n = n1
    elif len(n2) >= 1:
        n = n2
    else:
        n = os.path.basename(fl)
        n = n[0:-4]
    l = []
    f = open(fl, 'r')
    for line in f:
        if line.find('MIME-') > -1 or line.find('Content-') > -1:
            continue
        else:
            l.append(line.strip())
    f.close()
    s = ''.join(l)
    d = base64.b64decode(s.strip())
    return n, len(d)

def pack_b64(fl, n):
    h = 'MIME-Version: 1.0\nContent-Type: application/octet-stream; name="' + n + '"\nContent-Transfer-Encoding: base64\nContent-Disposition: attachment; filename="' + n + '"'
    f = open(n, 'rb')
    d = f.read()
    f.close()
    e = base64.b64encode(d)
    l = []
    for i in range(0, len(e), 76):
        l.append(e[i:i + 76])
    o = '\n'.join(l)
    f = open(fl, 'w')
    f.write(h + '\n\n' + o + '\n')
    f.close()

def unpack_b64(fl, n):
    l = []
    f = open(fl, 'r')
    for line in f:
        if line.find('MIME-') > -1 or line.find('Content-') > -1:
            continue
        else:
            l.append(line.strip())
    f.close()
    s = ''.join(l)
    d = base64.b64decode(s.strip())
    f = open(n, 'wb')
    f.write(d)
    f.close()

def get_name_and_size_uue(fl):
    j = 0
    s = 0
    l = []
    f = open(fl, 'r')
    for line in f:
        if j == 0:
            n1 = line[10:].strip()
            j = 1
            continue
        if line[0:3] == 'end':
            j = 2
            continue
        if j != 2:
            l.append(binascii.a2b_uu(line))
            continue
        i = line.find('size')
        if i > 0:
            s = line[7:].strip()
            i = s.find('/')
            s = s[i + 1:]
    f.close()
    if len(n1) >= 1:
        n = n1
    else:
        n = os.path.basename(fl)
        n = n[0:-4]
    if s > 0:
        return n, s
    else:
        s = ''.join(l)
        return n, len(s)

def pack_uue(fl, n):
    uu.encode(n, fl)

def unpack_uue(fl, n):
    uu.decode(fl, n)

if len(sys.argv) == 3 or len(sys.argv) == 4:
    if len(sys.argv) == 4:
        inm = sys.argv[3]
    else:
        inm = ''
    if sys.argv[1] == '-l':
        # Получаем имя и размер закодированного файла
        if sys.argv[2][-4:] == '.b64':
            nnm, sz = get_name_and_size_b64(sys.argv[2])
        elif sys.argv[2][-4:] == '.uue':
            nnm, sz = get_name_and_size_uue(sys.argv[2])
        else:
            sys.exit(1)
        # Получаем дату
        tm = time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime(os.path.getmtime(sys.argv[2])))
        sz = str(sz)
        print(tm + ' ....A ' + sz.rjust(12) + ' ' + nnm)
    elif sys.argv[1] == '-eb':
        pack_b64(sys.argv[2], inm)
    elif sys.argv[1] == '-eu':
        pack_uue(sys.argv[2], inm)
    elif sys.argv[1] == '-d':
        if sys.argv[2][-4:] == '.b64':
            unpack_b64(sys.argv[2], inm)
        elif sys.argv[2][-4:] == '.uue':
            unpack_uue(sys.argv[2], inm)
        else:
            sys.exit(1)
    else:
            sys.exit(1)
else:
    sys.exit(1)
