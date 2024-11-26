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

void VideoDeviceConfig::load()
{
	/*
	 * Load Constants First.
	 */
	for (size_t i = 0; i < dsVIDEO_ZOOM_MAX; i++) {
		_vDFCs.push_back(VideoDFC(i));
	}

	/*
	 * Initialize Video Devices (supported DFCs etc.)
	 */
	for (size_t i = 0; i < dsUTL_DIM(kConfigs); i++) {
		_vDevices.push_back(VideoDevice(i));

		for (size_t j = 0; j < kConfigs[i].numSupportedDFCs; j++) {
			_vDevices.at(i).addDFC(VideoDFC::getInstance(kConfigs[i].supportedDFCs[j]));
		}
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
