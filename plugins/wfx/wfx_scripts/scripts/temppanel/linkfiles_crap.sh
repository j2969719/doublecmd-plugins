#!/bin/sh

#-- someshit

vfs_init()
{
    echo -e "Fs_PushValue WFX_SCRIPT_STR_DIR\t$HOME"
    echo "Fs_SelectDir WFX_SCRIPT_STR_DIR"
    echo "Fs_GetValue_Needed"
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        WFX_SCRIPT_STR_DIR) echo -e "Fs_Set_DC_WFX_SCRIPT_DIR $value\nFs_MultiChoice WFX_SCRIPT_STR_TYPE\tWFX_SCRIPT_STR_SYM\tWFX_SCRIPT_STR_REL\tWFX_SCRIPT_STR_HARD" ;;
        WFX_SCRIPT_STR_TYPE) echo -e "Fs_Set_DC_WFX_SCRIPT_TYPE $value\nFs_Info_Message WFX_SCRIPT_STR_INFO" ;;
    esac
    exit 0
}

vfs_list()
{
    [ -z "$DC_WFX_SCRIPT_TYPE" ] && exit 1

    path="$DC_WFX_SCRIPT_DIR""$1"

    find "$path" -mindepth 1 -maxdepth 1 -name "*" -printf "%M %TF %TH:%TM:%TC %s %f\n"
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

    [ "$src" == "$dst" ] && exit 1

    if [[ "$DC_WFX_SCRIPT_TYPE" == "WFX_SCRIPT_STR_SYM" ]]; then
        ln -sf "$src" "$dst"
    elif [[ "$DC_WFX_SCRIPT_TYPE" == "WFX_SCRIPT_STR_REL" ]]; then
        dstdir=`dirname "$dst"`
        relpath=`realpath -m --relative-to="$dstdir" "$src"`
        ln -sf "$relpath" "$dst"
    elif [[ "$DC_WFX_SCRIPT_TYPE" == "WFX_SCRIPT_STR_HARD" ]]; then
        ln -f "$src" "$dst"
    else
        exit 1
    fi
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
    path=`readlink "$file"`
    inode=`stat --printf="WFX_SCRIPT_STR_INODE %i, WFX_SCRIPT_STR_NLINK %h." "$file"`
    [ -z "$path" ] && echo "Fs_Info_Message WFX_SCRIPT_STR_NOTLINK $inode" || echo 'Fs_Info_Message WFX_SCRIPT_STR_LINK "'"$path"'".'
    exit 0
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

    if [ ! -d "$file" ]; then
        nlinks=`stat -c%h "$file"`
        [ "$nlinks" -gt 1 ] && stat -c%i "$file"
    fi
    exit $?
}

case "$1" in
    init) vfs_init;;
    setopt) vfs_setopt "$2" "$3";;
    list) vfs_list "$2";;
    exists) vfs_fileexists "$2";;
    copyin) vfs_copyin "$2" "$3";;
    mkdir) vfs_mkdir "$2";;
    rm) vfs_rm "$2";;
    rmdir) vfs_rmdir "$2";;
    run) vfs_openfile "$2";;
    properties) vfs_properties "$2";;
    localname) vfs_localname "$2";;
    getvalue) vfs_getinfovalue "$2";;
esac
exit 1