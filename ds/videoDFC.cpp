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
 * @file videoDFC.cpp
 * @brief This file contains implementation of VideoDFC class methods, support functions
 * and variable assignments to manage the video decoder format conversions.
 */




/**
* @defgroup devicesettings
* @{
* @defgroup ds
* @{
**/


#include "videoDFC.hpp"
#include "illegalArgumentException.hpp"
#include "videoDeviceConfig.hpp"
#include "dsTypes.h"
#include "dsUtl.h"
#include "dslogger.h"
#include <string.h>

namespace {
	const char *_names[] = {
			"None",
			"Full",
			"Letterbox 16x9",
			"Letterbox 14x9",
			"CCO",
			"PanScan",
			"Letterbox 2.21 on 4x3",
			"Letterbox 2.21 on 16x9",
			"Platform",
			"Zoom 16x9",
			"Pillarbox 4x3",
			"Widescreen 4x3",
	};

	inline bool isValid(int id) {
		return dsVideoPortDFC_isValid(id);
	}

}

namespace device {
typedef int _SafetyCheck[(dsUTL_DIM(_names) == dsVIDEO_ZOOM_MAX) ? 1 : -1];

const int VideoDFC::kUnknown 				= dsVIDEO_ZOOM_UNKNOWN;
const int VideoDFC::kNone 					= dsVIDEO_ZOOM_NONE;
const int VideoDFC::kFull					= dsVIDEO_ZOOM_FULL;
const int VideoDFC::kLetterBox_16x9 		= dsVIDEO_ZOOM_LB_16_9;
const int VideoDFC::kLetterBox_14x9			= dsVIDEO_ZOOM_LB_14_9;
const int VideoDFC::kCCO					= dsVIDEO_ZOOM_CCO;
const int VideoDFC::kPanScan 				= dsVIDEO_ZOOM_PAN_SCAN;
const int VideoDFC::kLetterBox_221x1ON4x3	= dsVIDEO_ZOOM_LB_2_21_1_ON_4_3;
const int VideoDFC::kLetterBox_221x1ON16x9	= dsVIDEO_ZOOM_LB_2_21_1_ON_16_9;
const int VideoDFC::kPlatform       		= dsVIDEO_ZOOM_PLATFORM;
const int VideoDFC::kZoom_16x9				= dsVIDEO_ZOOM_16_9_ZOOM;
const int VideoDFC::kPillarBox_4x3			= dsVIDEO_ZOOM_PILLARBOX_4_3;
const int VideoDFC::kWideScreen_4x3			= dsVIDEO_ZOOM_WIDE_4_3;


/**
 * @fn VideoDFC::getInstance(int id)
 * @brief This function gets an instance of VideoDFC against the id specified, only if the
 * id passed is valid.
 *
 * @param[in] id Indicates the id for which the corresponding VideoDFC instance is required.
 *
 * @return Returns an instance of VideoDFC only if the specified id is valid else throws
 * an IllegalArgumentException.
 */
const VideoDFC & VideoDFC::getInstance(int id)
{
	if (::isValid(id)) {
		return VideoDeviceConfig::getInstance().getDFC(id);
	}
	else {
		throw IllegalArgumentException();
	}
}


/**
 * @fn VideoDFC::getInstance(const std::string &name)
 * @brief This function gets an instance of VideoDFC against the name specified, only if the
 * name passed is valid.
 *
 * @param[in] name Indicates the name of the instance required.
 *
 * @return Returns an instance of VideoDFC only if the specified id is valid else throws
 * an IllegalArgumentException.
 */
const VideoDFC & VideoDFC::getInstance(const std::string &name)
{
	for (size_t i = 0; i < dsUTL_DIM(_names); i++) {
		if (name.compare(_names[i]) == 0) {
			return VideoDFC::getInstance(i);
		}
	}

	throw IllegalArgumentException();
}


/**
 * @fn VideoDFC::VideoDFC(int id)
 * @brief This function is a parameterised constructor. It initializes the instance with
 * the id provided and the name corresponding to the id. If the id passed is invalid then
 * it throws an IllegalArgumentException.
 *
 * @param[in] id Indicates the id for the instance which is used to identify the video
 * zoom type. For instance id indicates letterbox_16x9 and so on.
 *
 * @return None.
 */
VideoDFC::VideoDFC(int id)
{
	if (::isValid(id)) {
		_id = id;
		_name = std::string(_names[id]);
	}
	else {
		throw IllegalArgumentException();
	}
}


/**
 * @fn VideoDFC::~VideoDFC()
 * @brief This function is the default destructor for VideoDFC.
 *
 * @return None
 */
VideoDFC::~VideoDFC()
{
}

}


/** @} */
/** @} */
