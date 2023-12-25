#!/bin/sh


vfs_setopt()
{
    option="$1"
    value="$2"

    file=`find ~/.cache/yay -maxdepth 2 -type f -name ${value:1}`

    case "$option" in
        "Install package") echo "Fs_RunTerm sudo pacman -U $file" ;;
        "Make package great again") echo "Fs_RunTermKeep cd `dirname $file` && makepkg -srfc" ;;
        "Show installed package info") echo "Fs_RunTermKeep pacman -Qiq $DC_WFX_TP_SCRIPT_DATA" ;;
    esac
    exit 0
}

vfs_list()
{
    path="$1"

    find ~/.cache/yay -maxdepth 2 -type f -name "*.pkg.tar.*" -printf "%M %TF %TH:%TM:%TC %s %f\n"
    exit $?
}

vfs_rm()
{
    file="$1"

    rm `find ~/.cache/yay -maxdepth 2 -type f -name ${file:1}`
    exit $?
}

vfs_properties()
{
    file="$1"

    output=`tar -xf $(find ~/.cache/yay -maxdepth 2 -type f -name ${file:1}) .PKGINFO -O`
    pkgname=`grep pkgname <<< $output | sed 's/^pkgname\s=\s//g'`
    echo -e "pkgname\t$pkgname"
    grep pkgdesc <<< $output | sed 's/\s=\s/\t/g'
    echo -e "png: \t`find /usr/share/icons/hicolor/48x48/apps /usr/share/pixmaps -name $pkgname.png`"
    pkgver=`grep pkgver <<< $output | sed 's/^pkgver\s=\s//g'`
    echo -e "pkgver\t$pkgver"
    instver=`LANG= pacman -Qiq $pkgname | grep "Version         :" | cut -c19-`
    echo -e "installed\t$instver"
    instsize=`grep size <<< $output | sed 's/^size\s=\s//g'`
    echo -e "size\t`numfmt --to=iec <<< $instsize` `printf "(%'d)" $instsize`"
    builddate=`grep builddate <<< $output | sed 's/^builddate\s=\s//g'`
    echo -e "builddate\t`date +'%Y-%m-%d %T' -d @$builddate`"
    declare -a fields=("url" "packager" "license" "conflict" "provides" "replaces" "arch")
    for field in "${fields[@]}"
    do
        grep $field <<< $output | sed 's/\s=\s/\t/g'
    done
    echo -e "Fs_Set_DC_WFX_TP_SCRIPT_DATA $pkgname"
    echo -e "Fs_PropsActs Install package\tMake package great again\tShow installed package info"

    exit 0
}

vfs_localname()
{
    file="$1"

    find ~/.cache/yay -maxdepth 2 -type f -name ${file:1}
    exit $?
}

case "$1" in
    setopt) vfs_setopt "$2" "$3";;
    list) vfs_list "$2";;
    rm) vfs_rm "$2";;
    properties) vfs_properties "$2";;
    localname) vfs_localname "$2";;
esac
exit 1