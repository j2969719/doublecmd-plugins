#!/bin/sh

path_to_jsonpath()
{
    sed 's#/\([^\[][^/]*\)#["\1"]#g' | sed 's#/\[#\[#g' | sed 's#^/\?#\.#'
}

jsonfile_select()
{
    echo -e "Fs_SelectFile WFX_SCRIPT_STR_SELECTFILE\tWFX_SCRIPT_STR_MASKJSON|*.json|WFX_SCRIPT_STR_MASKALL|*"
}

pyjson5_shenanigans()
{
    [ -z "$DC_WFX_SCRIPT_JSON" ] && exit 1

    tmpjson=`mktemp /tmp/wfx-json5data.XXXXXX` &&\
        echo "Fs_Set_DC_WFX_SCRIPT_JSON5 $DC_WFX_SCRIPT_JSON" &&\
        cat "$DC_WFX_SCRIPT_JSON" | pyjson5 --as-json > "$tmpjson" &&\
        echo "Fs_Set_DC_WFX_SCRIPT_JSON $tmpjson"
}

replace_jsondata()
{
    json_path="$1"
    data="$2"

    if [[ "$json_path" == "." ]] ; then
        echo $data | jq .
    else
        jq -r --argjson data "$data" ''"$json_path"' = $data' "$DC_WFX_SCRIPT_JSON"
    fi
}

write_jsondata()
{
    echo "$1" > "$DC_WFX_SCRIPT_JSON"
    [ -z "$DC_WFX_SCRIPT_JSON5" ] || echo "$1" | pyjson5 --indent 2 $DC_WFX_SCRIPT_COMMA > "$DC_WFX_SCRIPT_JSON5"
}

cleanup()
{
    if ! [[ -z "$DC_WFX_SCRIPT_JSON5" ]] ; then
        echo "Fs_Set_DC_WFX_SCRIPT_JSON5 "
        [ -z "$DC_WFX_SCRIPT_JSON" ] || rm "$DC_WFX_SCRIPT_JSON"
    fi
}

prop_actions()
{
    option="$1"

    case "$option" in
        "WFX_SCRIPT_STR_CHANGEFILE") jsonfile_select ;;
        "WFX_SCRIPT_STR_CHANGERO") echo "Fs_YesNo_Message WFX_SCRIPT_STR_OPENRO" ;;
        "WFX_SCRIPT_STR_CHANGECOMMA") echo "Fs_YesNo_Message WFX_SCRIPT_STR_COMMA" ;;
        "WFX_SCRIPT_STR_USEPYJSON5") which pyjson5 >/dev/null 2>&1 && pyjson5_shenanigans ||\
                                         echo "Fs_Info_Message WFX_SCRIPT_STR_INSTALL_PYJSON5" ;;
    esac
}


vfs_init()
{
    which jq >/dev/null 2>&1 || echo "Fs_Info_Message WFX_SCRIPT_STR_INSTALL_JQ"
    echo "Fs_Set_DC_WFX_SCRIPT_COMMA --no-trailing-commas"
    jsonfile_select
    exit $?
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        "WFX_SCRIPT_STR_SELECTFILE") cleanup && echo "Fs_Set_DC_WFX_SCRIPT_JSON $value" &&\
                                     echo "Fs_YesNo_Message WFX_SCRIPT_STR_OPENRO" ;;
        "WFX_SCRIPT_STR_OPENRO") echo "Fs_Set_DC_WFX_SCRIPT_GUARD $value" ;;
        "WFX_SCRIPT_STR_COMMA") [ "$value" == "No" ] &&\
					echo "Fs_Set_DC_WFX_SCRIPT_COMMA --no-trailing-commas" ||\
					echo "Fs_Set_DC_WFX_SCRIPT_COMMA " ;;
        "WFX_SCRIPT_STR_ACTS") prop_actions $value ;;
    esac

    exit 0
}

