#!/bin/sh

mountpoint="/GuestFS"

init_fail()
{
    message="$1"

    echo "Fs_Info_Message $message"
    echo "Fs_Set_DC_WFX_SCRIPT_INITFAIL TRUE"
    exit 1
}

fsdlg()
{
    file="$1"
    listfs=`virt-filesystems --long -h --no-title --filesystems -a "$file" |\
            awk '{print $1" "$3" "$5" "$4}' | tr '\n' '\t' | head -c -1`

    [ -z "$listfs" ] && init_fail WFX_SCRIPT_STR_ERRFS
    echo -e "Fs_MultiChoice WFX_SCRIPT_STR_FS\t$listfs"
    exit 0
}

img_mount()
{
    value=`echo "$1" | grep -Po '/[^\s]+'`
    tmpdir=`mktemp -d /tmp/wfx-guestfs.XXXXX`

    mkdir "$tmpdir$mountpoint"
    echo "Fs_Set_DC_WFX_SCRIPT_TMPDIR $tmpdir"
    guestmount -a "$DC_WFX_SCRIPT_FILE" -m "$value" "$tmpdir$mountpoint" ||\
        init_fail WFX_SCRIPT_STR_ERRMOUNT
    echo "Fs_Redirect $tmpdir$mountpoint"
}

vfs_init()
{
    which guestmount >/dev/null 2>&1 || init_fail "\"guestmount\" WFX_SCRIPT_STR_ERRNA"
    which virt-filesystems >/dev/null 2>&1 || init_fail "\"virt-filesystems\" WFX_SCRIPT_STR_ERRNA"

    echo "Fs_Redirect infinity and beyond"
    echo -e "Fs_SelectFile WFX_SCRIPT_STR_FILE\tWFX_SCRIPT_STR_IMGMASK|*.img;*.qcow2;*.vdi;*.wmdk;*.iso;*.vhd|WFX_SCRIPT_STR_ALLMASK|*"

    exit $?
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        WFX_SCRIPT_STR_FILE) echo "Fs_Set_DC_WFX_SCRIPT_FILE $value" && fsdlg "$value" ;;
        WFX_SCRIPT_STR_FS) img_mount "$value" ;;
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
    [ -z "$DC_WFX_SCRIPT_TMPDIR" ] ||\
        fusermount3 -u "$DC_WFX_SCRIPT_TMPDIR$mountpoint" &&\
        rm -r "$DC_WFX_SCRIPT_TMPDIR"

    exit $?
}

case "$1" in
    init) vfs_init;;
    setopt) vfs_setopt "$2" "$3";;
    list) vfs_list "$2";;
    deinit) vfs_deinit;;
esac
exit 1
