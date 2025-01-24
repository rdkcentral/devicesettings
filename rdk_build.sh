#!/bin/bash
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
#

#######################################
#
# Build Framework standard script for
#
# DeviceSettings component

# use -e to fail on any shell issue
# -e is the requirement from Build Framework
set -e


# default PATHs - use `man readlink` for more info
# the path to combined build
export RDK_PROJECT_ROOT_PATH=${RDK_PROJECT_ROOT_PATH-`readlink -m ..`}/
export COMBINED_ROOT=$RDK_PROJECT_ROOT_PATH

# path to build script (this script)
export RDK_SCRIPTS_PATH=${RDK_SCRIPTS_PATH-`readlink -m $0 | xargs dirname`}/

# path to components sources and target
export RDK_SOURCE_PATH=${RDK_SOURCE_PATH-`readlink -m .`}/
export RDK_TARGET_PATH=${RDK_TARGET_PATH-$RDK_SOURCE_PATH}

# fsroot and toolchain (valid for all devices)
export RDK_FSROOT_PATH=${RDK_FSROOT_PATH-`readlink -m $RDK_PROJECT_ROOT_PATH/sdk/fsroot/ramdisk`}/
export RDK_TOOLCHAIN_PATH=${RDK_TOOLCHAIN_PATH-`readlink -m $RDK_PROJECT_ROOT_PATH/sdk/toolchain/staging_dir`}/


# default component name
export RDK_COMPONENT_NAME=${RDK_COMPONENT_NAME-`basename $RDK_SOURCE_PATH`}


# parse arguments
INITIAL_ARGS=$@

function usage()
{
    set +x
    echo "Usage: `basename $0` [-h|--help] [-v|--verbose] [action]"
    echo "    -h    --help                  : this help"
    echo "    -v    --verbose               : verbose output"
    echo "    -p    --platform  =PLATFORM   : specify platform for DeviceSettings"
    echo
    echo "Supported actions:"
    echo "      configure, clean, build (DEFAULT), rebuild, install"
}

# options may be followed by one colon to indicate they have a required argument
if ! GETOPT=$(getopt -n "build.sh" -o hvp: -l help,verbose,platform: -- "$@")
then
    usage
    exit 1
fi

eval set -- "$GETOPT"

while true; do
  case "$1" in
    -h | --help ) usage; exit 0 ;;
    -v | --verbose ) set -x ;;
    -p | --platform ) CC_PLATFORM="$2" ; shift ;;
    -- ) shift; break;;
    * ) break;;
  esac
  shift
done

ARGS=$@


# component-specific vars
CC_PATH=$RDK_SOURCE_PATH
export FSROOT=${RDK_FSROOT_PATH}
export TOOLCHAIN_DIR=${RDK_TOOLCHAIN_PATH}
export WORK_DIR=${WORK_DIR-${RDK_PROJECT_ROOT_PATH}/work${RDK_PLATFORM_DEVICE^^}}
export BUILDS_DIR=$RDK_PROJECT_ROOT_PATH
export COMBINED_DIR=$COMBINED_ROOT


# functional modules

function configure()
{
    if [ $RDK_PLATFORM_SOC = "entropic" ]; then
       #Apply Patch for building 
       export DS_PATH=${BUILDS_DIR}/devicesettings
       cp ${BUILDS_DIR}/device/patches/rdk/devicesettings/Makefile ${DS_PATH}/
       cp ${BUILDS_DIR}/device/patches/rdk/devicesettings/ds/frontPanelTextDisplay.cpp ${DS_PATH}/ds
       cp ${BUILDS_DIR}/device/patches/rdk/devicesettings/rpc/srv/dsFPD.c ${DS_PATH}/rpc/srv

       #TODO
       #HACK, Entropic should provide these files, till then we need this HACK
       #TODO

       cp ${DS_PATH}/hal/include/dsVideoResolutionSettings_sample.h ${DS_PATH}/hal/include/dsVideoResolutionSettings.h
       cp ${DS_PATH}/hal/include/dsVideoPortSettings_sample.h ${DS_PATH}/hal/include/dsVideoPortSettings.h
       cp ${DS_PATH}/hal/include/dsVideoDeviceSettings_sample.h ${DS_PATH}/hal/include/dsVideoDeviceSettings.h
       cp ${DS_PATH}/hal/include/dsAudioSettings_sample.h ${DS_PATH}/hal/include/dsAudioSettings.h
    else
    true #use this function to perform any pre-build configuration
    fi
}

function clean()
{
    rm -rf ${CC_PATH}/hal/include/dsHALConfig.h
    true #use this function to provide instructions to clean workspace
}

