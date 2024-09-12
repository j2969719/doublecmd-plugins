#!/bin/sh

confname="j2969719.json"

init_fail()
{
    message="$1"

    echo "Fs_Info_Message $message"
    echo "Fs_Set_DC_WFX_SCRIPT_INITFAIL TRUE"
    exit 1
}

cleanup()
{
    test -f "$DC_WFX_SCRIPT_JSON" && rm "$DC_WFX_SCRIPT_JSON"
}

store_latest()
{
    repo="$DC_WFX_SCRIPT_REPO"
    confdir=`dirname "$COMMANDER_INI"`
    ver=`curl "https://api.github.com/repos/$repo/releases/latest" | jq '.tag_name'`
    [ -z "$ver" ] && echo "Fs_Info_Message WFX_SCRIPT_STR_ERR_CURL" && exit 1
    conf=`cat "$confdir/$confname"`
    [ -z "$conf" ] && conf="{}"
    echo "$conf" | jq --argjson ver "$ver" --argjson src "\"${0##*/}\"" --argjson repo "\"$repo\"" \
        '.[$src][$repo] = $ver'
    newconf=`echo "$conf" | jq --argjson ver "$ver" --argjson src "\"${0##*/}\"" --argjson repo "\"$repo\"" \
        '.[$src][$repo] = $ver'`
    [ -z "$newconf" ] && echo "Fs_Info_Message WFX_SCRIPT_STR_ERR_CONF" || echo "$newconf" > "$confdir/$confname"
}

remove_latest()
{
    repo="$DC_WFX_SCRIPT_REPO"
    confdir=`dirname "$COMMANDER_INI"`
    newconf=`cat "$confdir/$confname" | jq --argjson src "\"${0##*/}\"" --argjson repo "\"$repo\"" 'del(.[$src][$repo])'`
    [ -z "$newconf" ] && echo "Fs_Info_Message WFX_SCRIPT_STR_ERR_CONF" || echo "$newconf" > "$confdir/$confname"
}

check_latest()
{
    repo="$1"
    confdir=`dirname "$COMMANDER_INI"`
    conf=`cat "$confdir/$confname"`
    oldver=`echo "$conf" | jq --argjson src "\"${0##*/}\"" --argjson repo "\"$repo\"" '.[$src][$repo]'`
    if [ ! -z "$oldver" ] && [ "$oldver" != "null" ] ; then
        ver=`curl "https://api.github.com/repos/$repo/releases/latest" | jq '.tag_name'`
        if [ ! -z "$ver" ] && [ "$ver" != "null" ] && [ "$oldver" != "$ver" ] ; then
            echo "Fs_Info_Message WFX_SCRIPT_STR_LATEST $ver"
            newconf=`echo "$conf" | jq --argjson ver "$ver" --argjson src "\"${0##*/}\"" --argjson repo "\"$repo\"" \
                '.[$src][$repo] = $ver'`
            [ -z "$newconf" ] && echo "Fs_Info_Message WFX_SCRIPT_STR_ERR_CONF" || echo "$newconf" > "$confdir/$confname"
        fi
    fi
}

get_json()
{
    repo="$1"

    [ -z "$DC_WFX_SCRIPT_TMP" ] && tmpdir=`mktemp -d /tmp/wfx_ghreleases.XXXXX` && echo "Fs_Set_DC_WFX_SCRIPT_TMP $tmpdir" ||\
        tmpdir="$DC_WFX_SCRIPT_TMP"

    [ -z "$DC_WFX_SCRIPT_JSON" ] || cleanup

    tmpfile=`mktemp "$tmpdir/repo.XXXXX.json"`
    echo "Fs_Set_DC_WFX_SCRIPT_JSON $tmpfile"
    echo "Fs_Set_DC_WFX_SCRIPT_REPO $repo"
    curl "https://api.github.com/repos/$repo/releases" > "$tmpfile" || init_fail WFX_SCRIPT_STR_ERR_CURL
    err=`cat "$tmpfile" | jq -r '"\(.status): \(.message)"'`
    [ -z "$err" ] || init_fail "$err"
    check_latest "$repo"
}

get_version()
{
    ver="\"`dirname \"${1:1}\"`\""
    [ "$ver" == "\".\"" ] && ver="\"`basename \"$1\"`\""
    echo "$ver"
}

