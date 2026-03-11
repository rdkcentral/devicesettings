/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
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
#include <unistd.h>
#include "dsserverlogger.h"
#include "dsTypes.h"
#include "dsError.h"
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
    if (NULL == config) {
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
    
    // Deep copy each audio type config
    for (int i = 0; i < numElements; i++) {
        dsAudioTypeConfig_t *dest = &audioConfiguration.pKAudioConfigs[i];
        const dsAudioTypeConfig_t *src = &source[i];
        
        // Copy primitive fields
        dest->typeId = src->typeId;
        dest->numSupportedCompressions = src->numSupportedCompressions;
        dest->numSupportedEncodings = src->numSupportedEncodings;
        dest->numSupportedStereoModes = src->numSupportedStereoModes;
        
        // Deep copy name string
        if (src->name != NULL) {
            dest->name = strdup(src->name);
            if (dest->name == NULL) {
                INT_ERROR("Failed to duplicate name for audio config %d\n", i);
                // Cleanup previously allocated items
                for (int j = 0; j < i; j++) {
                    free((void*)audioConfiguration.pKAudioConfigs[j].name);
                    free((void*)audioConfiguration.pKAudioConfigs[j].compressions);
                    free((void*)audioConfiguration.pKAudioConfigs[j].encodings);
                    free((void*)audioConfiguration.pKAudioConfigs[j].stereoModes);
                }
                free(audioConfiguration.pKAudioConfigs);
                audioConfiguration.pKAudioConfigs = NULL;
                return -1;
            }
        } else {
            dest->name = NULL;
        }
        
        // Deep copy compressions array
        if (src->compressions != NULL && src->numSupportedCompressions > 0) {
            size_t size = src->numSupportedCompressions * sizeof(dsAudioCompression_t);
            dest->compressions = (dsAudioCompression_t*)malloc(size);
            if (dest->compressions == NULL) {
                INT_ERROR("Failed to allocate compressions for audio config %d\n", i);
                free((void*)dest->name);
                // Cleanup previously allocated items
                for (int j = 0; j < i; j++) {
                    free((void*)audioConfiguration.pKAudioConfigs[j].name);
                    free((void*)audioConfiguration.pKAudioConfigs[j].compressions);
                    free((void*)audioConfiguration.pKAudioConfigs[j].encodings);
                    free((void*)audioConfiguration.pKAudioConfigs[j].stereoModes);
                }
                free(audioConfiguration.pKAudioConfigs);
                audioConfiguration.pKAudioConfigs = NULL;
                return -1;
            }
            memcpy((void*)dest->compressions, src->compressions, size);
        } else {
            dest->compressions = NULL;
        }
        
        // Deep copy encodings array
        if (src->encodings != NULL && src->numSupportedEncodings > 0) {
            size_t size = src->numSupportedEncodings * sizeof(dsAudioEncoding_t);
            dest->encodings = (dsAudioEncoding_t*)malloc(size);
            if (dest->encodings == NULL) {
                INT_ERROR("Failed to allocate encodings for audio config %d\n", i);
                free((void*)dest->name);
                free((void*)dest->compressions);
                // Cleanup previously allocated items
                for (int j = 0; j < i; j++) {
                    free((void*)audioConfiguration.pKAudioConfigs[j].name);
                    free((void*)audioConfiguration.pKAudioConfigs[j].compressions);
                    free((void*)audioConfiguration.pKAudioConfigs[j].encodings);
                    free((void*)audioConfiguration.pKAudioConfigs[j].stereoModes);
                }
                free(audioConfiguration.pKAudioConfigs);
                audioConfiguration.pKAudioConfigs = NULL;
                return -1;
            }
            memcpy((void*)dest->encodings, src->encodings, size);
        } else {
            dest->encodings = NULL;
        }
        
        // Deep copy stereoModes array
        if (src->stereoModes != NULL && src->numSupportedStereoModes > 0) {
            size_t size = src->numSupportedStereoModes * sizeof(dsAudioStereoMode_t);
            dest->stereoModes = (dsAudioStereoMode_t*)malloc(size);
            if (dest->stereoModes == NULL) {
                INT_ERROR("Failed to allocate stereoModes for audio config %d\n", i);
                free((void*)dest->name);
                free((void*)dest->compressions);
                free((void*)dest->encodings);
                // Cleanup previously allocated items
                for (int j = 0; j < i; j++) {
                    free((void*)audioConfiguration.pKAudioConfigs[j].name);
                    free((void*)audioConfiguration.pKAudioConfigs[j].compressions);
                    free((void*)audioConfiguration.pKAudioConfigs[j].encodings);
                    free((void*)audioConfiguration.pKAudioConfigs[j].stereoModes);
                }
                free(audioConfiguration.pKAudioConfigs);
                audioConfiguration.pKAudioConfigs = NULL;
                return -1;
            }
            memcpy((void*)dest->stereoModes, src->stereoModes, size);
        } else {
            dest->stereoModes = NULL;
        }
    }
    
    INT_INFO("Deep copied %d audio configs (%s)", numElements, configType);
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
    
    // Deep copy each audio port config
    for (int i = 0; i < numElements; i++) {
        dsAudioPortConfig_t *dest = &audioConfiguration.pKAudioPorts[i];
        const dsAudioPortConfig_t *src = &source[i];
        
        // Copy port ID
        dest->id = src->id;
        
        // Deep copy connectedVOPs array - it's an array of size dsVIDEOPORT_TYPE_MAX
        if (src->connectedVOPs != NULL) {
            size_t arraySize = dsVIDEOPORT_TYPE_MAX * sizeof(dsVideoPortPortId_t);
            dest->connectedVOPs = (dsVideoPortPortId_t*)malloc(arraySize);
            if (dest->connectedVOPs == NULL) {
                INT_ERROR("Failed to allocate connectedVOPs for audio port %d\n", i);
                // Cleanup previously allocated items
                for (int j = 0; j < i; j++) {
                    free((void*)audioConfiguration.pKAudioPorts[j].connectedVOPs);
                }
                free(audioConfiguration.pKAudioPorts);
                audioConfiguration.pKAudioPorts = NULL;
                return -1;
            }
            memcpy((void*)dest->connectedVOPs, src->connectedVOPs, arraySize);
        } else {
            dest->connectedVOPs = NULL;
        }
    }
    
    INT_INFO("Deep copied %d audio ports (%s)", numElements, configType);
    return numElements;
}

