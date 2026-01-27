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
LIBNAME     := dshalcli
LIBNAMEFULL := lib$(LIBNAME).so
INSTALL     := $(PWD)/install

# Conditional compilation: Thunder vs IARM
ifdef USE_WPE_THUNDER_PLUGIN
    # Thunder mode - use dsFPD-com.cpp, exclude dsFPD.c
    OBJS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
    OBJS += $(patsubst %.c,%.o,$(filter-out dsFPD.c,$(wildcard *.c)))
else
    # IARM mode - use dsFPD.c, exclude dsFPD-com.cpp
    OBJS := $(patsubst %.cpp,%.o,$(filter-out dsFPD-com.cpp,$(wildcard *.cpp)))
    OBJS += $(patsubst %.c,%.o,$(wildcard *.c))
endif

OBJS        := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJS        += $(patsubst %.c,%.o,$(wildcard *.c))
INCLUDE     := -I$(PWD)                         \
               -I$(PWD)/hal/include             \
               -I$(PWD)/rpc/include             
          
INCLUDE      += $(HAL_INCLUDE)

CFLAGS      += $(INCLUDE)

# Conditional linking flags
ifdef USE_WPE_THUNDER_PLUGIN
    LDLIBS := -lWPEFrameworkCore -lWPEFrameworkCOM
else
    LDLIBS := -lIARMBus
endif

all: install
	@echo "Build Finished...."

library: $(OBJS)
	@echo "Building $(LIBNAMEFULL) ...."
	$(CXX) $(OBJS) $(CFLAGS) $(LDLIBS) -shared -o $(LIBNAMEFULL)

%.o: %.cpp
	@echo "Building $@ ...."
	$(CXX) -c $<  $(CFLAGS) -o $@

%.o: %.c
	@echo "Building $@ ...."
	$(CXX) -c $<  $(CFLAGS) -o $@

install: library
	@echo "installling...!"
	mkdir -p $(INSTALL)/lib
	@cp -f $(LIBNAMEFULL) $(INSTALL)/lib

uninstall:
	@$(RM) $(INSTALL)/lib/$(LIBNAMEFULL)

clean:
	@echo "Cleaning the directory..."
	@$(RM) $(OBJS) $(LIBNAMEFULL)


