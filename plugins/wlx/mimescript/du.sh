#!/bin/bash
cd "$1" && du -h --apparent-size | sort -hr