vfs_list()
{
    [ -z "$DC_WFX_SCRIPT_JSON" ] && exit 1
    [ -f "$DC_WFX_SCRIPT_JSON" ] || exit 1

    if [[ "$DC_WFX_SCRIPT_GUARD" == "Yes" ]] ; then
        fake_attr="0444"
    else
        fake_attr="0644"
    fi

    echo "$fake_attr 0000-00-00 00:00:00 - $ENV_WFX_SCRIPT_STR_DATA.json"

    if [[ "$DC_WFX_SCRIPT_MULTIFILEOP" == "delete" ]] ; then
        exit 0
    fi

    json_path=`path_to_jsonpath <<< "$1"`
    keys=`jq -r ''"$json_path"' | keys' "$DC_WFX_SCRIPT_JSON"`

    if [[ "$DC_WFX_SCRIPT_GUARD" == "Yes" ]] ; then
        fake_attr="dr-xr-xr-x"
    else
        fake_attr="drwxr-xr-x"
    fi

    echo "$keys" | grep -Po '^  ".*' | sed 's/^  "\([^"]\+\)",\?/'$fake_attr' 0000-00-00 00:00:00 - \1/'
    echo "$keys" | grep -Po '^  \d+' | sed 's/^  \([0-9]\+\),\?/'$fake_attr' 0000-00-00 00:00:00 - [\1]/'

    exit $?
}

vfs_copyout()
{
    json_path=`dirname "$1" | path_to_jsonpath`
    dst="$2"

    jq ''"$json_path"'' "$DC_WFX_SCRIPT_JSON" > "$dst"
    exit $?
}

vfs_fileexists()
{
    [ "$DC_WFX_SCRIPT_GUARD" == "Yes" ] && exit 1
    json_path=`dirname "$1" | path_to_jsonpath`

    jq ''"$json_path"'' "$DC_WFX_SCRIPT_JSON"
    exit $?
}

vfs_copyin()
{
    [ "$DC_WFX_SCRIPT_GUARD" == "Yes" ] && exit 1
    data=`cat "$1"`
    json_path=`dirname "$2" | path_to_jsonpath`

    new_data=`replace_jsondata "$json_path" "$data"`
    [ -z "$new_data" ] && exit 1 || write_jsondata "$new_data"
    exit $?
}

vfs_cp()
{
    [ "$DC_WFX_SCRIPT_GUARD" == "Yes" ] && exit 1
    json_path=`dirname "$1" | path_to_jsonpath`
    data=`jq ''"$json_path"'' "$DC_WFX_SCRIPT_JSON"`
    json_path=`dirname "$2" | path_to_jsonpath`

    new_data=`replace_jsondata "$json_path" "$data"`
    [ -z "$new_data" ] && exit 1 || write_jsondata "$new_data"
    exit $?
}

vfs_mv()
{
    [ "$DC_WFX_SCRIPT_GUARD" == "Yes" ] && exit 1
    src=`dirname "$1" | path_to_jsonpath`
    data=`jq ''$src'' "$DC_WFX_SCRIPT_JSON"`
    json_path=`dirname "$2" | path_to_jsonpath`

    new_data=`replace_jsondata "$json_path" "$data"`
    [ -z "$new_data" ] && exit 1
    [ "$json_path" != "." ] && new_data=`echo "$new_data" | jq -r 'del('$src')'`
    [ -z "$new_data" ] && exit 1 || write_jsondata "$new_data"
    exit $?
}

vfs_rm()
{
    [ "$DC_WFX_SCRIPT_GUARD" == "Yes" ] && exit 1
    json_path=`dirname "$1" | path_to_jsonpath`

    new_data=`jq -r 'del('"$json_path"')' "$DC_WFX_SCRIPT_JSON"`
    [ -z "$new_data" ] && exit 1 || write_jsondata "$new_data"
    exit $?
}

vfs_properties()
{
    actions=(CHANGEFILE CHANGERO USEPYJSON5 CHANGECOMMA)
    string=`printf '\tWFX_SCRIPT_STR_%s' "${actions[@]}"`
    echo -e "Fs_MultiChoice WFX_SCRIPT_STR_ACTS$string"
    exit 0
}

vfs_deinit()
{
    cleanup
    exit 0
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
    rm) vfs_rm "$2";;
    rmdir) exit 0;;
    properties) vfs_properties "$2";;
    deinit) vfs_deinit;;
esac
exit 1