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


#define BOOST_TEST_MODULE SetAudioConfig
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
#include "audioEncoding.hpp"
#include "audioCompression.hpp"
#include "audioStereoMode.hpp"


#include "dsUtl.h"
#include "dsError.h"
#include "dsVideoPort.h"

#undef _DS_VIDEOOUTPUTPORTSETTINGS_H
#include "dsVideoPortSettings.h"
#undef _DS_VIDEORESOLUTIONSETTINGS_H
#include "dsVideoResolutionSettings.h"

BOOST_AUTO_TEST_CASE(test_AudioOutputPort_set_Methods)
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
	 * Input From SM:
	 * 1) A Video Port Name (such as "HDMI0", "Component0" etc..) whose connected Audio Port is the target port.
	 * 2) A Property of the target port to set.
	 * 3) SM is responsible to convert the proerty value from string to corresponding types.
	 *
	 * Output to  SM: Void (or exception thrown upon error).
	 */
	try {
		/* convert the video port Name to the corresponding port object */
		device::VideoOutputPort vPort =
				device::Host::getInstance().getVideoOutputPort(std::string("HDMI0"));
		device::AudioOutputPort aPort = vPort.getAudioOutputPort();
		if (true) { /* These are dummy values */
			/* You can set the following 3 values using integer constants or use string names*/
			aPort.setEncoding(device::AudioEncoding::kAC3);
			aPort.setCompression(device::AudioCompression::kMedium);
			aPort.setStereoMode(device::AudioStereoMode::kSurround);
			aPort.setEncoding(std::string("AC3"));
			aPort.setCompression(std::string("MEDIUM"));
			aPort.setStereoMode(std::string("SURROUND"));

			aPort.setDB(1.0);
			aPort.setLevel(2.0);
			aPort.setLoopThru(false);
			aPort.setMuted(false);
		}
	}
	catch(...) {
		BOOST_CHECK(0);
	}
}

BOOST_AUTO_TEST_CASE(SetAudioConfig_InvalidNames)
{
	/*
	 * Output to  SM: Void (or exception thrown upon error).
	 */
	try {
		/* convert the video port Name to the corresponding port object */
		device::VideoOutputPort vPort =
				device::Host::getInstance().getVideoOutputPort(std::string("BadPort"));
		BOOST_CHECK(0);
	}
	catch(...) {
		BOOST_CHECK(1);
	}

	try {
		/* convert the video port Name to the corresponding port object */
		device::VideoOutputPort vPort =
				device::Host::getInstance().getVideoOutputPort(std::string("HDMI0"));
		device::AudioOutputPort aPort = vPort.getAudioOutputPort();
		aPort.setEncoding(device::AudioEncoding::getInstance("BadName").getId());
		BOOST_CHECK(0);
	}
	catch(...) {
		BOOST_CHECK(1);
	}

	try {
		/* convert the video port Name to the corresponding port object */
		device::VideoOutputPort vPort =
				device::Host::getInstance().getVideoOutputPort(std::string("HDMI0"));
		device::AudioOutputPort aPort = vPort.getAudioOutputPort();
		aPort.setCompression(device::AudioCompression::getInstance("BadName").getId());
		BOOST_CHECK(0);
	}
	catch(...) {
		BOOST_CHECK(1);
	}

	try {
		/* convert the video port Name to the corresponding port object */
		device::VideoOutputPort vPort =
				device::Host::getInstance().getVideoOutputPort(std::string("HDMI0"));
		device::AudioOutputPort aPort = vPort.getAudioOutputPort();
		aPort.setStereoMode(device::AudioStereoMode::getInstance("BadName").getId());
		BOOST_CHECK(0);
	}
	catch(...) {
		BOOST_CHECK(1);
	}


}
BOOST_AUTO_TEST_CASE(testDummy)
{
	BOOST_CHECK(1 == 1);
}


/** @} */
/** @} */
