#!/bin/sh

pling_url="https://www.appimagehub.com" # branch of yellowyoutube.com
tags=appimage

init_fail()
{
    message="$1"

    echo "Fs_Info_Message $message"
    echo "Fs_Set_DC_WFX_SCRIPT_INITFAIL TRUE"
    exit 1
}

cleanup()
{
    test -f "$DC_WFX_SCRIPT_XML" && rm "$DC_WFX_SCRIPT_XML"
}

get_xml()
{
    string="$1"
    [ -z "$DC_WFX_SCRIPT_TMP" ] && tmpdir=`mktemp -d /tmp/wfx_pling.XXXXX` && echo "Fs_Set_DC_WFX_SCRIPT_TMP $tmpdir" ||\
        tmpdir="$DC_WFX_SCRIPT_TMP"

    [ -z "$DC_WFX_SCRIPT_XML" ] || cleanup

    [ -z "$DC_WFX_SCRIPT_PAGESIZE" ] && pagesize=20 && echo "Fs_Set_DC_WFX_SCRIPT_PAGESIZE $pagesize" ||\
        pagesize=$DC_WFX_SCRIPT_PAGESIZE

    tmpfile=`mktemp $tmpdir/page.XXXXX.xml`
    echo "Fs_Set_DC_WFX_SCRIPT_XML $tmpfile"
    echo "Fs_Set_DC_WFX_SCRIPT_PAGE 0"
    echo "Fs_Set_DC_WFX_SCRIPT_STR $string"
    curl --get --data-urlencode "search=$string" --data-urlencode "sortmode=new" --data-urlencode "tags=$tags" --data-urlencode "pagesize=$pagesize" \
        "$pling_url/ocs/v1/content/data" > "$tmpfile" || init_fail WFX_SCRIPT_STR_ERR_CURL
    err=`xmllint --xpath '//status/text()' "$tmpfile"`
    [ ! -z "$err" ] && [ "$err" != "ok"] init_fail `xmllint --xpath '//message/text()' "$tmpfile"`
}

get_newpage()
{
    page="$1"

    [ -z "$2" ] && pagesize=$DC_WFX_SCRIPT_PAGESIZE || pagesize=$2
    echo "Fs_Set_DC_WFX_SCRIPT_PAGE $page"
    curl --get --data-urlencode "search=$DC_WFX_SCRIPT_STR" --data-urlencode "sortmode=new" --data-urlencode "pagesize=$pagesize" \
        --data-urlencode "page=$page" "$pling_url/ocs/v1/content/data" > "$DC_WFX_SCRIPT_XML"
    exit 0
}

get_prop()
{
    src="$1"
    caption="$2"
    value="$3"
    iter="$4"
    [ "$iter" == "dlfile" ] && xpath=`build_xpath_iter "$src" "$value"` || xpath=`build_xpath "$src" "$value"`
    xmllint --xpath "$xpath" "$DC_WFX_SCRIPT_XML" | sed 's/^/'"$caption"'|/' | tr '|' '\t'
}

build_xpath_iter()
{
    name="$1"
    value="$2"
    xpath='//data/content[downloadname1="'"$name"'"]/'"$value"'1/text()'
    for i in `seq 2 $DC_WFX_SCRIPT_DLFILES`; do
        xpath=$xpath'|//data/content[downloadname'$i'="'"$name"'"]/'"$value"$i'/text()'
    done
    echo "$xpath"
}

build_xpath()
{
    name="$1"
    value="$2"
    xpath='//data/content[downloadname1="'"$name"'"]/'"$value"'/text()'
    for i in `seq 2 $DC_WFX_SCRIPT_DLFILES`; do
        xpath=$xpath'|//data/content[downloadname'$i'="'"$name"'"]/'"$value"'/text()'
    done
    echo "$xpath"
}

