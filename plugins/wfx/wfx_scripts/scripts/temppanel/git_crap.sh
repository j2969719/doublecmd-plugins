#!/bin/sh

vfs_init()
{
    echo -e "Fs_PushValue repo dir\t$HOME"
    echo "Fs_SelectDir repo dir"
    echo "Fs_GetValue_Needed"
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        "repo dir") git -C "$value" rev-parse --is-inside-work-tree 1>/dev/null &&\
                    echo "Fs_Set_DC_WFX_SCRIPT_DIR $value" ||\
                    echo "Fs_Info_Message $value is not a git repo.";;
        "change git dir") echo "Fs_SelectDir repo dir";;
        "show commit") echo 'Fs_ShowOutput git -C "'"$DC_WFX_SCRIPT_DIR"'" show';;
        "pull") echo 'Fs_RunTermKeep git -C "'"$DC_WFX_SCRIPT_DIR"'" pull';;
        "push") echo 'Fs_RunTermKeep git -C "'"$DC_WFX_SCRIPT_DIR"'" push';;
        "add") echo 'Fs_RunTermKeep git -C "'"$DC_WFX_SCRIPT_DIR"'" add "'"${value:1}"'"';;
        "diff") echo 'Fs_ShowOutput git -C "'"$DC_WFX_SCRIPT_DIR"'" diff "'"${value:1}"'"';;
    esac
    exit 0
}

vfs_list()
{
    path="$DC_WFX_SCRIPT_DIR""$1"
    lstree="${1:1}"

    cd "$path" && git ls-files -d --format='%(path)' | grep  "/" | cut -d/ -f1 | sort -u | awk 'system("test -e "$0)!=0 { print "dr-xr-xr-x  0000-00-00 00:00:00 - "$0 }'
    git -C "$path" ls-files -d --format='%(objectmode) 0000-00-00 00:00:00 - %(path)' | grep -v "/"
    echo "lr-xr-xr-x 0000-00-00 00:00:00 - <.>.-->"
    [ -z "$DC_WFX_SCRIPT_DIR" ] && exit 1 || find "$path" -mindepth 1 -maxdepth 1 -name "*" -printf "%M %TF %TH:%TM:%TC %s %f\n"

    exit $?
}

vfs_copyout()
{
    src="$DC_WFX_SCRIPT_DIR""$1"
    dst="$2"

    [ `basename "$1"` == "<.>.-->" ] && exit 1
    cp "$src" "$dst"
    exit $?
}

vfs_fileexists()
{
    file="$DC_WFX_SCRIPT_DIR""$1"

    [ `basename "$1"` == "<.>.-->" ] && exit 0
    test -f "$file"
    exit $?
}

vfs_copyin()
{
    src="$1"
    dst="$DC_WFX_SCRIPT_DIR""$2"

    [ `basename "$2"` == "<.>.-->" ] && exit 1
    cp "$src" "$dst"
    exit $?
}

vfs_cp()
{
    src="$DC_WFX_SCRIPT_DIR""$1"
    dst="$DC_WFX_SCRIPT_DIR""$2"

    [ `basename "$1"` == "<.>.-->" ] && exit 1
    [ `basename "$2"` == "<.>.-->" ] && exit 1
    cp "$src" "$dst"
    exit $?
}

vfs_mv()
{
    src="$DC_WFX_SCRIPT_DIR""$1"
    dst="$DC_WFX_SCRIPT_DIR""$2"

    [ `basename "$1"` == "<.>.-->" ] && exit 1
    mv "$src" "$dst"
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


    [ `basename "$1"` == "<.>.-->" ] && exit 1
    rm "$file"
    exit $?
}

vfs_rmdir()
{
    file="$DC_WFX_SCRIPT_DIR""$1"

    rmdir "$file"
    exit $?
}

vfs_openfile()
{
    file="$DC_WFX_SCRIPT_DIR""$1"

    [ `basename "$1"` == "<.>.-->" ] && echo `dirname "$file"` && exit 0

    echo "Fs_OpenTerm $file"
    exit 0
}

vfs_properties()
{
    file="$DC_WFX_SCRIPT_DIR""$1"

    [ `basename "$1"` == "<.>.-->" ] && echo "Fs_Info_Message "`dirname "$file"` && exit 0

    test -f "$file" && echo -e "Info\t"`file -b "$file"`
    echo -e "Branch\t"`git -C "$DC_WFX_SCRIPT_DIR" rev-parse --abbrev-ref HEAD`
    info="Subject	%s%nCommit hash	%H%nCommitter	%cn%nCommitter email	%ce%nСommitter date	%cd%n"
    author="Author name	%an%nAuthor email	%ae%nAuthor date	%ad%n"
    git -C "$DC_WFX_SCRIPT_DIR" log -1 --pretty=format:"$info""$author" --date=iso
    stat --printf="Size\t%'s\nMode\t%A (%a)\nModification\t%y\nBirth\t%w\nAccess\t%x\nStatus change\t%z\n" "$file"
    test -f "$file" && echo -e "content_type\t"`file -b --mime-type "$file"`
    echo -e "Fs_PropsActs change git dir\tshow commit\tchange branch\tpull\tpush\tdiff"
    exit 0
}

vfs_chmod()
{
    file="$DC_WFX_SCRIPT_DIR""$1"
    newmode="$2"

    [ `basename "$1"` == "<.>.-->" ] && exit 1
    chmod "$newmode" "$file"
    exit $?
}

vfs_quote()
{
    string="$1"
    path="$DC_WFX_SCRIPT_DIR""$2"

    echo 'Fs_RunTermKeep cd "'"$path"'" && '"$string"
    exit $?
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

    [ `basename "$1"` == "<.>.-->" ] && echo "@" && exit 0
    status=`git  -C "$DC_WFX_SCRIPT_DIR" status --short "${1:1}" | cut -c-2 | tr -d ' '`
    count=`echo "$status" | tr '\n' ';' | tr -cd ';' | wc -c`
    [ "$count" -gt 1 ] && echo "$count" || echo "$status"
    exit $?
}

vfs_deinit()
{
    exit $?
}

vfs_reset()
{
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
    quote) vfs_quote "$2" "$3";;
    localname) vfs_localname "$2";;
    getvalue) vfs_getinfovalue "$2";;
    deinit) vfs_deinit;;
    reset) vfs_reset;;
esac
exit 1