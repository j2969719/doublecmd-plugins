#!/bin/sh

vfs_init()
{
    echo "Fs_CONNECT_Needed"
    echo "Fs_StatusInfo_Needed"
    echo "Fs_GetValue_Needed"

    echo "Fs_DisableFakeDates"

    echo "Fs_Set_DC_WFX_SCRIPT_STDERR 2>/dev/null"

    echo "Fs_LogInfo"
    adb start-server
}

filedlg_show()
{
    message="$1"
    filter="$2"
    default_ext="$3"
    dialog_mode="$4"

    echo -e "Fs_SelectFile $message\t$filter\t$default_ext\t$dialog_mode"
}

vfs_setopt()
{
    option="$1"
    value="$2"
    devname=`echo "$DC_WFX_SCRIPT_REMOTENAME" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"

    case "$option" in
        WFX_SCRIPT_STR_ACT_SHELL) echo "Fs_RunTerm adb $device shell" ;;
        WFX_SCRIPT_STR_ACT_SCRPNG) filedlg_show "WFX_SCRIPT_STR_SCRPNG" "PNG|*.png|WFX_SCRIPT_STR_ALL|*" "png" "save" ;;
        WFX_SCRIPT_STR_ACT_SCRCPY) scrcpy -s $device || echo "Fs_Info_Message WFX_SCRIPT_STR_ERRSCRCPY" ;;
        WFX_SCRIPT_STR_ACT_LOGCAT) echo "Fs_RunTerm adb $device logcat $DC_WFX_SCRIPT_LOGOPT" ;;
        WFX_SCRIPT_STR_ACT_BUG) echo -e "Fs_SelectDir WFX_SCRIPT_STR_BUG" ;;
        WFX_SCRIPT_STR_ACT_REBOOT) echo "Fs_LogInfo" && adb $device reboot ;;
        WFX_SCRIPT_STR_ACT_REBOOTREC) echo "Fs_LogInfo" && adb $device reboot recovery ;;
        WFX_SCRIPT_STR_ACT_REBOOTBL) echo "Fs_LogInfo" && adb $device reboot bootloader ;;

        WFX_SCRIPT_STR_SCRPNG) adb $device exec-out "screencap -p" > "$value" ;;
        WFX_SCRIPT_STR_BUG) echo "Fs_RunTerm cd \"$value\" && adb $device bugreport" ;;
        WFX_SCRIPT_STR_TCPPORT) echo "Fs_LogInfo" && adb tcpip "$value" ;;
        WFX_SCRIPT_STR_LOGOPT) echo -e "Fs_Set_DC_WFX_SCRIPT_LOGOPT $value" ;;


        WFX_SCRIPT_STR_ACT_APK) filedlg_show "WFX_SCRIPT_STR_APK" "Android package|*.apk" "apk" ;;
        WFX_SCRIPT_STR_ACT_OTA) filedlg_show "WFX_SCRIPT_STR_OTA" "ROM Zip|*.zip|WFX_SCRIPT_STR_ALL|*" ;;
        WFX_SCRIPT_STR_ACT_ATTACH) echo "Fs_LogInfo" && adb $device attach ;;
        WFX_SCRIPT_STR_ACT_DETACH) echo "Fs_LogInfo" && adb $device detach ;; 
        WFX_SCRIPT_STR_ACT_REBOOTSL) echo "Fs_LogInfo" && adb $device reboot sideload ;;
        WFX_SCRIPT_STR_ACT_REBOOTSLA) echo "Fs_LogInfo" && adb $device reboot sideload-auto-reboot ;;
        WFX_SCRIPT_STR_ACT_BACKUP) echo -e "Fs_SelectDir WFX_SCRIPT_STR_BACKUP" ;;
        WFX_SCRIPT_STR_ACT_RESTORE) filedlg_show "WFX_SCRIPT_STR_RESTORE" "backup|*.ab" ;;

        WFX_SCRIPT_STR_APK) echo "Fs_RunTermKeep adb $device install -r \"$value\"" ;;
        WFX_SCRIPT_STR_OTA) echo "Fs_RunTermKeep adb $device sideload \"$value\"" ;;
        WFX_SCRIPT_STR_BACKUP) echo "Fs_RunTerm cd \"$value\" && adb $device backup $DC_WFX_SCRIPT_BACKUPOPT" ;;
        WFX_SCRIPT_STR_RESTORE) echo "Fs_RunTermKeep adb $device restore \"$value\"" ;;

        *) echo "Fs_Info_Message \"$option\" WFX_SCRIPT_STR_ERRNA" ;;
    esac
    exit 0
}

vfs_list()
{
    if [ "$1" == "/" ]; then
        devinfo="dr-xr-xr-x 0000-00-00 00:00:00 -"
        adb devices | grep -G 'device$' | cut -d'	' -f1 | while read line; do echo "$devinfo $line"; done
        echo "-r-xr-xr-x 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_USB<.sh"
        echo "-r-xr-xr-x 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_TCP<.sh"
        echo "-r-xr-xr-x 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_ROOT<.sh"
        echo "-r-xr-xr-x 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_UNROOT<.sh"
        echo "-r-xr-xr-x 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_OFFLINE<.sh"
        echo "-r-xr-xr-x 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_LOG<.sh"

        if [ -z "$DC_WFX_SCRIPT_STDERR" ]; then
            echo "-r-xr-xr-x 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_STDERROFF<.sh"
        else
            echo "-r-xr-xr-x 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_STDERRON<.sh"
        fi

        exit 0
    fi

    [ "$DC_WFX_SCRIPT_OPBLOCK" == "TRUE" ] && exit 1
    devname=`echo "$1" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"
    path="/${1#/*/}"
    [ "$path" == "/$1" ] && path=/
    command="cd \"$path\" && for file in *; do [ \"\$file\" != '*' ] && stat -c '%A %.19y %s %n' \"\$file\" $DC_WFX_SCRIPT_STDERR ; done"
    adb $device shell "$command"

    exit 0
}

