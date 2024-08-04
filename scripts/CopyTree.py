#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# 2024.08.04
#
# For file list after cm_FlatView/cm_FlatViewSel and search result
#   (from the "///Search result" tab): copy selected files to target directory
#   with keeping source directory structure and without overwriting existing files.
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
    if ds == "/":
        dsl = 0
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
            tmp = shutil.copy(fn, tfn, follow_symlinks=False)
        except Exception:
            print("Failed to copy file: " + fn)
            continue
        if tmp == tfn:
            shutil.copystat(fn, tfn, follow_symlinks=False)
            print('Ok: ' + fn)
        else:
            print('Error: ' + fn)
        c += 1
    print("\nDone: " + str(c) + " file(s)\n")

if len(sys.argv) == 4:
    if sys.argv[2][0:3] == "///":
        copytree(sys.argv[1], "/", sys.argv[3])
    else:
        copytree(sys.argv[1], sys.argv[2], sys.argv[3])
else:
    print("Check parameters!")
    sys.exit(1)
