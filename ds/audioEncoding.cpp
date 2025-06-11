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
 * @file audioEncoding.cpp
 * @brief This file contains implementation of AudioEncoding class methods,
 * support functions and variable assignments to manage the audio encoding types.
 */



/**
* @defgroup devicesettings
* @{
* @defgroup ds
* @{
**/


#include "audioEncoding.hpp"
#include "audioOutputPortConfig.hpp"
#include "illegalArgumentException.hpp"
#include "dsTypes.h"
#include "dsUtl.h"
#include <string.h>
#include "dslogger.h"

namespace {
	const char *_names[] = {
			"NONE",
			"DISPLAY",
			"PCM",
			"AC3",
			"EAC3",
	};

	inline bool isValid(int id) {
		return dsAudioEncoding_isValid(id);
	}

}

namespace device {
static_assert(dsUTL_DIM(_names) == dsAUDIO_ENC_MAX, "Audio encoding names array size mismatch");

const int AudioEncoding::kNone 			= dsAUDIO_ENC_NONE;
const int AudioEncoding::kDisplay 		= dsAUDIO_ENC_DISPLAY;
const int AudioEncoding::kPCM 			= dsAUDIO_ENC_PCM;
const int AudioEncoding::kAC3 			= dsAUDIO_ENC_AC3;
const int AudioEncoding::kMax 			= dsAUDIO_ENC_MAX;

/**
 * @addtogroup dssettingsaudencodingapi
 * @{
 */
/**
 * @fn AudioEncoding::getInstance(int id)
 * @brief This function gets an AudioEncoding instance against the id parameter, only if the id
 * passed is valid.
 *
 * @param[in] id Indicates the id against which the AudioEncoding instance is required. The id
 * is used to identify the audio encoding types.
 *
 * @return Returns AudioEncoding instance if the id is valid else throws an IllegalArgumentException.
 */
const AudioEncoding & AudioEncoding::getInstance(int id)
{
	if (::isValid(id)) {
		return AudioOutputPortConfig::getInstance().getEncoding(id);
	}
	else {
		throw IllegalArgumentException();
	}
}


/**
 * @fn AudioEncoding::getInstance(const std::string &name)
 * @brief This function gets an AudioEncoding instance against the specified name, only if the name
 * passed is valid.
 *
 * @param[in] name Indicates the name against which the AudioEncoding instance is required and it is also
 * used to identify the audio encoding types.
 *
 * @return Returns AudioEncoding instance if the name is valid else throws an IllegalArgumentException.
 */
const AudioEncoding & AudioEncoding::getInstance(const std::string &name)
{
	for (size_t i = 0; i < dsUTL_DIM(_names); i++) {
		if (name.compare(_names[i]) == 0) {
			return AudioEncoding::getInstance(i);
		}
	}

	throw IllegalArgumentException();
}


/**
 * @fn AudioEncoding::AudioEncoding(int id)
 * @brief This function is the default constructor for AudioEncoding. It initializes the instance with
 * the id passed as input. If the id parameter is invalid then it throws an IllegalArgumentException.
 *
 * @param[in] id Indicates the audio encoding type.
 * <ul>
 * <li> id 0 indicates encoding type None
 * <li> id 1 indicates digital audio encoding format
 * <li> id 2 indicates PCM encoding type
 * <li> id 3 indicates AC3 encoding type
 * </ul>
 *
 * @return None
 */
AudioEncoding::AudioEncoding(int id)
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
 * @fn AudioEncoding::~AudioEncoding()
 * @brief This function is the default destructor of AudioEncoding class.
 *
 * @return None
 */
AudioEncoding::~AudioEncoding()
{
}


}

/** @} */

/** @} */
/** @} */
