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
    feed_url="https://appimage.github.io/feed.json"

    [ -z "$DC_WFX_SCRIPT_TMP" ] && tmpdir=`mktemp -d /tmp/wfx_appimages.XXXXX` && echo "Fs_Set_DC_WFX_SCRIPT_TMP $tmpdir" ||\
        tmpdir="$DC_WFX_SCRIPT_TMP"

    [ -z "$DC_WFX_SCRIPT_JSON" ] || cleanup

    tmpfile=`mktemp "$tmpdir/feed.XXXXX.json"`
    echo "Fs_Set_DC_WFX_SCRIPT_JSON $tmpfile"
    curl "$feed_url" > "$tmpfile" || init_fail WFX_SCRIPT_STR_ERR_CURL
}

vfs_init()
{
    which jq >/dev/null 2>&1 || init_fail "\"jq\" WFX_SCRIPT_STR_ERR_NA"
    which curl >/dev/null 2>&1 || init_fail "\"curl\" WFX_SCRIPT_STR_ERR_NA"
    which wget >/dev/null 2>&1 || init_fail "\"wget\" WFX_SCRIPT_STR_ERR_NA"
    echo "Fs_GetValues_Needed"
    get_json
    exit $?
}

vfs_list()
{
    [ -z "$DC_WFX_SCRIPT_INITFAIL" ] || exit 1
    [ -z "$DC_WFX_SCRIPT_JSON" ] && exit 1

    if [ "$1" == "/" ] ; then
        jsondate="\"`stat -c "%w" "$DC_WFX_SCRIPT_JSON" | cut -d. -f1`\""
        cat "$DC_WFX_SCRIPT_JSON" |jq -r --argjson jsondate "$jsondate" \
            '.items[] | select(.links[1].url != null) | "dr-xr-xr-x " + $jsondate + " - \(.name)"'
            #         ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    else
        name="${1:1}"
        repo=`cat "$DC_WFX_SCRIPT_JSON" | jq -r --argjson name "\"$name\"" \
            '.items[] | select(.name == $name) | .links[] | select(.type == "GitHub") | .url'`
        [ -z "$repo" ] && exit 1
        test -f "$DC_WFX_SCRIPT_TMP/$name.json" || curl -L "https://api.github.com/repos/$repo/releases/latest" > "$DC_WFX_SCRIPT_TMP/$name.json"
        if [[ -s "$DC_WFX_SCRIPT_TMP/$name.json" ]] ; then
            cat "$DC_WFX_SCRIPT_TMP/$name.json" | jq -r '.assets[] | select(.name | test("appimage$"; "i")) | "0755 \(.updated_at) \(.size) \(.name)"'
        else                                                                       #contains("AppImage")
            rm "$DC_WFX_SCRIPT_TMP/$name.json"
        fi
    fi
    exit $?
}

vfs_copyout()
{
    app="`dirname \"${1:1}\"`"
    asset="\"${1#/*/}\""
    url=`cat "$DC_WFX_SCRIPT_TMP/$app.json" | jq -r --argjson asset "$asset" \
        '.assets[] | select(.name == $asset) | .browser_download_url'`
    dst="$2"

    trap 'pkill --signal SIGTERM --parent $$' EXIT
    wget "$url" -O "$dst" 2>&1 && chmod 755 "$dst"
    #curl -L "$url" --output "$dst" 2>&1 && chmod 755 "$dst"
                                      #  ^^^^^^^^^^^^^^^^^^^ so safe, much secure
    exit $?
}

vfs_properties()
{
    app="`dirname \"${1:1}\"`"

    if [ "$app" != "." ] ; then
        asset="\"`basename \"$1\"`\""
        cat "$DC_WFX_SCRIPT_TMP/$app.json" | jq -r --argjson asset "$asset"  \
            ' .assets[] | select(.name == $asset) | "WFX_SCRIPT_STR_AUTHOR|\(.uploader.login)", "WFX_SCRIPT_STR_CTIME|\(.created_at)", "WFX_SCRIPT_STR_PTIME|\(.updated_at)", "WFX_SCRIPT_STR_DLOAD|\(.download_count)", "WFX_SCRIPT_STR_SIZE|\(.size)", "url|\(.browser_download_url)"'|\
            tr '|' '\t'
    else
        name=`basename "$1"`
        app="\"$name\""
        json=`cat "$DC_WFX_SCRIPT_JSON" | jq -r --argjson app "$app" '.items[] | select(.name == $app)'`

        test -d "$DC_WFX_SCRIPT_TMP/icons" || mkdir "$DC_WFX_SCRIPT_TMP/icons"
        icon=`echo "$json" | jq -r '.icons[0]'`
        src="$DC_WFX_SCRIPT_TMP/icons/"$name"_src.png"
        dst="$DC_WFX_SCRIPT_TMP/icons/"$name".png"
        test -f "$src" || curl "https://appimage.github.io/database/"$icon --output "$src"
        test -f "$dst" || convert -resize x48 "$src" "$dst"
        test -f "$dst" && echo -e "png: \t$dst"

        echo "$json" | jq -r '. | "WFX_SCRIPT_STR_AUTHOR|\(.authors[].name)", "WFX_SCRIPT_STR_INFO|\(.description)", "filetype|\(.categories[0])", "WFX_SCRIPT_STR_LIC|\(.license)", "url|\(.links[1].url)"'| tr '|' '\t'

        test -d "$DC_WFX_SCRIPT_TMP/screenshots" || mkdir "$DC_WFX_SCRIPT_TMP/screenshots"
        icon=`echo "$json" | jq -r '.screenshots[0]'`
        src="$DC_WFX_SCRIPT_TMP/screenshots/"$name"_src.png"
        dst="$DC_WFX_SCRIPT_TMP/screenshots/"$name".jpg"
        test -f "$src" || curl "https://appimage.github.io/database/"$icon --output "$src"
        test -f "$dst" || convert -resize 400x "$src" "$dst"
        test -f "$dst" && echo -e "png: \t$dst"

        #echo "$json" > "$DC_WFX_SCRIPT_TMP/"$name"_slice.json"
    fi

    exit $?
}

vfs_getinfovalues()
{
    if [ "$1" != "/" ] ; then
        app="`basename \"$1\"`"
        ver="\"`cat "$DC_WFX_SCRIPT_TMP/$app.json" | jq -r '.tag_name'`\""
        cat "$DC_WFX_SCRIPT_TMP/$app.json" | jq -r --argjson ver "$ver" \
            '.assets[] | "\(.name)|" + $ver + "    ⬇ \(.download_count)"' | tr '|' '\t'
    else
        cat "$DC_WFX_SCRIPT_JSON" | jq -r '.items[] | "\(.name)|\(.categories[0])"' | tr '|' '\t'
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
    list) vfs_list "$2";;
    copyout) vfs_copyout "$2" "$3";;
    properties) vfs_properties "$2";;
    getvalues) vfs_getinfovalues "$2";;
    deinit) vfs_deinit;;
esac
exit 1