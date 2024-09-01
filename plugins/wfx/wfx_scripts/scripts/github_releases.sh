#!/bin/sh

init_fail()
{
    message="$1"

    echo "Fs_Info_Message $message"
    echo "Fs_Set_DC_WFX_SCRIPT_INITFAIL TRUE"
    exit 1
}

get_json()
{
    repo="$1"
    tmpfile=`mktemp /tmp/wfx_ghreleases.XXXXX`
    echo "Fs_Set_DC_WFX_SCRIPT_JSON $tmpfile"
    curl "https://api.github.com/repos/$repo/releases" > "$tmpfile"
}

vfs_init()
{
    which jq >/dev/null 2>&1 || init_fail "\"jq\" WFX_SCRIPT_STR_ERR_NA"
    which curl >/dev/null 2>&1 || init_fail "\"curl\" WFX_SCRIPT_STR_ERR_NA"
    which wget >/dev/null 2>&1 || init_fail "\"wget\" WFX_SCRIPT_STR_ERR_NA"
    echo -e "Fs_PushValue WFX_SCRIPT_STR_REPO\tdoublecmd/doublecmd"
    echo "Fs_Request_Options"
    echo "WFX_SCRIPT_STR_REPO"
    exit $?
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        "WFX_SCRIPT_STR_REPO") get_json "$value" ;;
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
        cat "$DC_WFX_SCRIPT_JSON" | jq --argjson ver "\"${1:1}\"" \
            '.[] | select(.name == $ver) | .assets[] | "0444 \(.updated_at) \(.size) \(.name)"' |\
            sed 's/"$//' | sed 's/^"//'
    fi
    exit $?
}

vfs_copyout()
{
    ver="\"`dirname \"${1:1}\"`\""
    asset="\"${1#/*/}\""
    url=`cat "$DC_WFX_SCRIPT_JSON" | jq --argjson ver "$ver" --argjson asset "$asset" \
        '.[] | select(.name == $ver) | .assets[] | select(.name == $asset) | .browser_download_url'|\
        sed 's/"$//' | sed 's/^"//'`
    dst="$2"

    wget "$url" -O "$dst"

    exit $?
}

vfs_properties()
{
    ver="\"`dirname \"${1:1}\"`\""

    if [ "$ver" != "\".\"" ] ; then
        asset="\"${1#/*/}\""
        cat "$DC_WFX_SCRIPT_JSON" | jq --argjson ver "$ver" --argjson asset "$asset" \
            '.[] | select(.name == $ver) | .assets[] | select(.name == $asset) | "WFX_SCRIPT_STR_AUTHOR|\(.uploader.login)", "content_type|\(.content_type)", "WFX_SCRIPT_STR_CTIME|\(.created_at)", "WFX_SCRIPT_STR_PTIME|\(.updated_at)", "WFX_SCRIPT_STR_DLOAD|\(.download_count)", "WFX_SCRIPT_STR_SIZE|\(.size)", "url|\(.browser_download_url)"'|\
            sed 's/"$//' | sed 's/^"//' | tr '|' '\t'
    else
        ver="\"`basename \"$1\"`\""
        cat "$DC_WFX_SCRIPT_JSON" | jq --argjson ver "$ver" \
            '.[] | select(.name == $ver) | "WFX_SCRIPT_STR_AUTHOR|\(.author.login)", "WFX_SCRIPT_STR_CTIME|\(.created_at)", "WFX_SCRIPT_STR_PTIME|\(.published_at)", "WFX_SCRIPT_STR_TARGET|\(.target_commitish)", "WFX_SCRIPT_STR_TAG|\(.tag_name)", "url|\(.html_url)"' |\
            sed 's/"$//' | sed 's/^"//' | tr '|' '\t'
       pre=`cat "$DC_WFX_SCRIPT_JSON" | jq --argjson ver "$ver" '.[] | select(.name == $ver) | .prerelease'`
       [ "$pre" == "null" ] && echo -e "filetype\tWFX_SCRIPT_STR_PRERELEASE" || echo -e "filetype\tWFX_SCRIPT_STR_RELEASE"
    fi

    exit 0
}

vfs_deinit()
{
    test -f "$DC_WFX_SCRIPT_JSON" && rm "$DC_WFX_SCRIPT_JSON"
    exit 0
}


case "$1" in
    init) vfs_init;;
    setopt) vfs_setopt "$2" "$3";;
    list) vfs_list "$2";;
    copyout) vfs_copyout "$2" "$3";;
    properties) vfs_properties "$2";;
    deinit) vfs_deinit;;
esac
exit 1