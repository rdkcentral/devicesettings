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
* @defgroup rpc
* @{
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dsserverlogger.h"
#include "dsTypes.h"
#include "dsAudioSettings.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct audioConfigs
{
	dsAudioTypeConfig_t  *pKAudioConfigs;
	dsAudioPortConfig_t  *pKAudioPorts;
	int *pKConfigSize;
	int *pKPortSize;
}audioConfigs_t;

audioConfigs_t audioConfiguration = {0};

void audioDumpconfig(audioConfigs_t *config)
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
    if( NULL != config->pKAudioConfigs )
    {
        configSize = (config->pKConfigSize) ? *(config->pKConfigSize) : -1;

        for (int i = 0; i < configSize; i++) {
            const dsAudioTypeConfig_t *typeCfg = &(config->pKAudioConfigs[i]);
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

    if( NULL != config->pKAudioPorts )
    {
        portSize = (config->pKPortSize) ? *(config->pKPortSize) : -1;
        for (int i = 0; i < portSize; i++) {
            const dsAudioPortConfig_t *portCfg = &(config->pKAudioPorts[i]);
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

static int allocateAndCopyAudioConfigs(const dsAudioTypeConfig_t* source, int numElements, bool isDynamic)
{
    const char* configType = isDynamic ? "dynamic" : "static";
    
    if (numElements <= 0) {
        INT_ERROR("Invalid %s config numElements: %d\n", configType, numElements);
        return -1;
    }
    
    audioConfiguration.pKAudioConfigs = (dsAudioTypeConfig_t*)malloc(numElements * sizeof(dsAudioTypeConfig_t));
    if (audioConfiguration.pKAudioConfigs == NULL) {
        INT_ERROR("Failed to allocate memory for %s audio configs\n", configType);
        return -1;
    }
    
    memcpy(audioConfiguration.pKAudioConfigs, source, numElements * sizeof(dsAudioTypeConfig_t));
    INT_INFO("Allocated and copied %d audio configs (%s)", numElements, configType);
    return numElements;
}

static int allocateAndCopyAudioPorts(const dsAudioPortConfig_t* source, int numElements, bool isDynamic)
{
    const char* configType = isDynamic ? "dynamic" : "static";
    
    if (numElements <= 0) {
        INT_ERROR("Invalid %s port numElements: %d\n", configType, numElements);
        return -1;
    }
    
    audioConfiguration.pKAudioPorts = (dsAudioPortConfig_t*)malloc(numElements * sizeof(dsAudioPortConfig_t));
    if (audioConfiguration.pKAudioPorts == NULL) {
        INT_ERROR("Failed to allocate memory for %s audio ports\n", configType);
        return -1;
    }
    
    memcpy(audioConfiguration.pKAudioPorts, source, numElements * sizeof(dsAudioPortConfig_t));
    INT_INFO("Allocated and copied %d audio ports (%s)", numElements, configType);
    return numElements;
}

void dsLoadAudioOutputPortConfig(const audioConfigs_t* dynamicAudioConfigs)
{
    int configSize = -1, portSize = -1;
    int ret = -1;

    INT_INFO("Using '%s' config", dynamicAudioConfigs ? "dynamic" : "static");
    
    if (NULL != dynamicAudioConfigs)
    {
        // Reading Audio type configs
        configSize = (dynamicAudioConfigs->pKConfigSize) ? *(dynamicAudioConfigs->pKConfigSize) : -1;
        ret = allocateAndCopyAudioConfigs(dynamicAudioConfigs->pKAudioConfigs, configSize, true);
        if(ret == -1)
        {
            INT_ERROR("failed to create dynamic memory\n");
        }
        
        // Reading Audio port configs
        portSize = (dynamicAudioConfigs->pKPortSize) ? *(dynamicAudioConfigs->pKPortSize) : -1;
        ret = allocateAndCopyAudioPorts(dynamicAudioConfigs->pKAudioPorts, portSize, true);
        if(ret == -1)
        {
            INT_ERROR("failed to create dynamic memory\n");
        }
    }
    else {
        // Using static configuration
        configSize = dsUTL_DIM(kConfigs);
        ret = allocateAndCopyAudioConfigs(kConfigs, configSize, false);
        if(ret == -1)
        {
            INT_ERROR("failed to create dynamic memory\n");
        }

        portSize = dsUTL_DIM(kPorts);
        ret = allocateAndCopyAudioPorts(kPorts, portSize, false);
        if(ret == -1)
        {
            INT_ERROR("failed to create dynamic memory\n");
        }
    }

    // Store sizes for getter functions
    *(audioConfiguration.pKConfigSize) = configSize;
    *(audioConfiguration.pKPortSize) = portSize;

    INT_INFO("Audio Config[%p] ConfigSize[%d] Ports[%p] PortSize[%d]",
            audioConfiguration.pKAudioConfigs,
            *(audioConfiguration.pKConfigSize),
            audioConfiguration.pKAudioPorts,
            *(audioConfiguration.pKPortSize));
    audioDumpconfig(&audioConfiguration);
}

// Getter functions for use across srv code
void dsGetAudioTypeConfigs(int* outConfigSize, const dsAudioTypeConfig_t** outConfigs)
{
    if (outConfigSize != NULL) {
        *outConfigSize = *(audioConfiguration.pKConfigSize);
    }
    if (outConfigs != NULL) {
        *outConfigs = audioConfiguration.pKAudioConfigs;
    }
}

void dsGetAudioPortConfigs(int* outPortSize, const dsAudioPortConfig_t** outPorts)
{
    if (outPortSize != NULL) {
        *outPortSize = *(audioConfiguration.pKPortSize);
    }
    if (outPorts != NULL) {
        *outPorts = audioConfiguration.pKAudioPorts;
    }
}

#ifdef __cplusplus
}
#endif

/** @} */
/** @} */