vfs_copyout()
{
    src="/${1#/*/}"
    [ "$src" == "/$1" ] && exit 1
    dst="$2"
    devname=`echo "$1" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"

    adb $device pull "$src" "$dst"
    exit $?
}

vfs_fileexists()
{
    file="/${1#/*/}"
    [ "$file" == "/$1" ] && exit 1
    command="[[ -e \"$file\" ]]"
    devname=`echo "$1" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"

    adb $device shell "$command"
    exit $?
}

vfs_copyin()
{
    src="$1"
    dst="/${2#/*/}"
    [ "$dst" == "/$2" ] && exit 1
    devname=`echo "$2" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"

    adb $device push "$src" "$dst"
    exit $?
}

vfs_cp()
{
    src="/${1#/*/}"
    [ "$src" == "/$1" ] && exit 1
    dst="/${2#/*/}"
    [ "$dst" == "/$2" ] && exit 1
    command="cp \"$src\" \"$dst\""
    devname=`echo "$1" | cut -d/ -f2`
    devname2=`echo "$2" | cut -d/ -f2`

    [ -z "$devname" ] || device="-s $devname"

    if [ "$devname" != "$devname2" ]; then 
        [ -z "$devname2" ] || device2="-s $devname2"
        tempdir=$(mktemp -d /tmp/wfxadb.XXXXX)
        tempfile="$tempdir/`basename \"$dst\"`"
        adb $device pull "$src" "$tempfile" &&\
        adb $device2 push "$tempfile" "$dst"
        result=$?
        rm -rf "$tempdir"
        exit $result
    fi

    adb $device shell "$command"
    exit $?
}

vfs_mv()
{
    src="/${1#/*/}"
    [ "$src" == "/$1" ] && exit 1
    dst="/${2#/*/}"
    [ "$dst" == "/$2" ] && exit 1
    command="mv \"$src\" \"$dst\""
    devname=`echo "$1" | cut -d/ -f2`
    devname2=`echo "$2" | cut -d/ -f2` 

    [ -z "$devname" ] || device="-s $devname"

    if [ "$devname" != "$devname2" ]; then  
        [ -z "$devname2" ] || device2="-s $devname2"
        tempdir=$(mktemp -d /tmp/wfxadb.XXXXX)
        tempfile="$tempdir/`basename \"$dst\"`"
        adb $device pull "$src" "$tempfile" &&\
        adb $device2 push "$tempfile" "$dst" &&\
        adb $device shell "rm \"$src\""
        result=$?
        rm -rf "$tempdir"
        exit $result
    fi

    adb $device shell "$command"
    exit $?
}

vfs_mkdir()
{
    path="/${1#/*/}"
    [ "$path" == "/$1" ] && exit 1
    command="mkdir \"$path\""
    devname=`echo "$1" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"

    adb $device shell "$command"
    exit $?
}

vfs_rm()
{
    [ "$DC_WFX_SCRIPT_OPBLOCK" == "TRUE" ] && exit 1
    file="/${1#/*/}"
    [ "$file" == "/$1" ] && exit 1
    command="rm \"$file\""
    devname=`echo "$1" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"

    adb $device shell "$command"
    exit $?
}

