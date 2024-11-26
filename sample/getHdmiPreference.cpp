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
 


/**
* @defgroup devicesettings
* @{
* @defgroup sample
* @{
**/


#include <iostream>
#include "host.hpp"
#include "videoOutputPort.hpp"
#include "videoOutputPortType.hpp"
#include "videoResolution.hpp"
#include "manager.hpp"

#include "dsUtl.h"
#include "dsError.h"
#include "dsTypes.h"
#include "list.hpp"


#include "libIBus.h"


int main(int argc, char *argv[]) 
{
   
	IARM_Bus_Init("SampleDSClient");
	IARM_Bus_Connect();

    try{
        device::Manager::Initialize();

        device::VideoOutputPort vPort = device::Host::getInstance().getVideoOutputPort("HDMI0");

        switch(vPort.GetHdmiPreference()) {
            case dsHDCP_VERSION_1X:
                printf("HDMI Preference is dsHDCP_VERSION_1X \n");
                break;
            case dsHDCP_VERSION_2X:
                printf("HDMI Preference is dsHDCP_VERSION_2X \n");
                break;
            case dsHDCP_VERSION_MAX:
                printf("HDMI Preference is dsHDCP_VERSION_MAX \n");
                break;
            default:
                printf("GetHdmiPreference returned error value \n");
                break;
        }
        device::Manager::DeInitialize();
    }
    catch(...){
        printf("Exception caught\n");
    }
	IARM_Bus_Disconnect();
	IARM_Bus_Term();

    return 0;
}


/** @} */
/** @} */

