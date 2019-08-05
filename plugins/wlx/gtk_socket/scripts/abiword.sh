#!/bin/bash

export GDK_CORE_DEVICE_EVENTS=1
${0%.sh}/kostyl -w $1 -f "$2"