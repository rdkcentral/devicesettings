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
 * @file aspectRatio.cpp
 * @brief This file contains implementation of AspectRatio class methods,
 * variable assignments and support functions to manage the aspect ratio types.
 */



/**
* @defgroup devicesettings
* @{
* @defgroup ds
* @{
**/


#include "aspectRatio.hpp"
#include "illegalArgumentException.hpp"
#include "videoOutputPortConfig.hpp"
#include "dsTypes.h"
#include "dsUtl.h"
#include "dslogger.h"

namespace {
	const char *_names[] = {
			"4x3",
			"16x9",
	};

	inline bool isValid(int id) {
		return dsVideoPortAspectRatio_isValid(id);
	}
}

namespace device {
static_assert(dsUTL_DIM(_names) == dsVIDEO_ASPECT_RATIO_MAX, "Aspect ratio names array size mismatch");

const int AspectRatio::k4x3 	= dsVIDEO_ASPECT_RATIO_4x3;
const int AspectRatio::k16x9 	= dsVIDEO_ASPECT_RATIO_16x9;
const int AspectRatio::kMax		= dsVIDEO_ASPECT_RATIO_MAX;


/**
 * @fn AspectRatio::getInstance(int id)
 * @brief This function gets the instance of the AspectRatio against the id specified, only if
 * the id passed is valid.
 *
 * @param[in] id Indicates the id against which the AspectRatio instance is required. The id
 * is used to identify the type of aspect ratio. For example: id of 0 and 1 may correspond to
 * "4x3" and "16x9" aspect ratio respectively.
 *
 * @return Returns an instance of AspectRatio against the id parameter or else throws an
 * IllegalArgumentException indicating that the specified id is invalid.
 */
const AspectRatio & AspectRatio::getInstance(int id)
{
	if (::isValid(id)) {
		return VideoOutputPortConfig::getInstance().getAspectRatio(id);
	}
	else {
		throw IllegalArgumentException();
	}
}


/**
 * @fn AspectRatio::getInstance(const std::string &name)
 * @brief This function gets the instance of the AspectRatio against the name specified, only if
 * the name passed is valid.
 *
 * @param[in] name Indicates the name against which the AspectRatio instance is required. The name
 * is used to identify the type of aspect ratio. For example: id of 0 and 1 may correspond to
 * 4x3 and 16x9 aspect ratio respectively.
 *
 * @return Returns an AspectRatio instance against the name specified or else throws an
 * IllegalArgumentException indicating that the specified name is invalid.
 */
const AspectRatio & AspectRatio::getInstance(const std::string &name)
{
	for (size_t i = 0; i < dsUTL_DIM(_names); i++) {
		if (name.compare(_names[i]) == 0) {
			return AspectRatio::getInstance(i);
		}
	}

	throw IllegalArgumentException();
}


/**
 * @fn AspectRatio::AspectRatio(int id)
 * @brief This function is a parameterised constructor. It initializes the instance with
 * the specified id and also the name which is predefined for the id. It throws an
 * IllegalArgumentException if the id specified is invalid.
 *
 * @param[in] id Indicates the aspect ratio for the instance created. For example: 0 indicates "4x3"
 * aspect ratio and 1 indicates "16x9" aspect ratio .
 *
 * @return None
 */
AspectRatio::AspectRatio(int id)
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
 * @fn AspectRatio::~AspectRatio()
 * @brief This function is the default destructor of AspectRatio class.
 *
 * @return None
 */
AspectRatio::~AspectRatio()
{
}

}


/** @} */
/** @} */