vfs_init()
{
    which xmllint >/dev/null 2>&1 || init_fail "\"xmllint\" WFX_SCRIPT_STR_ERR_NA"
    which curl >/dev/null 2>&1 || init_fail "\"curl\" WFX_SCRIPT_STR_ERR_NA"
    which wget >/dev/null 2>&1 || init_fail "\"wget\" WFX_SCRIPT_STR_ERR_NA"
    echo "Fs_GetValue_Needed"
    echo "Fs_DisableFakeDates"
    echo "Fs_Request_Options"
    echo "WFX_SCRIPT_STR_SEARCH"
    echo "Fs_Set_DC_WFX_SCRIPT_DLFILES 10"
    exit $?
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        "WFX_SCRIPT_STR_SEARCH") get_xml "$value" ;;
        "WFX_SCRIPT_STR_ACT_CHANGE") echo -e "Fs_Request_Options\nWFX_SCRIPT_STR_SEARCH" ;;
        "WFX_SCRIPT_STR_ACT_PAGESIZE") echo -e "Fs_EditLine WFX_SCRIPT_STR_PAGESIZE\t$DC_WFX_SCRIPT_PAGESIZE" ;;
        "WFX_SCRIPT_STR_ACT_DLFILES") echo -e "Fs_EditLine WFX_SCRIPT_STR_DLFILES\t$DC_WFX_SCRIPT_DLFILES" ;;
        "Fs_EditLine WFX_SCRIPT_STR_PAGESIZE") (( value > 0 )) && echo "Fs_Set_DC_WFX_SCRIPT_PAGESIZE $((value))" &&\
                               get_newpage "$DC_WFX_SCRIPT_PAGE" "$((value))" ;;
        "Fs_EditLine WFX_SCRIPT_STR_DLFILES") (( value > 0 )) && echo "Fs_Set_DC_WFX_SCRIPT_DLFILES $((value))" ;;
        *) exit 1 ;;
    esac

    exit 0
}

