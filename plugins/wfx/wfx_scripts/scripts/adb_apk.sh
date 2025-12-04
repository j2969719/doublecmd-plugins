#!/bin/sh

init_fail()
{
    message="$1"

    echo "Fs_Info_Message $message"
    echo "Fs_Set_DC_WFX_SCRIPT_INITFAIL TRUE"
    exit 1
}

vfs_init()
{
    which adb >/dev/null 2>&1 || init_fail WFX_SCRIPT_STR_INSTALL_ADB
    echo "Fs_CONNECT_Needed"
    echo "Fs_StatusInfo_Needed"
    echo "Fs_DisableFakeDates"
    echo "Fs_Set_DC_WFX_SCRIPT_EXT .log"
    echo "Fs_LogInfo"
    adb start-server
    exit 0
}

vfs_setopt()
{
    option="$1"
    value="$2"
    devname=`echo "$DC_WFX_SCRIPT_REMOTENAME" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"
    filename="${DC_WFX_SCRIPT_REMOTENAME#/*/}"
    app="${filename%$DC_WFX_SCRIPT_EXT}"

    case "$option" in
        WFX_SCRIPT_STR_TCPPORT) echo "Fs_LogInfo" && adb tcpip "$value" ;;
        WFX_SCRIPT_STR_ACT_CLEAR) echo "Fs_Info_Message adb $device clear $app" ;;
        WFX_SCRIPT_STR_ACT_SETINSTLOC) echo -e "Fs_MultiChoice WFX_SCRIPT_STR_ACT_INSTLOC\tWFX_SCRIPT_STR_LOC0\tWFX_SCRIPT_STR_LOC1\tWFX_SCRIPT_STR_LOC2" ;;
        WFX_SCRIPT_STR_ACT_SHELL) echo "Fs_RunTerm adb $device shell" ;;

        WFX_SCRIPT_STR_ACT_INSTLOC)
            case "$value" in
                WFX_SCRIPT_STR_LOC0) echo "Fs_Info_Message adb $device set-install-location 0" ;;
                WFX_SCRIPT_STR_LOC1) echo "Fs_Info_Message adb $device set-install-location 1" ;;
                WFX_SCRIPT_STR_LOC2) echo "Fs_Info_Message adb $device set-install-location 2" ;;
            esac ;;
        *) echo "Fs_Info_Message \"$option\" WFX_SCRIPT_STR_ERRNA" ;;
    esac
    exit 0
}

vfs_list()
{
    output=`adb shell pm list packages`
    if [ "$1" == "/" ]; then
        adb devices | grep -G 'device$' | cut -d'	' -f1 | while read line; do\
            echo "dr-xr-xr-x 0000-00-00 00:00:00 - $line"; done
        echo "0111 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_USB<.sh"
        echo "0111 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_TCP<.sh"
        echo "0111 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_ROOT<.sh"
        echo "0111 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_UNROOT<.sh"
        echo "0111 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_OFFLINE<.sh"
        echo "0111 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_LSALL<.sh"
        echo "0111 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_LSSYS<.sh"
        echo "0111 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_LSUSR<.sh"
        if [ -z "$DC_WFX_SCRIPT_CPOPT" ]; then
            echo "0111 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_INSTR<.sh"
        else
            echo "0111 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_INST<.sh"
        fi
        if [ -z "$DC_WFX_SCRIPT_RMOPT" ]; then
            echo "0111 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_UNINSTK<.sh"
        else
            echo "0111 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_UNINST<.sh"
        fi
        if [ "$DC_WFX_SCRIPT_EXT" == ".log" ]; then
            echo "0111 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_LSAPK<.sh"
        else
            echo "0111 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_LSLOG<.sh"
        fi
    else
        [ "$DC_WFX_SCRIPT_OPBLOCK" == "TRUE" ] && exit 1
        devname=`echo "$1" | cut -d/ -f2`
        [ -z "$devname" ] || device="-s $devname"
        adb $device shell pm list packages $DC_WFX_SCRIPT_LSOPT | cut -c9- | while read line; do
            echo "0444 0000-00-00 00:00:00 - $line$DC_WFX_SCRIPT_EXT"; done
    fi
    exit 0
}

vfs_copyout()
{
    src="${1#/*/}"
    [ "$src" == "$1" ] && exit 1
    dst="$2"
    devname=`echo "$1" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"
    app="${src%$DC_WFX_SCRIPT_EXT}"

    if [ "$DC_WFX_SCRIPT_EXT" == ".log" ]; then
        adb $device shell pm dump "${src%$DC_WFX_SCRIPT_EXT}" > "$dst"
    else
        path=`adb $device shell pm path "$app" | cut -c9-`
        adb $device pull -q "$path" "$dst"
    fi
    exit $?
}

