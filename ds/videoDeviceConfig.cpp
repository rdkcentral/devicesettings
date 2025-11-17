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


void dumpconfig(dsVideoConfig_t *pKVideoDeviceConfigs, int videoDeviceConfigs_size)
{
	INT_INFO("%d:%s: Entering function\n", __LINE__, __func__);
	INT_INFO("%d:%s: pKVideoDeviceConfigs = %p\n", __LINE__, __func__, pKVideoDeviceConfigs);
	INT_INFO("%d:%s: videoDeviceConfigs_size = %d\n", __LINE__, __func__, videoDeviceConfigs_size);

	INT_INFO("\n\n=========================================================================================================================\n\n");
	if(pKVideoDeviceConfigs != NULL && videoDeviceConfigs_size != -1)
	{
		for (int i = 0; i < videoDeviceConfigs_size; i++) {
			INT_INFO("pKVideoDeviceConfigs[%d].numSupportedDFCs = %lu\n ", i, pKVideoDeviceConfigs[i].numSupportedDFCs);
			for (int j = 0; j < pKVideoDeviceConfigs[i].numSupportedDFCs; j++) {
				INT_INFO(" Address of pKVideoDeviceConfigs[%d].supportedDFCs[%d] = %d", i, j, pKVideoDeviceConfigs[i].supportedDFCs[j]);
			}
		}
	}
	else
	{
		INT_ERROR("%d:%s:  kVideoDeviceConfigs is NULL and  videoDeviceConfigs_size is -1\n", __LINE__, __func__);
	}

	INT_INFO("\n\n=========================================================================================================================\n\n");
	INT_INFO("%d:%s: Exit function\n", __LINE__, __func__);
}

typedef struct videoDeviceConfig
{
	dsVideoConfig_t *pKVideoDeviceConfigs;
	int *pKVideoDeviceConfigs_size;
}videoDeviceConfig_t;

void VideoDeviceConfig::load()
{
	int configSize, invalid_size = -1;
	videoDeviceConfig_t videoDeviceConfig = {0};
	const char* searchVaribles[] = {
        "kVideoDeviceConfigs",
        "kVideoDeviceConfigs_size",
    };
	bool ret = false;

	/*
	 * Load Constants First.
	 */
	for (size_t i = 0; i < dsVIDEO_ZOOM_MAX; i++) {
		_vDFCs.push_back(VideoDFC(i));
	}

	INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[0]);
	ret = searchConfigs(searchVaribles[0], (void **)&(videoDeviceConfig.pKVideoDeviceConfigs));
	if(ret == true)
	{
		INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[1]);
		ret = searchConfigs(searchVaribles[1], (void **)&(videoDeviceConfig.pKVideoDeviceConfigs_size));
		if(ret == false)
		{
			INT_ERROR("%s is not defined\n", searchVaribles[1]);
			videoDeviceConfig.pKVideoDeviceConfigs_size = &invalid_size;
		}
	}
	else
	{
		INT_INFO("Read Old Configs\n");
		videoDeviceConfig.pKVideoDeviceConfigs = (dsVideoConfig_t *)kConfigs;
		configSize = dsUTL_DIM(kConfigs);
		videoDeviceConfig.pKVideoDeviceConfigs_size = &configSize;
	}

	/*
	 * Initialize Video Devices (supported DFCs etc.)
	 */
	if (videoDeviceConfig.pKVideoDeviceConfigs != NULL && videoDeviceConfig.pKVideoDeviceConfigs_size != NULL &&
		*(videoDeviceConfig.pKVideoDeviceConfigs_size) > 0)
	{
		#if DEBUG
		dumpconfig(videoDeviceConfig.pKVideoDeviceConfigs, *(videoDeviceConfig.pKVideoDeviceConfigs_size));
		#endif
		for (size_t i = 0; i < *(videoDeviceConfig.pKVideoDeviceConfigs_size); i++) {
			_vDevices.push_back(VideoDevice(i));

			for (size_t j = 0; j < videoDeviceConfig.pKVideoDeviceConfigs[i].numSupportedDFCs; j++) {
				_vDevices.at(i).addDFC(VideoDFC::getInstance(videoDeviceConfig.pKVideoDeviceConfigs[i].supportedDFCs[j]));
			}
		}
	}
	else
	{
		INT_ERROR("%d:%s:  Congigs are NULL and  config size are -1\n", __LINE__, __func__);
	}
}

void VideoDeviceConfig::release()
{
        _vDFCs.clear();
        _vDevices.clear();
}

}


/** @} */
/** @} */
