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
 * @file stereoScopicMode.cpp
 * @brief This file contains implementation of StereoScopicMode class methods,
 * support functions and variable assignments to manage the video stereoscopy
 * types.
 */



/**
* @defgroup devicesettings
* @{
* @defgroup ds
* @{
**/


#include "stereoScopicMode.hpp"
#include "illegalArgumentException.hpp"
#include "videoOutputPortConfig.hpp"
#include "dsTypes.h"
#include "dsUtl.h"
#include "dslogger.h"

namespace {
	const char *_names[] = {
			"UNKOWN",
			"2D",
			"SIDE_BY_SIDE",
			"TOP_AND_BOTTOM"
	};

	inline bool isValid(int id) {
		return dsVideoPortStereoScopicMode_isValid(id);
	}
}


namespace device {
typedef int _SafetyCheck[(dsUTL_DIM(_names) == dsVIDEO_SSMODE_MAX) ? 1 : -1];

const int StereoScopicMode::kUnkown				= dsVIDEO_SSMODE_UNKNOWN;
const int StereoScopicMode::k2D					= dsVIDEO_SSMODE_2D;
const int StereoScopicMode::k3DSidebySide		= dsVIDEO_SSMODE_3D_SIDE_BY_SIDE;
const int StereoScopicMode::k3dTopAndBottom		= dsVIDEO_SSMODE_3D_TOP_AND_BOTTOM;
const int StereoScopicMode::kMax 				= dsVIDEO_SSMODE_MAX;


/**
 * @fn StereoScopicMode::getInstance(int id)
 * @brief This function gets an instance of the StereoScopicMode against the specified id, only
 * if the id passed is valid.
 *
 * @param[in] id Indicates the id of the StereoScopicMode instance required.
 *
 * @return Returns an instance of StereoScopicMode against the specified id only if the id is
 * valid else throws an IllegalArgumentException.
 */
const StereoScopicMode & StereoScopicMode::getInstance(int id)
{
	if (::isValid(id)) {
		return VideoOutputPortConfig::getInstance().getSSMode(id);
	}
	else {
		throw IllegalArgumentException();
	}
}


/**
 * @fn StereoScopicMode::StereoScopicMode(int id)
 * @brief This function is a parameterised constructor of StereoScopicMode class. It initializes
 * the instance with the specified id and the name corressponding to it. If the id passed
 * is invalid then IllegalArgumentException is thrown.
 *
 * @param[in] id Indicates the id for the instance created which is used to identify the
 * type of video Stereoscopy like 2D, 3D and so on.
 *
 * @return None
 */
StereoScopicMode::StereoScopicMode(int id)
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
 * @fn StereoScopicMode::~StereoScopicMode()
 * @brief This function is the default destructor for StereoScopicMode class.
 *
 * @return None
 */
StereoScopicMode::~StereoScopicMode()
{
}

}


/** @} */
/** @} */
