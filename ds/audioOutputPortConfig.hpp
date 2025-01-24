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


#ifndef _DS_AUDIOOUTPUTPORTCONFIG_HPP_
#define _DS_AUDIOOUTPUTPORTCONFIG_HPP_

#include "audioEncoding.hpp"
#include "audioCompression.hpp"
#include "audioStereoMode.hpp"
#include "audioOutputPortType.hpp"
#include "list.hpp"

#include <list>
#include <string>

namespace device {

class AudioOutputPortConfig {

    std::vector<AudioEncoding> 		 _aEncodings;
    std::vector<AudioCompression> 	 _aCompressions;
    std::vector<AudioStereoMode> 	 _aStereoModes;
    std::vector<AudioOutputPortType> _aPortTypes;
    std::vector<AudioOutputPort>     _aPorts;

	AudioOutputPortConfig();
	virtual ~AudioOutputPortConfig();
	//To Make the instance as thread-safe, using = delete, the result is, automatically generated methods (constructor, for example) from the compiler will not be created and, therefore, can not be called
	AudioOutputPortConfig(const AudioOutputPortConfig&)= delete;
	AudioOutputPortConfig& operator=(const AudioOutputPortConfig&)= delete;

public:
	static AudioOutputPortConfig & getInstance();

	const AudioEncoding	 		&getEncoding(int id) const;
	const AudioCompression 		&getCompression(int id) const;
	const AudioStereoMode 		&getStereoMode(int id) const;
	AudioOutputPortType 		&getPortType(int id);
	AudioOutputPort 			&getPort(int id);
	AudioOutputPort 			&getPort(const std::string &name);
	List<AudioOutputPort> 		 getPorts();
	List<AudioOutputPortType> 	 getSupportedTypes();

	void load();
	void release();

};

}

#endif /* _DS_AUDIOOUTPUTPORTCONFIG_HPP_ */


/** @} */
/** @} */
