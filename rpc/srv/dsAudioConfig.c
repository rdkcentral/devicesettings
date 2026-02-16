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

typedef struct audioConfigs
{
	const dsAudioTypeConfig_t  *pKAudioConfigs;
	const dsAudioPortConfig_t  *pKAudioPorts;
	int *pKConfigSize;
	int *pKPortSize;
}audioConfigs_t;

audioConfigs_t audioConfiguration = {0};
static dsAudioTypeConfig_t* allocatedAudioConfigs = NULL;
static dsAudioPortConfig_t* allocatedAudioPorts = NULL;
static int g_audioConfigSize = -1;
static int g_audioPortSize = -1;

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

static int allocateAndCopyAudioConfigs(const dsAudioTypeConfig_t* source, int size, const char* configType)
{
    if (size <= 0) {
        INT_ERROR("Invalid %s config size: %d\n", configType, size);
        return -1;
    }
    
    allocatedAudioConfigs = (dsAudioTypeConfig_t*)malloc(size * sizeof(dsAudioTypeConfig_t));
    if (allocatedAudioConfigs == NULL) {
        INT_ERROR("Failed to allocate memory for %s audio configs\n", configType);
        return -1;
    }
    
    memcpy(allocatedAudioConfigs, source, size * sizeof(dsAudioTypeConfig_t));
    audioConfiguration.pKAudioConfigs = allocatedAudioConfigs;
    INT_INFO("Allocated and copied %d audio configs (%s)", size, configType);
    return size;
}

static int allocateAndCopyAudioPorts(const dsAudioPortConfig_t* source, int size, const char* configType)
{
    if (size <= 0) {
        INT_ERROR("Invalid %s port size: %d\n", configType, size);
        return -1;
    }
    
    allocatedAudioPorts = (dsAudioPortConfig_t*)malloc(size * sizeof(dsAudioPortConfig_t));
    if (allocatedAudioPorts == NULL) {
        INT_ERROR("Failed to allocate memory for %s audio ports\n", configType);
        // Free previously allocated configs if allocation fails
        if (allocatedAudioConfigs != NULL) {
            free(allocatedAudioConfigs);
            allocatedAudioConfigs = NULL;
            audioConfiguration.pKAudioConfigs = NULL;
        }
        return -1;
    }
    
    memcpy(allocatedAudioPorts, source, size * sizeof(dsAudioPortConfig_t));
    audioConfiguration.pKAudioPorts = allocatedAudioPorts;
    INT_INFO("Allocated and copied %d audio ports (%s)", size, configType);
    return size;
}

void dsLoadAudioOutputPortConfig(const audioConfigs_t* dynamicAudioConfigs)
{
    int configSize = -1, portSize = -1;

    INT_INFO("Using '%s' config", dynamicAudioConfigs ? "dynamic" : "static");
    
    if (NULL != dynamicAudioConfigs)
    {
        // Reading Audio type configs
        configSize = (dynamicAudioConfigs->pKConfigSize) ? *(dynamicAudioConfigs->pKConfigSize) : -1;
        configSize = allocateAndCopyAudioConfigs(dynamicAudioConfigs->pKAudioConfigs, configSize, "dynamic");
        
        // Reading Audio port configs
        portSize = (dynamicAudioConfigs->pKPortSize) ? *(dynamicAudioConfigs->pKPortSize) : -1;
        portSize = allocateAndCopyAudioPorts(dynamicAudioConfigs->pKAudioPorts, portSize, "dynamic");
    }
    else {
        // Using static configuration
        configSize = dsUTL_DIM(kConfigs);
        configSize = allocateAndCopyAudioConfigs(kConfigs, configSize, "static");
        
        portSize = dsUTL_DIM(kPorts);
        portSize = allocateAndCopyAudioPorts(kPorts, portSize, "static");
    }

    // Store sizes for getter functions
    g_audioConfigSize = configSize;
    g_audioPortSize = portSize;

    INT_INFO("Audio Config[%p] ConfigSize[%d] Ports[%p] PortSize[%d]",
            audioConfiguration.pKAudioConfigs,
            configSize,
            audioConfiguration.pKAudioPorts,
            portSize);
    dumpconfig(&audioConfiguration);
}

// Getter functions for use across srv code
void dsGetAudioTypeConfigs(int* outConfigSize, const dsAudioTypeConfig_t** outConfigs)
{
    if (outConfigSize != NULL) {
        *outConfigSize = g_audioConfigSize;
    }
    if (outConfigs != NULL) {
        *outConfigs = audioConfiguration.pKAudioConfigs;
    }
}

void dsGetAudioPortConfigs(int* outPortSize, const dsAudioPortConfig_t** outPorts)
{
    if (outPortSize != NULL) {
        *outPortSize = g_audioPortSize;
    }
    if (outPorts != NULL) {
        *outPorts = audioConfiguration.pKAudioPorts;
    }
}

/** @} */
/** @} */
