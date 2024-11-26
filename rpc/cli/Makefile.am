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
libdshalcli_la_SOURCES = dsAudio.c dsclientlogger.c dsDisplay.c dsFPD.c dsHost.cpp dsVideoDevice.c dsVideoPort.c
