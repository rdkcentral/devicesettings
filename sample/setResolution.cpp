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
//#include "videoOutputPortConfig.hpp"
#include "videoResolution.hpp"
#include "videoDFC.hpp"
#include "manager.hpp"
#include "dsUtl.h"
#include "dsError.h"
#include "list.hpp"
#include <exception>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "libIBus.h"
#include "dsMgr.h"

void _DisplResolutionHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

int main(int argc, char *argv[]) 
{
   
	int i =0;

    if (argc != 4) {
        printf("%s : <Ports - HDMI, Component>  <Port Index - 0, 1..>  <Resolution Settings  - 480i, 720p, 1080i>\r\n", argv[0]);
        return 0;
    }

    char *portType = argv[1];
    char *portId = argv[2];
    char *resolution = argv[3];
		
	 printf("%s : %s : %s : %s :\r\n", argv[0],argv[1],argv[2],argv[3]);

	IARM_Bus_Init("SampleDSClient");
	IARM_Bus_Connect();

	IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, 
                                    IARM_BUS_DSMGR_EVENT_RES_POSTCHANGE, 
                                    _DisplResolutionHandler);
	 

	try {
		 device::Manager::Initialize();
		 device::VideoOutputPort vPort = device::Host::getInstance().getVideoOutputPort(std::string(portType).append(portId));
		 printf("Resolution Read is %s : \r\n",vPort.getResolution().getName().c_str());
         vPort.setResolution(resolution);
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


void _DisplResolutionHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    if (strcmp(owner,IARM_BUS_DSMGR_NAME) == 0)
    {
        printf("_DisplResolutionHandler called %d ......... \r\n",eventId);
		switch (eventId) {
            case IARM_BUS_DSMGR_EVENT_RES_POSTCHANGE:
                {
					IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
					int width = 0,height=0;
					width = eventData->data.resn.width;
					height = eventData->data.resn.height;
				   printf("Resolution data is %d : %d \r\n",width,height);
				}
                break;
            default:
               break;
        }
    }
}


/** @} */
/** @} */
