#!/bin/sh

vfs_init()
{
    echo -e "Fs_PushValue Path\t$HOME"
    echo -e "Fs_SelectDir Path"

    echo -e "Fs_Request_Options"
    echo -e "Fs_PushValue Mask\t*.jpg"
    echo -e "Mask"
    echo -e 'Fs_PushValue Commmand\tconvert "$src" "$dst.png"'

    echo "Fs_StatusInfo_Needed"
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        "Path") echo -e "Fs_Set_DC_WFX_SCRIPT_DIR $value" ;;
        "Mask") echo -e "Fs_Set_DC_WFX_SCRIPT_MASK $value" ;;
        "Set path") echo -e "Fs_SelectDir Path" ;;
        "Set mask") echo -e "Fs_Request_Options\nMask" ;;
        "Set command") echo -e "Fs_Request_Options\nCommmand" ;;
        "Commmand") echo -e "Fs_Set_DC_WFX_SCRIPT_CMD $value" ;;
    esac
    exit 0
}

vfs_list()
{
    path="$DC_WFX_SCRIPT_DIR""$1"

    if [ -z "$DC_WFX_SCRIPT_MASK" ]; then
        find "$path" -mindepth 1 -maxdepth 1 -name "*" -printf "%M %TF %TH:%TM:%TC %s %f\n"
    else
        find "$path" -mindepth 1 -maxdepth 1 -type d -name "*" -printf "%M %TF %TH:%TM:%TC %s %f\n"
        find "$path" -mindepth 1 -maxdepth 1 -not -type d -name "$DC_WFX_SCRIPT_MASK" $DC_WFX_SCRIPT_OPT -printf "%M %TF %TH:%TM:%TC %s %f\n"
    fi
    exit $?
}

vfs_copyout()
{
    src="$DC_WFX_SCRIPT_DIR""$1"
    dst="$2"

    eval "$DC_WFX_SCRIPT_CMD"
    exit $?
}

vfs_properties()
{
    file="$DC_WFX_SCRIPT_DIR""$1"
    echo -e "info\t"`file -b "$file"`
    frmtbasic="type\t%F\nsize\t%'s\nmode\t%A (%a)\n"
    frmtdates="mtime\t%y\nbtime\t%w\natime\t%x\nctime\t%z\n"
    frmtextra="owner\t%U (%u)\ngroup\t%G (%g)\ninode\t%i\nnlinks\t%h\n"
    stat --printf="$frmtbasic$frmtdates$frmtextra" "$file"
    echo -e "content_type\t"`file -b --mime-type "$file"`
    echo -e "Fs_PropsActs Set path\tSet mask\tSet command"
    exit 0
}


vfs_quote()
{
    string="$1"
    path="$DC_WFX_SCRIPT_DIR""$2"

    echo 'Fs_RunTermKeep cd "'"$path"'" && '"$string"
    exit $?
}

vfs_localname()
{
    file="$DC_WFX_SCRIPT_DIR""$1"

    echo "$file"
    exit $?
}

vfs_statusinfo()
{
    op="$1"
    path="$DC_WFX_SCRIPT_DIR""$2"

    if [ "$1" == "get_multi_thread start" ]; then
        echo "Fs_Request_Options"
        echo "Commmand"
    fi
    exit $?
}

case "$1" in
    init) vfs_init;;
    setopt) vfs_setopt "$2" "$3";;
    list) vfs_list "$2";;
    copyout) vfs_copyout "$2" "$3";;
    properties) vfs_properties "$2";;
    quote) vfs_quote "$2" "$3";;
    localname) vfs_localname "$2";;
    statusinfo) vfs_statusinfo "$2" "$3";;
esac
exit 1