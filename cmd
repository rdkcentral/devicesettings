#!/usr/bin/env bash
set -euo pipefail

# ----- Common includes -----
INCLUDES=(
  -I/usr/rdk-halif-device_settings/include
  -I/usr/rdkvhal-devicesettings-raspberrypi4
  -I/home/skamath/del/devicesettings/stubs
  -I/usr/iarmbus/core
  -I/usr/iarmbus/core/include
  -I/sysmgr/include
  -I/home/skamath/del/devicesettings/ds/include
  -I/home/skamath/del/devicesettings/rpc/include
  -I/usr/rdk-halif-power_manager/include/
  -I/mfr/include/
  -I/mfr/common
  -I/usr/rdk-halif-deepsleep_manager/include
  -I/hal/include
  -I/power
  -I/power/include
)

# ----- Base flags -----
CFLAGS_BASE='-fPIC -DDSMGR_LOGGER_ENABLED=ON -DRDK_DSHAL_NAME="libdshal.so"'
CFLAGS_BASE+=" ${INCLUDES[*]}"

# GLib via pkg-config if available; otherwise fallback to hardcoded paths
if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists glib-2.0; then
  CFLAGS_BASE+=" $(pkg-config --cflags glib-2.0)"
  LDFLAGS_BASE="$(pkg-config --libs glib-2.0) -lIARMBus -lWPEFrameworkPowerController -ldshal"
else
  CFLAGS_BASE+=" -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include"
  LDFLAGS_BASE='-L/usr/lib/x86_64-linux-gnu/ -L/usr/local/include -lglib-2.0 -lIARMBus -lWPEFrameworkPowerController -ldshal'
fi

# Parallelism (optional): uncomment to use all cores
# PAR="-j$(nproc)"

build() {
  local dir=$1
  echo "==> Building $dir"
  make ${PAR:-} -C "$dir" CFLAGS+="$CFLAGS_BASE" LDFLAGS="$LDFLAGS_BASE"
}

build rpc/srv
build rpc/cli
build ds

