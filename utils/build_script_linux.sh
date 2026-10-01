#!/bin/bash -x

# Environment prerequisites:
# 1) QT_PREFIX_PATH should be set to Qt libs folder
# 2) BOOST_ROOT should be set to the root of Boost
# 3) OPENSSL_ROOT_DIR should be set to the root of OpenSSL
#
# for example, place these lines to the end of your ~/.bashrc :
#
# export BOOST_ROOT=/home/user/boost_1_66_0
# export QT_PREFIX_PATH=/home/user/Qt5.10.1/5.10.1/gcc_64
# export OPENSSL_ROOT_DIR=/home/user/openssl

ARCHIVE_NAME_PREFIX=pdc-linux-x64-

: "${BOOST_ROOT:?BOOST_ROOT should be set to the root of Boost, ex.: /home/user/boost_1_84_0}"
: "${QT_PREFIX_PATH:?QT_PREFIX_PATH should be set to Qt libs folder, ex.: /home/user/Qt5.10.1/5.10.1/gcc_64}"
: "${OPENSSL_ROOT_DIR:?OPENSSL_ROOT_DIR should be set to OpenSSL root folder, ex.: /home/user/openssl}"

if [ -n "$build_prefix" ]; then
  ARCHIVE_NAME_PREFIX=${ARCHIVE_NAME_PREFIX}${build_prefix}-
  build_prefix_label="$build_prefix "
fi

if [ "$testnet" == true ]; then
  testnet_def="-D TESTNET=TRUE"
  testnet_label="testnet "
  ARCHIVE_NAME_PREFIX=${ARCHIVE_NAME_PREFIX}testnet-
fi

if [ "$testnet" == true ] || [ -n "$qt_dev_tools" ]; then
  copy_qt_dev_tools=true
  copy_qt_dev_tools_label="devtools "
  ARCHIVE_NAME_PREFIX=${ARCHIVE_NAME_PREFIX}devtools-
fi


prj_root=$(pwd)

echo "---------------- BUILDING PROJECT ----------------"
echo "--------------------------------------------------"

echo "Building...."

rm -rf build; mkdir -p build/release; cd build/release;
cmake $testnet_def -D STATIC=true -D ARCH=x86-64 -D BUILD_GUI=TRUE -D OPENSSL_ROOT_DIR="$OPENSSL_ROOT_DIR" -D CMAKE_PREFIX_PATH="$QT_PREFIX_PATH" -D CMAKE_BUILD_TYPE=Release ../..
if [ $? -ne 0 ]; then
    echo "Failed to run cmake"
    exit 1
fi

make -j2 daemon simplewallet connectivity_tool
if [ $? -ne 0 ]; then
    echo "Failed to make!"
    exit 1
fi

make -j1 Pdc
if [ $? -ne 0 ]; then
    echo "Failed to make!"
    exit 1
fi


read version_str <<< $(./src/pdcd --version | awk '/^PDC/ { print $2 }')
version_str=${version_str}
echo $version_str

rm -rf Pdc;
mkdir -p Pdc;

rsync -a ../../src/gui/qt-daemon/layout/html ./Pdc --exclude less --exclude package.json --exclude gulpfile.js
cp -Rv ../../utils/Pdc.sh ./Pdc
chmod 777 ./Pdc/Pdc.sh
mkdir ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libicudata.so.56 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libicui18n.so.56 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libicuuc.so.56 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5Core.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5DBus.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5Gui.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5Network.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5OpenGL.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5Positioning.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5PrintSupport.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5Qml.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5Quick.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5Sensors.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5Sql.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5Widgets.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5WebEngine.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5WebEngineCore.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5WebEngineWidgets.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5WebChannel.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5XcbQpa.so.5 ./Pdc/lib
cp $QT_PREFIX_PATH/lib/libQt5QuickWidgets.so.5 ./Pdc/lib
# Prefer build-tree OpenSSL 1.1 when present (Qt 5.12 vs host OpenSSL 3).
if [ -n "${OPENSSL_ROOT_DIR:-}" ] && [ -d "${OPENSSL_ROOT_DIR}/lib" ]; then
  cp -a "${OPENSSL_ROOT_DIR}/lib/libssl.so"* ./Pdc/lib/ 2>/dev/null || true
  cp -a "${OPENSSL_ROOT_DIR}/lib/libcrypto.so"* ./Pdc/lib/ 2>/dev/null || true
fi
cp $QT_PREFIX_PATH/libexec/QtWebEngineProcess ./Pdc
cp ../../../utils/buildings/linux/64bit/qt.conf ./Pdc/
mkdir -p ./Pdc/resources
cp $QT_PREFIX_PATH/resources/qtwebengine_resources.pak ./Pdc/resources/
cp $QT_PREFIX_PATH/resources/qtwebengine_resources_100p.pak ./Pdc/resources/
cp $QT_PREFIX_PATH/resources/qtwebengine_resources_200p.pak ./Pdc/resources/
cp $QT_PREFIX_PATH/resources/icudtl.dat ./Pdc/resources/

if [ "$copy_qt_dev_tools" = true ] ; then
  cp $QT_PREFIX_PATH/resources/qtwebengine_devtools_resources.pak ./Pdc/resources/
fi

mkdir -p ./Pdc/translations
if [ -d "$QT_PREFIX_PATH/translations/qtwebengine_locales" ]; then
  cp -a "$QT_PREFIX_PATH/translations/qtwebengine_locales" ./Pdc/translations/
fi

mkdir ./Pdc/lib/platforms
cp $QT_PREFIX_PATH/plugins/platforms/libqxcb.so ./Pdc/lib/platforms
mkdir -p ./Pdc/lib/xcbglintegrations
cp $QT_PREFIX_PATH/plugins/xcbglintegrations/libqxcb-glx-integration.so ./Pdc/lib/xcbglintegrations/

cp -Rv src/pdcd src/Pdc src/simplewallet  src/connectivity_tool ./Pdc

package_filename=${ARCHIVE_NAME_PREFIX}${version_str}.tar.bz2

rm -f ./$package_filename
tar -cjvf $package_filename Pdc
if [ $? -ne 0 ]; then
    echo "Failed to pack"
    exit 1
fi

echo "Build success"

if [ -z "$upload_build" ]; then
    exit 0
fi

echo "Uploading..."


exit 0
