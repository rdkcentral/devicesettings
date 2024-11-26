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


#define BOOST_TEST_MODULE GetAudioConfig
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


#include "dsUtl.h"
#include "dsError.h"
#include "dsVideoPort.h"
#include "list.hpp"

#undef _DS_VIDEOOUTPUTPORTSETTINGS_H
#include "dsVideoPortSettings.h"
#undef _DS_VIDEORESOLUTIONSETTINGS_H
#include "dsVideoResolutionSettings.h"

BOOST_AUTO_TEST_CASE(test_AudioOutputPort_get_Methods)
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
	 * A VideoOutPort has a AudioOutputPort.
	 * A AudioOutPort has
	 * 1) a set of properties (db, mute, level etc) and
	 * 2) A list of supported AudioEncodings.
	 * 3) A list of supported AudioCompressions.
	 * 4) A list of supported AUdioStereoModes.
	 *
	 * Input From SM: None.
	 * Output to  SM: Audio Port configurations for all audio port instances.
	 */
	try {
		device::List<device::VideoOutputPort> vPorts = device::Host::getInstance().getVideoOutputPorts();
		BOOST_CHECK(vPorts.size() != 0);
		for (int i = 0; i < vPorts.size(); i++) {
			device::AudioOutputPort &aPort = vPorts.at(i).getAudioOutputPort();

			aPort.getCompression();
			aPort.getEncoding();
			aPort.getStereoMode();
			aPort.getGain();
			aPort.getDB();
			aPort.getMaxDB();
			aPort.getMinDB();
			aPort.getOptimalLevel();
			aPort.isLoopThru();
			aPort.isMuted();

			{
				const device::List<device::AudioCompression> aCompressions = aPort.getSupportedCompressions();
				BOOST_CHECK(aCompressions.size() != 0);
				for (size_t j = 0; j < aCompressions.size(); j++) {
					std::cout << "Compressions supported = " <<  aCompressions.at(j).toString() << " FOR " << aPort.getType().getName() <<  std::endl;
				}
			}

			{
				const device::List<device::AudioEncoding> aEncodings = aPort.getSupportedEncodings();
				BOOST_CHECK(aEncodings.size() != 0);
				for (size_t j = 0; j < aEncodings.size(); j++) {
					std::cout << "Encodings supported = " <<  aEncodings.at(j).toString() << " FOR " << aPort.getType().getName() <<  std::endl;
				}
			}

			{
				const device::List<device::AudioStereoMode> & aStereoModes = aPort.getSupportedStereoModes();
				BOOST_CHECK(aStereoModes.size() != 0);
				for (size_t j = 0; j < aStereoModes.size(); j++) {
					std::cout << "fSTereoModes supported = " <<  aStereoModes.at(j).toString() << " FOR " << aPort.getType().getName() <<  std::endl;
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
