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


#include "videoDevice.hpp"
#include "videoDFC.hpp"
#include "dsVideoDevice.h"
#include "illegalArgumentException.hpp"
#include "exception.hpp"
#include "videoDeviceConfig.hpp"
#include "dsVideoResolutionSettings.h"
#include "host.hpp"

#include "dslogger.h"
#include "dsError.h"
#include "dsTypes.h"
#include "dsUtl.h"
#include <iostream>
#include <sstream>
#include <string.h>

#include "safec_lib.h"

/**
 * @file videoDevice.cpp
 * @brief Video Device is also called "Decoder".
 * VideoDevice objects are instantiated by the Device Settings module upon initialization.
 * Applications do not need to create any such objects on its own.
 * References to these objects can be retrieved by applications via Host::getVideoDevices()
 */
extern int stringToNumber (std::string text);

namespace {
	const char *_names[] = {
			"Decoder 0",
	};

	inline bool isValid(int id) {
		return true;
	}
}

namespace device {

const char * VideoDevice::kPropertyDFC = ".DFC";


/**
 * @addtogroup dssettingsvideodeviceapi
 * @{
 */
/**
 * @fn  VideoDevice & VideoDevice::getInstance(int id)
 * @brief This API is used to get the instance of the video device port based on the port id returned by the getsupported videodevice config.
 *
 * @param[in] id port id
 * @return Reference to the instance of the port id
 */
VideoDevice & VideoDevice::getInstance(int id)
{
	if (::isValid(id)) {
		return VideoDeviceConfig::getInstance().getDevice(id);
	}
	else {
		throw IllegalArgumentException();
	}
}


/**
 * @fn VideoDevice::VideoDevice(int id)
 * @brief Constructor for videodevice class.
 * This function initializes the the handle for the corresponding video device based on id.
 * IllegalArgumentException will be thrown if the specified id of the device is not recognized.
 *
 * @param[in] id Port id.
 * @return None.
 */
VideoDevice::VideoDevice(int id)
{
	dsError_t ret = dsGetVideoDevice(id, &_handle);


	if (ret == dsERR_NONE) {
		_id = id;
		{
			/* Construct Port Name as "Type+Index", such as "HDMI0" */
			std::stringstream out;
			out << "VideoDevice" << _id;
			_name = out.str();
		}
	}
	if (ret != dsERR_NONE) {
		throw IllegalArgumentException();
	}
}


/**
 * @fn VideoDevice::~VideoDevice()
 * @brief This is a default destructor of the class VideoDevice.
 *
 * @return None.
 */
VideoDevice::~VideoDevice() {
	// TODO Auto-generated destructor stub
}


/**
 * @fn void  VideoDevice::setDFC(const VideoDFC & dfc)
 * @brief This API is used to set the DFC[Decoder format convention used for zoom purpose] setting by its property name.
 * Exception  will be thrown if the DFC name is not recognized.
 *
 * @param[in] dfc new zoom setting
 *
 * @return None
 */
void VideoDevice::setDFC(const VideoDFC & dfc)
{
	dsError_t ret = dsSetDFC(_handle, (dsVideoZoom_t)dfc.getId());
	if (ret != dsERR_NONE) {
		throw Exception(ret);
	}
	_dfc = dfc.getId();
}


/**
 * @fn void  VideoDevice::setDFC(int dfc)
 * @brief This API is used to set the DFC with the passed dfc parameter.
 * This function is used to get the instance of VideoDFC.
 *
 * @param[in] dfc  new zoom setting
 *
 * @return None
 */
void VideoDevice::setDFC(int dfc)
{
	setDFC(VideoDFC::getInstance(dfc));
}


/**
 * @fn void VideoDevice::setDFC(const std::string & dfc)
 * @brief This API is used to set the DFC with the passed dfc parameter.
 * This function is used to get the instance of VideoDFC
 * IllegalArgumentException will be thrown if the DFC name is not recognized.
 * UnsupportedOperationException will be thrown if DFC cannot be changed
 *
 * @param[in] dfc Name of the new zoom setting
 *
 * @return None
 */
void VideoDevice::setDFC(const std::string & dfc)
{
	setDFC(VideoDFC::getInstance(dfc));
}


/**
 * @fn void VideoDevice::setDFC()
 * @brief This API is used to set the default dfc.
 *
 * @return None
 */
void VideoDevice::setDFC()
{
	/* sets the default dfc */
	setDFC(VideoDeviceConfig::getInstance().getDefaultDFC());

}


/**
 * @fn void VideoDevice::setPlatformDFC()
 * @brief This API is used to set the DFC setting to the default one supported by the platform.
 * The actual Platform DFC is determined by underlying platform implementation.
 * Applications can use getDFC() API to query the actual DFC set after calling this API.
 *
 * @return None
 */
void VideoDevice::setPlatformDFC()
{
	setDFC(VideoDFC::kPlatform);
}


/**
 * @fn const VideoDFC & VideoDevice::getDFC()
 * @brief This API is used to get the current DFC setting.
 *
 * @return Reference to the current DFC setting
 */
const VideoDFC & VideoDevice::getDFC(){

	dsVideoZoom_t dfc;
	
	dsGetDFC(_handle,&dfc);
	
	_dfc = dfc;
	
	return VideoDFC::getInstance(_dfc);
}


/**
 * @fn const List <VideoDFC> VideoDevice::getSupportedDFCs() const
 * @brief This API is used to get the list of supported DFC (i.e. Zoom Settings) by the video device.
 *
 * @return A List of supported DFC settings
 */
const List <VideoDFC> VideoDevice::getSupportedDFCs() const
{
	return _supportedDFCs;
}


/**
 * @fn void VideoDevice::addDFC(const VideoDFC &dfc)
 * @brief This function is used to push DFC into the list of supported DFC (i.e. Zoom Settings) by the device.
 *
 * @param[in] dfc Supported Zoom setting.
 * @return None.
 */
void VideoDevice::addDFC(const VideoDFC &dfc)
{
	_supportedDFCs.push_back(dfc);
}

void VideoDevice::getHDRCapabilities(int *capabilities)
{
    dsGetHDRCapabilities(_handle, capabilities);
}

void VideoDevice::getSettopSupportedResolutions(std::list<std::string>& stbSupportedResoltuions)
{
	size_t numResolutions = dsUTL_DIM(kResolutions);
	for (size_t i = 0; i < numResolutions; i++)
	{
		dsVideoPortResolution_t *resolution = &kResolutions[i];
		stbSupportedResoltuions.push_back(std::string(resolution->name));
	}

	return;
}

unsigned int VideoDevice::getSupportedVideoCodingFormats() const
{
	unsigned int formats = 0;
	if(dsERR_NONE != dsGetSupportedVideoCodingFormats(_handle, &formats))
	{
		return 0;
	}
	else
	{
		return formats;
	}
}

dsVideoCodecInfo_t VideoDevice::getVideoCodecInfo(dsVideoCodingFormat_t format) const
{
	dsVideoCodecInfo_t info = {0};
	dsGetVideoCodecInfo(_handle, format, &info);
	return info;
}

int VideoDevice::forceDisableHDRSupport(bool disable)
{
	dsForceDisableHDRSupport(_handle, disable);
	return 0;
}

int VideoDevice::setFRFMode(int frfmode) const
{
        dsError_t ret;
        ret = dsSetFRFMode(_handle, frfmode);
        return 0;
}

int VideoDevice::getFRFMode(int *frfmode) const
{
        dsError_t ret;
        int frfmode1;
        ret = dsGetFRFMode(_handle, &frfmode1);
        *frfmode = frfmode1;
        return 0;
}

int VideoDevice::setDisplayframerate(const char *framerate) const
{
        dsError_t ret;
        char buf[20] = {0};
	    strncpy(buf, framerate, sizeof(buf)-1);

        ret = dsSetDisplayframerate(_handle, buf);
        return 0;
}

int VideoDevice::getCurrentDisframerate(char *framerate) const
{
        dsError_t ret;
        char getframerate[20];
        ret = dsGetCurrentDisplayframerate(_handle, getframerate);
	    strncpy(framerate, getframerate, 20);

     	return 0;
}


/**
 * @fn void  VideoDevice::Register(IEvent *listener)
 * @brief This API is used to register the Events
 *
 * @return unint32_t
 */
uint32_t VideoDevice::Register(IEvent *listener) 
{
  return 0;
}


/**
 * @fn void  VideoDevice::UnRegister(IEvent *listener)
 * @brief This API is used to UnRegister the Events
 *
 * @return unint32_t
 */
uint32_t VideoDevice::UnRegister(IEvent *listener) 
{
  return 0;
}

}

/** @} */

/** @} */
/** @} */
