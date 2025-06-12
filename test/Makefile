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
RM:= rm -rf 

ifneq ($(STANDALONE_BUILD_ENABLED),y)
INCLUDE:= -I$(PWD)/ds                              \
    	  -I$(PWD)/ds/include                     \
    	  -I$(PWD)/hal/include                     \
    	  -I$(PWD)/hal/src                         \
	  -I$(PWD)/boost_1_50_0 
else
INCLUDE:= -I$(PWD)/ds                              \
		  -I$(PWD)/ds/include                     \
		  -I$(PWD)/hal/include                     \
	  -I$(PWD)/boost_1_50_0
endif



INCLUDE      += $(HAL_INCLUDE)

INSTALL     := $(PWD)/install

FUSION_LIBS := -lUIIARM -lfusiondale
FUSION_DLIBS:= -ldirect -lfusion -lz -lpthread
LDFLAGS     := -L$(INSTALL)/lib -ldshalcli -lds       

CFLAGS      += -std=c++0x  -g -fPIC -D_REENTRANT -Wall $(INCLUDE)

LDFLAGS += $(HAL_LDFLAGS)

.PHONY: $(OUTPUT)

OUTPUT      :=  testHost                \
                testPersistence         
               
              
#OUTPUT      :=  testAOP                 \
 #               testVOP                 \
  #              testGetAudioConfig      \
   #             testSetAudioConfig      \
    #            testGetVideoOutputPorts \
     #           testVideoDevice         \
      #          testHost                \
       #         testPersistence         \
        #        testFPD



all: $(OUTPUT)
	@echo "Build Finished...."

testAOP:
	@echo "Building $@ ...."
	@$(CXX) $(CFLAGS) -o testAOP testConfigAOP.cpp -L../install/lib  $(LDFLAGS)

testVOP:
	@echo "Building $@ ...."
	@$(CXX) $(CFLAGS) -o testVOP testConfigVOP.cpp -L../install/lib  $(LDFLAGS)

testGetAudioConfig:
	@echo "Building $@ ...."
	@$(CXX) $(CFLAGS) -o testGetAudioConfig testGetAudioConfig.cpp -L../install/lib  $(LDFLAGS)

testSetAudioConfig:
	@echo "Building $@ ...."
	@$(CXX) $(CFLAGS) -o testSetAudioConfig testSetAudioConfig.cpp -L../install/lib  $(LDFLAGS)

testGetVideoOutputPorts:
	@echo "Building $@ ...."
	@$(CXX) $(CFLAGS) -o testGetVideoOutputPorts testGetVideoOutputPorts.cpp -L../install/lib  $(LDFLAGS)

testVideoDevice:
	@echo "Building $@ ...."
	@$(CXX) $(CFLAGS) -o testVideoDevice testVideoDevice.cpp -L../install/lib  $(LDFLAGS)
testHost:
	@echo "Building $@ ...."
	@$(CXX) $(CFLAGS) -o testHost testHost.cpp -L../install/lib  $(LDFLAGS)

testPersistence:
	@echo "Building $@ ...."
	@$(CXX) $(CFLAGS) -std=c++0x -o testPersistence testPersistence.cpp -L../install/lib  $(LDFLAGS) 

testFPD:
	@echo "Building $@ ...."
	@$(CXX) $(CFLAGS) -std=c++0x -o testFPD testFrontPannel.cpp -L../install/lib  $(LDFLAGS)

uninstall: clean
	@echo "Uninstalling $@ ...."

clean:
	@echo "Cleaning the directory..."
	@$(RM) $(OUTPUT)



