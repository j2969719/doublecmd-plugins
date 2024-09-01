#!/bin/sh

# so shell, much secure

init_fail()
{
    message="$1"

    echo "Fs_Info_Message $message"
    echo "Fs_Set_DC_WFX_SCRIPT_INITFAIL TRUE"
    exit 1
}

ssh_exec_command()
{
    [ -z "$DC_WFX_SCRIPT_HOST" ] && exit 1
    command="$1"

   sshpass -e ssh $DC_WFX_SCRIPT_OPT -n "$DC_WFX_SCRIPT_HOST" "$command"
}

vfs_init()
{
    which ssh >/dev/null 2>&1 || init_fail "ssh WFX_SCRIPT_STR_ERR_NA"
    which scp >/dev/null 2>&1 || init_fail "scp WFX_SCRIPT_STR_ERR_NA"
    which sshpass >/dev/null 2>&1 || init_fail "sshpass WFX_SCRIPT_STR_ERR_NA"
    echo "Fs_CONNECT_Needed"
    echo "Fs_GetValues_Needed"
    echo "Fs_DisableFakeDates"
    echo "Fs_RequestOnce"
    echo "Fs_YesNo_Message WFX_SCRIPT_STR_STRICT"
    echo "Fs_Request_Options"
    echo "WFX_SCRIPT_STR_HOST"
    echo "Fs_LogInfo"
    exit 0
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        WFX_SCRIPT_STR_STRICT) [ "$value" == "No" ] && echo "Fs_Set_DC_WFX_SCRIPT_OPT $DC_WFX_SCRIPT_OPT -o StrictHostKeyChecking=no" ;;
        WFX_SCRIPT_STR_HOST) echo -e "Fs_Set_SSHPASS \nFs_Set_DC_WFX_SCRIPT_HOST $value\nFs_Request_Options\nPASSWORD" ;;
        WFX_SCRIPT_STR_ACT_HOST) echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_HOST" ;;
        WFX_SCRIPT_STR_ACT_TERM) echo "Fs_YesNo_Message WFX_SCRIPT_STR_TERM" ;;
        WFX_SCRIPT_STR_TERM) echo "Fs_Set_DC_WFX_SCRIPT_TERM $value" ;;
        PASSWORD) echo "Fs_Set_SSHPASS $value" ;;
    esac
    exit 0
}

vfs_list()
{
    [ -z "$DC_WFX_SCRIPT_INITFAIL" ] || exit 1

    path="$1"
    command="find \"$path\" -mindepth 1 -maxdepth 1 -printf \"%M %TF %TH:%TM:%TC %s %f\n\""

    ssh_exec_command "$command"

    exit $?
}

vfs_copyout()
{
    src="$DC_WFX_SCRIPT_HOST:$1"
    dst="$2"

    sshpass -e scp "$src" "$dst"

    exit $?
}

vfs_fileexists()
{
    file="$1"
    command="[[ -e \"$file\" ]]"

    ssh_exec_command "$command"

    exit $?
}

vfs_copyin()
{
    src="$1"
    dst="$DC_WFX_SCRIPT_HOST:$2"

    sshpass -e scp "$src" "$dst"

    exit $?
}

vfs_cp()
{
    src="$1"
    dst="$2"
    command="cp \"$src\" \"$dst\""

    ssh_exec_command "$command"

    exit $?
}

vfs_mv()
{
    src="$1"
    dst="$2"
    command="mv \"$src\" \"$dst\""

    ssh_exec_command "$command"

    exit $?
}

vfs_mkdir()
{
    path="$1"
    command="mkdir \"$path\""

    ssh_exec_command "$command"

    exit $?
}

vfs_rm()
{
    file="$1"
    command="rm \"$file\""

    ssh_exec_command "$command"

    exit $?
}

vfs_rmdir()
{
    path="$1"
    command="rmdir \"$path\""

    ssh_exec_command "$command"

    exit $?
}

vfs_openfile()
{
    file="$1"
    command="readlink \"$file\""
    link=`ssh_exec_command "$command"`

    [ -z "$link" ] || ssh_exec_command "[[ -d \"$file\" ]]" &&\
        echo "./$link" && exit 0

    exit 1
}



vfs_properties()
{
    file="$1"

    info=`ssh_exec_command "file -b \"$file\""`
    [ -z "$info" ] || echo -e "WFX_SCRIPT_STR_INFO\t$info"

    frmtbasic="WFX_SCRIPT_STR_TYPE\t%F\nWFX_SCRIPT_STR_SIZE\t%'s\nWFX_SCRIPT_STR_PERM\t%A (%a)\n"
    frmtdates="WFX_SCRIPT_STR_MTIME\t%y\nWFX_SCRIPT_STR_BTIME\t%w\nWFX_SCRIPT_STR_ATIME\t%x\nWFX_SCRIPT_STR_CTIME\t%z\n"
    frmtextra="WFX_SCRIPT_STR_OWNER\t%U (%u)\nWFX_SCRIPT_STR_GROUP\t%G (%g)\nWFX_SCRIPT_STR_INODE\t%i\nWFX_SCRIPT_STR_NLINKS\t%h\n"
    command="stat --printf=\"$frmtbasic$frmtdates$frmtextra\" \"$file\""

    ssh_exec_command "$command"

    mime=`ssh_exec_command "file -b --mime-type \"$file\""`
    [ -z "$mime" ] || echo -e "content_type\t$mime"

    link=`ssh_exec_command "readlink \"$file\""`
    [ -z "$link" ] || echo -e "path:WFX_SCRIPT_STR_LINK\t$link"

    echo -e "Fs_PropsActs WFX_SCRIPT_STR_ACT_HOST\tWFX_SCRIPT_STR_ACT_TERM"

    exit $?
}

vfs_chmod()
{
    path="$1"
    newmode="$2"
    command="chmod \"$newmode\" \"$path\""

    ssh_exec_command "$command"

    exit $?
}

vfs_modtime()
{
    path="$1"
    newdate="$2"
    command="touch -m -d\"$newdate\" \"$path\""

    ssh_exec_command "$command"

    exit $?
}

vfs_quote()
{
    string="$1"
    path="$2"
    command="cd \"$path\" && $string"

    if [ "$DC_WFX_SCRIPT_TERM" == "Yes" ] ; then
        echo "Fs_RunTermKeep sshpass -p \"$SSHPASS\" ssh -n \"$DC_WFX_SCRIPT_HOST\" \"$command\""
    else
        echo "Fs_LogInfo"
        echo "$path> $command"
        ssh_exec_command "$command"
    fi
    exit $?
}

vfs_getinfovalues()
{
    path="$1"
    command="find \"$path\" -mindepth 1 -maxdepth 1 -printf \"%f\t%u/%g\n\""

    ssh_exec_command "$command"

    exit $?
}

vfs_deinit()
{

    exit $?
}

vfs_reset()
{
    # https://ubuntuforums.org/showthread.php?t=1157670&page=4
    beep -f 784 -r 3 -l 100
    sleep .1
    beep -f 784 -l 600
    beep -f 622 -l 600
    beep -f 698 -l 600
    beep -f 784 -l 200
    sleep .2
    beep -f 698 -l 200
    beep -f 784 -l 800

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
    getvalues) vfs_getinfovalues "$2";;
    deinit) vfs_deinit;;
    reset) vfs_reset;;
esac
exit 1