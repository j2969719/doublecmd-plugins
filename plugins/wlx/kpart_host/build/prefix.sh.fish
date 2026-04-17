# SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-3-Clause

set PATH "/usr/local/bin:$PATH"

# LD_LIBRARY_PATH only needed if you are building without rpath
# set -x LD_LIBRARY_PATH "/usr/local/lib:$LD_LIBRARY_PATH"

if test -z "$XDG_DATA_DIRS"
    set -x --path XDG_DATA_DIRS /usr/local/share/ /usr/share/
end
set -x --path XDG_DATA_DIRS "/usr/local/share" $XDG_DATA_DIRS

if test -z "$XDG_CONFIG_DIRS"
    set -x --path XDG_CONFIG_DIRS /etc/xdg
end
set -x --path XDG_CONFIG_DIRS "/usr/local/etc/xdg" $XDG_CONFIG_DIRS

set -x --path QT_PLUGIN_PATH "/usr/local/lib/plugins" $QT_PLUGIN_PATH
set -x --path QML2_IMPORT_PATH "/usr/local/lib/qml" $QML2_IMPORT_PATH

set -x --path QT_QUICK_CONTROLS_STYLE_PATH "/usr/local/lib/qml/QtQuick/Controls.2/" $QT_QUICK_CONTROLS_STYLE_PATH
