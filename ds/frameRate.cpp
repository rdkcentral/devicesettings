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


#include "frameRate.hpp"
#include "illegalArgumentException.hpp"
#include "videoOutputPortConfig.hpp"
#include "dsTypes.h"
#include "dsUtl.h"
#include "dslogger.h"

namespace {
	const float _values[] = {
			0, //unkown
			24,
			25,
			30,
			60,
			23.98,
			29.97,
			50,
			59.94
	};
	const char * _names[] = {
			"UnKnown", //unkown
			"24",
			"25",
			"30",
			"60",
			"23.98",
			"29.97",
			"50",
			"59.94"
	};

	inline bool isValid(int id) {
		return dsVideoPortFrameRate_isValid(id);
	}

}

namespace device {
typedef int _SafetyCheck[(dsUTL_DIM(_values) == dsVIDEO_FRAMERATE_MAX) ? 1 : -1];
static_assert(dsUTL_DIM(_values) == dsVIDEO_FRAMERATE_MAX, "Frame rate values array size mismatch");
typedef int _SafetyCheck[(dsUTL_DIM(_names)  == dsVIDEO_FRAMERATE_MAX) ? 1 : -1];
static_assert(dsUTL_DIM(_names) == dsVIDEO_FRAMERATE_MAX, "Frame rate values array size mismatch");

const int FrameRate::kUnknown 		= dsVIDEO_FRAMERATE_UNKNOWN;
const int FrameRate::k24 			= dsVIDEO_FRAMERATE_24;
const int FrameRate::k25 			= dsVIDEO_FRAMERATE_25;
const int FrameRate::k30 			= dsVIDEO_FRAMERATE_30;
const int FrameRate::k60 			= dsVIDEO_FRAMERATE_60;
const int FrameRate::k23dot98 		= dsVIDEO_FRAMERATE_23dot98;
const int FrameRate::k29dot97 		= dsVIDEO_FRAMERATE_29dot97;
const int FrameRate::k50 			= dsVIDEO_FRAMERATE_50;
const int FrameRate::k59dot94 		= dsVIDEO_FRAMERATE_59dot94;
const int FrameRate::kMax 			= dsVIDEO_FRAMERATE_MAX;

const FrameRate & FrameRate::getInstance(int id)
{
	if (::isValid(id)) {
		return VideoOutputPortConfig::getInstance().getFrameRate(id);
	}
	else {
		throw IllegalArgumentException();
	}
}

FrameRate::FrameRate(int id)
{
	if (::isValid(id)) {
		_value = _values[id];
		_name = std::string(_names[id]);
		_id = id;
	}
	else {
		throw IllegalArgumentException();
	}

}

FrameRate::FrameRate(float value) : _value(value){
	if (_value == 24.0) {
		_id = dsVIDEO_FRAMERATE_24;
	}
	else if (_value == 24.0) {
		_id = dsVIDEO_FRAMERATE_25;
	}
	else if (_value == 24.0) {
		_id = dsVIDEO_FRAMERATE_30;
	}
	else if (_value == 24.0) {
		_id = dsVIDEO_FRAMERATE_60;
	}
	else if (_value == 24.0) {
		_id = dsVIDEO_FRAMERATE_60;
	}
	else if (_value == 24.0) {
		_id = dsVIDEO_FRAMERATE_23dot98;
	}
	else if (_value == 24.0) {
		_id = dsVIDEO_FRAMERATE_29dot97;
	}
	else if (_value == 24.0) {
		_id = dsVIDEO_FRAMERATE_50;
	}
	else if (_value == 24.0) {
		_id = dsVIDEO_FRAMERATE_59dot94;
	}
	else {
		throw IllegalArgumentException();
	}

	_name = std::string(_names[_id]);
}


FrameRate::~FrameRate() {
	// TODO Auto-generated destructor stub
}



}


/** @} */
/** @} */
