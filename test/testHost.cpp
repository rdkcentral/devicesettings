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


#define BOOST_TEST_MODULE Host 
#define BOOST_TEST_MAIN
#include "boost/test/included/unit_test.hpp"
#include <iostream>

#include "videoOutputPort.hpp"
#include "host.hpp"
#include "videoOutputPortConfig.hpp"
#include "videoResolution.hpp"
#include "audioOutputPort.hpp"
#include "audioEncoding.hpp"
#include "audioCompression.hpp"
#include "audioStereoMode.hpp"
#include "audioOutputPortType.hpp"
#include "videoOutputPort.hpp"


#include "dsUtl.h"
#include "dsError.h"
#include "dsVideoPort.h"

#undef _DS_VIDEOOUTPUTPORTSETTINGS_H
#include "dsVideoPortSettings.h"
#undef _DS_VIDEORESOLUTIONSETTINGS_H
#include "dsVideoResolutionSettings.h"

BOOST_AUTO_TEST_CASE(test_Host)
{
	try {
		//BOOST_CHECK(device::Host::getInstance() is instance of hostImpl);
		device::List<device::AudioOutputPort> aPorts = device::Host::getInstance().getAudioOutputPorts();
		BOOST_CHECK(aPorts.size() != 0);

	
        /* Exact Host Module */
        {
            class power: public device::PowerModeChangeListener
            {
            public:
                void powerModeChanged(int newMode)
                {
                    return;
                }
            };

            class disp:public device::DisplayConnectionChangeListener
            {
            public:
                void displayConnectionChanged(device::VideoOutputPort &port, int newConnectionStatus)
                {
                    return;
                }
            };

            device::Host::getInstance().setPowerMode(1);
            device::Host::getInstance().setPowerMode(0);
            device::Host::getInstance().getPowerMode();

#if 0    
            power p;
            disp d;
	    device::Host::getInstance().addDisplayConnectionListener(&d);
            device::Host::getInstance().removeDisplayConnectionListener(&d);
            device::Host::getInstance().addPowerModeListener(&p);
            device::Host::getInstance().removePowerModeChangeListener(&p);
#endif

        }
    }
	catch(...) {
		BOOST_CHECK(0);
	}
}

BOOST_AUTO_TEST_CASE(testDummy)
{
	BOOST_CHECK(1 == 1);
}


/** @} */
/** @} */
