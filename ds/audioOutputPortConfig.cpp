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

#define DEBUG 1

static dsAudioTypeConfig_t  *kConfigs1 = NULL;
static dsAudioPortConfig_t  *kPorts1 = NULL;
int *pKConSize, *pKPortSize;
static int kConfig_size_local = -1, kPort_size_local = -1;

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

bool searchConfigs()
{
	IARM_BUS_Lock(lock);

        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            kConfigs1 = (dsAudioTypeConfig_t *) dlsym(dllib, "kAudioConfigs");
            if (kConfigs1) {
                INT_INFO("kAudioConfigs is defined and loaded kConfigs1 = %p\r\n", kConfigs1);
				printf("%d:%s: kAudioConfigs is defined and loaded kConfigs1 = %p\n", __LINE__, __func__, kConfigs1);
            }
            else {
                INT_ERROR("kAudioConfigs is not defined\r\n");
				INT_ERROR("%d:%s: kAudioConfigs is not defined\n", __LINE__, __func__);
            }

			pKConSize = (int *) dlsym(dllib, "kAudioConfigs_size");
			if(pKConSize)
			{
				kConfig_size_local = *pKConSize;
				INT_INFO("%d:%s: kAudioConfigs_size is defined and loaded kConfig_size_local = %d\n", __LINE__, __func__, kConfig_size_local);
			}
			else
			{
				INT_ERROR("%d:%s: kAudioConfigs_size is not defined\n", __LINE__, __func__);
				kConfig_size_local = -1;
			}

			kPorts1 = (dsAudioPortConfig_t *) dlsym(dllib, "kAudioPorts");
            if (kPorts1) {
                INT_INFO("kAudioPorts is defined and loaded kPorts1 = %p\r\n", kPorts1);
				INT_INFO("%d:%s: kAudioPorts is defined and loaded kPorts1 = %p\n", __LINE__, __func__, kPorts1);
            }
            else {
                INT_ERROR("kAudioPorts is not defined\r\n");
				INT_ERROR("%d:%s: kAudioPorts is not defined\n", __LINE__, __func__);
            }

			pKPortSize = (int *) dlsym(dllib, "kAudioPorts_size");
			if(pKPortSize)
			{
				kPort_size_local = *pKPortSize;
				INT_INFO("%d:%s: kAudioPorts_size is defined and loaded kPort_size_local = %d\n", __LINE__, __func__, kPort_size_local);
			}
			else
			{
				INT_ERROR("%d:%s: kAudioPorts_size is not defined\n", __LINE__, __func__);
				kPort_size_local = -1;
			}
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening libdshalsrv.so failed\r\n");
			INT_ERROR("%d:%s: Opening libdshalsrv.so failed\n", __LINE__, __func__);
        }
	IARM_BUS_Unlock(lock);
#if DEBUG
	INT_INFO("\n\n=========================================================================================================================\n\n");
	INT_INFO("\n%d:%s print configs using extern\n", __LINE__, __func__);
	//printf("%d:%s: size of(kConfig_audio) = %d size of(kPort_audio) = %d\n", __LINE__, __func__, kConfig_size, kPort_size);
	//kConfig_size_local = kConfig_size;
	//kPort_size_local = kPort_size;

	if(kConfigs1 != NULL && kConfig_size_local != -1)
	{
	for (size_t i = 0; i < kConfig_size_local; i++) {
			const dsAudioTypeConfig_t *typeCfg = &kConfigs1[i];
			//AudioOutputPortType &aPortType = AudioOutputPortType::getInstance(typeCfg->typeId);
			//aPortType.enable();
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
	if(kPorts1 != NULL && kPort_size_local != -1)
	{
		/*
		 * set up ports based on kPorts[]
		 */
		for (size_t i = 0; i < kPort_size_local; i++) {
			const dsAudioPortConfig_t *port = &kPorts1[i];
			INT_INFO("%d:%s: port->id.type = %d\n", __LINE__, __func__, port->id.type);
			INT_INFO("%d:%s: port->id.index = %d\n", __LINE__, __func__, port->id.index);
		}
	}
	else
	{
		INT_ERROR("%d:%s: kAudioPorts is NULL and kPort_size_local is -1\n", __LINE__, __func__);
	}
	INT_INFO("\n\n=========================================================================================================================\n\n");
#endif
	if(kConfigs1 == NULL || kConfig_size_local == -1 || kPorts1 == NULL || kPort_size_local == -1)
	{
		printf("Either kAudioConfigs or kAudioPorts is NULL and kConfig_size_local is -1, kPort_size_local is -1\n");
		return false;
	}
	else
	{
		printf("Both kAudioConfigs and kAudioPorts, kConfig_size_local, kPort_size_local are valid\n");
		return true;
	}
	//return (kConfigs1 && kPorts1);
}


void AudioOutputPortConfig::load()
{
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

		if(searchConfigs() ==  true)
		{
			/*
		 	* Initialize Audio portTypes (encodings, compressions etc.)
		 	* and its port instances (db, level etc)
		 	*/
			if(kConfigs1 != NULL)
			{
				for (size_t i = 0; i < kConfig_size_local; i++) {
					const dsAudioTypeConfig_t *typeCfg = &kConfigs1[i];
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
				if(kPorts1 != NULL)
				{
					for (size_t i = 0; i < kPort_size_local; i++) {
						const dsAudioPortConfig_t *port = &kPorts1[i];
						_aPorts.push_back(AudioOutputPort((port->id.type), port->id.index, i));
						_aPortTypes.at(port->id.type).addPort(_aPorts.at(i));
					}
				}
			}

		}
		else{
		/*
		 * Initialize Audio portTypes (encodings, compressions etc.)
		 * and its port instances (db, level etc)
		 */
		for (size_t i = 0; i < dsUTL_DIM(kConfigs); i++) {
			const dsAudioTypeConfig_t *typeCfg = &kConfigs[i];
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
		for (size_t i = 0; i < dsUTL_DIM(kPorts); i++) {
			const dsAudioPortConfig_t *port = &kPorts[i];
			_aPorts.push_back(AudioOutputPort((port->id.type), port->id.index, i));
			_aPortTypes.at(port->id.type).addPort(_aPorts.at(i));
		}
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
