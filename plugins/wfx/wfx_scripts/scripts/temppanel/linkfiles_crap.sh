#!/bin/sh

#-- someshit

vfs_init()
{
    echo -e "Fs_PushValue Select directory\t$HOME"
    echo "Fs_SelectDir Select directory"
    echo -e "Fs_MultiChoice Select link type\tSymbolic\tSymbolic (relative)\tHard"
    echo "Fs_Info_Message Copy the file(s) into the virtual panel to create a link(s)."
    echo "Fs_GetValue_Needed"
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        "Select directory") echo "Fs_Set_DC_WFX_SCRIPT_DIR $value" ;;
        "Select link type") echo "Fs_Set_DC_WFX_SCRIPT_TYPE $value" ;;
    esac
    exit 0
}

vfs_list()
{
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

    if [[ "$DC_WFX_SCRIPT_TYPE" == "Symbolic" ]]; then
        ln -sf "$src" "$dst"
    elif [[ "$DC_WFX_SCRIPT_TYPE" == "Symbolic (relative)" ]]; then
        dstdir=`dirname "$dst"`
        relpath=`realpath -m --relative-to="$dstdir" "$src"`
        ln -sf "$relpath" "$dst"
    elif [[ "$DC_WFX_SCRIPT_TYPE" == "Hard" ]]; then
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
    inode=`stat --printf="Inode: %i, number of links: %h." "$file"`
    [ -z "$path" ] && echo "Fs_Info_Message This is not a symbolic link. $inode" || echo 'Fs_Info_Message The link points to "'"$path"'".'
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