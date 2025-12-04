#!/bin/sh

# fast and furious

vfs_init()
{
    echo -e "Fs_DisableFakeDates\nFs_GetValue_Needed"
    echo -e 'Fs_PushValue\tCommand\tnmblookup -A $IP_FOR_CMD'
    ip=`ip route get 1 2>/dev/null | grep -oE 'via [0-9.]+ dev' | grep -oE '[0-9.]+' || echo '192.168.0.1'`
    echo -e "Fs_EditLine\tStart\t$ip"
    exit 0
}

vfs_setopt()
{
    option="$1"
    value="$2"

    if [ "$option" == "Fs_EditLine Start" ]; then
        echo -e "Fs_Set_WFX_IP_START\t$value"
        echo -e "Fs_EditLine\tEnd\t${value%.*}.255"
    elif [ "$option" == "Fs_EditLine End" ]; then
        echo -e "Fs_Set_WFX_IP_END\t$value"
    elif [ "$option" == "Command" ]; then
        command=`eval echo $value`
        echo "Fs_RunTermKeep $command"
    fi

    exit 0
}

vfs_list()
{
    [ -z "$WFX_IP_START" ] && exit 1
    [ -z "$WFX_IP_END" ] && exit 1

    x1_start=`echo $WFX_IP_START | cut -d'.' -f1`
    x1_end=`echo $WFX_IP_END | cut -d'.' -f1`
    x2_start=`echo $WFX_IP_START | cut -d'.' -f2`
    x2_end=`echo $WFX_IP_END | cut -d'.' -f2`
    x3_start=`echo $WFX_IP_START | cut -d'.' -f3`
    x3_end=`echo $WFX_IP_END | cut -d'.' -f3`
    x4_start=`echo $WFX_IP_START | cut -d'.' -f4`
    x4_end=`echo $WFX_IP_END | cut -d'.' -f4`

    for ((x1=$x1_start; x1<=$x1_end; x1++)); do
        for ((x2=$x2_start; x2<=$x2_end; x2++)); do
            for ((x3=$x3_start; x3<=$x3_end; x3++)); do
                for ((x4=$x4_start; x4<=$x4_end; x4++)); do
                    echo "0000 0000-00-00 00:00:00 - $x1.$x2.$x3.$x4.ip"
                done
            done
        done
    done

    exit 0
}

vfs_openfile()
{
    ip="${1%.*}"
    echo -e "Fs_Set_IP_FOR_CMD\t${ip:1}"
    echo -e "Fs_Request_Options\nCommand"

    exit 0
}

vfs_getinfovalue()
{
    ip="${1%.*}"
    ping -c 1 -i 0.2 -w 1 -W 1 "${ip:1}" >/dev/null && echo -e "OK" || echo -e "nope"
    exit 0
}

case "$1" in
    init) vfs_init;;
    setopt) vfs_setopt "$2" "$3";;
    list) vfs_list "$2";;
    copyout) vfs_copyout "$2" "$3";;
    run) vfs_openfile "$2";;
    properties) vfs_properties "$2";;
    getvalue) vfs_getinfovalue "$2";;
esac
exit 1