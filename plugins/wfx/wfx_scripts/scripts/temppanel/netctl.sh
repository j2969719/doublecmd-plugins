#!/bin/sh

vfs_init()
{
    echo "Fs_GetValue_Needed"
}

vfs_setopt()
{
    connection="$1"
    option="$2"

    case "$option" in
        start) echo "Fs_RunTerm sudo netctl start $connection" ;;
        stop) echo "Fs_RunTerm sudo netctl stop $connection" ;;
        restart) echo "Fs_RunTerm sudo netctl restart $connection" ;;
        switch-to) echo "Fs_RunTerm sudo netctl switch-to $connection" ;;
        disable) echo "Fs_RunTerm sudo netctl disable $connection" ;;
        enable) echo "Fs_RunTerm sudo netctl enable $connection" ;;
        reenable) echo "Fs_RunTerm sudo netctl reenable $connection" ;;
        verify) echo "Fs_RunTerm sudo netctl verify $connection" ;;
    esac
    exit 0
}

vfs_list()
{
    find "/etc/netctl/" -mindepth 1 -maxdepth 1 -not -type d -printf "%M %TF %TH:%TM:%TC %s %f\n"
    exit $?
}

vfs_openfile()
{
    connection="${1:1}"
    echo -e "Fs_MultiChoice $connection\tstart\trestart\tstop\tswitch-to\tenable\tdisable\treenable\tverify"
    exit 0
}

vfs_properties()
{
    connection="${1:1}"
    echo "Fs_RunTermKeep netctl status $connection"
    exit 0
}


vfs_localname()
{
    file="/etc/netctl$1"

    echo "$file"
    exit $?
}

vfs_getinfovalue()
{
    connection="${1:1}"
    netctl is-active "$connection"
}

case "$1" in
    init) vfs_init;;
    setopt) vfs_setopt "$2" "$3";;
    list) vfs_list "$2";;
    run) vfs_openfile "$2";;
    properties) vfs_properties "$2";;
    localname) vfs_localname "$2";;
    getvalue) vfs_getinfovalue "$2";;
esac
exit 1