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
RM          := rm -rf 
CFLAGS      += -g -fPIC -D_REENTRANT -Wall
LIBNAME     := dshalsrv
LIBNAMEFULL := lib$(LIBNAME).so
INSTALL     := $(PWD)/install
OBJS        := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJS        += $(patsubst %.c,%.o,$(wildcard *.c))
ifneq ($(STANDALONE_BUILD_ENABLED),y)
INCLUDE     := -I$(PWD)                         \
               -I$(PWD)/hal/include             \
               -I$(PWD)/hal/src             \
               -I$(PWD)/rpc/include  
else
INCLUDE     := -I$(PWD)                         \
			   -I$(PWD)/hal/include             \
			   -I$(PWD)/rpc/include   
endif        


INCLUDE      += $(HAL_INCLUDE)
INCLUDE	     += -I=$(includedir)/rdk/ds-hal

CFLAGS      += $(INCLUDE)

# DeviceSettings plugin mode. The devicesettings recipe builds with "make -e", so Makefile
# appends to CFLAGS are ignored there: plugin mode is therefore detected from the environment
# CFLAGS (-DDEVICESETTINGS_PLUGIN_ENABLED) or from DEVICESETTINGS_PLUGIN_ENABLED=y|1|ON|yes on the
# make command line, and the flags it needs are passed through PLUGIN_FLAGS on every compile.
ifneq (,$(or $(findstring -DDEVICESETTINGS_PLUGIN_ENABLED,$(CFLAGS)),$(filter y Y 1 ON on yes,$(DEVICESETTINGS_PLUGIN_ENABLED))))
DS_PLUGIN_MODE := y
PLUGIN_FLAGS   := -DDEVICESETTINGS_PLUGIN_ENABLED
$(info $(CURDIR): DeviceSettings plugin mode, COM-RPC to the DeviceSettings plugin is used)
endif

CFLAGS      +=$(HAL_BUILDFLAGS)

# Add RDK_DSHAL_NAME for non-yocto builds
ifeq (,$(findstring -DYOCTO_BUILD,$(CFLAGS)))
CFLAGS      +=-DRDK_DSHAL_NAME=\"libdshal.so\"
endif

all: install
	@echo "Build Finished...."

library: $(OBJS)
	@echo "Building $(LIBNAMEFULL) ...."
	$(CXX) $(OBJS) $(CFLAGS) $(LDFLAGS) -shared -o $(LIBNAMEFULL)

%.o: %.c
	@echo "Building $@ ...."
	$(CXX) -c $<  $(CFLAGS) $(PLUGIN_FLAGS) -o $@

%.o: %.cpp
	@echo "Building $@ ...."
	$(CXX) -std=c++0x -c $<  $(CFLAGS) $(PLUGIN_FLAGS) -o $@

install: library
	@echo "installling...!"
	@cp -f $(LIBNAMEFULL) $(INSTALL)/lib

uninstall:
	@$(RM) $(INSTALL)/lib/$(LIBNAMEFULL)

clean:
	@echo "Cleaning the directory..."
	@$(RM) $(OBJS) $(LIBNAMEFULL)