dsError_t dsLoadAudioOutputPortConfig(const audioConfigs_t* dynamicAudioConfigs)
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
        return dsERR_GENERAL;
    }
    
    // Allocate and copy audio port configs
    if (allocateAndCopyAudioPorts(audioPorts, portSize, isDynamic) == -1) {
        INT_ERROR("Failed to allocate audio port configs\n");
        // Clean up previously allocated audio configs with their deep-copied fields
        if (audioConfiguration.pKAudioConfigs != NULL) {
            for (int i = 0; i < configSize; i++) {
                dsAudioTypeConfig_t *config = &audioConfiguration.pKAudioConfigs[i];
                if (config->name != NULL) {
                    free((void*)config->name);
                }
                if (config->compressions != NULL) {
                    free((void*)config->compressions);
                }
                if (config->encodings != NULL) {
                    free((void*)config->encodings);
                }
                if (config->stereoModes != NULL) {
                    free((void*)config->stereoModes);
                }
            }
            free(audioConfiguration.pKAudioConfigs);
            audioConfiguration.pKAudioConfigs = NULL;
        }
        return dsERR_GENERAL;
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
    return dsERR_NONE;
}

// Getter functions for use across srv code
dsError_t _dsGetAudioTypeConfigs(int* outConfigSize, const dsAudioTypeConfig_t** outConfigs)
{
    
    if((outConfigSize == NULL) || (outConfigs == NULL))
    {
        INT_ERROR("Invalid argument pointer\n");
        return dsERR_GENERAL;
    }

    if (outConfigSize != NULL) {
        *outConfigSize = audioConfiguration.kConfigSize;
    }
    if (outConfigs != NULL) {
        *outConfigs = audioConfiguration.pKAudioConfigs;
    }

    return dsERR_NONE;
}

dsError_t _dsGetAudioPortConfigs(int* outPortSize, const dsAudioPortConfig_t** outPorts)
{
    
    if((outPortSize == NULL) || (outPorts == NULL))
    {
        INT_ERROR("Invalid argument pointer\n");
        return dsERR_GENERAL;
    }
    if (outPortSize != NULL) {
        *outPortSize = audioConfiguration.kPortSize;
    }
    if (outPorts != NULL) {
        *outPorts = audioConfiguration.pKAudioPorts;
    }
    
    return dsERR_NONE;
}

void dsAudioConfigFree(void)
{
    INT_INFO("Freeing Audio configuration resources\n");
    
    // Free audio configs and their deep-copied fields
    if (audioConfiguration.pKAudioConfigs != NULL) {
        for (int i = 0; i < audioConfiguration.kConfigSize; i++) {
            dsAudioTypeConfig_t *config = &audioConfiguration.pKAudioConfigs[i];
            
            // Free name string
            if (config->name != NULL) {
                free((void*)config->name);
            }
            
            // Free compressions array
            if (config->compressions != NULL) {
                free((void*)config->compressions);
            }
            
            // Free encodings array
            if (config->encodings != NULL) {
                free((void*)config->encodings);
            }
            
            // Free stereoModes array
            if (config->stereoModes != NULL) {
                free((void*)config->stereoModes);
            }
        }
        
        free(audioConfiguration.pKAudioConfigs);
        audioConfiguration.pKAudioConfigs = NULL;
        INT_INFO("Freed pKAudioConfigs and all deep-copied fields\n");
    }
    
    // Free audio ports and their deep-copied fields
    if (audioConfiguration.pKAudioPorts != NULL) {
        for (int i = 0; i < audioConfiguration.kPortSize; i++) {
            dsAudioPortConfig_t *port = &audioConfiguration.pKAudioPorts[i];
            
            // Free connectedVOPs array
            if (port->connectedVOPs != NULL) {
                free((void*)port->connectedVOPs);
            }
        }
        
        free(audioConfiguration.pKAudioPorts);
        audioConfiguration.pKAudioPorts = NULL;
        INT_INFO("Freed pKAudioPorts and all deep-copied fields\n");
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
