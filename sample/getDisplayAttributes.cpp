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


#include "libIBus.h"



int main(int argc, char *argv[]) 
{
   
	uint8_t physicalAddressA=1,physicalAddressB=0,physicalAddressC =0,physicalAddressD=0;

	IARM_Bus_Init("SampleDSClient");
	IARM_Bus_Connect();

    try{
        device::Manager::Initialize();
        
	    device::VideoOutputPort vPort = device::Host::getInstance().getVideoOutputPort("HDMI0");
    
        if (vPort.isDisplayConnected()) {
            
	    	printf("\t Display [%s] connected\r\n", vPort.isDisplayConnected() ? "is" : "is NOT");
            
	    	printf("\t Display has AspectRatio [%s]\r\n", vPort.getDisplay().getAspectRatio().getName().c_str());
            
	    	printf("\t Display has EDID [week=%d, year=%d, pcode=0x%x, pserial=0x%x]\r\n",
                    vPort.getDisplay().getManufacturerWeek(),
                    vPort.getDisplay().getManufacturerYear(),
                    vPort.getDisplay().getProductCode(),
	    	        vPort.getDisplay().getSerialNumber());
    
	    	printf("\t Display Connected Device is [%s]\r\n", vPort.getDisplay().getConnectedDeviceType() ? "HDMI" : "DVI");
	    	vPort.getDisplay().getPhysicallAddress(physicalAddressA,physicalAddressB,physicalAddressC,physicalAddressD);
	    	printf("Physical Addres is %d.%d.%d.%d \r\n", physicalAddressA,physicalAddressB,physicalAddressC,physicalAddressD);
    
    
    
	    }
        else 
	    { 
            printf("\t Display [%s] connected\r\n", vPort.isDisplayConnected() ? "is" : "is NOT");
        }
    
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