get_body()
{
    ver=`get_version "$1"`
    tmpfile=`mktemp "$DC_WFX_SCRIPT_TMP/notes.XXXXX.txt"`
    cat "$DC_WFX_SCRIPT_JSON" | jq -r --argjson ver "$ver" '.[] | select(.tag_name == $ver) | .body' > "$tmpfile"
    test -s "$tmpfile" && echo "Fs_ShowOutput cat \"$tmpfile\"" || echo "Fs_Info_Message WFX_SCRIPT_STR_ERR_BODY"
}

get_src()
{
    dst="$1"
    arc="$2"
    ver=`get_version "$DC_WFX_SCRIPT_REMOTENAME"`
    link=`cat "$DC_WFX_SCRIPT_JSON" | jq -r --argjson ver "$ver" '.[] | select(.tag_name == $ver) | '"$arc"''`
    echo "Fs_RunTermKeep wget \"$link\" -O \"$dst\""
}

get_repo()
{
    value="$1"
    repos=`curl --get --data-urlencode "q=$value" 'https://api.github.com/search/repositories' |\
         jq -r '.items[] | .full_name' | tr '\n' '\t'`
    [ -z "$repos" ] && init_fail WFX_SCRIPT_STR_ERR_CURL
    if [ "$repos" == "\t" ] ; then
         echo -e "Fs_Info_Message \"$value\" WFX_SCRIPT_STR_ERR_NOTFOUND\nFs_Request_Options\nWFX_SCRIPT_STR_SEARCH"
    else
         echo -e "Fs_MultiChoice WFX_SCRIPT_STR_REPO\t$repos"
    fi
}

vfs_init()
{
    which jq >/dev/null 2>&1 || init_fail "\"jq\" WFX_SCRIPT_STR_ERR_NA"
    which curl >/dev/null 2>&1 || init_fail "\"curl\" WFX_SCRIPT_STR_ERR_NA"
    which wget >/dev/null 2>&1 || init_fail "\"wget\" WFX_SCRIPT_STR_ERR_NA"
    echo -e "Fs_PushValue WFX_SCRIPT_STR_REPO\tdoublecmd/doublecmd"
    echo "Fs_GetValues_Needed"
    echo "Fs_DisableFakeDates"
    echo "Fs_YesNo_Message WFX_SCRIPT_STR_ASKSEARCH"
    exit $?
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        "WFX_SCRIPT_STR_ASKSEARCH") echo "Fs_Request_Options" && [ "$value" == "Yes" ] &&\
                                    echo "WFX_SCRIPT_STR_SEARCH" || echo "WFX_SCRIPT_STR_REPO" ;;
        "WFX_SCRIPT_STR_SEARCH") get_repo "$value" ;;
        "WFX_SCRIPT_STR_REPO") get_json "$value" ;;
        "WFX_SCRIPT_STR_BODY") get_body "$value" ;;
        "WFX_SCRIPT_STR_CHANGEREPO") echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_REPO" ;;
        "WFX_SCRIPT_STR_NEWSEARCH") echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_SEARCH" ;;
        "WFX_SCRIPT_STR_STORE") store_latest ;;
        "WFX_SCRIPT_STR_REMOVE") remove_latest ;;
        "WFX_SCRIPT_STR_GETZIP") echo -e "Fs_SelectFile WFX_SCRIPT_STR_ZIP\tWFX_SCRIPT_STR_ARC|*.zip\tzip\tsave" ;;
        "WFX_SCRIPT_STR_GETTARBALL") echo -e "Fs_SelectFile WFX_SCRIPT_STR_TARBALL\tWFX_SCRIPT_STR_ARC|*.tar.gz;*.tgz\ttgz\tsave" ;;
        "WFX_SCRIPT_STR_TARBALL") get_src "$value" ".tarball_url" ;;
        "WFX_SCRIPT_STR_ZIP") get_src "$value" ".zipball_url" ;;
        *) exit 1 ;;
    esac

    exit 0
}

