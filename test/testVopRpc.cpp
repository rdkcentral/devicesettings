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


#define BOOST_TEST_MODULE rpAOP
#define BOOST_TEST_MAIN
#include "boost/test/included/unit_test.hpp"
#include <iostream>

#include "audioOutputPortConfig.hpp"

#include "dsUtl.h"
#include "dsError.h"
#include "dsVideoPort.h"
#include "rpAudioOutputPort.h"
#include "list.hpp"
#include "libIARM.h"
#include "dsTypes.h"

#ifdef __cplusplus
extern "C" {
#endif
    IARM_Result_t UIDev_Init(char *name);
#ifdef __cplusplus
}
#endif



#undef _DS_AUDIOOUTPUTPORTSETTINGS_H
#include "dsVideoPortSettings.h"

BOOST_AUTO_TEST_CASE(test_VideoOutputPortConfig_load)
{
    UIDev_Init("DSCli");
    BOOST_CHECK(rpVOP_init() == dsERR_NONE);
    {
		intptr_t handle = 0;
        bool enabled = false;
    	BOOST_CHECK(rpVOP_getPortHandle(rpVOP_TYPE_HDMI, 0, &handle) == dsERR_NONE);
        BOOST_CHECK(rpVOP_isEnabled(handle, &enabled) != dsERR_NONE);
        bool connected = false;
        BOOST_CHECK(rpVOP_isDisplayConnected(handle, &connected) == dsERR_NONE);
        std::cout << "Display Is Connected = " << connected << std::endl;
    	BOOST_CHECK(rpVOP_term() == dsERR_NONE);
    }
}

BOOST_AUTO_TEST_CASE(testDummy)
{
	BOOST_CHECK(1 == 1);
}


/** @} */
/** @} */
