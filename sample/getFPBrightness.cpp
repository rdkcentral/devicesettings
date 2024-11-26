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


#include <iostream>
#include <unistd.h>
#include "frontPanelIndicator.hpp"
#include "manager.hpp"
#include "exception.hpp"


#include "libIBus.h"


int main(int argc, char *argv[]) 
{
   
	IARM_Bus_Init("SampleDSClient");
	IARM_Bus_Connect();

    try{
        device::Manager::Initialize();
    }
    catch (const std::exception e) {
        printf("Exception caught\r\n");
    }
   

    if (argc != 2) {
        printf("%s : <Indicator  - Message|Power|Record|Remote|RfByPass>\n", argv[0]);
        return 0;
    }
    char *pIndicatorName = argv[1];

    try {
		printf("Sample Application: get FrontPanel brightness\n");
        int bright = device::FrontPanelIndicator::getInstance(pIndicatorName).getBrightness();
		printf("Sample Application: brightness is %d\n", bright);
    }
    catch (...) {
    	printf("Exception Caught during [%s]\r\n", argv[0]);
    }

    try{
        device::Manager::DeInitialize();
    }
    catch (const std::exception e) {
        printf("Exception caught\r\n");
    }

	sleep(1);
	IARM_Bus_Disconnect();
	IARM_Bus_Term();
	sleep(1);
    return 0;
}


/** @} */
/** @} */
