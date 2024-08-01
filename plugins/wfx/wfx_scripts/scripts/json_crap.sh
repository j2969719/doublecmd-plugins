#!/bin/sh

path_to_jsonpath()
{
    sed 's#/\([^\[][^/]*\)#["\1"]#g' | sed 's#/\[#\[#g' | sed 's#^/\?#\.#'
}

jsonfile_select()
{
    echo -e "Fs_SelectFile WFX_SCRIPT_STR_SELECTFILE\tWFX_SCRIPT_STR_MASKJSON|*.json|WFX_SCRIPT_STR_MASKALL|*"
}

vfs_init()
{
    which jq >/dev/null 2>&1 || echo "Fs_Info_Message WFX_SCRIPT_STR_INSTALL_JQ"
    jsonfile_select
    exit $?
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        "WFX_SCRIPT_STR_SELECTFILE") echo -e "Fs_Set_DC_WFX_SCRIPT_JSON $value\nFs_YesNo_Message WFX_SCRIPT_STR_OPENRO" ;;
        "WFX_SCRIPT_STR_OPENRO") echo "Fs_Set_DC_WFX_SCRIPT_GUARD $value" ;;
    esac

    exit 0
}

vfs_list()
{
    [ -z "$DC_WFX_SCRIPT_JSON" ] && exit 1
    [ -f "$DC_WFX_SCRIPT_JSON" ] || exit 1
    json_path=`path_to_jsonpath <<< "$1"`
    keys=`jq -r ''"$json_path"' | keys' "$DC_WFX_SCRIPT_JSON"`

    echo "$keys" | grep -Po '^  ".*' | sed 's/^  "\([^"]\+\)",\?/drwxr-xr-x 0000-00-00 00:00:00 - \1/'
    echo "$keys" | grep -Po '^  \d+' | sed 's/^  \([0-9]\+\),\?/drwxr-xr-x 0000-00-00 00:00:00 - [\1]/'
    echo "0644 0000-00-00 00:00:00 - $ENV_WFX_SCRIPT_STR_DATA.json"
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

    new_data=`jq -r --argjson data "$data" ''"$json_path"' = $data' "$DC_WFX_SCRIPT_JSON"`
    [ -z "$new_data" ] && exit 1 || echo "$new_data" > "$DC_WFX_SCRIPT_JSON"
    exit $?
}

vfs_cp()
{
    [ "$DC_WFX_SCRIPT_GUARD" == "Yes" ] && exit 1
    json_path=`dirname "$1" | path_to_jsonpath`
    data=`jq ''"$json_path"'' "$DC_WFX_SCRIPT_JSON"`
    json_path=`dirname "$2" | path_to_jsonpath`

    new_data=`jq -r --argjson data "$data" ''"$json_path"' = $data' "$DC_WFX_SCRIPT_JSON"`
    [ -z "$new_data" ] && exit 1 || echo "$new_data" > "$DC_WFX_SCRIPT_JSON"
    exit $?
}

vfs_mv()
{
    [ "$DC_WFX_SCRIPT_GUARD" == "Yes" ] && exit 1
    src=`dirname "$1" | path_to_jsonpath`
    data=`jq ''$src'' "$DC_WFX_SCRIPT_JSON"`
    json_path=`dirname "$2" | path_to_jsonpath`

    new_data=`jq -r --argjson data "$data" ''"$json_path"' = $data' "$DC_WFX_SCRIPT_JSON"`
    [ -z "$new_data" ] && exit 1 || echo "$new_data" | jq -r 'del('$src')' > "$DC_WFX_SCRIPT_JSON"
    exit $?
}

vfs_rm()
{
    [ "$DC_WFX_SCRIPT_GUARD" == "Yes" ] && exit 1
    json_path=`dirname "$1" | path_to_jsonpath`

    new_data=`jq -r 'del('"$json_path"')' "$DC_WFX_SCRIPT_JSON"`
    [ -z "$new_data" ] && exit 1 || echo "$new_data" > "$DC_WFX_SCRIPT_JSON"
    exit $?
}

vfs_properties()
{
    jsonfile_select
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
esac
exit 1