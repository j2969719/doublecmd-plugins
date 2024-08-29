#!/bin/sh

init_fail()
{
    message="$1"

    echo "Fs_Info_Message $message"
    echo "Fs_Set_DC_WFX_SCRIPT_INITFAIL TRUE"
    exit 1
}

launch_mpv()
{
    sock=`mktemp -u "/tmp/mpvpnlsckt.XXXXX"`

    echo "Fs_Set_MPV_SERVER_SOCK $sock"
    echo "Fs_RunAsync mpv --idle=yes --window-minimized=yes --input-ipc-server=$sock"
}

vfs_init()
{
    which mpv >/dev/null 2>&1 && launch_mpv || init_fail WFX_SCRIPT_STR_INSTALL_MPV
    which socat >/dev/null 2>&1 || init_fail WFX_SCRIPT_STR_INSTALL_SOCAT
    echo "Fs_GetValues_Needed"
    exit $?
}

vfs_list()
{
    [ -z "$DC_WFX_SCRIPT_INITFAIL" ] || exit 1

    path="$1"

    echo '{"command": ["get_property", "playlist"]}' | socat - $MPV_SERVER_SOCK | jq -r '.data[].filename' | sed 's#.*/##' | nl -v0 -w1 -s'|' | sed 's/^/lr-xr-xr-x  0000-00-00 00:00:00 - /'

    exit $?
}

vfs_copyin()
{
    src="$1"
    dst="/${2#/*/}"

    echo '{"command": ["loadfile", "'$src'", "append"]}' | socat - $MPV_SERVER_SOCK

    exit $?
}

vfs_rm()
{
    file="$DC_WFX_SCRIPT_DIR""$1"
    id=`echo ${1:1} | sed 's/|.*//'`

    echo '{"command": ["playlist-remove", '$id']}' | socat - $MPV_SERVER_SOCK > /dev/null

    exit $?
}


vfs_openfile()
{
    file="$1"
    id=`echo ${1:1} | sed 's/|.*//'`

    echo '{"command": ["set_property", "playlist-pos", '$id']}' | socat - $MPV_SERVER_SOCK > /dev/null

    exit 0
}

vfs_properties()
{
    file="$1"
    id=`echo ${1:1} | sed 's/|.*//'`

    echo '{"command": ["expand-text", "${playlist/'$id'}"]}' | socat - /tmp/mympvskt | jq -r '.data' | sed 's/^filename=/path=/'| sed 's/=/\t/'
    exit 0
}


vfs_localname()
{
    file="$1"
    id=`echo ${1:1} | sed 's/|.*//'`

    echo '{"command": ["get_property", "playlist"]}' | socat - $MPV_SERVER_SOCK | jq -r '.data['$id'].filename'

    exit $?
}

vfs_getinfovalues()
{
    path="$1"

    echo '{"command": ["get_property", "playlist"]}' | socat - $MPV_SERVER_SOCK | jq -r '.data[] | "\(.filename) \(.playing)"' | nl -v0 -w1 -s'|' | grep -E ' true$'| sed 's#\([0-9]\+|\)/.\+/\([^/]\+\) true$#\1\2\t\*#'

    exit $?
}

vfs_deinit()
{
    echo '{"command": ["quit"]}' | socat - $MPV_SERVER_SOCK
    rm -f "$MPV_SERVER_SOCK"

    exit $?
}

case "$1" in
    init) vfs_init;;
    list) vfs_list "$2";;
    copyin) vfs_copyin "$2" "$3";;
    rm) vfs_rm "$2";;
    run) vfs_openfile "$2";;
    properties) vfs_properties "$2";;
    localname) vfs_localname "$2";;
    getvalues) vfs_getinfovalues "$2";;
    deinit) vfs_deinit;;
esac
exit 1