vfs_copyin()
{
    src="$1"
    dst="${2#/*/}"
    [ "$dst" == "$2" ] && exit 1
    devname=`echo "$2" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"

    adb $device install $DC_WFX_SCRIPT_CPOPT "$src"
    exit $?
}

vfs_rm()
{
    [ "$DC_WFX_SCRIPT_OPBLOCK" == "TRUE" ] && exit 1
    file="${1#/*/}"
    [ "$file" == "$1" ] && exit 1
    devname=`echo "$1" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"
    app="${file%$DC_WFX_SCRIPT_EXT}"

    adb $device shell pm uninstall $DC_WFX_SCRIPT_RMOPT "$app"
    exit $?
}

vfs_openfile()
{
    file="${1#/*/}"
    if [ "$file" == "$1" ]; then
        echo "Fs_LogInfo"
        case "${1:1}" in
            ">$ENV_WFX_SCRIPT_STR_USB<.sh") adb usb ;;
            ">$ENV_WFX_SCRIPT_STR_TCP<.sh") echo -e "Fs_PushValue WFX_SCRIPT_STR_TCPPORT\t5555" &&\
                                            echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_TCPPORT" ;;
            ">$ENV_WFX_SCRIPT_STR_ROOT<.sh") adb root ;;
            ">$ENV_WFX_SCRIPT_STR_UNROOT<.sh") adb unroot ;;
            ">$ENV_WFX_SCRIPT_STR_OFFLINE<.sh") adb reconnect offline ;;
            ">$ENV_WFX_SCRIPT_STR_LSALL<.sh") echo "Fs_Set_DC_WFX_SCRIPT_LSOPT " ;;
            ">$ENV_WFX_SCRIPT_STR_LSSYS<.sh") echo "Fs_Set_DC_WFX_SCRIPT_LSOPT -s" ;;
            ">$ENV_WFX_SCRIPT_STR_LSUSR<.sh") echo "Fs_Set_DC_WFX_SCRIPT_LSOPT -3" ;;
            ">$ENV_WFX_SCRIPT_STR_INST<.sh") echo "Fs_Set_DC_WFX_SCRIPT_CPOPT " ;;
            ">$ENV_WFX_SCRIPT_STR_INSTR<.sh") echo "Fs_Set_DC_WFX_SCRIPT_CPOPT -r" ;;
            ">$ENV_WFX_SCRIPT_STR_UNINST<.sh") echo "Fs_Set_DC_WFX_SCRIPT_RMOPT " ;;
            ">$ENV_WFX_SCRIPT_STR_UNINSTK<.sh") echo "Fs_Set_DC_WFX_SCRIPT_RMOPT -k" ;;
            ">$ENV_WFX_SCRIPT_STR_LSAPK<.sh") echo "Fs_Set_DC_WFX_SCRIPT_EXT .apk" ;;
            ">$ENV_WFX_SCRIPT_STR_LSLOG<.sh") echo "Fs_Set_DC_WFX_SCRIPT_EXT .log" ;;

            *) echo "Fs_Info_Message \"${1:1}\" WFX_SCRIPT_STR_ERRNA" ;;
        esac
     else
        devname=`echo "$1" | cut -d/ -f2`
        [ -z "$devname" ] || device="-s $devname"
        app="${file%$DC_WFX_SCRIPT_EXT}"
        echo Fs_RunAsync adb $device shell monkey -p "$app" -c android.intent.category.LAUNCHER 1
    fi
    exit 0
}

vfs_properties()
{
    file="${1#/*/}"
    if [ "$file" != "$1" ]; then
        app="${file%$DC_WFX_SCRIPT_EXT}"
        path=`adb $device shell pm path "$app" | cut -c9-`
        [ -z "$path" ] && exit 0
        echo -e "app\t$app"
        echo -e "path\t$path"
        actions=(CLEAR SETINSTLOC SHELL)
        string=`printf '\tWFX_SCRIPT_STR_ACT_%s' "${actions[@]}"`
        echo -e "Fs_PropsActs$string"
    fi
    exit 0
}

vfs_statusinfo()
{
    op="$1"
    path="${2#/*/}"
    if [ "$path" == "$2" ]; then
        if [ "$op" == "delete start" ]; then
            beep -f 3655
            echo -e "Fs_Set_DC_WFX_SCRIPT_OPBLOCK TRUE"
        elif [ "$op" == "delete end" ]; then
            echo -e "Fs_Set_DC_WFX_SCRIPT_OPBLOCK "
        fi
    fi
    exit $?
}

vfs_deinit()
{
    adb kill-server &
    exit $?
}

case "$1" in
    init) vfs_init;;
    setopt) vfs_setopt "$2" "$3";;
    list) vfs_list "$2";;
    copyout) vfs_copyout "$2" "$3";;
    copyin) vfs_copyin "$2" "$3";;
    run) vfs_openfile "$2";;
    properties) vfs_properties "$2";;
    statusinfo) vfs_statusinfo "$2" "$3";;
    deinit) vfs_deinit;;
esac
exit 1
