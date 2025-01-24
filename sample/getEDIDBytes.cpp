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

    try {
        device::Manager::Initialize();
        
        device::VideoOutputPort vPort = device::Host::getInstance().getVideoOutputPort("HDMI0");

        if (vPort.isDisplayConnected()) {
            printf("\t Display [%s] connected\r\n", vPort.isDisplayConnected() ? "is" : "is NOT");
            std::vector<unsigned char> bytes;
            vPort.getDisplay().getEDIDBytes(bytes);
            printf("\t Display [%s] has %d bytes EDID\r\n", "HDMI",  bytes.size());
            /* Dump the bytes */
            for (int i = 0; i < bytes.size(); i++) {
                if (i % 16 == 0) {
                    printf("\r\n");
                }
                if (i % 128 == 0) {
                    printf("\r\n");
                }
                printf("%02X ", bytes[i]);
            }

            printf("\r\n");
            {    
                unsigned char sum = 0;
                for (int i = 0; i < 128; i++) {
                    sum += bytes[i];
                }    

                if (sum != 0) { 
                    printf("[EDID Sanity Warning] : Checksum is invalid\r\n");
                }    
                else {
                    printf("[EDID Checksum is valid \r\n");
                }
            }    

        }
        else 
        { 
            printf("\t Display [%s] connected\r\n", vPort.isDisplayConnected() ? "is" : "is NOT");
        }

        device::Manager::DeInitialize();
    }
    catch(...) {
        printf("Exception Caught, Abort operation\r\n");
    }
	IARM_Bus_Disconnect();
	IARM_Bus_Term();

    return 0;
}


/** @} */
/** @} */