vfs_list()
{
    [ -z "$DC_WFX_SCRIPT_INITFAIL" ] || exit 1
    [ -z "$DC_WFX_SCRIPT_XML" ] && exit 1

    total=`xmllint --xpath '//totalitems/text()' "$DC_WFX_SCRIPT_XML"`
    num=$((DC_WFX_SCRIPT_PAGE+1))
    shown=$((DC_WFX_SCRIPT_PAGESIZE*num))
    (( total > shown )) && echo "lr-xr-xr-x 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_NEXT<.sh"
    (( DC_WFX_SCRIPT_PAGE != 0 )) && echo "lr-xr-xr-x 0000-00-00 00:00:00 - >$ENV_WFX_SCRIPT_STR_PREV<.sh"

    for i in `seq 1 $DC_WFX_SCRIPT_DLFILES`; do
        names=$names"\n"`xmllint --xpath '//data/content/downloadname'$i'/text()' "$DC_WFX_SCRIPT_XML" 2>/dev/null`
    done
    names=`echo -e "$names" | sort | uniq`
    for name in ${names//\\n/}; do
        xpath=`build_xpath_iter "$name" "downloadsize"`
        size=`xmllint --xpath "$xpath" "$DC_WFX_SCRIPT_XML" 2>/dev/null | tail -n 1 |sed -r 's/^([0-9]+).*/\1/'`
        xpath=`build_xpath "$name" "created"`
        echo $mtime
        mtime=`xmllint --xpath "$xpath" "$DC_WFX_SCRIPT_XML" 2>/dev/null | tail -n 1 | sed -r 's/^([0-9\-]+T[0-9:]+).+/\1/'`
        echo -e "0444\t$mtime\t"$size"000\t$name"
    done

    exit $?
}

vfs_copyout()
{
    src="${1:1}"
    dst="$2"
    xpath=`build_xpath_iter "$src" "downloadlink"`
    url=`xmllint --xpath "$xpath" "$DC_WFX_SCRIPT_XML"`

    wget "$url" -O "$dst" 2>/dev/null

    exit $?
}

vfs_openfile()
{
    if [ "$1" == "/>$ENV_WFX_SCRIPT_STR_NEXT<.sh" ] ; then
        DC_WFX_SCRIPT_PAGE=$((DC_WFX_SCRIPT_PAGE+1))
        get_newpage "$DC_WFX_SCRIPT_PAGE"
    elif [ "$1" == "/>$ENV_WFX_SCRIPT_STR_PREV<.sh" ] ; then
        DC_WFX_SCRIPT_PAGE=$((DC_WFX_SCRIPT_PAGE-1))
        get_newpage "$DC_WFX_SCRIPT_PAGE"
    fi
    exit 1
}

vfs_properties()
{
    cache="$DC_WFX_SCRIPT_TMP/${1:1}_props"

    if [ "$1" == "/>$ENV_WFX_SCRIPT_STR_NEXT<.sh" ] || [ "$1" == "/>$ENV_WFX_SCRIPT_STR_PREV<.sh" ] ; then
        echo -e "filetype\tWFX_SCRIPT_STR_MAGICBUTTON"
    elif test -f "$cache" ; then
        cat "$cache"
    else
        tmpfile=`mktemp $DC_WFX_SCRIPT_TMP/smallpic1.XXXXX.png`
        xpath=`build_xpath "${1:1}" "smallpreviewpic1"`
        url=`xmllint --xpath "$xpath" "$DC_WFX_SCRIPT_XML"`
        wget "$url" -O "$tmpfile"
        echo -e "png: \t$tmpfile" > "$cache"
        tmpfile=`mktemp $DC_WFX_SCRIPT_TMP/preview.XXXXX.png`
        xpath=`build_xpath "${1:1}" "previewpic2"`
        url=`xmllint --xpath "$xpath" "$DC_WFX_SCRIPT_XML"`
        wget "$url" -O "$tmpfile"
        echo -e "png: \t$tmpfile" >> "$cache"
        get_prop "${1:1}" "WFX_SCRIPT_STR_NAME" "name" >> "$cache"
        get_prop "${1:1}" "WFX_SCRIPT_STR_VER" "download_version" "dlfile" >> "$cache"
        get_prop "${1:1}" "WFX_SCRIPT_STR_TYPE" "typename" >> "$cache"
        get_prop "${1:1}" "WFX_SCRIPT_STR_SUMMARY" "summary" >> "$cache"
        get_prop "${1:1}" " " "description" >> "$cache"
        get_prop "${1:1}" "url" "detailpage" >> "$cache"
        get_prop "${1:1}" "WFX_SCRIPT_STR_ARCH" "download_package_arch" "dlfile" >> "$cache"
        get_prop "${1:1}" "WFX_SCRIPT_STR_CTIME" "created" >> "$cache"
        get_prop "${1:1}" "WFX_SCRIPT_STR_MTIME" "changed" >> "$cache"
        get_prop "${1:1}" "WFX_SCRIPT_STR_DLOADS" "downloads" >> "$cache"
        get_prop "${1:1}" "WFX_SCRIPT_STR_SCORE" "score" >> "$cache"
        get_prop "${1:1}" "WFX_SCRIPT_STR_PRICE" "downloadprice" "dlfile" >> "$cache"
        get_prop "${1:1}" "WFX_SCRIPT_STR_TAGS" "tags" >> "$cache"
        cat "$cache"
    fi
    echo -e "WFX_SCRIPT_STR_PAGE\t$((DC_WFX_SCRIPT_PAGE+1))"
    echo -e "Fs_PropsActs WFX_SCRIPT_STR_ACT_CHANGE\tWFX_SCRIPT_STR_ACT_PAGESIZE\tWFX_SCRIPT_STR_ACT_DLFILES"
    exit 0
}

vfs_getinfovalue()
{
    if [ "$1" == "/>$ENV_WFX_SCRIPT_STR_NEXT<.sh" ] ; then
        echo "WFX_SCRIPT_STR_PAGE "$((DC_WFX_SCRIPT_PAGE+2))
    elif [ "$1" == "/>$ENV_WFX_SCRIPT_STR_PREV<.sh" ] ; then
        echo "WFX_SCRIPT_STR_PAGE "$DC_WFX_SCRIPT_PAGE
    else
        xpath=`build_xpath_iter "${1:1}" "download_version"`
        xmllint --xpath "$xpath" "$DC_WFX_SCRIPT_XML"
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
    run) vfs_openfile "$2";;
    properties) vfs_properties "$2";;
    getvalue) vfs_getinfovalue "$2";;
    deinit) vfs_deinit;;
esac
exit 1