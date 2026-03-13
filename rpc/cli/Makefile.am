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


SUBDIRS =
DIST_SUBDIRS =

INCLUDE_FILES = -I=$(includedir)/rdk/halif/ds-hal \
                -I ../include \
                -I=$(includedir)/rdk/iarmbus

lib_LTLIBRARIES = libdshalcli.la
libdshalcli_la_CPPFLAGS = $(INCLUDE_FILES)
libdshalcli_la_CFLAGS = -g -fPIC -D_REENTRANT -Wall

# Conditional compilation for Thunder COM-RPC
if USE_THUNDER_PLUGIN
    FPD_SOURCE = dsFPD-com.cpp
    HDMIIN_SOURCE = dsHdmiIn-com.cpp
    COMPOSITEIN_SOURCE =
    THUNDER_SOURCES = dsConnectionManager.cpp
    THUNDER_LIBS = -lWPEFrameworkCore -lWPEFrameworkCOM
else
    FPD_SOURCE = dsFPD.c
    HDMIIN_SOURCE = dsHdmiIn.c
    COMPOSITEIN_SOURCE = dsCompositeIn.c
    THUNDER_SOURCES =
    THUNDER_LIBS =
endif

libdshalcli_la_SOURCES = dsAudio.c dsclientlogger.c dsDisplay.c $(FPD_SOURCE) $(HDMIIN_SOURCE) $(COMPOSITEIN_SOURCE) $(THUNDER_SOURCES) dsHost.cpp dsVideoDevice.c dsVideoPort.c
libdshalcli_la_LIBADD = $(THUNDER_LIBS)
