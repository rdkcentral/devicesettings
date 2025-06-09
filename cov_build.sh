#!/bin/bash

WORKDIR=`pwd`
export ROOT=/usr
export INSTALL_DIR=${ROOT}/local
mkdir -p $INSTALL_DIR

export CC=gcc
export CXX=g++
export AR=ar
export LD=ld
export NM=nm
export RANLIB=ranlib
export STRIP=strip

export RFC_PATH=$ROOT/rfc
export IARMBUS_PATH=$ROOT/iarmbus
export IARM_PATH=$IARMBUS_PATH
export IARMMGRS_PATH=$ROOT/iarmmgrs
export RDKLOGGER_PATH=$ROOT/rdk_logger
export TELEMETRY_PATH=$ROOT/telemetry
export DS_PATH=$ROOT/devicesettings
export DS_IF_PATH=$ROOT/rdk-halif-device_settings
export POWER_IF_PATH=$ROOT/rdk-halif-power_manager
export DEEPSLEEP_IF_PATH=$ROOT/rdk-halif-deepsleep_manager
export DS_HAL_PATH=$ROOT/rdkvhal-devicesettings-raspberrypi4

# Build and deploy stubs for IARMBus
echo "Building Devicesttings stubs"
cd $WORKDIR
cd ./stubs
#g++ -fPIC -shared -o libIARMBus.so iarm_stubs.cpp -I$WORKDIR/stubs -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I$IARMBUS_PATH/core -I$IARMBUS_PATH/core/include -fpermissive
#g++ -fPIC -shared -o libWPEFrameworkPowerController.so powerctrl_stubs.cpp  -I$WORKDIR/stubs -I${POWER_IF_PATH}/include -fpermissive

gcc -fPIC -shared -o libdshal.so dshal_stubs.c -I${DS_IF_PATH}/include -I$WORKDIR/mfr/include
#g++ -fPIC -shared -o libdshalsrv.so dshalsrv_stubs.c -I${DS_IF_PATH}/include -I${DS_PATH}/rpc/include
#g++ -fPIC -shared -o libds.so ds_stubs.cpp -I${DS_IF_PATH}/include/ -I${DS_PATH}/ds/include -I${DS_PATH}/rpc/include

#cp libIARMBus.so /usr/local/lib/
#cp libtelemetry_msgsender.so /usr/local/lib/
#cp libWPEFrameworkPowerController.so /usr/local/lib/libWPEFrameworkPowerController.so

#cp libds.so /usr/local/lib/
cp libdshal.so /usr/local/lib/
#cp libdshalsrv.so /usr/local/lib/

echo "##### Building DEVICESETTINGS modules"
cd $WORKDIR

# default PATHs - use `man readlink` for more info
# the path to combined build
export RDK_PROJECT_ROOT_PATH=${RDK_PROJECT_ROOT_PATH-`readlink -m ../../..`}

# path to build script (this script)
export RDK_SCRIPTS_PATH=${RDK_SCRIPTS_PATH-`readlink -m $0 | xargs dirname`}

# path to components sources and target
export RDK_SOURCE_PATH=${RDK_SOURCE_PATH-$RDK_SCRIPTS_PATH}

# default component name
export RDK_COMPONENT_NAME=${RDK_COMPONENT_NAME-`basename $RDK_SOURCE_PATH`}
cd ${RDK_SOURCE_PATH}

export STANDALONE_BUILD_ENABLED=y
export DS_MGRS=$WORKDIR
export UTILS_PATH=$IARM_MGRS/utils

find $WORKDIR -iname "*.o" -exec rm -v {} \;
find $WORKDIR -iname "*.so*" -exec rm -v {} \;

echo "##### Triggering make"

make CXXFLAGS="-DRDK_DSHAL_NAME=libdshal.so"  CFLAGS="-fPIC -I${DS_IF_PATH}/include -I${DS_HAL_PATH} -I${DS_MGRS}/stubs  -I${IARMBUS_PATH}/core -I${IARMBUS_PATH}/core/include -I$UTILS_PATH -I${IARM_MGRS}/sysmgr/include -I${DS_PATH}/ds/include -I${DS_PATH}/rpc/include -I${POWER_IF_PATH}/include/ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I${IARM_MGRS}/mfr/include/ -I${IARM_MGRS}/mfr/common -I${DEEPSLEEP_IF_PATH}/include -I${IARM_MGRS}/hal/include -I${IARM_MGRS}/power -I${IARM_MGRS}/power/include" LDFLAGS="-L/usr/lib/x86_64-linux-gnu/ -L/usr/local/include -lglib-2.0 -lIARMBus -lWPEFrameworkPowerController -lds -ldshal -ldshalsrv -liarmUtils"
