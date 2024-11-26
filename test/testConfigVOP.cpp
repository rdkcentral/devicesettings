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
#include "exception.hpp"


#undef _DS_VIDEOOUTPUTPORTSETTINGS_H
#include "dsVideoPortSettings.h"
#undef _DS_VIDEORESOLUTIONSETTINGS_H
#include "dsVideoResolutionSettings.h"

BOOST_AUTO_TEST_CASE(test_VideoOutputPortConfig_load)
{
	BOOST_CHECK(rpVOP_init() == dsERR_NONE);
	{
		try {
			device::AudioOutputPortConfig::getInstance().load();

			device::VideoOutputPortConfig & vConfig = device::VideoOutputPortConfig::getInstance();
			device::List<device::VideoOutputPortType> vTypes = vConfig.getSupportedTypes();
			BOOST_CHECK(vTypes.size() == 0);

			vConfig.load();
			 vTypes = vConfig.getSupportedTypes();
			BOOST_CHECK(vTypes.size() == dsUTL_DIM(kSupportedPortTypes));
			/* Verify Constants */
			for (size_t i = 0; i < rpVOP_PIXELRES_MAX; i++) {
				BOOST_CHECK(device::VideoOutputPortConfig::getInstance().getPixelResolution(i).getId() == i);
				BOOST_CHECK(!device::VideoOutputPortConfig::getInstance().getPixelResolution(i).getName().empty());
				BOOST_CHECK(device::VideoOutputPortConfig::getInstance().getPixelResolution(i).getId() == device::PixelResolution::getInstance(i).getId());
			}
			for (size_t i = 0; i < rpVOP_ASPECT_MAX; i++) {
				BOOST_CHECK(device::VideoOutputPortConfig::getInstance().getAspectRatio(i).getId() == i);
				BOOST_CHECK(!device::VideoOutputPortConfig::getInstance().getAspectRatio(i).getName().empty());
				BOOST_CHECK(device::VideoOutputPortConfig::getInstance().getAspectRatio(i).getId() == device::AspectRatio::getInstance(i).getId());

			}
			for (size_t i = 0; i < rpVOP_SSMODE_MAX; i++) {
				BOOST_CHECK(device::VideoOutputPortConfig::getInstance().getSSMode(i).getId() == i);
				BOOST_CHECK(!device::VideoOutputPortConfig::getInstance().getPortType(i).getName().empty());
				BOOST_CHECK(device::VideoOutputPortConfig::getInstance().getSSMode(i).getId() == device::StereoScopicMode::getInstance(i).getId());

			}
			for (size_t i = 0; i < rpVOP_FRAMERATE_MAX; i++) {
				BOOST_CHECK(device::VideoOutputPortConfig::getInstance().getFrameRate(i).getId() == i);
				BOOST_CHECK(!device::VideoOutputPortConfig::getInstance().getFrameRate(i).getName().empty());
				BOOST_CHECK(device::VideoOutputPortConfig::getInstance().getFrameRate(i).getId() == device::FrameRate::getInstance(i).getId());

			}
			BOOST_CHECK(device::VideoOutputPortConfig::getInstance().getPorts().size() == dsUTL_DIM(kPorts));

			for (size_t i = 0; i < dsUTL_DIM(kPorts); i++) {
				BOOST_CHECK(device::VideoOutputPortConfig::getInstance().getPort(i).getId() == i);
			}

			/* verify Capability */
			vTypes = vConfig.getSupportedTypes();

			BOOST_CHECK(vTypes.size() == dsUTL_DIM(kSupportedPortTypes));

			for (size_t i = 0; i < vTypes.size(); i++) {
				/* find this type in the kConfigs */
				{
					bool found = false;
					for (size_t j = 0; j < dsUTL_DIM(kConfigs); j++) {
						if (vTypes.at(i).getId() == kConfigs[i].typeId) {
							BOOST_CHECK(std::string(vTypes.at(i).getName()).compare(kConfigs[i].name) == 0);
							BOOST_CHECK(vTypes.at(i).isHDCPSupported() == kConfigs[i].hdcpSupported);
							BOOST_CHECK(vTypes.at(i).getRestrictedResolution() == kConfigs[i].restrictedResollution);
							BOOST_CHECK(vTypes.at(i).isDTCPSupported() == kConfigs[i].dtcpSupported);
							found = true;
							break;
						}
					}
				}

				BOOST_CHECK(vTypes.at(i).getSupportedResolutions().size() == dsUTL_DIM(kResolutions));

				for (size_t j = 0; j < vTypes.at(i).getSupportedResolutions().size(); j++) {
					BOOST_CHECK(std::string(vTypes.at(i).getSupportedResolutions().at(j).getName()).compare(std::string(kResolutions[j].name)) == 0);

					BOOST_CHECK(vTypes.at(i).getSupportedResolutions().at(j).getPixelResolution() == kResolutions[j].pixelResolution);

					BOOST_CHECK(vTypes.at(i).getSupportedResolutions().at(j).getAspectRatio() == kResolutions[j].aspectRatio);

					BOOST_CHECK(vTypes.at(i).getSupportedResolutions().at(j).getStereoscopicMode() == kResolutions[j].stereoScopicMode);

					BOOST_CHECK(vTypes.at(i).getSupportedResolutions().at(j).getFrameRate().getId() == kResolutions[j].frameRate);

					BOOST_CHECK(vTypes.at(i).getSupportedResolutions().at(j).isInterlaced() == kResolutions[j].interlaced);
					BOOST_CHECK(vTypes.at(i).getSupportedResolutions().at(j).isEnabled());
				}

				{
					/* Check Ports */
					BOOST_CHECK(vConfig.getPorts().size() == dsUTL_DIM(kPorts));
					device::List<device::VideoOutputPortType> vPortTypes = vConfig.getSupportedTypes();
					size_t k = 0;
					for (size_t i = 0; i < vPortTypes.size(); i++) {

						for (size_t j = 0; j < vPortTypes.at(i).getPorts().size(); j++) {

							{ /* find this port in kPorts */
								bool found = false;
								for (size_t k = 0; k < dsUTL_DIM(kPorts); k++) {
									if (vPortTypes.at(i).getId() == kPorts[k].id.type && vPortTypes.at(i).getPorts().at(j).getIndex() == kPorts[k].id.index) {
										BOOST_CHECK(vPortTypes.at(i).getPorts().at(j).getType().getId() == kPorts[k].id.type);
										BOOST_CHECK(std::string(vPortTypes.at(i).getPorts().at(j).getResolution().getName()).compare(std::string(kPorts[k].defaultResolution)) == 0);
										found = true;
										break;
									} else {
									}
								}
								BOOST_CHECK(found);
							}
						}
					}
				}
			}
		}
		catch (const device::IllegalArgumentException &e)
		{
			BOOST_CHECK(0);

		}
		catch (const int &e)
		{
			BOOST_CHECK(0);

		}
		catch (const dsError_t &e)
		{
			BOOST_CHECK(0);

		}
		catch (const device::Exception &e) {
			//e.dump();
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
