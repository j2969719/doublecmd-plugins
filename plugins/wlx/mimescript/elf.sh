#!/bin/bash
file -b "$1"
ldd -r -v "$1"
nm -C -D "$1"
