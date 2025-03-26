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

        if ((argc < 2) || (argc > 16)) {
            printf("%s : <SAD1, SAD2, ...max upto 15 SADs> \r\n", argv[0]);
            return 0;
        }

        std::vector<int> sad_list;
        for(int i=0; i<(argc-1); i++) {
            sad_list.push_back(atoi((const char *)argv[i+1]));
        }
		printf("Sample Application: set Short Audio Descriptor\r\n");
		device::AudioOutputPort aPort = device::Host::getInstance().getAudioOutputPort("HDMI_ARC0");
		aPort.setSAD(std::move(sad_list));
		printf("Sample Application: set called\r\n");

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