vfs_rmdir()
{
    [ "$DC_WFX_SCRIPT_OPBLOCK" == "TRUE" ] && exit 1
    path="/${1#/*/}"
    [ "$path" == "/$1" ] && exit 1
    command="rmdir \"$path\""
    devname=`echo "$1" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"

    adb $device shell "$command"
    exit $?
}

vfs_openfile()
{
    file="/${1#/*/}"
    if [ "$file" == "/$1" ]; then
        echo "Fs_LogInfo"
        case "${1:1}" in
        ">$ENV_WFX_SCRIPT_STR_USB<.sh") adb usb ;;
        ">$ENV_WFX_SCRIPT_STR_TCP<.sh") echo -e "Fs_PushValue WFX_SCRIPT_STR_TCPPORT\t5555" &&\
                                        echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_TCPPORT" ;;
        ">$ENV_WFX_SCRIPT_STR_LOG<.sh") echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_LOGOPT" ;;

        ">$ENV_WFX_SCRIPT_STR_ROOT<.sh") adb root ;;
        ">$ENV_WFX_SCRIPT_STR_UNROOT<.sh") adb unroot ;;
        ">$ENV_WFX_SCRIPT_STR_OFFLINE<.sh") adb reconnect offline ;;
        ">$ENV_WFX_SCRIPT_STR_STDERRON<.sh") echo "Fs_Set_DC_WFX_SCRIPT_STDERR " ;;
        ">$ENV_WFX_SCRIPT_STR_STDERROFF<.sh") echo "Fs_Set_DC_WFX_SCRIPT_STDERR 2>/dev/null" ;;

        *) echo "Fs_Info_Message \"${1:1}\" WFX_SCRIPT_STR_ERRNA" ;;
        esac
        exit 0
    fi
    command="readlink \"$file\""
    devname=`echo "$1" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"
    output=`adb $device shell "$command"`

    if [ -z "$output" ]; then
        exit 1
    else
        adb $device shell "[[ -d \"$file\" ]]" && echo "./$devname$file"
        exit 0
    fi

    exit 1
}



vfs_properties()
{
    file="/${1#/*/}"
    if [ "$file" == "/$1" ]; then
        magicfile=`echo "$1" | grep '<.sh'`
        if [ -z "$magicfile" ]; then
            echo -e "filetype\tWFX_SCRIPT_STR_DEV"
            brand=`adb -s "${1:1}" shell getprop ro.product.brand 2>/dev/null`
            [ -z "$brand" ] || echo -e "WFX_SCRIPT_STR_BRAND\t$brand"
            model=`adb -s "${1:1}" shell getprop ro.product.model 2>/dev/null`
            [ -z "$model" ] || echo -e "WFX_SCRIPT_STR_MODEL\t$model"
            propdev=`adb -s "${1:1}" shell getprop ro.product.device 2>/dev/null`
            [ -z "$propdev" ] || echo -e "WFX_SCRIPT_STR_MDEV\t$propdev"
            board=`adb -s "${1:1}" shell getprop ro.product.board 2>/dev/null`
            [ -z "$board" ] || echo -e "WFX_SCRIPT_STR_BOARD\t$board"
            abilist=`adb -s "${1:1}" shell getprop ro.product.cpu.abilist 2>/dev/null`
            [ -z "$abilist" ] || echo -e "WFX_SCRIPT_STR_ABI\t$abilist"
            ramsize=`adb -s "${1:1}" shell getprop ro.ramsize 2>/dev/null`
            [ -z "$ramsize" ] || echo -e "WFX_SCRIPT_STR_RAM\t$ramsize"
            propver=`adb -s "${1:1}" shell getprop ro.build.version.release 2>/dev/null`
            [ -z "$propver" ] || echo -e "WFX_SCRIPT_STR_VER\t$propver"
            prop=`adb -s "${1:1}" get-devpath 2>/dev/null`
            [ -z "$prop" ] || echo -e "WFX_SCRIPT_STR_DEVPATH\t$prop"

            # actions=(SHELL SCRPNG SCRCPY LOGCAT APK ATTACH DETACH BUG REBOOT REBOOTREC REBOOTBL REBOOTSL REBOOTSLA BACKUP RESTORE)
            actions=(SHELL SCRPNG SCRCPY LOGCAT BUG REBOOT REBOOTREC)
            string=`printf '\tWFX_SCRIPT_STR_ACT_%s' "${actions[@]}"`
            echo -e "Fs_PropsActs$string"
        else
            echo -e "filetype\tWFX_SCRIPT_STR_MAGIC"
        fi
        exit 0
    fi
    devname=`echo "$1" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"

    output=`adb $device shell "stat -c '%s' \"$file\" 2>/dev/null"`
    [ -z "$output" ] || echo -e "WFX_SCRIPT_STR_SIZE\t`numfmt --to=iec <<< $output` `printf "(%'d)" $output`"

    output=`adb $device shell "stat -c '%U (%u)' \"$file\" 2>/dev/null"`
    [ -z "$output" ] || echo -e "WFX_SCRIPT_STR_UID\t$output"

    output=`adb $device shell "stat -c '%G (%g)' \"$file\" 2>/dev/null"`
    [ -z "$output" ] || echo -e "WFX_SCRIPT_STR_GID\t$output"

    output=`adb $device shell "stat -c '%A (%a)' \"$file\" 2>/dev/null"`
    [ -z "$output" ] || echo -e "WFX_SCRIPT_STR_MODE\t$output"

    output=`adb $device shell "stat -c '%x' \"$file\" 2>/dev/null"`
    [ -z "$output" ] || echo -e "WFX_SCRIPT_STR_ACCESS\t$output"

    output=`adb $device shell "stat -c '%y' \"$file\" 2>/dev/null"`
    [ -z "$output" ] || echo -e "WFX_SCRIPT_STR_MODIFY\t$output"

    output=`adb $device shell "stat -c '%z' \"$file\" 2>/dev/null"`
    [ -z "$output" ] || echo -e "WFX_SCRIPT_STR_CHANGE\t$output"


    output=`adb $device shell "file -L \"$file\" 2>/dev/null"`
    [ -z "$output" ] || echo -e "filetype\t$output"
    output=`adb $device shell "readlink \"$file\" 2>/dev/null"`
    [ -z "$output" ] || echo -e "path\t$output"

    exit 0
}

vfs_chmod()
{
    path="/${1#/*/}"
    [ "$path" == "/$1" ] && exit 1
    newmode="$2"
    command="chmod \"$newmode\" \"$path\""
    devname=`echo "$1" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"

    adb $device shell "$command"
    exit $?
}

