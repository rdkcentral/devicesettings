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
            printf("%s : <Ducking action 0-start 1-stop>  <type 0-absolute  1-relative> <level 0-100>\r\n", argv[0]);
            return 0;
        }

        unsigned char action =  (unsigned char) atoi((const char *)argv[1]);
        unsigned char  type = (unsigned char) atoi((const char *)argv[2]);
        unsigned char  level = (unsigned char) atoi((const char *)argv[3]);

	    printf("Sample Application: set Audio Ducking\r\n");
	    device::List<device::AudioOutputPort> aPorts = device::Host::getInstance().getAudioOutputPorts();
        for (size_t i = 0; i < aPorts.size(); i++)
        {
            printf("setting ducking for port: %s\n",aPorts.at(i).getName().c_str());
	    aPorts.at(i).setAudioDucking((dsAudioDuckingAction_t)action,(dsAudioDuckingType_t)type,level);

        }
	    printf("Sample Application: set Audio Ducking is completed\r\n");
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
