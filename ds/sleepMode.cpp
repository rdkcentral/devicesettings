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
 * @file sleepMode.cpp
 * @brief This file contains definitions of SleepMode class.
 */



/**
* @defgroup devicesettings
* @{
* @defgroup ds
* @{
**/


#include "sleepMode.hpp"
#include "illegalArgumentException.hpp"
#include "dsTypes.h"
#include "dsUtl.h"
#include "dslogger.h"
#include <vector>
#include "list.hpp"
#include "dsRpc.h"

namespace {
	const char *_names[] = {
			"LIGHT_SLEEP",
			"DEEP_SLEEP",
	};

	inline bool isValid(int id) {
		return dsSleepMode_isValid(id);
	}
}

namespace device {

const int SleepMode::kLightSleep	= dsHOST_SLEEP_MODE_LIGHT;
const int SleepMode::kDeepSleep 	= dsHOST_SLEEP_MODE_DEEP;
const int SleepMode::kMax		= dsHOST_SLEEP_MODE_MAX;
static std::vector<SleepMode> _vSleepModes;


/**
 * @fn SleepMode::getInstance(int id)
 * @brief This function is used to get an instance of SleepMode against the specified id,
 * only if the id passed is valid.
 *
 * @param[in] id Indicates the id against which the SleepMode instance is required.
 * The valid id's are 0 and 1 which indicates light and deep sleep mode respectively.
 *
 * @return Returns an instance of SleepMode against the specified id if the id is valid else
 * throws an IllegalArgumentException.
 */
SleepMode & SleepMode::getInstance(int id)
{
	static bool FirstTime = true;
	if (::isValid(id)) {
		if(FirstTime)
		{
			for(size_t i=0; i < dsHOST_SLEEP_MODE_MAX; i++)
			{ 
				_vSleepModes.push_back(SleepMode(i));
			}
			FirstTime = false;	
		}
		return _vSleepModes.at(id);
	}
	else {
		throw IllegalArgumentException();
	}
}


/**
 * @fn SleepMode::getSleepModes()
 * @brief This function is used to get all the platform supported types of sleep modes.
 *
 * @return Returns sleepModes, which is a list containing all the supported sleep mode id's.
 */
List<SleepMode> SleepMode::getSleepModes() 
{
	List<SleepMode> sleepModes;
	for(std::vector<SleepMode>::const_iterator it = _vSleepModes.begin(); 
			it != _vSleepModes.end(); it++)
	{
		sleepModes.push_back(*it);
	}
	return sleepModes;
}


/**
 * @fn SleepMode::getInstance(const std::string &name)
 * @brief This function gets a SleepMode instance of the type specified by 'name' parameter.
 *
 * @param[in] name Indicates the name of the sleep mode whose instance is required.
 * The valid names are "LIGHT_SLEEP" and "DEEP_SLEEP".
 *
 * @return Returns the SleepMode instance of specified type only if the name parameter
 * passed is valid else throws an IllegalArgumentException.
 */
SleepMode & SleepMode::getInstance(const std::string &name)
{
	for (size_t i = 0; i < dsUTL_DIM(_names); i++) {
		if (name.compare(_names[i]) == 0) {
			return SleepMode::getInstance(i);
		}
	}

	throw IllegalArgumentException();
}


/**
 * @fn SleepMode::SleepMode(int id)
 * @brief This function is a parameterised constructor of SleepMode class. It initializes
 * the SleepMode instance with the id specified and the name corresponding to it. If the
 * id passed is invalid then it throws an IllegalArgumentException.
 *
 * @param[in] id Indicates the id against which the SleepMode instance is required.
 *
 * @return None
 */
SleepMode::SleepMode(int id)
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
 * @fn SleepMode::~SleepMode()
 * @brief This function is the default destructor of SleepMode class.
 *
 * @return None
 */
SleepMode::~SleepMode()
{
}

}


/** @} */
/** @} */
