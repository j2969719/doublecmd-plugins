#!/bin/sh

readme="`basename $0`_readme.txt"

initdlg()
{
    actions=(HELP INPUT FILE DIR TEMP)
    string=`printf '\tWFX_SCRIPT_STR_%s' "${actions[@]}"`
    echo -e "Fs_MultiChoice WFX_SCRIPT_STR_INIT$string"
    exit 0
}

multichoice()
{
    option="$1"
    prefix="$HOME/.avfs"

    case "$option" in
        WFX_SCRIPT_STR_TEMP) echo "Fs_Redirect $prefix/#volatile" ;;
        WFX_SCRIPT_STR_INPUT)  echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_CUSTOM" ;;
        WFX_SCRIPT_STR_FILE) echo -e "Fs_SelectFile WFX_SCRIPT_STR_FILE\t*.*" ;;
        WFX_SCRIPT_STR_DIR) echo "Fs_SelectDir WFX_SCRIPT_STR_DIR" ;;
        WFX_SCRIPT_STR_HELP) echo "Fs_ShowOutput cat $PWD/$readme" ;;
    esac
}

vfs_init()
{
    echo "Fs_Redirect nope"
    mountavfs
    echo -e "Fs_PushValue WFX_SCRIPT_STR_CUSTOM\t/#avfsstat"
    echo -e "Fs_PushValue WFX_SCRIPT_STR_META\t#patchfs"
    initdlg
    exit $?
}

vfs_setopt()
{
    option="$1"
    value="$2"

    prefix="$HOME/.avfs"

    case "$option" in
        WFX_SCRIPT_STR_INIT) multichoice "$value" ;;
        WFX_SCRIPT_STR_FILE) echo "Fs_Set_WFX_SCRIPT_FILE $prefix$value" &&\
                             echo "Fs_YesNo_Message WFX_SCRIPT_STR_SETMETA" ;;
        WFX_SCRIPT_STR_SETMETA) [ "$value" == "Yes" ] && echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_META" ||\
                                    echo "Fs_Redirect $WFX_SCRIPT_FILE#" ;;
        WFX_SCRIPT_STR_META) echo "Fs_Redirect $WFX_SCRIPT_FILE$value" ;;
        *) echo "Fs_Redirect $prefix$value" ;;
    esac
    exit 0
}

vfs_list()
{
    exit 0
}

vfs_deinit()
{
    echo "Fs_RunAsync umountavfs"
    exit $?
}

case "$1" in
    init) vfs_init;;
    setopt) vfs_setopt "$2" "$3";;
    list) vfs_list "$2";;
    deinit) vfs_deinit;;
esac
exit 1