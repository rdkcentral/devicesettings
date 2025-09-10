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
#include "libIARM.h"
#include "libIBus.h"
#include "iarmUtil.h"

#define IARM_BUS_Lock(lock) pthread_mutex_lock(&dsLock)
#define IARM_BUS_Unlock(lock) pthread_mutex_unlock(&dsLock)
extern dsAudioTypeConfig_t  *kConfig_audio;
extern dsAudioPortConfig_t  *kPort_audio;
static dsAudioTypeConfig_t  *kConfigs1 = NULL;
static dsAudioPortConfig_t  *kPorts1 = NULL;

static pthread_mutex_t dsLock = PTHREAD_MUTEX_INITIALIZER;

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
	//static dsAudioTypeConfig_t  *kConfigs1 = NULL;
	//static dsAudioPortConfig_t  *kPorts1 = NULL;

	//if ( kConfigs1 == NULL) 
	{
        void *dllib = dlopen("libdshalsrv.so", RTLD_LAZY);
        if (dllib) {
            kConfigs1 = (dsAudioTypeConfig_t *) dlsym(dllib, "kConfig_audio");
            if (kConfigs1) {
                INT_DEBUG("kConfig_audio is defined and loaded kConfigs1 = %p\r\n", kConfigs1);
				printf("%d:%s: kConfig_audio is defined and loaded kConfigs1 = %p\n", __LINE__, __func__, kConfigs1);
            }
            else {
                INT_INFO("kConfig_audio is not defined\r\n");
				printf("%d:%s: kConfig_audio is not defined\n", __LINE__, __func__);
                //IARM_BUS_Unlock(lock);
                dlclose(dllib);
                return IARM_RESULT_INVALID_STATE;
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening libdshalsrv.so failed\r\n");
			printf("%d:%s: Opening libdshalsrv.so failed\n", __LINE__, __func__);
        }
    }

	//if ( kPorts1 == NULL) 
	{
        void *dllib = dlopen("libdshalsrv.so", RTLD_LAZY);
        if (dllib) {
            kPorts1 = (dsAudioPortConfig_t *) dlsym(dllib, "kPort_audio");
            if (kPorts1) {
                INT_DEBUG("kPort_audio is defined and loaded kPorts1 = %p\r\n", kPorts1);
				printf("%d:%s: kPort_audio is defined and loaded kPorts1 = %p\n", __LINE__, __func__, kPorts1);
            }
            else {
                INT_INFO("kPort_audio is not defined\r\n");
				printf("%d:%s: kPort_audio is not defined\n", __LINE__, __func__);
                //IARM_BUS_Unlock(lock);
                dlclose(dllib);
                return IARM_RESULT_INVALID_STATE;
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening libdshalsrv.so failed\r\n");
			printf("%d:%s: Opening libdshalsrv.so failed\n", __LINE__, __func__);
        }
    }
	printf("\n\n=========================================================================================================================\n\n");
	printf("\n%d:%s print configs\n", __LINE__, __func__);	
	for (size_t i = 0; i < dsUTL_DIM(kConfigs1); i++) {
			const dsAudioTypeConfig_t *typeCfg = &kConfigs1[i];
			AudioOutputPortType &aPortType = AudioOutputPortType::getInstance(typeCfg->typeId);
			aPortType.enable();
			printf("%d:%s: typeCfg->typeId = %d\n", __LINE__, __func__, typeCfg->typeId);
			printf("%d:%s: typeCfg->name = %s\n", __LINE__, __func__, typeCfg->name);
			printf("%d:%s: typeCfg->numSupportedEncodings = %zu\n", __LINE__, __func__, typeCfg->numSupportedEncodings);
			printf("%d:%s: typeCfg->numSupportedCompressions = %zu\n", __LINE__, __func__, typeCfg->numSupportedCompressions);
			printf("%d:%s: typeCfg->numSupportedStereoModes = %zu\n", __LINE__, __func__, typeCfg->numSupportedStereoModes);
		}


		/*
		 * set up ports based on kPorts[]
		 */
		for (size_t i = 0; i < dsUTL_DIM(kPorts1); i++) {
			const dsAudioPortConfig_t *port = &kPorts1[i];
			printf("%d:%s: port->id.type = %d\n", __LINE__, __func__, port->id.type);
			printf("%d:%s: port->id.index = %d\n", __LINE__, __func__, port->id.index);
		}
	printf("\n\n=========================================================================================================================\n\n");
	
	printf("\n\n=========================================================================================================================\n\n");
	printf("\n%d:%s print configs using extern\n", __LINE__, __func__);	
	for (size_t i = 0; i < dsUTL_DIM(kConfig_audio); i++) {
			const dsAudioTypeConfig_t *typeCfg = &kConfig_audio[i];
			AudioOutputPortType &aPortType = AudioOutputPortType::getInstance(typeCfg->typeId);
			aPortType.enable();
			printf("%d:%s: typeCfg->typeId = %d\n", __LINE__, __func__, typeCfg->typeId);
			printf("%d:%s: typeCfg->name = %s\n", __LINE__, __func__, typeCfg->name);
			printf("%d:%s: typeCfg->numSupportedEncodings = %zu\n", __LINE__, __func__, typeCfg->numSupportedEncodings);
			printf("%d:%s: typeCfg->numSupportedCompressions = %zu\n", __LINE__, __func__, typeCfg->numSupportedCompressions);
			printf("%d:%s: typeCfg->numSupportedStereoModes = %zu\n", __LINE__, __func__, typeCfg->numSupportedStereoModes);
		}


		/*
		 * set up ports based on kPorts[]
		 */
		for (size_t i = 0; i < dsUTL_DIM(kPort_audio); i++) {
			const dsAudioPortConfig_t *port = &kPort_audio[i];
			printf("%d:%s: port->id.type = %d\n", __LINE__, __func__, port->id.type);
			printf("%d:%s: port->id.index = %d\n", __LINE__, __func__, port->id.index);
		}
	printf("\n\n=========================================================================================================================\n\n");


	return (kConfigs1 || kPorts1);
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
#if 1
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
		searchConfigs();
#else //gsk
	if(searchConfigs())
		{
		/*if(kConfig_audio == NULL || kPort_audio == NULL) {
			throw IllegalArgumentException();
		}*/
		//dsAudioTypeConfig_t  *kConfigs = kConfig_audio;
		//dsAudioPortConfig_t  *kPorts = kPort_audio;	
         /*
		 * Initialize Audio portTypes (encodings, compressions etc.)
		 * and its port instances (db, level etc)
		 */
		for (size_t i = 0; i < dsUTL_DIM(kConfigs1); i++) {
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
		for (size_t i = 0; i < dsUTL_DIM(kPorts1); i++) {
			const dsAudioPortConfig_t *port = &kPorts1[i];
			_aPorts.push_back(AudioOutputPort((port->id.type), port->id.index, i));
			_aPortTypes.at(port->id.type).addPort(_aPorts.at(i));
		}
	}
	else 
	{
		printf("%d:%s: kConfig and kPorts are not available\n", __LINE__, __func__);
	}
#endif

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
