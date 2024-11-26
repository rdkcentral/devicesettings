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
#include "videoDevice.hpp"
#include "videoDFC.hpp"
#include "manager.hpp"

#include "dsUtl.h"
#include "dsError.h"
#include "list.hpp"
#include "libIBus.h"



int main(int argc, char *argv[]) 
{
   
	IARM_Bus_Init("SampleDSClient");
	IARM_Bus_Connect();
	
    try {
        device::Manager::Initialize();
        device::VideoDevice decoder = device::Host::getInstance().getVideoDevices().at(0);
        printf("ZoomSettings - [%s]\r\n", decoder.getDFC().getName().c_str());
        device::Manager::DeInitialize();
    }
	catch (const std::exception e) {
		printf("Exception caught\r\n");
	}

	IARM_Bus_Disconnect();
	IARM_Bus_Term();

    return 0;
}


/** @} */
/** @} */
