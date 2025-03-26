/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
 
// TODO: Include your class to test here.


/**
* @defgroup devicesettings
* @{
* @defgroup sample
* @{
**/


#include "videoOutputPort.hpp"
#include "videoOutputPortType.hpp"
#include "host.hpp"
#include "audioOutputPort.hpp"
#include "dsUtl.h"
#include "dsError.h"
#include "stdlib.h"
#include "manager.hpp"



#include "libIBus.h"


#include <string>



int main(int argc, char *argv[]) 
{
   
	IARM_Bus_Init("SampleDSClient");
	IARM_Bus_Connect();

    try {
        device::Manager::Initialize();
        if (argc != 4) {
            printf("%s : <Port Type - HDMI, SPEAKER> <Port Number-0, 1, 2...> <Profile Name to configure>\r\n", argv[0]);
            return 0;
        }

        char *portType = argv[1];
        char *portId = argv[2];
        std::string profileSet = argv[3];
        std::vector<std::string> list;
		printf("Sample Application: Get Audio profile List \r\n");
		device::AudioOutputPort aPort = device::Host::getInstance().getAudioOutputPort(std::string(portType).append(portId));
		list = aPort.getMS12AudioProfileList();
		for(int i=0; i<list.size(); i++) {
		    printf("Profile[%d]: %s \n",i+1,list.at(i).c_str());
		}

		printf("Current configured profile: %s\n",aPort.getMS12AudioProfile().c_str());
		printf("Configure new profile : %s\n",profileSet.c_str());
		aPort.setMS12AudioProfile(std::move(profileSet));
		printf("(Check) Configured profile after set: %s\n",aPort.getMS12AudioProfile().c_str());

        device::Manager::DeInitialize();
    }
    catch (...) {
    	printf("Exception Caught during [%s]\r\n", argv[0]);
    }

	IARM_Bus_Disconnect();
	IARM_Bus_Term();

    return 0;
}


/** @} */
/** @} */
