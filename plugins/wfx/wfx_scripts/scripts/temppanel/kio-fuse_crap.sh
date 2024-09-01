#!/bin/sh

mountpoint="/KIO"

init_fail()
{
    message="$1"

    echo "Fs_Info_Message $message"
    echo "Fs_Set_DC_WFX_SCRIPT_INITFAIL TRUE"
    exit 1
}

find_exec()
{
    dirs=( "/bin" "/usr/bin" "/usr/lib" "/usr/share/lib" )

    for dir in "${dirs[@]}" ; do
        if test -x "$dir/kio-fuse" ; then
            path="$dir/kio-fuse"
        fi
    done
    if [ -z "$path" ] ; then
        init_fail WFX_SCRIPT_STR_ERREXEC
    else
        echo "Fs_Set_DC_WFX_SCRIPT_EXEC $path"
        echo "Fs_RequestOnce"
        echo "Fs_YesNo_Message WFX_SCRIPT_STR_RUNTERM"
    fi
}

run_kiofuse()
{
    value="$1"
    tmpdir=`mktemp -d /tmp/wfx-kio-fuse.XXXXX`

    echo "Fs_Set_DC_WFX_SCRIPT_TMPDIR $tmpdir"
    mkdir "$tmpdir$mountpoint"

    [ "$value" == "Yes" ] && echo "Fs_RunTerm $DC_WFX_SCRIPT_EXEC -d $tmpdir$mountpoint" ||\
                             echo "Fs_RunAsync $DC_WFX_SCRIPT_EXEC -d $tmpdir$mountpoint"
    sleep 2
    echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_URI"
}

send_dbusmsg()
{
    value="$1"

    localpath=`dbus-send --session --print-reply --type=method_call\
              --dest=org.kde.KIOFuse /org/kde/KIOFuse org.kde.KIOFuse.VFS.mountUrl \
              string:"$value" |  grep -Po 'string "\\K[^"]*'`

    [ -z "$localpath" ] && init_fail WFX_SCRIPT_STR_ERRREPLY || echo "Fs_Redirect $localpath"
}

cleanup()
{
     fusermount3 -u "$DC_WFX_SCRIPT_TMPDIR$mountpoint" &&\
     rm -r "$DC_WFX_SCRIPT_TMPDIR"
}

vfs_init()
{
    echo "Fs_Redirect infinity and beyond"
    echo "Fs_RequestOnce"
    echo "Fs_YesNo_Message WFX_SCRIPT_STR_MANUAL"
    exit $?
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        WFX_SCRIPT_STR_URI) send_dbusmsg "$value" ;;
        WFX_SCRIPT_STR_MANUAL) [ "$value" == "Yes" ] && find_exec ||\
                                    echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_URI" ;;
        WFX_SCRIPT_STR_RUNTERM) run_kiofuse "$value" ;;
    esac
    exit 0
}

vfs_list()
{
    [ -z "$DC_WFX_SCRIPT_INITFAIL" ] || exit 1
    exit 0
}

vfs_deinit()
{
    [ -z "$DC_WFX_SCRIPT_TMPDIR" ] || cleanup
    exit $?
}

case "$1" in
    init) vfs_init;;
    setopt) vfs_setopt "$2" "$3";;
    list) vfs_list "$2";;
    deinit) vfs_deinit;;
esac
exit 1