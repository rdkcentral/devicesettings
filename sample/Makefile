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
ifneq ($(STANDALONE_BUILD_ENABLED),y)
INCLUDE     := -I$(PWD)/ds/include                      \
    	       -I$(PWD)/ds                              \
			   -I$(PWD)/rpc/include 					\
    	       -I$(PWD)/hal/include                     \
    	       -I$(PWD)/hal/src
else
INCLUDE     := -I$(PWD)/ds/include                      \
    	       -I$(PWD)/ds                              \
			   -I$(PWD)/rpc/include 					\
    	       -I$(PWD)/hal/include                     
endif


INCLUDE      += $(HAL_INCLUDE)

INSTALL     := $(PWD)/install

#LDFLAGS     := -L$(INSTALL)/lib -lds 


LDFLAGS		+= $(HAL_LDFLAGS)
LDFLAGS		+= $(GLIB_LDFLAGS)

CFLAGS      +=   -g -fPIC -D_REENTRANT -Wall $(INCLUDE)

.PHONY: $(OUTPUT)

BINARIES    := $(patsubst %.cpp,%,$(wildcard *.cpp))
UNINSTALL   := $(patsubst %,$(PWD)/install/bin/%, $(BINARIES))

#all: $(BINARIES)  install
all: $(BINARIES)
	@echo "Build Finished...."

#frontPanelTest:
$(BINARIES):
	@echo "Building $@ ...."
	$(CXX) $(CFLAGS) $@.cpp -o $@ $(LDFLAGS)
#	@cp -f $@ $(INSTALL)/bin

install:
	@echo "Copying the binaries to bin install folder..."
	@cp $(BINARIES) $(INSTALL)/bin

uninstall:
	@echo "Removing bin from install folder..."
	@$(RM) $(UNINSTALL)

clean:
	@echo "Cleaning the directory..."
	@$(RM) $(BINARIES)