vfs_modtime()
{
    path="/${1#/*/}"
    [ "$path" == "/$1" ] && exit 1
    newdate="$2"
    command="touch -m -d\"$newdate\" \"$path\""
    devname=`echo "$1" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"

    adb $device shell "$command"
    exit $?
}

vfs_quote()
{
    string="$1"
    path="/${2#/*/}"
    [ "$path" == "/$2" ] && echo "Fs_RunTermKeep $string" ; exit 1
    command="cd \"$path\" && $string"
    devname=`echo "$2" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"

    echo -e "Fs_LogInfo"
    adb $device shell "$command"
    exit $?
}

vfs_getinfovalue()
{
    path="/${1#/*/}"
    if [ "$path" == "/$1" ]; then
        magicfile=`echo "$1" | grep '<.sh'`
        if [ -z "$magicfile" ]; then
            adb -s "${1:1}" shell getprop ro.product.model 2>/dev/null
        else
            echo "WFX_SCRIPT_STR_MAGIC"
        fi
        exit 0
    fi
    command="stat -c '%U/%G' \"$path\" $DC_WFX_SCRIPT_STDERR"
    devname=`echo "$1" | cut -d/ -f2`
    [ -z "$devname" ] || device="-s $devname"

    adb $device shell "$command"
    exit $?
}

vfs_statusinfo()
{
    op="$1"
    path="/${2#/*/}"
    if [ "$path" == "/$2" ]; then
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
    adb kill-server
    exit $?
}

vfs_reset()
{
    # https://blog.dhampir.no/content/fun-with-beep
    beep -f 130 -l 100 -n -f 262 -l 100 -n -f 330 -l 100 -n -f 392 -l 100 -n -f 523 -l 100 -n -f 660 -l 100 -n -f 784 -l 300 -n -f 660 -l 300 -n -f 146 -l 100 -n -f 262 -l 100 -n -f 311 -l 100 -n -f 415 -l 100 -n -f 523 -l 100 -n -f 622 -l 100 -n -f 831 -l 300 -n -f 622 -l 300 -n -f 155 -l 100 -n -f 294 -l 100 -n -f 349 -l 100 -n -f 466 -l 100 -n -f 588 -l 100 -n -f 699 -l 100 -n -f 933 -l 300 -n -f 933 -l 100 -n -f 933 -l 100 -n -f 933 -l 100 -n -f 1047 -l 400

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
    getvalue) vfs_getinfovalue "$2";;
    statusinfo) vfs_statusinfo "$2" "$3";;
    deinit) vfs_deinit;;
    reset) vfs_reset;;
esac
exit 1