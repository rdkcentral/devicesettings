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
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([ds-generic], [1.0], [lakshmim@tataelxsi.co.in])
AC_CONFIG_SRCDIR([ds])

AC_CONFIG_MACRO_DIR([cfg])
AC_CONFIG_AUX_DIR([cfg])
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 1.9])
AC_CONFIG_HEADERS([cfg/config.h])

LT_INIT
AC_LANG([C++])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],
[AM_DEFAULT_VERBOSITY=1
AC_SUBST(AM_DEFAULT_VERBOSITY)])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX

# Checks for header files.
AC_CHECK_HEADERS([locale.h stdlib.h unistd.h])
AC_CHECK_HEADERS([dsHALConfig.h])
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.24.0])
PKG_CHECK_MODULES([GTHREAD], [gthread-2.0 >= 2.24.0])
PKG_CHECK_MODULES([DIRECT], [direct])
PKG_CHECK_MODULES([FUSION], [fusion])
PKG_CHECK_MODULES([DBUS], [dbus-1])

AC_CHECK_LIB(gthread-2.0, g_thread_init)

# Thunder COM-RPC plugin support
AC_ARG_ENABLE([thunder-plugin],
    AS_HELP_STRING([--enable-thunder-plugin], [Enable Thunder COM-RPC plugin support (default: no)]),
    [enable_thunder_plugin=$enableval],
    [enable_thunder_plugin=no])

AM_CONDITIONAL([USE_THUNDER_PLUGIN], [test "x$enable_thunder_plugin" = "xyes"])

AS_IF([test "x$enable_thunder_plugin" = "xyes"],
    [AC_DEFINE([USE_WPE_THUNDER_PLUGIN], [1], [Define to 1 to enable Thunder COM-RPC plugin support])
     AC_MSG_NOTICE([Thunder COM-RPC plugin support enabled])],
    [AC_MSG_NOTICE([Thunder COM-RPC plugin support disabled])])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T

AC_CONFIG_FILES([Makefile
                 rpc/cli/Makefile
                 rpc/srv/Makefile
                 ds/Makefile])
AC_OUTPUT
