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
#include "rpAudioOutputPort.h"
#include "audioEncoding.hpp"
#include "audioCompression.hpp"
#include "audioStereoMode.hpp"
#include "list.hpp"
#include "exception.hpp"


#undef _DS_AUDIOOUTPUTPORTSETTINGS_H
#include "rpAudioOutputPortSettings.h"

BOOST_AUTO_TEST_CASE(test_AudioOutputPortConfig_load)
{
    BOOST_CHECK(rpAOP_init() == dsERR_NONE);
    {
        try {
            device::AudioOutputPortConfig & aConfig = device::AudioOutputPortConfig::getInstance();
            device::List<device::AudioOutputPortType> aTypes = aConfig.getSupportedTypes();
            BOOST_CHECK(aTypes.size() == 0);
            aConfig.load();
			aTypes = aConfig.getSupportedTypes();
            BOOST_CHECK(aTypes.size() == dsUTL_DIM(kSupportedPortTypes));

            /* Verify Constants */
            for (size_t i = 0; i < rpAOP_ENC_MAX; i++) {
            	BOOST_CHECK(device::AudioOutputPortConfig::getInstance().getEncoding(i).getId() == i);
            	BOOST_CHECK(!device::AudioOutputPortConfig::getInstance().getEncoding(i).getName().empty());
            	BOOST_CHECK(device::AudioOutputPortConfig::getInstance().getEncoding(i).getId() == device::AudioEncoding::getInstance(i).getId());
            }
            for (size_t i = 0; i < rpAOP_CMP_MAX; i++) {
            	BOOST_CHECK(device::AudioOutputPortConfig::getInstance().getCompression(i).getId() == i);
            	BOOST_CHECK(!device::AudioOutputPortConfig::getInstance().getCompression(i).getName().empty());
            	BOOST_CHECK(device::AudioOutputPortConfig::getInstance().getCompression(i).getId() == device::AudioEncoding::getInstance(i).getId());
            }
            for (size_t i = 0; i < rpAOP_STMODE_MAX; i++) {
            	BOOST_CHECK(device::AudioOutputPortConfig::getInstance().getStereoMode(i).getId() == i);
            	BOOST_CHECK(!device::AudioOutputPortConfig::getInstance().getStereoMode(i).getName().empty());
            	BOOST_CHECK(device::AudioOutputPortConfig::getInstance().getStereoMode(i).getId() == device::AudioEncoding::getInstance(i).getId());
            }
            for (size_t i = 0; i < rpAOP_TYPE_MAX; i++) {
            	BOOST_CHECK(device::AudioOutputPortConfig::getInstance().getPortType(i).getId() == i);
            	BOOST_CHECK(!device::AudioOutputPortConfig::getInstance().getPortType(i).getName().empty());
            	BOOST_CHECK(device::AudioOutputPortConfig::getInstance().getPortType(i).getId() == device::AudioEncoding::getInstance(i).getId());
            }

            BOOST_CHECK(device::AudioOutputPortConfig::getInstance().getPorts().size() == dsUTL_DIM(kPorts));
            for (size_t i = 0; i < dsUTL_DIM(kPorts); i++) {
                BOOST_CHECK(device::AudioOutputPortConfig::getInstance().getPort(i).getId() == i);
            }

            /* verify Capability */
            aTypes = aConfig.getSupportedTypes();

            BOOST_CHECK(aTypes.size() == dsUTL_DIM(kSupportedPortTypes));

            for (size_t i = 0; i < aTypes.size(); i++) {

            	BOOST_CHECK(std::string(aTypes.at(i).getName()).compare(kConfigs[i].name) == 0);
            	BOOST_CHECK(aTypes.at(i).getSupportedEncodings().size() == kConfigs[i].numSupportedEncodings);
            	{
            	    for (size_t j = 0; j < aTypes.at(i).getSupportedEncodings().size(); j++) {
		            	BOOST_CHECK(aTypes.at(i).getSupportedEncodings().at(j).getId() == int(kConfigs[i].encodings[j]));
		            	BOOST_CHECK(std::string(aTypes.at(i).getSupportedEncodings().at(j).toString()).empty() == false);
		            	//std::cout << aTypes.at(i).getSupportedEncodings().at(j).toString() << std::endl;
					}
				}
            	BOOST_CHECK(aTypes.at(i).getSupportedCompressions().size() == kConfigs[i].numSupportedCompressions);
            	{
            	    for (size_t j = 0; j < aTypes.at(i).getSupportedCompressions().size(); j++) {
		            	BOOST_CHECK(aTypes.at(i).getSupportedCompressions().at(j).getId() == int(kConfigs[i].compressions[j]));
		            	BOOST_CHECK(std::string(aTypes.at(i).getSupportedCompressions().at(j).toString()).empty() == false);
		            	//std::cout << aTypes.at(i).getSupportedCompressions().at(j).toString() << std::endl;
					}
				}
            	BOOST_CHECK(aTypes.at(i).getSupportedStereoModes().size() == kConfigs[i].numSupportedStereoModes);
            	{
            	    for (size_t j = 0; j < aTypes.at(i).getSupportedStereoModes().size(); j++) {
		            	BOOST_CHECK(aTypes.at(i).getSupportedStereoModes().at(j).getId() == int(kConfigs[i].stereoModes[j]));
		            	BOOST_CHECK(std::string(aTypes.at(i).getSupportedStereoModes().at(j).toString()).empty() == false);
		            	//std::cout << aTypes.at(i).getSupportedStereoModes().at(j).toString() << std::endl;
					}
				}
            }

            {
            	/* Check Ports */
            	BOOST_CHECK(aConfig.getPorts().size() == dsUTL_DIM(kPorts));
                device::List<device::AudioOutputPortType> vPortTypes = aConfig.getSupportedTypes();
                size_t k = 0;
                for (size_t i = 0; i < vPortTypes.size(); i++) {
                	for (size_t j = 0; j < vPortTypes.at(i).getPorts().size(); j++) {
						BOOST_CHECK(vPortTypes.at(i).getPorts().at(j).getType() == kPorts[k].id.type);
						BOOST_CHECK(vPortTypes.at(i).getPorts().at(j).getId() == k);
						k++;
                	}
                }
            }

        }
        catch (device::Exception &e)
        {
        	e.dump();
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
