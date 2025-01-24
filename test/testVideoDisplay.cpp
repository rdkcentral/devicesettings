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


#define BOOST_TEST_MODULE dsAOP
#define BOOST_TEST_MAIN
#include "boost/test/included/unit_test.hpp"
#include <iostream>

#include "audioOutputPortConfig.hpp"

#include "dsUtl.h"
#include "dsError.h"
#include "dsAudio.h"
#include "audioEncoding.hpp"
#include "audioCompression.hpp"
#include "audioStereoMode.hpp"
#include "list.hpp"
#include "dsTypes.h"
#include "dsDisplay.h"


#undef _DS_AUDIOOUTPUTPORTSETTINGS_H
#include "dsAudioSettings.h"

BOOST_AUTO_TEST_CASE(test_AudioOutputPortConfig_load)
{
    BOOST_CHECK(rpVDISP_init() == dsERR_NONE);
    {
		intptr_t handle = 0;
    	BOOST_CHECK(rpVDISP_getHandle(rpVOP_TYPE_HDMI, 0, &handle) == dsERR_NONE);
    	BOOST_CHECK(rpVDISP_getHandle(rpVOP_TYPE_HDMI, 1, &handle) != dsERR_NONE);
    	BOOST_CHECK(rpVDISP_getHandle(rpVOP_TYPE_COMPONENT, 0, &handle) != dsERR_NONE);

        rpVOP_AspectRatio_t aspect;
        rpVDISP_EDID_t edid;
    	BOOST_CHECK(rpVDISP_getAspectRatio(handle, &aspect) == dsERR_NONE);
    	BOOST_CHECK(rpVDISP_getEDID(handle, &edid) == dsERR_NONE);
    	BOOST_CHECK(rpVDISP_term() == dsERR_NONE);
    }
}

BOOST_AUTO_TEST_CASE(testDummy)
{
	BOOST_CHECK(1 == 1);
}


/** @} */
/** @} */
