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
 * @file audioCompression.cpp
 * @brief This file contains implementation of audioCompression class methods,
 * variable assignments and support functions to manage the audio compression types.
 */



/**
* @defgroup devicesettings
* @{
* @defgroup ds
* @{
**/


#include "audioCompression.hpp"
#include "audioOutputPortConfig.hpp"
#include "illegalArgumentException.hpp"
#include "dsTypes.h"
#include "dsUtl.h"
#include <string.h>
#include "dslogger.h"

namespace {
	const char *_names[] = {
			"NONE",
			"LIGHT",
			"MEDIUM",
			"HEAVY",
	};

	inline bool isValid(int id) {
		return dsAudioCompression_isValid(id);
	}

}

namespace device {
typedef int _SafetyCheck[(dsUTL_DIM(_names) == dsAUDIO_CMP_MAX) ? 1 : -1];

const int AudioCompression::kNone 			= dsAUDIO_CMP_NONE;
const int AudioCompression::kLight 			= dsAUDIO_CMP_LIGHT;
const int AudioCompression::kMedium 		= dsAUDIO_CMP_MEDIUM;
const int AudioCompression::kHeavy 			= dsAUDIO_CMP_HEAVY;
const int AudioCompression::kMax 			= dsAUDIO_CMP_MAX;


/**
 * @fn AudioCompression::getInstance(int id)
 * @brief This function gets an instance of AudioCompression against the specified id, only if
 * the id passed is valid.
 *
 * @param[in] id Indicates id/index into the list of AudioCompression to get required instance.
 * Id is used to identify the type of compression like light, heavy or medium.
 *
 * @return Returns an instance of AudioCompression against the id specified or else throws an
 * IllegalArgumentException indicating that the instance against specified id was not found.
 */
const AudioCompression & AudioCompression::getInstance(int id)
{
	if (::isValid(id)) {
		return AudioOutputPortConfig::getInstance().getCompression(id);
	}
	else {
		throw IllegalArgumentException();
	}
}


/**
 * @fn AudioCompression::getInstance(const std::string &name)
 * @brief This function gets an instance of AudioCompression against the specified name, only if the
 * name passed is valid.
 *
 * @param[in] name Indicates the name against which the AudioCompression instance is required.
 * The name string indicates the audio compression type like "NONE", "LIGHT", "MEDIUM" and
 * "HEAVY"
 *
 * @return Returns an instance of AudioCompression against the name specified or else throws an
 * IllegalArgumentException indicating that the instance against specified name was not found.
 */
const AudioCompression & AudioCompression::getInstance(const std::string &name)
{
	for (size_t i = 0; i < dsUTL_DIM(_names); i++) {
		if (name.compare(_names[i]) == 0) {
			return AudioCompression::getInstance(i);
		}
	}

	throw IllegalArgumentException();
}


/**
 * @fn AudioCompression::AudioCompression(int id)
 * @brief This function is a parameterised constructor. It initializes the AudioCompression
 * instance with the parameters passed as input, only if the id passed is valid.
 *
 * @param[in] id Indicates the id for the instance created. The id is used to identify the
 * audio compression types. For example:
 * <ul>
 * <li> id 0 indicates no audio compression.
 * <li> id 1 indicates light audio compression.
 * <li> id 2 indicates medium audio compression.
 * <li> id 3 indicates heavy audio compression.
 * <ul/"
 *
 * @return None
 */
AudioCompression::AudioCompression(int id)
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
 * @fn AudioCompression::~AudioCompression()
 * @brief This function is the default destructor for AudioCompression.
 *
 * @return None
 */
AudioCompression::~AudioCompression()
{
}

}


/** @} */
/** @} */
