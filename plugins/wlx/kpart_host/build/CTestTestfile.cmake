# CMake generated Testfile for 
# Source directory: /home/pplupo/repos/mpv_wayland/plugins/wlx/kpart_host
# Build directory: /home/pplupo/repos/mpv_wayland/plugins/wlx/kpart_host/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(appstreamtest "/usr/bin/cmake" "-DAPPSTREAMCLI=/usr/bin/appstreamcli" "-DINSTALL_FILES=/home/pplupo/repos/mpv_wayland/plugins/wlx/kpart_host/build/install_manifest.txt" "-P" "/usr/share/ECM/kde-modules/appstreamtest.cmake")
set_tests_properties(appstreamtest PROPERTIES  _BACKTRACE_TRIPLES "/usr/share/ECM/kde-modules/KDECMakeSettings.cmake;177;add_test;/usr/share/ECM/kde-modules/KDECMakeSettings.cmake;195;appstreamtest;/usr/share/ECM/kde-modules/KDECMakeSettings.cmake;0;;/home/pplupo/repos/mpv_wayland/plugins/wlx/kpart_host/CMakeLists.txt;42;include;/home/pplupo/repos/mpv_wayland/plugins/wlx/kpart_host/CMakeLists.txt;0;")
