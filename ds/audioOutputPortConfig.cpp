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

#include "dsHALConfig.h"
#include "audioOutputPortConfig.hpp"
#include "dsAudioSettings.h"
#include "illegalArgumentException.hpp"
#include "dsError.h"
#include "dsUtl.h"
#include "stdlib.h"
#include "dslogger.h"
#include <dlfcn.h>
#include "manager.hpp"


#define DEBUG 1 // Using for dumpconfig 

typedef struct audioConfigs
{
	const dsAudioTypeConfig_t  *pKConfigs;
	const dsAudioPortConfig_t  *pKPorts;
	int *pKConfigSize;
	int *pKPortSize;
}audioConfigs_t;

namespace device {

//To Make the instance as thread-safe, using = default, that can request special methods from the compiler. They are Special because only compiler can create them.

AudioOutputPortConfig::AudioOutputPortConfig()= default;

AudioOutputPortConfig::~AudioOutputPortConfig()= default;


AudioOutputPortConfig & AudioOutputPortConfig::getInstance()
{
    static AudioOutputPortConfig _singleton; // _singleton instance is in thread-safe now.
	return _singleton;
}

const AudioEncoding &AudioOutputPortConfig::getEncoding(int id) const
{
	return _aEncodings.at(id);
}
const AudioCompression &AudioOutputPortConfig::getCompression(int id) const
{
	return _aCompressions.at(id);

}
const AudioStereoMode &AudioOutputPortConfig::getStereoMode(int id) const
{
	return _aStereoModes.at(id);

}
AudioOutputPortType & AudioOutputPortConfig::getPortType(int id)
{
	return _aPortTypes.at(id);
}

AudioOutputPort &AudioOutputPortConfig::getPort(int id)
{
	return _aPorts.at(id);
}

AudioOutputPort &AudioOutputPortConfig::getPort(const std::string & name)
{
	for (size_t i = 0; i < _aPorts.size(); i++) {
		if (name.compare(_aPorts.at(i).getName()) == 0) {
			return _aPorts.at(i);
		}
	}

	throw IllegalArgumentException();
}

List<AudioOutputPort> AudioOutputPortConfig::getPorts()
{
	List <AudioOutputPort> rPorts;

	for (size_t i = 0; i < _aPorts.size(); i++) {
		rPorts.push_back(_aPorts.at(i));
	}

	return rPorts;
}

List<AudioOutputPortType>  AudioOutputPortConfig::getSupportedTypes()
{
	List<AudioOutputPortType> supportedTypes;
	for (std::vector<AudioOutputPortType>::const_iterator it = _aPortTypes.begin(); it != _aPortTypes.end(); it++) {
		if (it->isEnabled()) {
			supportedTypes.push_back(*it);
		}
	}

	return supportedTypes;
}

void dumpconfig(audioConfigs_t *config)
{
	INT_INFO("%d:%s: Entering function\n", __LINE__, __func__);
	INT_INFO("%d:%s: pKConfigs = %p\n", __LINE__, __func__, config->pKConfigs);
	INT_INFO("%d:%s: pKPorts = %p\n", __LINE__, __func__, config->pKPorts);
	INT_INFO("%d:%s: pKConfigSize %p = %d \n", __LINE__, __func__, config->pKConfigSize, *(config->pKConfigSize));
	INT_INFO("%d:%s: pKPortSize %p = %d \n", __LINE__, __func__, config->pKPortSize, *(config->pKPortSize));

	INT_INFO("\n\n=========================================================================================================================\n\n");
	if(config->pKConfigs != NULL && *(config->pKConfigSize) != -1)
	{
	for (size_t i = 0; i < *(config->pKConfigSize); i++) {
			const dsAudioTypeConfig_t *typeCfg = &(config->pKConfigs[i]);
			INT_INFO("%d:%s: typeCfg->typeId = %d\n", __LINE__, __func__, typeCfg->typeId);
			INT_INFO("%d:%s: typeCfg->name = %s\n", __LINE__, __func__, typeCfg->name);
			INT_INFO("%d:%s: typeCfg->numSupportedEncodings = %zu\n", __LINE__, __func__, typeCfg->numSupportedEncodings);
			INT_INFO("%d:%s: typeCfg->numSupportedCompressions = %zu\n", __LINE__, __func__, typeCfg->numSupportedCompressions);
			INT_INFO("%d:%s: typeCfg->numSupportedStereoModes = %zu\n", __LINE__, __func__, typeCfg->numSupportedStereoModes);
		}
	}
	else
	{
		INT_ERROR("%d:%s: kAudioConfigs is NULL and kConfig_size_local is -1\n", __LINE__, __func__);
	}
	if(config->pKPorts != NULL && *(config->pKPortSize) != -1)
	{
		for (size_t i = 0; i < *(config->pKPortSize); i++) {
			const dsAudioPortConfig_t *port = &(config->pKPorts[i]);
			INT_INFO("%d:%s: port->id.type = %d\n", __LINE__, __func__, port->id.type);
			INT_INFO("%d:%s: port->id.index = %d\n", __LINE__, __func__, port->id.index);
		}
	}
	else
	{
		INT_ERROR("%d:%s: kAudioPorts is NULL and kPort_size_local is -1\n", __LINE__, __func__);
	}
	INT_INFO("\n\n=========================================================================================================================\n\n");
	INT_INFO("%d:%s: Exit function\n", __LINE__, __func__);
}


void AudioOutputPortConfig::load()
{
	int configSize, portSize;
	audioConfigs_t configuration = {0};
	const char* searchVaribles[] = {
        "kAudioConfigs",
        "kAudioPorts",
        "kAudioConfigs_size",
        "kAudioPorts_size"
    };
	bool ret = false;

	try {
		/*
		 * Load Constants First.
		 */
		for (int i = 0; i < dsAUDIO_ENC_MAX; i++) {
			_aEncodings.push_back(AudioEncoding(i));
		}

		for (int i = 0; i < dsAUDIO_CMP_MAX; i++) {
			_aCompressions.push_back(AudioCompression(i));

		}

		for (int i = 0; i < dsAUDIO_STEREO_MAX; i++) {
			_aStereoModes.push_back(AudioStereoMode(i));

		}

		for (int i = 0; i < dsAUDIOPORT_TYPE_MAX; i++) {
			_aPortTypes.push_back(AudioOutputPortType(i));

		}

		INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[0]);
		ret = searchConfigs((void **)&configuration.pKConfigs, searchVaribles[0]);
		if(ret == true)
		{
			INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[2]);
			ret = searchConfigs((void **)&configuration.pKConfigSize, (char *)searchVaribles[2]);
			if(ret == false)
			{
				INT_ERROR("%s is not defined\n", searchVaribles[2]);
			}
			INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[1]);
			ret = searchConfigs((void **)&configuration.pKPorts, searchVaribles[1]);
			if(ret == false)
			{
				INT_ERROR("%s is not defined\n", searchVaribles[1]);
			}
			INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[3]);
			ret = searchConfigs((void **)&configuration.pKPortSize, (char *)searchVaribles[3]);
			if(ret == false)
			{
				INT_ERROR("%s is not defined\n", searchVaribles[3]);
			}
		}
		else
		{
			INT_ERROR("Read Old Configs\n");
			configuration.pKConfigs = kConfigs;
			configSize = dsUTL_DIM(kConfigs);
			configuration.pKConfigSize = &configSize;
			configuration.pKPorts = kPorts;
			portSize = dsUTL_DIM(kPorts);
			configuration.pKPortSize = &portSize;
			INT_INFO("configuration.pKConfigs =%p, configuration.pKPorts =%p, *(configuration.pKConfigSize) = %d, *(configuration.pKPortSize) = %d\n", configuration.pKConfigs, configuration.pKPorts, *(configuration.pKConfigSize), *(configuration.pKPortSize));
		}
		/*
		 * Check if configs are loaded properly
		 */
		if ( configuration.pKConfigs != NULL || configuration.pKPorts != NULL ||
			configuration.pKConfigSize != NULL || configuration.pKPortSize != NULL) {
			#if DEBUG
			dumpconfig(&configuration);
			#endif
			INT_INFO("%d:%s: Audio Configs loaded successfully\n", __LINE__, __func__);
			/*
			* Initialize Audio portTypes (encodings, compressions etc.)
			* and its port instances (db, level etc)
			*/
			for (size_t i = 0; i < *(configuration.pKConfigSize); i++) {
				const dsAudioTypeConfig_t *typeCfg = &(configuration.pKConfigs[i]);
				AudioOutputPortType &aPortType = AudioOutputPortType::getInstance(typeCfg->typeId);
				aPortType.enable();
				for (size_t j = 0; j < typeCfg->numSupportedEncodings; j++) {
					aPortType.addEncoding(AudioEncoding::getInstance(typeCfg->encodings[j]));
					_aEncodings.at(typeCfg->encodings[j]).enable();
				}
				for (size_t j = 0; j < typeCfg->numSupportedCompressions; j++) {
					aPortType.addCompression(typeCfg->compressions[j]);
					_aCompressions.at(typeCfg->compressions[j]).enable();
				}
				for (size_t j = 0; j < typeCfg->numSupportedStereoModes; j++) {
					aPortType.addStereoMode(typeCfg->stereoModes[j]);
					_aStereoModes.at(typeCfg->stereoModes[j]).enable();
				}
			}

			/*
	 		* set up ports based on kPorts[]
	 		*/
			for (size_t i = 0; i < *(configuration.pKPortSize); i++) {
				const dsAudioPortConfig_t *port = &configuration.pKPorts[i];
				_aPorts.push_back(AudioOutputPort((port->id.type), port->id.index, i));
				_aPortTypes.at(port->id.type).addPort(_aPorts.at(i));
			}
		}
		else {
			INT_ERROR("%d:%s: Audio Configs loading failed\n", __LINE__, __func__);
		}
	}
	catch(const Exception &e) {
		throw e;
	}
}

void AudioOutputPortConfig::release()
  {
	try {
              _aEncodings.clear(); 
              _aCompressions.clear();
              _aStereoModes.clear();
              _aPortTypes.clear();
              _aPorts.clear();              
	}
	catch(const Exception &e) {
		throw e;
	}

  }
}



/** @} */
/** @} */
