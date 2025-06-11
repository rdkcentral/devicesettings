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

#include <unistd.h>
#include "dsHALConfig.h"
#include "audioOutputPortType.hpp"
#include "audioOutputPortConfig.hpp"
#include "illegalArgumentException.hpp"
#include "list.hpp"
#include "dsTypes.h"
#include "dsUtl.h"
#include <iostream>
#include "dslogger.h"


/**
 * @file audioOutputPortType.cpp
 * @brief AudioOutputPortType objects are instantiated by the Device Settings module upon initialization.
 * Applications do not need to create any such objects on its own.
 * References to these objects can be retrieved using a AudioOutputPort object invoking AudioOutputPort::getType()
 */
namespace {
	const char *_names[] = {
			"IDLR",
			"HDMI",
			"SPDIF",
                        "SPEAKER",
			"HDMI_ARC",
			"HEADPHONE",
	};

	inline bool isValid(int id) {
		return dsAudioType_isValid(id);
	}
}

namespace device {

static_assert(dsUTL_DIM(_names) == dsAUDIOPORT_TYPE_MAX, "Audio output port type names array size mismatch");

const int AudioOutputPortType::kIDLR 			= dsAUDIOPORT_TYPE_ID_LR;
const int AudioOutputPortType::kHDMI 			= dsAUDIOPORT_TYPE_HDMI;
const int AudioOutputPortType::kSPDIF 			= dsAUDIOPORT_TYPE_SPDIF;
const int AudioOutputPortType::kSPEAKER                   = dsAUDIOPORT_TYPE_SPEAKER;
const int AudioOutputPortType::kARC                     = dsAUDIOPORT_TYPE_HDMI_ARC;
const int AudioOutputPortType::kHEADPHONE 		= dsAUDIOPORT_TYPE_HEADPHONE;



/**
 * @addtogroup dssettingsaudoutporttypeapi
 * @{
 */

/**
 * @fn AudioOutputPortType & AudioOutputPortType::getInstance(int id)
 * @brief This function is used to get the instance of the AudioOutputPortType based on the port id,
 * only if the id passed is valid.
 *
 * @param[in] id Port id
 *
 * @return Returns a reference to the instance of the audioOutputPortType. If the id passed
 * is invalid then it throws an IllegalArgumentException.
 */
AudioOutputPortType & AudioOutputPortType::getInstance(int id)
{
	if (::isValid(id)) {
		return AudioOutputPortConfig::getInstance().getPortType(id);
	}
	else {
		throw IllegalArgumentException();
	}
}


/**
 * @fn AudioOutputPortType::AudioOutputPortType(int id)
 * @brief This is a default constructor of class AudioOutputPortType. It initializes the AudioOutputPortType
 * instance based on the port id. If the id passed is invalid then it throws an IllegalArgumentException.
 *
 * @param[in] id Port id
 * @return None.
 */
AudioOutputPortType::AudioOutputPortType(int id)
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
 * @fn AudioOutputPortType::~AudioOutputPortType()
 * @brief This is a default destructor of class AudioOutputPortType.
 *
 * @return None.
 */
AudioOutputPortType::~AudioOutputPortType()
{
}


/**
 * @fn AudioOutputPortType::getSupportedCompressions() const
 * @brief This API is used to get the list of audio compressions supported by the audio port.
 *
 * @return Returns a list of audio compressions supported.
 */
const List<AudioCompression> AudioOutputPortType::getSupportedCompressions() const
{
	return _compressions;
}


/**
 * @fn AudioOutputPortType::getSupportedEncodings() const
 * @brief This API is used to get the list of audio encodings supported by the audio port .
 *
 * @return A list of audio encodings supported.
 */
const List<AudioEncoding> AudioOutputPortType::getSupportedEncodings() const
{
	return _encodings;
}


/**
 * @fn AudioOutputPortType::getSupportedStereoModes() const
 * @brief This API is used to get the list of audio stereo modes supported by the audio port.
 *
 * @param  None
 *
 * @return A list of stereo modes supported
 */
const List<AudioStereoMode> AudioOutputPortType::getSupportedStereoModes() const
{
	return _stereoModes;
}


/**
 * @fn const List<AudioOutputPort> AudioOutputPortType::getPorts() const
 * @brief This function is used to get the list of platform supported audio output ports.
 *
 * @return _aPorts List of audiooutputports.
 */
const List<AudioOutputPort> AudioOutputPortType::getPorts() const
{
	return _aPorts;
}


/**
 * @fn AudioOutputPort &AudioOutputPortType::getPort(int index)
 * @brief This function is used to get the AudioOutputPort instance based on the index.
 *
 * @param[in] index Index of the audio output port.
 * @return Reference to the instance of the audiooutputport.
 */
AudioOutputPort &AudioOutputPortType::getPort(int index)
{
	for (size_t i = 0; i < _aPorts.size(); i++) {
		if (_aPorts.at(i).getIndex() == index) {
			return _aPorts.at(i);
		}
	}

	throw IllegalArgumentException();
}


/**
 * @fn void AudioOutputPortType::addEncoding(const AudioEncoding & encoding)
 * @brief This function is used to add the specified encoding types to the list of supported encodings for
 * AudioOutputPortType.
 *
 * @param[in] encoding Type of encoding used in the audio output port type.
 */
void AudioOutputPortType::addEncoding(const AudioEncoding & encoding)
{
	_encodings.push_back(encoding);
}


/**
 * @fn void AudioOutputPortType::addCompression(const AudioCompression & compression)
 * @brief This function is used to add the specified compression types to the list of supported compressions for
 * AudioOutputPortType.
 *
 * @param[in] compression Type of compression used in the audio output port type.
 *
 * @return None.
 */
void AudioOutputPortType::addCompression(const AudioCompression & compression)
{
	_compressions.push_back(compression);
}


/**
 * @fn void AudioOutputPortType::addStereoMode(const AudioStereoMode & stereoMode)
 * @brief This function is used to add the specified stereoMode types to the list of supported stereo modes for
 * AudioOutputPortType.
 *
 * @param[in] stereoMode Type of stereoMode used in the audio output port type.
 *
 * @return None.
 */
void AudioOutputPortType::addStereoMode(const AudioStereoMode & stereoMode)
{
	_stereoModes.push_back(stereoMode);
}


/**
 * @fn void AudioOutputPortType::addPort(const AudioOutputPort & port)
 * @brief This function is used to add the specified audio port to the list of supported audio ports.
 *
 * @param[in] port Indicates the port to be added to the list of supported audio portsi.
 *
 * @return None.
 */
void AudioOutputPortType::addPort(const AudioOutputPort & port)
{
	_aPorts.push_back(port);
}

/**
 * @fn void AudioOutputPortType::isModeSupported(int newMode)
 * @brief This function is used to find out if requested Audio mode is supported by Platform
 *
 * @param[in] newMode Indicates the requested Audio mode to be validated against supported Audio modes.
 *
 * @return True means Supported..
 */
bool AudioOutputPortType::isModeSupported(int newMode)
{
	bool supported = false;
	const device::List<device::AudioStereoMode> & aStereoModes = _stereoModes;

	for (size_t j = 0; j < aStereoModes.size(); j++) 
	{
		//std::cout << "StereoModes requested = " <<  newMode << std::endl;
		//std::cout << "StereoModes supported = " <<  aStereoModes.at(j).getId() << std::endl;
		if(newMode == aStereoModes.at(j).getId())
		{
			supported = true;
			break;
		}
	}
	return supported;
}

}

/** @} */

/** @} */
/** @} */
