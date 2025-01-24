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
int main(int argc, char *argv[]) 
{

        char c = 'c';  
	string sleep_mode_str; 
	int ret;
	IARM_Bus_Init("powerModeTest");
	IARM_Bus_Connect();

	       
	try
	{
	
		device::Manager::Initialize();

		while(1)
		{
			printf("GetPreferredSleepMode - 'g'\n");
			printf("SetPreferredSleepMode - 's'\n");
			printf("All Sleep Modes - 'l' \n");
			printf("Exit - 'x'\n");
			c = getchar();
			switch(c)
			{
			case 'g':
			{
				const device::SleepMode &mode = device::Host::getInstance().getPreferredSleepMode();
				std::cout << "preffered sleep mode is" << mode.toString() << std::endl; 	
			break;
			}
			case 's':
			{
				std::cout << "Please provide the sleep mode" << std::endl;
				std::cin >> sleep_mode_str;
				const device::SleepMode &mode= device::SleepMode::getInstance(sleep_mode_str);
				ret = device::Host::getInstance().setPreferredSleepMode(mode);
				std::cout << "Standby mode set retruned " << ret << std::endl; 	
			break;
			}
	        case 'l':
	        {
				int i;
				std::cout << "LIst of Sleep Modes" << std::endl;
				const device::List<device::SleepMode> sleepModes = device::Host::getInstance().getAvailableSleepModes();
	            for(i=0;i < sleepModes.size(); i++) 
	            { 
	                    
	                   std::cout<<i<<std::endl<<sleepModes.at(i).toString()<<std::endl;
	            }
	        break;        
	        }
			case 'x':
			{
	        	device::Manager::DeInitialize();
				IARM_Bus_Disconnect();
				IARM_Bus_Term();		
				return 0;
			}
			}
		}
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
	IARM_Bus_Disconnect();
	IARM_Bus_Term();
	
    return 0;
}


/** @} */
/** @} */
