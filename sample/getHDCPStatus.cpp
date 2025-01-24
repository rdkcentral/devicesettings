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


 #include <stdint.h>
#include <iostream>
#include "host.hpp"
#include "videoOutputPort.hpp"
#include "videoOutputPortType.hpp"
#include "videoResolution.hpp"
#include "manager.hpp"

#include "dsUtl.h"
#include "dsError.h"
#include "list.hpp"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "libIBus.h"
#include "dsMgr.h"
int iCallback = 0;
void _HDCPHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

int main(int argc, char *argv[]) 
{
   int hdpcStatus = 0;

	IARM_Bus_Init("SampleDSClient");
	IARM_Bus_Connect();
	

    IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, 
                                    IARM_BUS_DSMGR_EVENT_HDCP_STATUS, 
                                    _HDCPHandler);
	
    
	try{
        device::Manager::Initialize();
		device::VideoOutputPort vPort = device::Host::getInstance().getVideoOutputPort("HDMI0");
		if (vPort.isDisplayConnected()) {
			
			hdpcStatus = vPort.getHDCPStatus();
			printf("\n >>>>>>>> hdpcStatus : %d\n",hdpcStatus);

			while(1)
			{
				printf("\n >>>>>>>> Perform a HDMI Hot Plug to get a HDCP status \r\n");
				if(iCallback)
				{
					printf("\n >>>>>>>> Got a HDCP status .. \r\n");
					break;
				}	
				sleep(30);
			}

			hdpcStatus = vPort.getHDCPStatus();
			printf("\n >>>>>>>> hdpcStatus : %d\n",hdpcStatus);

			switch (hdpcStatus)
			{            
				case dsHDCP_STATUS_AUTHENTICATED:
				    {
						printf("\n >>>>>>>> HDCP Authenticated  \r\n");
					}
					break;
				case dsHDCP_STATUS_AUTHENTICATIONFAILURE:
					{
						printf("\n >>>>>>>> HDCP Failure  \r\n");

					}
				 break;

				case dsHDCP_STATUS_UNAUTHENTICATED:
				    {
						printf("\n >>>>>>>> HDCP Authentication not Initiated  \r\n");
					}	
					break;
			}
			printf("\n >>>>>>>> hdpcStatus : %d\n",hdpcStatus);
		}
    	else 
		{ 
    	    printf("\t Display [%s] connected\r\n", vPort.isDisplayConnected() ? "is" : "is NOT");
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


void _HDCPHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    if (strcmp(owner,IARM_BUS_DSMGR_NAME) == 0)
    {
        printf("_HDCPHandler called %d ......... \r\n",eventId);
		switch (eventId) {
            case IARM_BUS_DSMGR_EVENT_HDCP_STATUS:
                {
					iCallback = 1;
				}
                break;
            default:
               break;
        }
    }
}


/** @} */
/** @} */