function build()
{
    touch ${CC_PATH}/hal/include/dsHALConfig.h

    if [ $RDK_PLATFORM_SOC = "intel" ] || [ $RDK_PLATFORM_SOC = "broadcom" ] || [ $RDK_PLATFORM_SOC = "stm" ]; then
    DeviceSettings_PATH=${CC_PATH}
    CURR_DIR=`pwd`

    cd $DeviceSettings_PATH
    echo "Setting Device Settings Build Environment Variables..."
    
    if [ -f $DeviceSettings_PATH/config/env/dsenv.sh ]; then
      source $DeviceSettings_PATH/config/env/dsenv.sh
      echo "Using DS Config Env Settings..."
    elif [ -f $DeviceSettings_PATH/hal/src/halenv.sh ] ; then
      source $DeviceSettings_PATH/hal/src/halenv.sh
      echo "Using DS HAL Env Settings..."
    fi

    elif [ $RDK_PLATFORM_SOC = "entropic" ]; then
       export SDK_CONFIG=stb597_V3_xi3
       export BUILD_DIR=$BUILDS_DIR
       source ${BUILD_DIR}/build_scripts/setupSDK.sh
       echo ${GCC_BASE}
       echo ${GCC_PREFIX}
       DS_DIR=${BUILD_DIR}/devicesettings

       export BUILDS_DIR=${BUILD_DIR}
       export COMBINED_DIR=${BUILDS_DIR}
       export DS_PATH=${BUILD_DIR}/devicesettings
       export IARM_PATH=${BUILDS_DIR}/iarm
       export FSROOT=${BUILDS_DIR}/fsroot
       export HAL_INCLUDE="-I${BUILDS_DIR}/logger/include/"
       CROSS_TOOLCHAIN=$GCC_BASE/bin
       CROSS_COMPILE=$GCC_PREFIX
       export CC=$CROSS_COMPILE-gcc 
       export CXX="$CROSS_COMPILE-g++ -I${BUILDS_DIR}/iarmbus/core/include -I${BUILDS_DIR}/iarmmgrs/generic/power/include -I${BUILDS_DIR}/iarmmgrs/generic/ir/include"
       export buildReport=$BUILDS_DIR/../Logs/buildDeviceSettReport.txt

       cd $DS_DIR
    fi

    if [ $RDK_PLATFORM_SOC = "intel" ]; then
        echo "#undef HAS_AUDIO_PASSTHRU" > $DeviceSettings_PATH/hal/include/dsHALConfig.h
    elif [ $RDK_PLATFORM_SOC = "broadcom" ] && [ "$BRCM_RDK_REFSW_PKG" = "13.2" ]; then
        echo "#undef HAS_AUDIO_PASSTHRU" > $DeviceSettings_PATH/hal/include/dsHALConfig.h
    elif [ $RDK_PLATFORM_SOC = "broadcom" ] && [ $RDK_PLATFORM_DEVICE = "rng150" ]; then
        echo "#undef HAS_AUDIO_PASSTHRU" > $DeviceSettings_PATH/hal/include/dsHALConfig.h
    else 
        echo "#define HAS_AUDIO_PASSTHRU" > $DeviceSettings_PATH/hal/include/dsHALConfig.h
    fi


    echo "Building Device Setting libraries..."
    make all

    cd $CURR_DIR
}

function rebuild()
{
    clean
    build
}

function install()
{
    DeviceSettings_PATH=${CC_PATH}
    if [ $RDK_PLATFORM_SOC = "entropic" ] ; then
       DeviceSettings_PATH=${BUILDS_DIR}/devicesettings
       RDK_FSROOT_PATH=$BUILDS_DIR/fsroot
    fi

    CURR_DIR=`pwd`
    
    if [ -f ${BUILDS_DIR}/devicesettings/hal/src/sample/tenableHDCP ] ; then
      cp ${BUILDS_DIR}/devicesettings/hal/src/sample/tenableHDCP ${RDK_FSROOT_PATH}/usr/local/bin
    fi

    cd $DeviceSettings_PATH/install

    if [ -d "lib" ]; then

      rsync -rplEogDWI --force --exclude=.svn lib ${RDK_FSROOT_PATH}/usr/local/

    fi


    if [ -d "bin" ]; then

      rsync -rplEogDWI --force --exclude=.svn bin ${RDK_FSROOT_PATH}/usr/local/

    fi

    cd $CURR_DIR
}


# run the logic

#these args are what left untouched after parse_args
HIT=false

for i in "$ARGS"; do
    case $i in
        configure)  HIT=true; configure ;;
        clean)      HIT=true; clean ;;
        build)      HIT=true; build ;;
        rebuild)    HIT=true; rebuild ;;
        install)    HIT=true; install ;;
        *)
            #skip unknown
        ;;
    esac
done

# if not HIT do build by default
if ! $HIT; then
  build
fi
