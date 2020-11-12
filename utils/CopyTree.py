#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# 2020.09.01
#
# For file list after cm_FlatView/cm_FlatViewSel:
#   copy selected files to target directory with keeping source directory
#   structure and without overwriting existing files.
#
# How to use: make it executable and add button
#   command: path/to/CopyTree.py
#   parameters: %t1 %"0 "%LU" "%Ds" "%Dt"

import os
import shutil
import sys

def copytree(fl, ds, dt):
    f = open(fl, "r")
    af = f.read().splitlines()
    f.close()
    dsl = len(ds)
    c = 0
    for fn in af:
        if not os.path.isfile(fn):
            print("Not a file: " + fn)
            continue
        tfn = dt + fn[dsl:]
        if os.path.isfile(tfn):
            print("Already exists: " + tfn)
            continue
        td = os.path.dirname(tfn)
        if not os.path.isdir(td):
            try:
                os.makedirs(td)
            except Exception:
                print("Target directory can not be created: " + td)
                continue
        try:
            tmp = shutil.copy(fn, tfn)
        except Exception:
            print("Failed to copy file: " + fn)
            continue
        if tmp == tfn:
            shutil.copystat(fn, tfn)
            print('Ok: ' + fn)
        else:
            print('Error: ' + fn)
        c += 1
    print("\nDone: " + str(c) + " file(s)\n")

if len(sys.argv) == 4:
    copytree(sys.argv[1], sys.argv[2], sys.argv[3])
else:
    print("Check parameters!")
    sys.exit(1)

