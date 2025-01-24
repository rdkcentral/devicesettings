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
#include "host.hpp"
#include "frontPanelConfig.hpp"
#include "frontPanelTextDisplay.hpp"
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

void _EventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
void _EventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    if (strcmp(owner,IARM_BUS_DSMGR_NAME) == 0)
    {
        printf("Ds Mgr Event Handler called %d ......... \r\n",eventId);
		switch (eventId) {
            case IARM_BUS_DSMGR_EVENT_TIME_FORMAT_CHANGE:
                {
					IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
					printf("EVENT_TIME_FORMAT_CHANGE Event %d Recvd \r\n",eventId);
				    printf("Time Format change to  %s \r\n",eventData->data.FPDTimeFormat.eTimeFormat == dsFPD_TIME_12_HOUR ?"12_HR":"24_HR");
					try {
                        printf("Set Time format \r\n");
                        device::FrontPanelConfig::getInstance().getTextDisplay("Text").setTime(18,30);
                        printf("Set Time format over\r\n");
                    }   
                    catch (...) {
                        printf("Exception Caught during setting  time format \r\n");
                    }
				}
                break;
            default:
               break;
        }
    }
}



int main(int argc, char *argv[]) 
{
    if (argc != 2) {
        printf("%s : <Timezone - 0(12_HR)-1(24_HR)> >\r\n", argv[0]);
        return 0;
    }
 
	int timzone = atoi((const char *)argv[1]);
   

	IARM_Bus_Init("TestTimeFormat");
	IARM_Bus_Connect();

	IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, 
                                    IARM_BUS_DSMGR_EVENT_TIME_FORMAT_CHANGE, 
                                    _EventHandler);

    
    
    try {
          device::Manager::Initialize();
		
    	  printf("Before setting  - Time zone read from DS is  %d \r\n",device::FrontPanelConfig::getInstance().getTextDisplay("Text").getCurrentTimeFormat());
		  device::FrontPanelConfig::getInstance().getTextDisplay("Text").setTimeFormat(timzone);
          printf("After setting - Time zone read from DS is now %d \r\n",device::FrontPanelConfig::getInstance().getTextDisplay("Text").getCurrentTimeFormat());

          sleep(5);
    
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
