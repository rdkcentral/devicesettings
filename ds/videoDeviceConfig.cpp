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
* @defgroup ds
* @{
**/


#include "videoDeviceConfig.hpp"
#include "dsVideoDeviceSettings.h"
#include "videoDevice.hpp"
#include "videoDFC.hpp"
#include <iostream>
#include "dslogger.h"
#include "manager.hpp"

#define DEBUG 1 // Using for dumpconfig 

namespace device {

VideoDeviceConfig::VideoDeviceConfig() {
	// TODO Auto-generated constructor stub

}

VideoDeviceConfig::~VideoDeviceConfig() {
	// TODO Auto-generated destructor stub
}

VideoDeviceConfig & VideoDeviceConfig::getInstance() {
    static VideoDeviceConfig _singleton;
	return _singleton;
}

List<VideoDevice>  VideoDeviceConfig::getDevices()
{
	List<VideoDevice> devices;
	for (std::vector<VideoDevice>::const_iterator it = _vDevices.begin(); it != _vDevices.end(); it++) {
		devices.push_back(*it);
	}

	return devices;
}

VideoDevice &VideoDeviceConfig::getDevice(int i)
{
	return _vDevices.at(i);
}

List<VideoDFC>  VideoDeviceConfig::getDFCs()
{
	List<VideoDFC> DFCs;
	for (std::vector<VideoDFC>::iterator it = _vDFCs.begin(); it != _vDFCs.end(); it++) {
		DFCs.push_back(*it);
	}

	return DFCs;
}

VideoDFC & VideoDeviceConfig::getDFC(int id)
{
	return _vDFCs.at(id);
}

VideoDFC & VideoDeviceConfig::getDefaultDFC()
{
	return _vDFCs.back();
}

void dumpconfig(videoDeviceConfig_t *config)
{
    if (nullptr == config) {
        INT_ERROR("Video config is NULL");
        return;
    }
    if ( -1 == access("/opt/dsMgrDumpDeviceConfigs", F_OK) ) {
        INT_INFO("Dumping of Device configs is disabled");
        return;
    }

    INT_INFO("\n\n=========================================================================================================================\n\n");
    INT_INFO("Starting to Dump VideoDevice Configs");
	if( nullptr != config->pKVideoDeviceConfigs )
	{
        int configSize = (config->pKVideoDeviceConfigs_size) ? *(config->pKVideoDeviceConfigs_size) : -1;
		INT_INFO("pKVideoDeviceConfigs = %p", config->pKVideoDeviceConfigs);
		INT_INFO("videoDeviceConfigs_size = %d", configSize);
		for (int i = 0; i < configSize; i++) {
            dsVideoConfig_t* videoDeviceConfig = &config->pKVideoDeviceConfigs[i];
            INT_INFO("pKVideoDeviceConfigs[%d].numSupportedDFCs = %lu ", i, videoDeviceConfig->numSupportedDFCs);
			for (int j = 0; j < videoDeviceConfig->numSupportedDFCs; j++) {
				INT_INFO(" Address of pKVideoDeviceConfigs[%d].supportedDFCs[%d] = %d", i, j, videoDeviceConfig->supportedDFCs[j]);
			}
		}
	}
	else
	{
		INT_ERROR(" kVideoDeviceConfigs is NULL");
	}
    INT_INFO("Dump VideoDevice Configs done");
	INT_INFO("\n\n=========================================================================================================================\n\n");
}

void VideoDeviceConfig::load(videoDeviceConfig_t* dynamicVideoDeviceConfigs)
{
	int configSize = -1;
	videoDeviceConfig_t configuration = {0};

	INT_INFO("Enter function");
	/*
	 * Load Constants First.
	 */
	for (size_t i = 0; i < dsVIDEO_ZOOM_MAX; i++) {
		_vDFCs.push_back(VideoDFC(i));
	}

    INT_INFO("Using '%s' config", dynamicVideoDeviceConfigs ? "dynamic" : "static");
    if ( nullptr != dynamicVideoDeviceConfigs )
    {
        configuration = *dynamicVideoDeviceConfigs;
        configSize = (configuration.pKVideoDeviceConfigs_size) ? *(configuration.pKVideoDeviceConfigs_size) : -1;
    }
    else {
        configuration.pKVideoDeviceConfigs = (dsVideoConfig_t *)kConfigs;
		configSize = dsUTL_DIM(kConfigs);
        configuration.pKVideoDeviceConfigs_size = &configSize;
    }

    INT_INFO("VideoDevice Config[%p] ConfigSize[%d]", configuration.pKVideoDeviceConfigs, configSize);

    dumpconfig(&configuration);

	/*
	 * Initialize Video Devices (supported DFCs etc.)
	 */
	if ( nullptr != configuration.pKVideoDeviceConfigs )
	{
		for (size_t i = 0; i < configSize; i++) {
            dsVideoConfig_t* videoDeviceCfg = &configuration.pKVideoDeviceConfigs[i];
            _vDevices.push_back(VideoDevice(i));

			for (size_t j = 0; j < videoDeviceCfg->numSupportedDFCs; j++) {
				_vDevices.at(i).addDFC(VideoDFC::getInstance(videoDeviceCfg->supportedDFCs[j]));
			}
		}
	}
	else
	{
		INT_ERROR(" Configs are NULL and  config size are -1");
	}
	INT_INFO("Exit function");
}

void VideoDeviceConfig::release()
{
        _vDFCs.clear();
        _vDevices.clear();
}

}


/** @} */
/** @} */
