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


#define BOOST_TEST_MODULE GetVideoOutputPorts
#define BOOST_TEST_MAIN
#include "boost/test/included/unit_test.hpp"
#include <iostream>

#include "videoOutputPort.hpp"
#include "host.hpp"
#include "videoOutputPortConfig.hpp"
#include "videoResolution.hpp"
#include "frameRate.hpp"
#include "audioOutputPort.hpp"
#include "audioEncoding.hpp"
#include "audioCompression.hpp"
#include "audioStereoMode.hpp"
#include "audioOutputPortType.hpp"


#include "dsUtl.h"
#include "dsError.h"
#include "dsVideoPort.h"

#undef _DS_VIDEOOUTPUTPORTSETTINGS_H
#include "dsVideoPortSettings.h"
#undef _DS_VIDEORESOLUTIONSETTINGS_H
#include "dsVideoResolutionSettings.h"

BOOST_AUTO_TEST_CASE(test_VideoOutputPort_get_Methods)
{
	/*
	 * The following code demonstrate how to get the complete audio configuration information for SM.
	 * All audio ports connected to existing Video Ports are traversed.
	 *
	 * Please note current SM only requires information for HDMI port, alghough such requirment is not
	 * indicated in SM's request message.
	 *
	 * Basic relationship:
	 * A host has a list of VideoOutputPorts.
	 * A VideoOutPort has
	 * 1) a VideoOutPortType, that has a list of supported resolutions.
	 * 2) A set of port specific properties.
	 *
	 */
	try {
		device::List<device::VideoOutputPort> vPorts = device::Host::getInstance().getVideoOutputPorts();
		BOOST_CHECK(vPorts.size() != 0);
		for (size_t i = 0; i < vPorts.size(); i++) {
			device::VideoOutputPort port = vPorts.at(i);
			port.getName();
			port.isDisplayConnected();
			//port.isContentProtected();
			port.isEnabled();
			port.getResolution(); /* Current Resolution */
			BOOST_CHECK(port.getResolution().getName() == std::string(kPorts[i].defaultResolution));

			port.getResolution().getName();
			port.getResolution().getAspectRatio().toString();
			port.getResolution().getPixelResolution().toString();
			port.getResolution().getStereoscopicMode().toString();
			port.getResolution().getFrameRate();
			port.getResolution().isInterlaced();

			port.getType().getId();
			port.getType().isDynamicResolutionsSupported();
			port.getType().isDTCPSupported();
			port.getType().isHDCPSupported();
			port.getType().getRestrictedResolution();
			const device::List<device::VideoResolution > resolutions = port.getType().getSupportedResolutions();
			{
				/* Iterated through supported Resolutions */
				for (size_t j = 0; j < resolutions.size(); j++) {
					device::VideoResolution resolution = resolutions.at(j);
					resolution.getName();
					resolution.getAspectRatio().toString();
					resolution.getPixelResolution().toString();
					resolution.getStereoscopicMode().toString();
					resolution.getFrameRate();
					resolution.isInterlaced();
				}
			}
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
