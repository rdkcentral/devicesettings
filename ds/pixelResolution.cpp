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
 * @file pixelResolution.cpp
 * @brief This file contains implementation of PixelResolution class methods,
 * support functions and variable assignments to manage the video resolutions.
 */



/**
* @defgroup devicesettings
* @{
* @defgroup ds
* @{
**/


#include "dsTypes.h"
#include "dsUtl.h"
#include "pixelResolution.hpp"
#include "illegalArgumentException.hpp"
#include "videoOutputPortConfig.hpp"
#include "dslogger.h"

namespace {
    const char *_names[] = {
            "720x480",
            "720x576",
            "1280x720",
            "1366x768",
            "1920x1080",
            "3840x2160",
            "4096x2160",

    };

    inline bool isValid(int id) {
        return dsVideoPortPixelResolution_isValid(id);
    }
}


namespace device {
static_assert(dsUTL_DIM(_names) == dsVIDEO_PIXELRES_MAX, "Pixel resolution names array size mismatch");

const int PixelResolution::k720x480             = dsVIDEO_PIXELRES_720x480;
const int PixelResolution::k720x576             = dsVIDEO_PIXELRES_720x576;
const int PixelResolution::k1280x720            = dsVIDEO_PIXELRES_1280x720;
const int PixelResolution::k1366x768            = dsVIDEO_PIXELRES_1366x768;
const int PixelResolution::k1920x1080           = dsVIDEO_PIXELRES_1920x1080;
const int PixelResolution::k3840x2160           = dsVIDEO_PIXELRES_3840x2160;
const int PixelResolution::k4096x2160           = dsVIDEO_PIXELRES_4096x2160;

const int PixelResolution::kMax                 = dsVIDEO_PIXELRES_MAX;


/**
 * @fn PixelResolution::getInstance(int id)
 * @brief This function gets the instance of PixelResolution against the specified id,
 * only if the id passed is valid.
 *
 * @param[in] id Indicates the id against which the PixelResolution instance is required.
 *
 * @return Returns an instance of PixelResolution if the specified id is valid else
 * throws an IllegalArgumentException.
 */
const PixelResolution & PixelResolution::getInstance(int id)
{
	if (::isValid(id)) {
		return VideoOutputPortConfig::getInstance().getPixelResolution(id);
	}
	else {
		throw IllegalArgumentException();
	}
}


/**
 * @fn PixelResolution::PixelResolution(int id)
 * @brief This function is a parameterised constructor for PixelResolution. It
 * initializes the instance with the specified id and the name corresponding
 * to it. If the id passed is invalid then it throws an IllegalArgumentException.
 *
 * @param[in] id Indicates the id for the instance with is used to identify
 * the pixel resolution type.
 *
 * @return None
 */
PixelResolution::PixelResolution(int id)
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
 * @fn PixelResolution::~PixelResolution()
 * @brief This function is the default destructor for PixelResolution.
 *
 * @return None
 */
PixelResolution::~PixelResolution()
{
}

}


/** @} */
/** @} */
