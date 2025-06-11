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
 * @file audioStereoMode.cpp
 * @brief This file This file contains implementation of AudioStereoMode class methods,
 * support functions and variable assignments to manage the audio modes like stereo, mono
 * pass through and so on.
 */



/**
* @defgroup devicesettings
* @{
* @defgroup ds
* @{
**/


#include "audioStereoMode.hpp"
#include "audioOutputPortConfig.hpp"
#include "illegalArgumentException.hpp"
#include "dsTypes.h"
#include "dsUtl.h"
#include <string.h>
#include "dslogger.h"


namespace {
	const char *_names[] = {
			"UNKNOWN",
			"MONO",
			"STEREO",
			"SURROUND",
			"PASSTHRU",
			"DOLBYDIGITAL",
			"DOLBYDIGITALPLUS",
	};

	inline bool isValid(int id) {
		return dsAudioStereoMode_isValid(id);
	}
}

namespace device {
static_assert(dsUTL_DIM(_names) == dsAUDIO_STEREO_MAX, "Audio stereo mode names array size mismatch");

const int AudioStereoMode::kMono 			= dsAUDIO_STEREO_MONO;
const int AudioStereoMode::kStereo 			= dsAUDIO_STEREO_STEREO;
const int AudioStereoMode::kSurround 		= dsAUDIO_STEREO_SURROUND;
const int AudioStereoMode::kPassThru 		= dsAUDIO_STEREO_PASSTHRU;
const int AudioStereoMode::kDD   		= dsAUDIO_STEREO_DD;
const int AudioStereoMode::kDDPlus 		= dsAUDIO_STEREO_DDPLUS;
const int AudioStereoMode::kMax 			= dsAUDIO_STEREO_MAX;


/**
 * @fn AudioStereoMode::getInstance(int id)
 * @brief This function gets an instance of AudioStereoMode against the specified id, only
 * if the id passed is valid.
 *
 * @param[in] id Indicates id against which the AudioStereoMode instance is required.
 *
 * @return Returns an instance of AudioStereoMode if the id is valid else throws an
 * IllegalArgumentException.
 */
const AudioStereoMode & AudioStereoMode::getInstance(int id)
{
	if (::isValid(id)) {
		return AudioOutputPortConfig::getInstance().getStereoMode(id);
	}
	else {
		throw IllegalArgumentException();
	}
}


/**
 * @fn AudioStereoMode::getInstance(const std::string &name)
 * @brief This function gets an instance of AudioStereoMode against the specified name, only if
 * the name passed is valid.
 *
 * @param[in] name Indicates name string against which the AudioStereoMode instance is required.
 *
 * @return Returns an instance of AudioStereoMode if an instance with the specified
 * name is present else throws an IllegalArgumentException.
 */
const AudioStereoMode & AudioStereoMode::getInstance(const std::string &name)
{
	for (size_t i = 0; i < dsUTL_DIM(_names); i++) {
		if (name.compare(_names[i]) == 0) {
			return AudioStereoMode::getInstance(i);
		}
	}

	throw IllegalArgumentException();
}


/**
 * @fn AudioStereoMode::AudioStereoMode(int id)
 * @brief This function is a parameterised constructor of AudioStereoMode class.
 * It initializes the instance with the specified id and the name corresponding to it.
 *
 * @param[in] id Indicates the id for initializing the instance. The id
 * can be used to identify the audio stereo mode. Ex: Id of 0 indicates mono
 * audio whereas id of 1 indicates surround audio.
 *
 * @return None
 */
AudioStereoMode::AudioStereoMode(int id)
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
 * @fn AudioStereoMode::~AudioStereoMode()
 * @brief This function is the default destructor of AudioStereoMode class.
 *
 * @return None
 */
AudioStereoMode::~AudioStereoMode()
{
}



}


/** @} */
/** @} */
