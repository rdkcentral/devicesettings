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
#include "dsUtl.h"
#include "dsError.h"
#include "manager.hpp"
#include "list.hpp"
#include "libIBus.h"
#include "sleepMode.hpp"
#include <string>
#include "list.hpp"
#include "dsConstant.hpp"
#include "dsTypes.h"

int main(int argc, char *argv[]) 
{
	uint32_t versionNumber;

	IARM_Bus_Init("SampleDSClient");
	IARM_Bus_Connect();
    try{
	    device::Manager::Initialize();
    
	    versionNumber = device::Host::getInstance().getVersion();
	    printf("DS HAL Version Number before setting is: 0x%08X \r\n",versionNumber);
	    printf("DS HAL Version before setting is - %d.%d \r\n",dsHAL_APIVER_MAJOR(versionNumber),dsHAL_APIVER_MINOR(versionNumber));
    
    
	    versionNumber = device::Host::getInstance().getVersion();
	    printf("DS HAL Version Number after setting is: 0x%08X \r\n",versionNumber);
	    printf("DS HAL Version after settings is - %d.%d \r\n",dsHAL_APIVER_MAJOR(versionNumber),dsHAL_APIVER_MINOR(versionNumber));

    
        
        device::Manager::DeInitialize();
    }
    catch(...)
    {
        printf("Exception caught\n");
    }
	IARM_Bus_Disconnect();
	IARM_Bus_Term();
    return 0;
}


/** @} */
/** @} */
