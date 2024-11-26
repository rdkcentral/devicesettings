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
* @defgroup test
* @{
**/


#define BOOST_TEST_MODULE rpVOP
#define BOOST_TEST_MAIN
#include "boost/test/included/unit_test.hpp"
#include <iostream>

#include "videoOutputPort.hpp"
#include "videoOutputPortConfig.hpp"
#include "audioOutputPortConfig.hpp"

#include "videoResolution.hpp"
#include "frameRate.hpp"
#include "dsUtl.h"
#include "dsError.h"
#include "dsVideoPort.h"
#include "illegalArgumentException.hpp"
#include "list.hpp"

#include "videoDevice.hpp"
#include "videoDeviceConfig.hpp"
#include "dsVideoDevice.h"
#include "dsVideoDeviceSettings.h"


BOOST_AUTO_TEST_CASE(test_VideoOutputPortConfig_load)
{
    BOOST_CHECK(rpVID_init() == dsERR_NONE);
    {
    	device::VideoDeviceConfig::getInstance().load();
        try {
        	BOOST_CHECK(device::VideoDeviceConfig::getInstance().getDevices().size() == dsUTL_DIM(kConfigs));
        	device::List<device::VideoDevice>  vDevices = device::VideoDeviceConfig::getInstance().getDevices();

        	for (size_t i = 0; i < vDevices.size(); i++) {
        		BOOST_CHECK(device::VideoDeviceConfig::getInstance().getDevices().at(i).getSupportedDFCs().size() == kConfigs[i].numSupportedDFCs);
        	}

        }
        catch (...)
        {
            BOOST_CHECK(0);
        }
    }

}

BOOST_AUTO_TEST_CASE(testDummy)
{
	BOOST_CHECK(1 == 1);
}


/** @} */
/** @} */
