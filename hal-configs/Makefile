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
CONFIG      := configs
RM          := rm -rf 
CFLAGS      += -g -fPIC -D_REENTRANT -Wall
LIBNAME     := dshallocal
LIBNAMEFULL := lib$(LIBNAME).so
INSTALL     := $(PWD)/install
#SRC_CONFIG      :=  $(wildcard $(CONFIG)/*.cpp)
#OBJ_CONFIG      :=  $(patsubst $(CONFIG)/%.cpp, $(CONFIG)/%.o,$(SRC_CONFIG))
OBJ_CONFIG        := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
INCLUDE     := -I$(PWD)                         \
               -I$(PWD)/hal/include             \
               -I$(PWD)/rpc/include             
          
INCLUDE      += $(HAL_INCLUDE)

CFLAGS      += $(INCLUDE)
CPPFLAGS     += $(INCLUDE)

all: install
	@echo "Build Finished...."

library: $(OBJ_CONFIG)
	@echo "Building $(LIBNAMEFULL) ...."
	$(CXX) $(OBJ_CONFIG) $(CFLAGS) $(CPPFLAGS) -shared -o $(LIBNAMEFULL)

%.o: %.cpp
	@echo "Building $@ ...."
	$(CXX) -c $<  $(CFLAGS) $(CPPFLAGS) -o $@

%.o: %.c
	@echo "Building $@ ...."
	$(CXX) -c $<  $(CFLAGS) $(CPPFLAGS) -o $@

install: library
	@echo "installling...!"
	mkdir -p $(INSTALL)/lib
	@cp -f $(LIBNAMEFULL) $(INSTALL)/lib

uninstall:
	@$(RM) $(INSTALL)/lib/$(LIBNAMEFULL)

clean:
	@echo "Cleaning the directory..."
	@$(RM) $(OBJ_CONFIG) $(LIBNAMEFULL)