vfs_list()
{
    [ -z "$DC_WFX_SCRIPT_INITFAIL" ] || exit 1
    [ -z "$DC_WFX_SCRIPT_JSON" ] && exit 1

    if [ "$1" == "/" ] ; then
        cat "$DC_WFX_SCRIPT_JSON" |\
            jq '.[] | "dr-xr-xr-x \(.published_at) - \(.tag_name)"' |\
            sed 's/"$//' | sed 's/^"//'
    else
        cat "$DC_WFX_SCRIPT_JSON" | jq -r --argjson ver "\"${1:1}\"" \
            '.[] | select(.tag_name == $ver) | .assets[] | "0444 \(.updated_at) \(.size) \(.name)"'
    fi
    exit $?
}

vfs_copyout()
{
    ver="\"`dirname \"${1:1}\"`\""
    asset="\"${1#/*/}\""
    url=`cat "$DC_WFX_SCRIPT_JSON" | jq -r --argjson ver "$ver" --argjson asset "$asset" \
        '.[] | select(.tag_name == $ver) | .assets[] | select(.name == $asset) | .browser_download_url'`
    dst="$2"

    wget "$url" -O "$dst"

    exit $?
}

vfs_properties()
{
    ver="\"`dirname \"${1:1}\"`\""

    if [ "$ver" != "\".\"" ] ; then
        asset="\"${1#/*/}\""
        cat "$DC_WFX_SCRIPT_JSON" | jq -r --argjson ver "$ver" --argjson asset "$asset" \
            '.[] | select(.tag_name == $ver) | .assets[] | select(.name == $asset) | "WFX_SCRIPT_STR_AUTHOR|\(.uploader.login)", "content_type|\(.content_type)", "WFX_SCRIPT_STR_CTIME|\(.created_at)", "WFX_SCRIPT_STR_PTIME|\(.updated_at)", "WFX_SCRIPT_STR_DLOAD|\(.download_count)", "WFX_SCRIPT_STR_SIZE|\(.size)", "url|\(.browser_download_url)"'|\
            tr '|' '\t'
    else
        ver="\"`basename \"$1\"`\""
        cat "$DC_WFX_SCRIPT_JSON" | jq -r --argjson ver "$ver" \
            '.[] | select(.tag_name == $ver) | "WFX_SCRIPT_STR_NAME|\(.name)", "WFX_SCRIPT_STR_AUTHOR|\(.author.login)", "WFX_SCRIPT_STR_CTIME|\(.created_at)", "WFX_SCRIPT_STR_PTIME|\(.published_at)", "WFX_SCRIPT_STR_TARGET|\(.target_commitish)", "url|\(.html_url)"' |\
            tr '|' '\t'
        pre=`cat "$DC_WFX_SCRIPT_JSON" | jq --argjson ver "$ver" '.[] | select(.tag_name == $ver) | .prerelease'`
        [ "$pre" == "true" ] && echo -e "filetype\tWFX_SCRIPT_STR_PRERELEASE" || echo -e "filetype\tWFX_SCRIPT_STR_RELEASE"
    fi

    actions=(BODY STORE REMOVE GETTARBALL GETZIP CHANGEREPO NEWSEARCH)
    string=`printf '\tWFX_SCRIPT_STR_%s' "${actions[@]}"`
    echo -e "Fs_PropsActs $string"

    exit $?
}

vfs_getinfovalues()
{
    if [ "$1" != "/" ] ; then
        ver="\"`basename \"$1\"`\""
        cat "$DC_WFX_SCRIPT_JSON" | jq -r --argjson ver "$ver" \
            '.[] | select(.tag_name == $ver) | .assets[] | "\(.name)|⬇ \(.download_count)"' |\
            tr '|' '\t'
    else
        cat "$DC_WFX_SCRIPT_JSON" | jq -r \
            '.[] | "\(.tag_name)|" + if .prerelease == true then "!" else "*" end + "   \(.name)"' |\
            tr '|' '\t'
    fi

    exit $?
}

vfs_deinit()
{
    cleanup

    [ -z "$DC_WFX_SCRIPT_TMP" ] || test -d "$DC_WFX_SCRIPT_TMP" && rm -rf "$DC_WFX_SCRIPT_TMP"

    exit 0
}


case "$1" in
    init) vfs_init;;
    setopt) vfs_setopt "$2" "$3";;
    list) vfs_list "$2";;
    copyout) vfs_copyout "$2" "$3";;
    properties) vfs_properties "$2";;
    getvalues) vfs_getinfovalues "$2";;
    deinit) vfs_deinit;;
esac
exit 1