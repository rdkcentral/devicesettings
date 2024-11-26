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


#include "videoResolution.hpp"
#include "pixelResolution.hpp"
#include "videoOutputPortConfig.hpp"
#include "illegalArgumentException.hpp"
#include <string>
#include "dslogger.h"

/**
 * @file videoResolution.cpp
 * @brief This file defines the videoResolution objects by the device settings module  upon intialization.
 *
 */
namespace device {


/**
 * @addtogroup dssettingsvidresolutionapi
 * @{
 */
/**
 * @fn const VideoResolution & VideoResolution::getInstance(int id)
 * @brief This API is used to get the instance of the video resolution port based on the port id returned by the getsupported video resolution .
 *
 * @param[in] id port id
 * @return Reference to the instance of the port id.
 */
const VideoResolution & VideoResolution::getInstance(int id)
{
	return VideoOutputPortConfig::getInstance().getVideoResolution(id);
}


/**
 * @fn const VideoResolution & VideoResolution::getInstance(const std::string &name, bool isIgnoreEdid)
 * @brief This API is used to get the instance of the video resolution port with the name as passed parameter and comparing name with the
 * supported resolution . If matched, it returns the supported resolution.
 *
 * @param[in] name Name of the port
 * @param[in] isIgnoreEdid request to ignore Edid Check.
 * @return Reference to the instance of the name of the port
 */
const VideoResolution & VideoResolution::getInstance(const std::string &name, bool isIgnoreEdid)
{
	const List<VideoResolution> supportedResollutions = VideoOutputPortConfig::getInstance().getSupportedResolutions(isIgnoreEdid);

	for (size_t i = 0; i < supportedResollutions.size(); i++) {
		if (name.compare(std::string(supportedResollutions.at(i).getName())) == 0) {
			return supportedResollutions.at(i);
		}
	}
 	throw IllegalArgumentException();
}


/**
 * @fn VideoResolution::VideoResolution(const int id, const std::string &name,int pixelResolutionId, int ratioId,
 * int ssModeId,int frameRateId, bool interlaced, bool enabled)
 * @brief This function is a default constructor for videoResolution. It initialises the data members
 * of video Resolution instance with the parameters passed.
 *
 * @param[in] type Type of video Resolution.
 * @param[in] name Name of the video Resolution port.
 * @param[in] pixelResolutionId Pixel resolution[720x480,720x480,..] id.
 * @param[in] frameRateId Frame rate[24,25, 59.94,.... frames per second] id.
 * @param[in] ratioId Aspect ratio[4:3, 16:9,...] id.
 * @param[in] ssModeId Stereoscopic mode[2D, 3D,...] id.
 * @param[in] interlaced Scan mode type . True if interlaced or False.
 * @param[in] enabled True if video port is enabled. Else, false.
 *
 * @return None
 */
VideoResolution::VideoResolution(const int id, const std::string &name,
						int pixelResolutionId, int ratioId, int ssModeId,
						int frameRateId, bool interlaced, bool enabled) :
						DSConstant(id, name),
						_pixelResolutionId(pixelResolutionId),
						_aspectRatioId(ratioId),
						_stereoScopicModeId(ssModeId),
						_frameRateId(frameRateId),
						_interlaced(interlaced),
						_enabled(enabled)
{
	_id = id;
	_name = name;
}


/**
 * @fn VideoResolution::~VideoResolution()
 * @brief This is a default destructor of class VideoResolution.
 *
 * @return None.
 */
VideoResolution::~VideoResolution()
{
}


/**
 * @fn  VideoResolution::getPixelResolution() const
 * @brief This API is used to get the pixel format of the given video output port.
 *
 * @return A list of pixel resolution instance
 */
const PixelResolution & VideoResolution::getPixelResolution() const
{
	return PixelResolution::getInstance(_pixelResolutionId);
}


/**
 * @fn AspectRatio & VideoResolution::getAspectRatio() const
 * @brief This API is used to get the current Aspect Ratio setting of the Display Device (i.e. TV) that is connected to the port.
 * Its value is independent to the current zoom settings (which includes the Aspect Ratio of the output format) on the Video Output Port.
 * IllegalStateException will be thrown if the display is not connected.
 * UnsupportedOperationException will be thrown if the Display's Aspect Ratio setting is not available.
 *
 * @return Aspect ratio instance
 */
const AspectRatio & VideoResolution::getAspectRatio() const
{
	return AspectRatio::getInstance(_aspectRatioId);
}


/**
 * @fn VideoResolution::getStereoscopicMode() const
 * @brief This API is used to get the stereoscopic mode of the given video output port.
 *
 * @return Reference to the stereoscopicemode instance
 */
const StereoScopicMode & VideoResolution::getStereoscopicMode() const {
	return StereoScopicMode::getInstance(_stereoScopicModeId);
}


/**
 * @fn VideoResolution::getFrameRate() const
 * @brief This API is used to get the frame rate of the given video output port.
 *
 * @return Reference to the frame rate instance id
 */
const FrameRate & VideoResolution::getFrameRate() const{
	return FrameRate::getInstance(_frameRateId);
}


/**
 * @fn VideoResolution::isInterlaced() const
 * @brief This API is used to check the video is interlaced or not.
 *
 * @return True or False
 * @retval True If video is interlaced
 * @retval False If video is not interlaced
 */
bool VideoResolution::isInterlaced() const{
	return _interlaced;
}


/**
 * @fn VideoResolution::isEnabled() const
 * @brief This API is used to check whether the current resolution is enabled or not.
 *
 * @return True or False
 * @retval True If current resolution is enabled
 * @retval False If current resolution is disabled
 */
bool VideoResolution::isEnabled() const{
	return _enabled;
}


}

/** @} */ //End of Doxygen tag

/** @} */
/** @} */
