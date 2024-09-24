#!/bin/sh
SCRIPT_DIRECTORY=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

source ./config.sh && {

clear

ln -s $( locate -Nwl1 libgcc_s_dw2-1.dll ) "$MBAA_DIR/libgcc_s_dw2-1.dll"
ln -s $( locate -Nwl1 libwinpthread-1.dll ) "$MBAA_DIR/libwinpthread-1.dll"

cd $SCRIPT_DIRECTORY/d3d9-wrapper && ./build.sh
cd $SCRIPT_DIRECTORY/just_another_modloader && ./build_release.sh
cd $SCRIPT_DIRECTORY/states_dll && ./build.sh

cd $SCRIPT_DIRECTORY/addons && ./build.sh
cd $SCRIPT_DIRECTORY/addons/example && ./build.sh
cd $SCRIPT_DIRECTORY/addons/log && ./build.sh
cd $SCRIPT_DIRECTORY/addons/actual_addon && ./build.sh
cd $SCRIPT_DIRECTORY/addons/keyboard_addon && ./build.sh

}
