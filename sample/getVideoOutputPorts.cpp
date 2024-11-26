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
#include "unsupportedOperationException.hpp"

#include "dsUtl.h"
#include "dsError.h"
#include "list.hpp"
#include "libIBus.h"


int main(int argc, char *argv[]) 
{
   
	IARM_Bus_Init("SampleDSClient");
	IARM_Bus_Connect();
    try{
        device::Manager::Initialize();
    }
    catch(...){
        printf("Exception caught\n");
    }

    device::List<device::VideoOutputPort> vPorts = device::Host::getInstance().getVideoOutputPorts();
    for (size_t i = 0; i < vPorts.size(); i++) {
        device::VideoOutputPort &vPort = vPorts.at(i);
        printf("VideoOuputPort Name- [%s] =======================\r\n",    vPort.getName().c_str());
        printf("\t Enabled- [%s]\r\n",    vPort.isEnabled() ? "Yes" : "No");
        try {
         printf("\t Active - [%s]\r\n",    vPort.isActive() ? "Yes" : "No");
        }
        catch (device::UnsupportedOperationException &e) {
            printf("RxSense is NOT supported\r\n");
        }
        printf("\t Connected- [%s]\r\n",    vPort.isDisplayConnected() ? "Yes" : "No");
   	
		const device::List<device::VideoResolution > resolutions = vPort.getType().getSupportedResolutions();
		{
			printf("Resolution Size [%d] =======================\r\n",resolutions.size());

			for (size_t j = 0; j < resolutions.size(); j++) {
				device::VideoResolution resolution = resolutions.at(j);
				 printf("Resolution Name  Name- [%s] =======================\r\n",resolution.getName().c_str());
				}
		}

		printf("\t Content Protected- [%s]\r\n",    vPort.isContentProtected() ? "Yes" : "No");
        try{
            printf("\t Support Dynamic Configuration- [%s]\r\n",    vPort.isDynamicResolutionSupported() ? "Yes" : "No");
        }
        catch(...){
            printf("\t\t error in getting dynamic resolution support \r\n");
        }
        printf("\t Port Type- [%s]\r\n",   vPort.getType().getName().c_str());
        printf("\t\tDTCP Support- [%s]\r\n",    vPort.getType().isDTCPSupported() ? "Yes" : "No");
        printf("\t\tHDCP Support- [%s]\r\n",    vPort.getType().isHDCPSupported() ? "Yes" : "No");
        printf("\t\tResolution Restriction- [%d]\r\n",    vPort.getType().getRestrictedResolution());
        printf("\t\tCurrent Resolution- [%s]\r\n",    vPort.getResolution().getName().c_str());
        printf(" ==========================================\r\n\r\n");
   	}

    try{
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
