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
#include "libIBus.h"
#include "hdmiIn.hpp"
#include "manager.hpp"
#include "dsUtl.h"
#include "dsError.h"
#include <exception>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) 
{
   
	int i =0;

    if (argc != 2) {
        printf("%s : <HDMI Input Port Index - 0, 1..> \r\n", argv[0]);
        return 0;
    }

    char *portId = argv[1];
		
	 printf("%s : %s\r\n", argv[0],argv[1]);

	IARM_Bus_Init("SampleDSClient");
	IARM_Bus_Connect();

	try {
         device::Manager::Initialize();
		 int numInputs = device::HdmiInput::getInstance().getNumberOfInputs();
		 printf("Device has %d HDMI inputs \r\n", numInputs);
         int selectedPort = atoi(portId);
         if(selectedPort < numInputs) {
             printf("Start HDMI port, press ENTER to stop\r\n");
             device::HdmiInput::getInstance().selectPort(selectedPort,true,dsVideoPlane_PRIMARY,false);
             getc(stdin);
             printf("Stop  HDMI port \r\n");
             device::HdmiInput::getInstance().selectPort(HDMI_IN_PORT_NONE,true,dsVideoPlane_PRIMARY,false);
         }
         else {
             printf("Wrong input port number %d\r\n", selectedPort);
         }

         device::Manager::DeInitialize();
	}
	catch (const std::exception e) {
		printf("Exception caught\r\n");
	}

	while(++i < 2) {
		printf("SampleDSClient Hearbeat\r\n");
		sleep(2);
	}	

	IARM_Bus_Disconnect();
	IARM_Bus_Term();

    return 0;
}


/** @} */
/** @} */
