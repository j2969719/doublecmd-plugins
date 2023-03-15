#!/bin/bash
ldd  "$1" | grep "not found"
readelf -a "$1"
