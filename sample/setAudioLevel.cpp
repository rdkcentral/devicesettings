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
#include "dsMgr.h"
#include "libIBus.h"
#include <string>

int main(int argc, char *argv[]) 
{
    IARM_Bus_Init("SampleDSClient");
    IARM_Bus_Connect();

    device::Manager::Initialize();

    if (argc != 2)
    {
        printf("%s <Volume Level >", argv[0]);
        return 0;
    }
    try
    {
        device::Manager::Initialize();

        if (argc != 2)
        {
            printf("%s <Volume Level >", argv[0]);
            device::Manager::DeInitialize();
            return 0;
        }
        device::VideoOutputPort vPort = device::Host::getInstance().getVideoOutputPort("HDMI0");
        float volume = atof((const char *)argv[1]);
	    printf("Sample Application: set Audio level :%f \r\n",volume);

	    vPort.getAudioOutputPort().setLevel(volume);
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
