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
#include "dsAudioConfig.h"
#include "dsAudioSettings.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct audioConfigsLocal
{
	dsAudioTypeConfig_t  *pKAudioConfigs;
	dsAudioPortConfig_t  *pKAudioPorts;
	int kConfigSize;
	int kPortSize;
}audioConfigsLocal_t;

static audioConfigsLocal_t audioConfiguration = {0};

void audioDumpconfig(audioConfigsLocal_t *config)
{
    if (nullptr == config) {
        INT_ERROR("Audio config is NULL\n");
        return;
    }
    if ( -1 == access("/opt/dsMgrDumpDeviceConfigs", F_OK) ) {
        INT_INFO("Dumping of Device configs is disabled\n");
        return;
    }

    int configSize = -1, portSize = -1;
    INT_INFO("\n=============== Starting to Dump Audio Configs ===============\n");
    if( NULL != config->pKAudioConfigs )
    {
        configSize = config->kConfigSize;

        for (int i = 0; i < configSize; i++) {
            const dsAudioTypeConfig_t *typeCfg = &(config->pKAudioConfigs[i]);
            INT_INFO("typeCfg->typeId = %d\n", typeCfg->typeId);
            INT_INFO("typeCfg->name = %s\n", typeCfg->name);
            INT_INFO("typeCfg->numSupportedEncodings = %zu\n", typeCfg->numSupportedEncodings);
            INT_INFO("typeCfg->numSupportedCompressions = %zu\n", typeCfg->numSupportedCompressions);
            INT_INFO("typeCfg->numSupportedStereoModes = %zu\n", typeCfg->numSupportedStereoModes);
        }
    }
    else
    {
        INT_ERROR("kAudioConfigs is NULL\n");
    }

    if( NULL != config->pKAudioPorts )
    {
        portSize = config->kPortSize;
        for (int i = 0; i < portSize; i++) {
            const dsAudioPortConfig_t *portCfg = &(config->pKAudioPorts[i]);
            INT_INFO("portCfg->id.type = %d\n", portCfg->id.type);
            INT_INFO("portCfg->id.index = %d\n", portCfg->id.index);
        }
    }
    else
    {
        INT_ERROR("kAudioPorts is NULL\n");
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

int dsLoadAudioOutputPortConfig(const audioConfigs_t* dynamicAudioConfigs)
{
    int configSize, portSize;
    const dsAudioTypeConfig_t* audioConfigs;
    const dsAudioPortConfig_t* audioPorts;
    bool isDynamic;

    INT_INFO("Using '%s' config\n", dynamicAudioConfigs ? "dynamic" : "static");
    
    // Set up parameters based on config source
    if (NULL != dynamicAudioConfigs) {
        configSize = *(dynamicAudioConfigs->pKConfigSize);
        portSize = *(dynamicAudioConfigs->pKPortSize);
        audioConfigs = dynamicAudioConfigs->pKAudioConfigs;
        audioPorts = dynamicAudioConfigs->pKAudioPorts;
        isDynamic = true;
    } else {
        configSize = dsUTL_DIM(kConfigs);
        portSize = dsUTL_DIM(kPorts);
        audioConfigs = kConfigs;
        audioPorts = kPorts;
        isDynamic = false;
    }

    // Allocate and copy audio type configs
    if (allocateAndCopyAudioConfigs(audioConfigs, configSize, isDynamic) == -1) {
        INT_ERROR("Failed to allocate audio type configs\n");
        return -1;
    }
    
    // Allocate and copy audio port configs
    if (allocateAndCopyAudioPorts(audioPorts, portSize, isDynamic) == -1) {
        INT_ERROR("Failed to allocate audio port configs\n");
        // Clean up previously allocated memory
        if (audioConfiguration.pKAudioConfigs != NULL) {
            free(audioConfiguration.pKAudioConfigs);
            audioConfiguration.pKAudioConfigs = NULL;
        }
        return -1;
    }

    INT_INFO("Store sizes configSize =%d, portSize =%d\n", configSize, portSize);

    audioConfiguration.kConfigSize = configSize;
    audioConfiguration.kPortSize = portSize;
    INT_INFO("Store sizes audioConfiguration.kConfigSize = %d\n", audioConfiguration.kConfigSize);
    INT_INFO("Store sizes audioConfiguration.kPortSize = %d\n", audioConfiguration.kPortSize);
 
    INT_INFO("Audio Config[%p] ConfigSize[%d] Ports[%p] PortSize[%d]\n",
            audioConfiguration.pKAudioConfigs ? audioConfiguration.pKAudioConfigs : NULL,
            audioConfiguration.kConfigSize,
            audioConfiguration.pKAudioPorts? audioConfiguration.pKAudioPorts : NULL,
            audioConfiguration.kPortSize);

    audioDumpconfig(&audioConfiguration);
    return 0;
}

// Getter functions for use across srv code
void dsGetAudioTypeConfigs(int* outConfigSize, const dsAudioTypeConfig_t** outConfigs)
{
    if (outConfigSize != NULL) {
        *outConfigSize = audioConfiguration.kConfigSize;
    }
    if (outConfigs != NULL) {
        *outConfigs = audioConfiguration.pKAudioConfigs;
    }
}

void dsGetAudioPortConfigs(int* outPortSize, const dsAudioPortConfig_t** outPorts)
{
    if (outPortSize != NULL) {
        *outPortSize = audioConfiguration.kPortSize;
    }
    if (outPorts != NULL) {
        *outPorts = audioConfiguration.pKAudioPorts;
    }
}

void dsAudioConfigFree(void)
{
    INT_INFO("Freeing Audio configuration resources\n");
    
    // Free audio configs
    if (audioConfiguration.pKAudioConfigs != NULL) {
        free(audioConfiguration.pKAudioConfigs);
        audioConfiguration.pKAudioConfigs = NULL;
        INT_INFO("Freed pKAudioConfigs\n");
    }
    
    // Free audio ports
    if (audioConfiguration.pKAudioPorts != NULL) {
        free(audioConfiguration.pKAudioPorts);
        audioConfiguration.pKAudioPorts = NULL;
        INT_INFO("Freed pKAudioPorts\n");
    }
    
    // Reset size variables
    audioConfiguration.kConfigSize = 0;
    audioConfiguration.kPortSize = 0;
    
    INT_INFO("Audio configuration freed successfully\n");
}

#ifdef __cplusplus
}
#endif

/** @} */
/** @} */
