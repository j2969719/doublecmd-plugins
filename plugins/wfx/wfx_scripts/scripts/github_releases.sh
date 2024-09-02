#!/bin/sh

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

get_json()
{
    repo="$1"

    [ -z "$DC_WFX_SCRIPT_JSON" ] || cleanup

    tmpfile=`mktemp /tmp/wfx_ghreleases.XXXXX`
    echo "Fs_Set_DC_WFX_SCRIPT_JSON $tmpfile"
    curl "https://api.github.com/repos/$repo/releases" > "$tmpfile" || init_fail WFX_SCRIPT_STR_ERR_CURL
    err=`cat "$tmpfile" | jq -r '"\(.status): \(.message)"'`
    [ -z "$err" ] || init_fail "$err"
}

get_body()
{
    ver="\"`dirname \"${1:1}\"`\""

    [ -z "$DC_WFX_SCRIPT_TMP" ] && tmpdir=`mktemp -d /tmp/wfx_ghreleases.XXXXX` && echo "Fs_Set_DC_WFX_SCRIPT_TMP $tmpdir" ||\
        tmpdir="$DC_WFX_SCRIPT_TMP"

    tmpfile=`mktemp "$tmpdir/notes.XXXXX"`

    [ "$ver" == "\".\"" ] && ver="\"`basename \"$1\"`\""

    cat "$DC_WFX_SCRIPT_JSON" | jq -r --argjson ver "$ver" '.[] | select(.name == $ver) | .body' > "$tmpfile"
    test -s "$tmpfile" && echo "Fs_ShowOutput cat \"$tmpfile\"" || echo "Fs_InfoMessage WFX_SCRIPT_STR_ERR_BODY"
}

vfs_init()
{
    which jq >/dev/null 2>&1 || init_fail "\"jq\" WFX_SCRIPT_STR_ERR_NA"
    which curl >/dev/null 2>&1 || init_fail "\"curl\" WFX_SCRIPT_STR_ERR_NA"
    which wget >/dev/null 2>&1 || init_fail "\"wget\" WFX_SCRIPT_STR_ERR_NA"
    echo -e "Fs_PushValue WFX_SCRIPT_STR_REPO\tdoublecmd/doublecmd"
    echo "Fs_Request_Options"
    echo "Fs_GetValues_Needed"
    echo "Fs_DisableFakeDates"
    echo "WFX_SCRIPT_STR_REPO"
    exit $?
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        "WFX_SCRIPT_STR_REPO") get_json "$value" ;;
        "WFX_SCRIPT_STR_BODY") get_body "$value" ;;
        "WFX_SCRIPT_STR_CHANGEREPO") echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_REPO" ;;
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
            jq '.[] | "dr-xr-xr-x \(.published_at) - \(.name)"' |\
            sed 's/"$//' | sed 's/^"//'
    else
        cat "$DC_WFX_SCRIPT_JSON" | jq -r --argjson ver "\"${1:1}\"" \
            '.[] | select(.name == $ver) | .assets[] | "0444 \(.updated_at) \(.size) \(.name)"'
    fi
    exit $?
}

vfs_copyout()
{
    ver="\"`dirname \"${1:1}\"`\""
    asset="\"${1#/*/}\""
    url=`cat "$DC_WFX_SCRIPT_JSON" | jq -r --argjson ver "$ver" --argjson asset "$asset" \
        '.[] | select(.name == $ver) | .assets[] | select(.name == $asset) | .browser_download_url'`
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
            '.[] | select(.name == $ver) | .assets[] | select(.name == $asset) | "WFX_SCRIPT_STR_AUTHOR|\(.uploader.login)", "content_type|\(.content_type)", "WFX_SCRIPT_STR_CTIME|\(.created_at)", "WFX_SCRIPT_STR_PTIME|\(.updated_at)", "WFX_SCRIPT_STR_DLOAD|\(.download_count)", "WFX_SCRIPT_STR_SIZE|\(.size)", "url|\(.browser_download_url)"'|\
            tr '|' '\t'
    else
        ver="\"`basename \"$1\"`\""
        cat "$DC_WFX_SCRIPT_JSON" | jq -r --argjson ver "$ver" \
            '.[] | select(.name == $ver) | "WFX_SCRIPT_STR_AUTHOR|\(.author.login)", "WFX_SCRIPT_STR_CTIME|\(.created_at)", "WFX_SCRIPT_STR_PTIME|\(.published_at)", "WFX_SCRIPT_STR_TARGET|\(.target_commitish)", "WFX_SCRIPT_STR_TAG|\(.tag_name)", "url|\(.html_url)"' |\
            tr '|' '\t'
       pre=`cat "$DC_WFX_SCRIPT_JSON" | jq --argjson ver "$ver" '.[] | select(.name == $ver) | .prerelease'`
       [ "$pre" == "true" ] && echo -e "filetype\tWFX_SCRIPT_STR_PRERELEASE" || echo -e "filetype\tWFX_SCRIPT_STR_RELEASE"
    fi

    echo -e "Fs_PropsActs WFX_SCRIPT_STR_BODY\tWFX_SCRIPT_STR_CHANGEREPO"

    exit $?
}

vfs_getinfovalues()
{
    if [ "$1" != "/" ] ; then
        ver="\"`basename \"$1\"`\""
        cat "$DC_WFX_SCRIPT_JSON" | jq -r --argjson ver "$ver" \
            '.[] | select(.name == $ver) | .assets[] | "\(.name)|⬇ \(.download_count)"' |\
            tr '|' '\t'
    else
        cat "$DC_WFX_SCRIPT_JSON" | jq -r \
            '.[] | "\(.name)|" + if .prerelease == true then "!" else "*" end + "   \(.tag_name)"' |\
            tr '|' '\t'
    fi

    exit $?
}

vfs_deinit()
{
    cleanup

    [ -z "$DC_WFX_SCRIPT_TMP" ] || rm -rf "$DC_WFX_SCRIPT_TMP"

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