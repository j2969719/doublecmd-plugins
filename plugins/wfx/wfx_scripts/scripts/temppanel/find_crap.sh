#!/bin/sh

vfs_init()
{
    echo -e "Fs_PushValue WFX_SCRIPT_STR_DIR\t$HOME"
    echo -e "Fs_SelectDir WFX_SCRIPT_STR_DIR"

    echo -e "Fs_Request_Options"
    echo -e "Fs_PushValue WFX_SCRIPT_STR_MASK\t*"
    echo -e "WFX_SCRIPT_STR_MASK"

    echo -e "Fs_RequestOnce"
    echo -e "Fs_YesNo_Message WFX_SCRIPT_STR_ASKOPT"

    echo -e "Fs_GetValue_Needed"
#   echo -e "Fs_GetValues_Needed"
    echo -e 'Fs_PushValue WFX_SCRIPT_STR_VAL\tstat -c"%i" "$file"'
    echo -e "WFX_SCRIPT_STR_VAL"

#   echo "Fs_StatusInfo_Needed"
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        WFX_SCRIPT_STR_DIR) echo -e "Fs_Set_DC_WFX_SCRIPT_DIR $value" ;;
        WFX_SCRIPT_STR_MASK) echo -e "Fs_Set_DC_WFX_SCRIPT_MASK $value" ;;
        WFX_SCRIPT_STR_ASKOPT) [ "$value" == "Yes" ] && echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_OPT" ;;
        WFX_SCRIPT_STR_OPT) echo -e "Fs_Set_DC_WFX_SCRIPT_OPT $value" ;;
        WFX_SCRIPT_STR_VAL) echo -e "Fs_Set_DC_WFX_SCRIPT_VAL $value" ;;
        WFX_SCRIPT_STR_ACTDIR) echo -e "Fs_SelectDir WFX_SCRIPT_STR_DIR" ;;
        WFX_SCRIPT_STR_ACTMASK) echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_MASK" ;;
        WFX_SCRIPT_STR_ACTOPT) echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_OPT" ;;
        WFX_SCRIPT_STR_ACTVAL) echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_VAL" ;;
        WFX_SCRIPT_STR_ACTMAN) echo "Fs_ShowOutput man find" ;; #echo "Fs_RunTerm man find" ;;
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

    cp "$src" "$dst"
    exit $?
}

vfs_fileexists()
{
    file="$DC_WFX_SCRIPT_DIR""$1"

    test -f "$file"
    exit $?
}

vfs_copyin()
{
    src="$1"
    dst="$DC_WFX_SCRIPT_DIR""$2"

    cp "$src" "$dst"
    exit $?
}

vfs_cp()
{
    src="$DC_WFX_SCRIPT_DIR""$1"
    dst="$DC_WFX_SCRIPT_DIR""$2"

    cp "$src" "$dst"
    exit $?
}

vfs_mv()
{
    src="$DC_WFX_SCRIPT_DIR""$1"
    dst="$DC_WFX_SCRIPT_DIR""$2"

    mv "$src" "$dst"
    exit $?
}

vfs_mkdir()
{
    file="$DC_WFX_SCRIPT_DIR""$1"

    mkdir "$file"
    exit $?
}

vfs_rm()
{
    file="$DC_WFX_SCRIPT_DIR""$1"

    gio trash "$file" || rm "$file"
    exit $?
}

vfs_rmdir()
{
    file="$DC_WFX_SCRIPT_DIR""$1"

    gio trash "$file" || rmdir "$file"
    exit $?
}

vfs_openfile()
{
    file="$DC_WFX_SCRIPT_DIR""$1"
    echo "Fs_OpenTerm $file"
    exit 0
}

