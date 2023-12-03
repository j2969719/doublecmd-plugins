#!/bin/sh


vfs_init()
{
    echo "Fs_DisableFakeDates"
    #echo "Fs_RunAsync mountavfs"
    mountavfs
    exit $?
}

vfs_setopt()
{
    option="$1"
    value="$2"

    prefix="$HOME/.avfs"
    path=${value#"$prefix"}

    case "$option" in
        "Custom AVFS path") echo "Fs_Set_DC_WFX_SCRIPT_AVFSPATH $path" ;;
        "Custom AVFS path (file)") echo "Fs_Set_DC_WFX_SCRIPT_AVFSPATH $path#" ;;
    esac
    exit 0
}

vfs_list()
{
    path="$1"

    echo "lr-xr-xr-x 0000-00-00 00:00:00 - <root>.-->"
    echo "lr-xr-xr-x 0000-00-00 00:00:00 - <home>.-->"
    echo "lr-xr-xr-x 0000-00-00 00:00:00 - <#avfsstat>.-->"
    echo "lr-xr-xr-x 0000-00-00 00:00:00 - <#volatile>.-->"
    echo "lr-xr-xr-x 0000-00-00 00:00:00 - <custom>.-->"
    echo "-r-xr-xr-x 0000-00-00 00:00:00 - >set custom<.run"
    echo "-r-xr-xr-x 0000-00-00 00:00:00 - >set custom (file)<.run"
    exit $?
}

vfs_openfile()
{
    file="$1"
    prefix=$HOME/.avfs

    if [ "$file" == "/<root>.-->" ]; then
        echo "$prefix/"
    elif [ "$file" == "/<home>.-->" ]; then
        echo "$prefix$HOME"
    elif [ "$file" == "/<#avfsstat>.-->" ]; then
        echo "$prefix/#avfsstat"
    elif [ "$file" == "/<#volatile>.-->" ]; then
        echo "$prefix/#volatile"
    elif [ "$file" == "/>set custom<.run" ]; then
        echo -e "Fs_PushValue Custom AVFS path\t/#avfsstat"
        echo -e "Fs_Request_Options\nCustom AVFS path"
    elif [ "$file" == "/>set custom (file)<.run" ]; then
        echo -e "Fs_SelectFile Custom AVFS path (file)\t*.*"
    elif [ "$file" == "/<custom>.-->" ]; then
        if [ -z "$DC_WFX_SCRIPT_AVFSPATH" ]; then
            echo "Fs_Info_Message Custom AVFS path is not set."
        else
            echo "$prefix$DC_WFX_SCRIPT_AVFSPATH"
        fi
    fi
    exit 0
}


vfs_deinit()
{
    echo "Fs_RunAsync umountavfs"
    #umountavfs
    exit $?
}

case "$1" in
    init) vfs_init;;
    setopt) vfs_setopt "$2" "$3";;
    list) vfs_list "$2";;
    properties) exit 0;;
    run) vfs_openfile "$2";;
    deinit) vfs_deinit;;
esac
exit 1