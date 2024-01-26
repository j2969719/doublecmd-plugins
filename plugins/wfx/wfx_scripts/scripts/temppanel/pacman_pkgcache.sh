#!/bin/sh

vfs_setopt()
{
    option="$1"
    value="$2"

    file="/var/cache/pacman/pkg$value"

    case "$option" in
        "Install package") echo "Fs_RunTermKeep sudo pacman -U $file" ;;
        "Install package (overwrite)") echo "Fs_RunTermKeep sudo pacman -U --overwrite \* $file" ;;
        "Show installed package info") echo "Fs_RunTermKeep pacman -Qiq $DC_WFX_TP_SCRIPT_DATA" ;;
    esac
    exit 0
}

vfs_list()
{
    path="/var/cache/pacman/pkg$1"

    find "$path" -mindepth 1 -maxdepth 1 -not -type d -name "*.pkg.tar.zst" -printf "%M %TF %TH:%TM:%TC %s %f\n"
    exit $?
}

vfs_copyout()
{
    src="/var/cache/pacman/pkg$1"
    dst="$2"

    cp "$src" "$dst"
    exit $?
}

vfs_rm()
{
    file="/var/cache/pacman/pkg$1"

    echo "Fs_RunTermKeep sudo rm $file"
    exit $?
}

vfs_properties()
{
    file="/var/cache/pacman/pkg$1"

    output=`tar -xf "$file" .PKGINFO -O`
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
    echo -e "Fs_PropsActs Install package\tInstall package (overwrite)\tShow installed package info"

    exit 0
}

vfs_quote()
{
    string="$1"
    path="/var/cache/pacman/pkg$2"

    echo 'Fs_RunTermKeep cd "'"$path"'" && '"$string"
    exit $?
}

vfs_localname()
{
    file="/var/cache/pacman/pkg$1"

    echo "$file"
    exit $?
}

case "$1" in
    setopt) vfs_setopt "$2" "$3";;
    list) vfs_list "$2";;
    copyout) vfs_copyout "$2" "$3";;
    rm) vfs_rm "$2";;
    properties) vfs_properties "$2";;
    quote) vfs_quote "$2" "$3";;
    localname) vfs_localname "$2";;
esac
exit 1