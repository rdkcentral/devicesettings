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
    if (nullptr == config) {
        INT_ERROR("Audio config is NULL");
        return;
    }
    if ( -1 == access("/opt/dsMgrDumpDeviceConfigs", F_OK) ) {
        INT_INFO("Dumping of Device configs is disabled");
        return;
    }

    int configSize = -1, portSize = -1;
    INT_INFO("\n=============== Starting to Dump Audio Configs ===============\n");
    if( nullptr != config->pKConfigs )
    {
        configSize = (config->pKConfigSize) ? *(config->pKConfigSize) : -1;

        for (int i = 0; i < configSize; i++) {
            const dsAudioTypeConfig_t *typeCfg = &(config->pKConfigs[i]);
            INT_INFO("typeCfg->typeId = %d", typeCfg->typeId);
            INT_INFO("typeCfg->name = %s", typeCfg->name);
            INT_INFO("typeCfg->numSupportedEncodings = %zu", typeCfg->numSupportedEncodings);
            INT_INFO("typeCfg->numSupportedCompressions = %zu", typeCfg->numSupportedCompressions);
            INT_INFO("typeCfg->numSupportedStereoModes = %zu", typeCfg->numSupportedStereoModes);
        }
    }
    else
    {
        INT_ERROR("kAudioConfigs is NULL");
    }

    if( nullptr != config->pKPorts )
    {
        portSize = (config->pKPortSize) ? *(config->pKPortSize) : -1;
        for (int i = 0; i < portSize; i++) {
            const dsAudioPortConfig_t *portCfg = &(config->pKPorts[i]);
            INT_INFO("portCfg->id.type = %d", portCfg->id.type);
            INT_INFO("portCfg->id.index = %d", portCfg->id.index);
        }
    }
    else
    {
        INT_ERROR("kAudioPorts is NULL");
    }

    INT_INFO("\n=============== Dump Audio Configs done ===============\n");
}

void AudioOutputPortConfig::load(audioConfigs_t* dynamicAudioConfigs)
{
    int configSize = -1, portSize = -1;
    audioConfigs_t configuration = {0};

    INT_INFO("Enter function");
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

        INT_INFO("Using '%s' config", dynamicAudioConfigs ? "dynamic" : "static");
        if ( nullptr != dynamicAudioConfigs )
        {
            configuration = *dynamicAudioConfigs;
            configSize = (configuration.pKConfigSize) ? *(configuration.pKConfigSize) : -1;
            portSize = (configuration.pKPortSize) ? *(configuration.pKPortSize) : -1;
        }
        else {
            configuration.pKConfigs = kConfigs;
            configSize = dsUTL_DIM(kConfigs);
            configuration.pKConfigSize = &configSize;
            configuration.pKPorts = kPorts;
            portSize = dsUTL_DIM(kPorts);
            configuration.pKPortSize = &portSize;
        }

        INT_INFO("Audio Config[%p] ConfigSize[%d] Ports[%p] PortSize[%d]",
                configuration.pKConfigs,
                configSize,
                configuration.pKPorts,
                portSize);

        dumpconfig(&configuration);

        /*
        * Check if configs are loaded properly
        */
        if (( nullptr != configuration.pKConfigs ) && ( nullptr != configuration.pKPorts ))
        {
            /*
            * Initialize Audio portTypes (encodings, compressions etc.)
            * and its port instances (db, level etc)
            */
            for (int i = 0; i < configSize; i++) {
                const dsAudioTypeConfig_t *typeCfg = &(configuration.pKConfigs[i]);
                AudioOutputPortType &aPortType = AudioOutputPortType::getInstance(typeCfg->typeId);
                aPortType.enable();
                for (int j = 0; j < typeCfg->numSupportedEncodings; j++) {
                    const dsAudioEncoding_t* encoding = &typeCfg->encodings[j];
                    aPortType.addEncoding(AudioEncoding::getInstance(*encoding));
                    _aEncodings.at(*encoding).enable();
                }
                for (int j = 0; j < typeCfg->numSupportedCompressions; j++) {
                    const dsAudioCompression_t* compression = &typeCfg->compressions[j];
                    aPortType.addCompression(*compression);
                    _aCompressions.at(*compression).enable();
                }
                for (int j = 0; j < typeCfg->numSupportedStereoModes; j++) {
                    const dsAudioStereoMode_t *stereoMode = &typeCfg->stereoModes[j];
                    aPortType.addStereoMode(*stereoMode);
                    _aStereoModes.at(*stereoMode).enable();
                }
            }

            /*
            * set up ports based on kPorts[]
            */
            for (int i = 0; i < portSize; i++) {
                const dsAudioPortConfig_t *portCfg = &configuration.pKPorts[i];
                _aPorts.push_back(AudioOutputPort((portCfg->id.type), portCfg->id.index, i));
                _aPortTypes.at(portCfg->id.type).addPort(_aPorts.at(i));
            }
            INT_INFO("Audio Configs loaded successfully");
        }
        else {
            INT_ERROR("Audio Configs loading failed");
        }
    }
    catch(const Exception &e) {
        throw e;
    }
    INT_INFO("Exit function");
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
