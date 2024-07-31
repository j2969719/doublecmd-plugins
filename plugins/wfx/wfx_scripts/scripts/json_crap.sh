#!/bin/sh

vfs_init()
{
    which jq >/dev/null 2>&1 || echo "Fs_Info_Message WFX_SCRIPT_STR_INSTALL_JQ"
    echo -e "Fs_SelectFile WFX_SCRIPT_STR_SELECTFILE\tWFX_SCRIPT_STR_MASKJSON|*.json|WFX_SCRIPT_STR_MASKALL|*.*"
    exit $?
}

vfs_setopt()
{
    option="$1"
    value="$2"

    case "$option" in
        "WFX_SCRIPT_STR_SELECTFILE") echo "Fs_Set_DC_WFX_SCRIPT_JSON $value" ;;
    esac

    exit 0
}

vfs_list()
{
    json_path=`echo "$1"  | sed 's#/\[#\[#' | sed 's#/\([^/]\+\)#["\1"]#g' | sed 's#^/\?#\.#'`
    keys=`jq -r ''$json_path' | keys' "$DC_WFX_SCRIPT_JSON"`

    echo "$keys" | grep -Po '^  ".*' | sed 's/^  "\([^"]\+\)",\?/drwxr-xr-x 0000-00-00 00:00:00 - \1/'
    echo "$keys" | grep -Po '^  \d+' | sed 's/^  \([0-9]\+\),\?/drwxr-xr-x 0000-00-00 00:00:00 - [\1]/'
    echo "0644 0000-00-00 00:00:00 - $ENV_WFX_SCRIPT_STR_DATA.json"
    exit $?
}

vfs_copyout()
{
    json_path=`dirname "$1" | sed 's#/\[#\[#' | sed 's#/\([^/]\+\)#["\1"]#g' | sed 's#^/\?#\.#'`
    echo "$json_path"
    dst="$2"

    jq ''$json_path'' "$DC_WFX_SCRIPT_JSON" > "$dst"
    exit $?
}

vfs_fileexists()
{
    json_path=`dirname "$1" | sed 's#/\[#\[#' | sed 's#/\([^/]\+\)#["\1"]#g' | sed 's#^/\?#\.#'`

    jq ''$json_path'' "$DC_WFX_SCRIPT_JSON"
    exit $?
}

vfs_copyin()
{
    data=`cat "$1"`
    json_path=`dirname "$2" | sed 's#/\[#\[#' | sed 's#/\([^/]\+\)#["\1"]#g' | sed 's#^/\?#\.#'`

    new_data=`jq -r --argjson data "$data" ''$json_path' = $data' "$DC_WFX_SCRIPT_JSON"`
    ["$new_data" != "\n"] exit 1 || echo "$new_data" > "$DC_WFX_SCRIPT_JSON"
    exit $?
}

vfs_cp()
{
    json_path=`dirname "$1" | sed 's#/\[#\[#' | sed 's#/\([^/]\+\)#["\1"]#g' | sed 's#^/\?#\.#'`
    data=`jq ''$json_path'' "$DC_WFX_SCRIPT_JSON"`
    json_path=`dirname "$2" | sed 's#/\[#\[#' | sed 's#/\([^/]\+\)#["\1"]#g' | sed 's#^/\?#\.#'`

    new_data=`jq -r --argjson data "$data" ''$json_path' = $data' "$DC_WFX_SCRIPT_JSON"`
    ["$new_data" != "\n"] exit 1 || echo "$new_data" > "$DC_WFX_SCRIPT_JSON"
    exit $?
}

vfs_mv()
{
    src=`dirname "$1" | sed 's#/\[#\[#' | sed 's#/\([^/]\+\)#["\1"]#g' | sed 's#^/\?#\.#'`
    data=`jq ''$src'' "$DC_WFX_SCRIPT_JSON"`
    json_path=`dirname "$2" | sed 's#/\[#\[#' | sed 's#/\([^/]\+\)#["\1"]#g' | sed 's#^/\?#\.#'`

    new_data=`jq -r --argjson data "$data" ''$json_path' = $data' "$DC_WFX_SCRIPT_JSON"`
    ["$new_data" != "\n"] exit 1 || echo "$new_data" | jq -r 'del('$src')' > "$DC_WFX_SCRIPT_JSON"
    exit $?
}

vfs_rm()
{
    json_path=`dirname "$1" | sed 's#/\[#\[#' | sed 's#/\([^/]\+\)#["\1"]#g' | sed 's#^/\?#\.#'`

    new_data=`jq -r 'del('$json_path')' "$DC_WFX_SCRIPT_JSON"`
    ["$new_data" != "\n"] exit 1 || echo "$new_data" > "$DC_WFX_SCRIPT_JSON"
    exit $?
}

vfs_properties()
{
    echo -e "Fs_SelectFile WFX_SCRIPT_STR_SELECTFILE\tWFX_SCRIPT_STR_MASKJSON|*.json|WFX_SCRIPT_STR_MASKALL|*.*"
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