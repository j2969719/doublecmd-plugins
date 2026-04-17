#!/bin/bash

which socat >/dev/null || exit 1

socket_dir=$(mktemp -d -t mpv-sockets-XXXXXX)
trap 'rm -rf "$socket_dir"' EXIT
echo "READY *silent"

if [[ -z "$SCRIPT_DEBUG" ]]; then
    exec >/dev/null 2>&1
else
    exec 1>&2
fi


while IFS=$'\t' read -r command xid param flags; do
    socket="$socket_dir/mpv-$xid.sock"
    echo "$command ($xid) $param"

    case "$command" in
        "?CREATE")
            #mpv --wid="$xid" --input-ipc-server="$socket" --idle --force-window=yes  --script-opts=osc-visibility=always >&2 &
            MPV-Media-Pla1yer_0.35.1-4_GLIBC-2.36-9_x86-64.AppImage --wid="$xid" --input-ipc-server="$socket" --idle --force-window=yes  --script-opts=osc-visibility=always &
            mpv_pid=$!
            while [ ! -S "$socket" ]; do
                sleep 0.1
            done
            responce=$(echo '{"command": ["get_property", "idle-active"]}' | socat - "$socket" 2>/dev/null)
            while [ -S "$socket" ] && [[ "$responce" != *"\"error\":"* ]]; do
                responce=$(echo '{"command": ["get_property", "idle-active"]}' | socat - "$socket" 2>/dev/null)
                sleep 0.1
            done
            echo "$responce"
            if [[ "$responce" != *"success"* ]]; then
                kill "$mpv_pid" 2>/dev/null
                rm -f "$socket"
                [ -z "$SCRIPT_DEBUG" ] || echo "$xid : mpv with PID $mpv_pid killed, socket removed"
            fi
            ;;

        "?LOAD")
            if [ -S "$socket" ]; then
                echo "{\"command\": [\"loadfile\", \"$param\", \"replace\"]}" | socat - "$socket"
            fi
            ;;

        "?DESTROY")
            if [ -S "$socket" ]; then
                echo '{"command": ["quit"]}' | socat - "$socket"
                rm -f "$socket"
            fi
            ;;
        *)
            [ -z "$SCRIPT_DEBUG" ] || echo "nothanks"
            ;;
    esac
done

#rm -rf "$socket_dir"