vfs_properties()
{
    file="$DC_WFX_SCRIPT_DIR""$1"
    echo -e "WFX_SCRIPT_STR_INFO\t"`file -b "$file"`
    frmtbasic="WFX_SCRIPT_STR_TYPE\t%F\nWFX_SCRIPT_STR_SIZE\t%'s\nWFX_SCRIPT_STR_PERM\t%A (%a)\n"
    frmtdates="WFX_SCRIPT_STR_MTIME\t%y\nWFX_SCRIPT_STR_BTIME\t%w\nWFX_SCRIPT_STR_ATIME\t%x\nWFX_SCRIPT_STR_CTIME\t%z\n"
    frmtextra="WFX_SCRIPT_STR_OWNER\t%U (%u)\nWFX_SCRIPT_STR_GROUP\t%G (%g)\nWFX_SCRIPT_STR_INODE\t%i\nWFX_SCRIPT_STR_NLINKS\t%h\n"
    stat --printf="$frmtbasic$frmtdates$frmtextra" "$file"
    echo -e "content_type\t"`file -b --mime-type "$file"`
    echo -e "Fs_PropsActs WFX_SCRIPT_STR_ACTDIR\tWFX_SCRIPT_STR_ACTMASK\tWFX_SCRIPT_STR_ACTOPT\tWFX_SCRIPT_STR_ACTVAL\tWFX_SCRIPT_STR_ACTMAN"
    exit 0
}

vfs_chmod()
{
    file="$DC_WFX_SCRIPT_DIR""$1"
    newmode="$2"

    chmod "$newmode" "$file"
    exit $?
}

vfs_modtime()
{
    file="$DC_WFX_SCRIPT_DIR""$1"
    newdate="$2"

    touch -m --date="$newdate" "$file"
    exit $?
}

vfs_quote()
{
    string="$1"
    path="$DC_WFX_SCRIPT_DIR""$2"

#   echo 'Fs_RunTermKeep cd "'"$path"'" && '"$string"
    echo 'Fs_RunTerm cd "'"$path"'" && '"$string"
    exit $?
}

vfs_localname()
{
    file="$DC_WFX_SCRIPT_DIR""$1"

    echo "$file"
    exit $?
}

vfs_getinfovalue()
{
    file="$DC_WFX_SCRIPT_DIR""$1"

    [ -z "$DC_WFX_SCRIPT_VAL" ] || eval "$DC_WFX_SCRIPT_VAL"
    exit $?
}

vfs_getinfovalues()
{
    path="$DC_WFX_SCRIPT_DIR""$1"
    command='file="$path$0" ; val=`eval "$DC_WFX_SCRIPT_VAL"` ; [ -z "$DC_WFX_SCRIPT_VAL" ] || echo -e "`basename "$0"`\t$val"'
    file="$DC_WFX_SCRIPT_DIR""$1"

    if [ -z "$DC_WFX_SCRIPT_MASK" ]; then
        find "$path" -mindepth 1 -maxdepth 1 -name "*" -exec sh -c "$command" '{}' \;
    else
        find "$path" -mindepth 1 -maxdepth 1 -type d -name "*" -exec sh -c "$command" '{}' \;
        find "$path" -mindepth 1 -maxdepth 1 -not -type d -name "$DC_WFX_SCRIPT_MASK" $DC_WFX_SCRIPT_OPT -exec sh -c "$command" '{}' \;
    fi

    exit $?
}

vfs_statusinfo()
{
    op="$1"
    path="$DC_WFX_SCRIPT_DIR""$2"

    echo "Fs_LogInfo"
    echo "$1" "$DC_WFX_SCRIPT_DIR""$2" "$DC_WFX_SCRIPT_REMOTENAME"
    exit $?
}

vfs_deinit()
{
    exit $?
}

vfs_reset()
{
    exit $?
}

case "$1" in
    init) vfs_init;;
    setopt) vfs_setopt "$2" "$3";;
    list) vfs_list "$2";;
    copyout) vfs_copyout "$2" "$3";;
    exists) vfs_fileexists "$2";;
    copyin) vfs_copyin "$2" "$3";;
    cp) vfs_cp "$2" "$3";;
    mv) vfs_mv "$2" "$3";;
    mkdir) vfs_mkdir "$2";;
    rm) vfs_rm "$2";;
    rmdir) vfs_rmdir "$2";;
    run) vfs_openfile "$2";;
    properties) vfs_properties "$2";;
    chmod) vfs_chmod "$2" "$3";;
    modtime) vfs_modtime "$2" "$3";;
    quote) vfs_quote "$2" "$3";;
    localname) vfs_localname "$2";;
    getvalue) vfs_getinfovalue "$2";;
    getvalues) vfs_getinfovalues "$2";;
    statusinfo) vfs_statusinfo "$2" "$3";;
    deinit) vfs_deinit;;
    reset) vfs_reset;;
esac
exit 1