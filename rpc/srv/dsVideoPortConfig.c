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
#include "dsVideoPortSettings.h"
#include "dsVideoResolutionSettings.h"
#include "dsVideoPortConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct videoPortConfigsLocal
{
	dsVideoPortTypeConfig_t  *pKVideoPortConfigs;
	int kVideoPortConfigs_size;
	dsVideoPortPortConfig_t  *pKVideoPortPorts;
	int kVideoPortPorts_size;
	dsVideoPortResolution_t *pKVideoPortResolutionsSettings;
	int kResolutionsSettings_size;
	int kDefaultResIndex;
}videoPortConfigsLocal_t;

static videoPortConfigsLocal_t videoPortConfiguration = {0};

void videoPortDumpconfig(videoPortConfigsLocal_t *config)
{
    if (NULL == config) {
        INT_ERROR("Video config is NULL\n");
        return;
    }
    if ( -1 == access("/opt/dsMgrDumpDeviceConfigs", F_OK) ) {
        INT_INFO("Dumping of Device configs is disabled\n");
        return;
    }
    INT_INFO("\n=============== Starting to Dump VideoPort Configs ===============\n");

    int configSize = -1, portSize = -1, resolutionSize = -1;
    if (( NULL != config->pKVideoPortConfigs ) && ( NULL != config->pKVideoPortPorts ) && ( NULL != config->pKVideoPortResolutionsSettings ))
    {
        configSize = config->kVideoPortConfigs_size;
        portSize = config->kVideoPortPorts_size;
        resolutionSize = config->kResolutionsSettings_size;
        INT_INFO("pKVideoPortConfigs = %p\n", config->pKVideoPortConfigs);
        INT_INFO("kVideoPortConfigs_size = %d\n", configSize);
        INT_INFO("pKVideoPortPorts = %p\n", config->pKVideoPortPorts);
        INT_INFO("kVideoPortPorts_size = %d\n", portSize);
        INT_INFO("pKResolutionsSettings = %p\n", config->pKVideoPortResolutionsSettings);
        INT_INFO("kResolutionsSettings_size = %d\n", resolutionSize);

        INT_INFO("\n\n############### Dumping Video Resolutions Settings ############### \n\n");

        for (int i = 0; i < resolutionSize; i++) {
            dsVideoPortResolution_t *resolution = &(config->pKVideoPortResolutionsSettings[i]);
            INT_INFO("resolution->name = %s\n", (resolution->name)? resolution->name : "NULL");
            INT_INFO("resolution->pixelResolution= %d\n", resolution->pixelResolution);
            INT_INFO("resolution->aspectRatio= %d\n", resolution->aspectRatio);
            INT_INFO("resolution->stereoScopicMode= %d\n", resolution->stereoScopicMode);
            INT_INFO("resolution->frameRate= %d\n", resolution->frameRate);
            INT_INFO("resolution->interlaced= %d\n", resolution->interlaced);
        }

        INT_INFO("\n ############### Dumping Video Port Configurations ############### \n");

        for (int i = 0; i < configSize; i++)
        {
            const dsVideoPortTypeConfig_t *typeCfg = &(config->pKVideoPortConfigs[i]);
            INT_INFO("typeCfg->typeId = %d\n", typeCfg->typeId);
            INT_INFO("typeCfg->name = %s\n", (typeCfg->name) ? typeCfg->name : "NULL");
            INT_INFO("typeCfg->dtcpSupported= %d\n", typeCfg->dtcpSupported);
            INT_INFO("typeCfg->hdcpSupported = %d\n", typeCfg->hdcpSupported);
            INT_INFO("typeCfg->restrictedResollution = %d\n", typeCfg->restrictedResollution);
            INT_INFO("typeCfg->numSupportedResolutions= %lu\n", typeCfg->numSupportedResolutions);

            if ((typeCfg->supportedResolutions != NULL) && (typeCfg->numSupportedResolutions > 0))
            {
                INT_INFO("typeCfg->supportedResolutions->name = %s\n",
                         (typeCfg->supportedResolutions->name) ? typeCfg->supportedResolutions->name : "NULL");
                INT_INFO("typeCfg->supportedResolutions->pixelResolution= %d\n",
                         typeCfg->supportedResolutions->pixelResolution);
                INT_INFO("typeCfg->supportedResolutions->aspectRatio= %d\n",
                         typeCfg->supportedResolutions->aspectRatio);
                INT_INFO("typeCfg->supportedResolutions->stereoScopicMode= %d\n",
                         typeCfg->supportedResolutions->stereoScopicMode);
                INT_INFO("typeCfg->supportedResolutions->frameRate= %d\n",
                         typeCfg->supportedResolutions->frameRate);
                INT_INFO("typeCfg->supportedResolutions->interlaced= %d\n",
                         typeCfg->supportedResolutions->interlaced);
            }
            else
            {
                INT_INFO("typeCfg has no supportedResolutions entries to dump (pointer is %p, numSupportedResolutions = %lu)\n",
                         typeCfg->supportedResolutions,
                         typeCfg->numSupportedResolutions);
            }
        }
        INT_INFO("\n############### Dumping Video Port Connections ###############\n");

        for (int i = 0; i < portSize; i++) {
            const dsVideoPortPortConfig_t *portCfg = &(config->pKVideoPortPorts[i]);
            INT_INFO("portCfg->id.type = %d\n", portCfg->id.type);
            INT_INFO("portCfg->id.index = %d\n", portCfg->id.index);
            INT_INFO("portCfg->connectedAOP.type = %d\n", portCfg->connectedAOP.type);
            INT_INFO("portCfg->connectedAOP.index = %d\n", portCfg->connectedAOP.index);
            INT_INFO("portCfg->defaultResolution = %s\n", (portCfg->defaultResolution) ? portCfg->defaultResolution : "NULL");
        }
    }
    else
    {
        INT_ERROR("pKConfigs or pKPorts or pKResolutionsSettings is NULL\n");
    }
    INT_INFO("\n=============== Dump VideoPort Configs done ===============\n");
    INT_INFO("Exit function\n");
}

static int allocateAndCopyVideoPortConfigs(const dsVideoPortTypeConfig_t* source, int numElements, bool isDynamic)
{
    const char* configType = isDynamic ? "dynamic" : "static";
    
    if (numElements <= 0) {
        INT_ERROR("Invalid %s video port config numElements: %d\n", configType, numElements);
        return -1;
    }
    
    videoPortConfiguration.pKVideoPortConfigs = (dsVideoPortTypeConfig_t*)malloc(numElements * sizeof(dsVideoPortTypeConfig_t));
    if (videoPortConfiguration.pKVideoPortConfigs == NULL) {
        INT_ERROR("Failed to allocate memory for %s video port configs\n", configType);
        return -1;
    }
    
    /* First copy the structures themselves. Pointer fields (such as 'name') will be
     * fixed up below to avoid shallow copies into HAL-owned memory. */
    memcpy(videoPortConfiguration.pKVideoPortConfigs, source, numElements * sizeof(dsVideoPortTypeConfig_t));
    
    /* Deep copy the 'name' pointer for each config so it does not reference
     * memory owned by the HAL library, which may be unloaded. */
    for (int i = 0; i < numElements; ++i)
    {
        if (source[i].name != NULL)
        {
            char *dupName = strdup(source[i].name);
            if (dupName == NULL)
            {
                INT_ERROR("Failed to duplicate %s video port config name at index %d\n", configType, i);
                /* Clean up any names already duplicated and the configs array itself. */
                for (int j = 0; j < i; ++j)
                {
                    if (videoPortConfiguration.pKVideoPortConfigs[j].name != NULL)
                    {
                        free((void*)videoPortConfiguration.pKVideoPortConfigs[j].name);
                        videoPortConfiguration.pKVideoPortConfigs[j].name = NULL;
                    }
                }
                free(videoPortConfiguration.pKVideoPortConfigs);
                videoPortConfiguration.pKVideoPortConfigs = NULL;
                return -1;
            }
            videoPortConfiguration.pKVideoPortConfigs[i].name = dupName;
        }
        else
        {
            videoPortConfiguration.pKVideoPortConfigs[i].name = NULL;
        }
    }
    
    INT_INFO("Allocated and copied %d video port configs (%s)\n", numElements, configType);
    return numElements;
}

static int allocateAndCopyVideoPortPorts(const dsVideoPortPortConfig_t* source, int numElements, bool isDynamic)
{
    const char* configType = isDynamic ? "dynamic" : "static";
    
    if (numElements <= 0) {
        INT_ERROR("Invalid %s video port port numElements: %d\n", configType, numElements);
        return -1;
    }
    
    videoPortConfiguration.pKVideoPortPorts = (dsVideoPortPortConfig_t*)malloc(numElements * sizeof(dsVideoPortPortConfig_t));
    if (videoPortConfiguration.pKVideoPortPorts == NULL) {
        INT_ERROR("Failed to allocate memory for %s video port ports\n", configType);
        return -1;
    }
    
    /* First copy the structures themselves. Pointer fields (such as 'defaultResolution') will be
     * fixed up below to avoid shallow copies into HAL-owned memory. */
    memcpy(videoPortConfiguration.pKVideoPortPorts, source, numElements * sizeof(dsVideoPortPortConfig_t));
    
    /* Deep copy the 'defaultResolution' pointer for each port so it does not reference
     * memory owned by the HAL library, which may be unloaded. */
    for (int i = 0; i < numElements; ++i)
    {
        if (source[i].defaultResolution != NULL)
        {
            char *dupResolution = strdup(source[i].defaultResolution);
            if (dupResolution == NULL)
            {
                INT_ERROR("Failed to duplicate %s video port default resolution at index %d\n", configType, i);
                /* Clean up any resolutions already duplicated and the ports array itself. */
                for (int j = 0; j < i; ++j)
                {
                    if (videoPortConfiguration.pKVideoPortPorts[j].defaultResolution != NULL)
                    {
                        free((void*)videoPortConfiguration.pKVideoPortPorts[j].defaultResolution);
                        videoPortConfiguration.pKVideoPortPorts[j].defaultResolution = NULL;
                    }
                }
                free(videoPortConfiguration.pKVideoPortPorts);
                videoPortConfiguration.pKVideoPortPorts = NULL;
                return -1;
            }
            videoPortConfiguration.pKVideoPortPorts[i].defaultResolution = dupResolution;
        }
        else
        {
            videoPortConfiguration.pKVideoPortPorts[i].defaultResolution = NULL;
        }
    }
    
    INT_INFO("Allocated and copied %d video port ports (%s)\n", numElements, configType);
    return numElements;
}

static int allocateAndCopyVideoPortResolutions(const dsVideoPortResolution_t* source, int numElements, bool isDynamic)
{
    const char* configType = isDynamic ? "dynamic" : "static";
    
    if (numElements <= 0) {
        INT_ERROR("Invalid %s video port resolution numElements: %d\n", configType, numElements);
        return -1;
    }
    
    videoPortConfiguration.pKVideoPortResolutionsSettings = (dsVideoPortResolution_t*)malloc(numElements * sizeof(dsVideoPortResolution_t));
    if (videoPortConfiguration.pKVideoPortResolutionsSettings == NULL) {
        INT_ERROR("Failed to allocate memory for %s video port resolutions\n", configType);
        return -1;
    }
    
    /* Copy the structures. The 'name' field is a fixed array (char[32]), 
     * so memcpy correctly copies the entire content, not just a pointer. */
    memcpy(videoPortConfiguration.pKVideoPortResolutionsSettings, source, numElements * sizeof(dsVideoPortResolution_t));
    INT_INFO("Allocated and copied %d video port resolutions (%s)\n", numElements, configType);
    return numElements;
}

dsError_t dsLoadVideoOutputPortConfig(const videoPortConfigs_t* dynamicVideoPortConfigs)
{
    int configSize, portSize, resolutionSize, defaultResIndex;
    const dsVideoPortTypeConfig_t* videoPortConfigs;
    const dsVideoPortPortConfig_t* videoPortPorts;
    const dsVideoPortResolution_t* videoPortResolutions;
    bool isDynamic;

    INT_INFO("Using '%s' config\n", dynamicVideoPortConfigs ? "dynamic" : "static");
    
    // Set up parameters based on config source
    if (NULL != dynamicVideoPortConfigs) {
        configSize = *(dynamicVideoPortConfigs->pKVideoPortConfigs_size);
        portSize = *(dynamicVideoPortConfigs->pKVideoPortPorts_size);
        resolutionSize = *(dynamicVideoPortConfigs->pKResolutionsSettings_size);
        defaultResIndex = *(dynamicVideoPortConfigs->pKDefaultResIndex);
        videoPortConfigs = dynamicVideoPortConfigs->pKVideoPortConfigs;
        videoPortPorts = dynamicVideoPortConfigs->pKVideoPortPorts;
        videoPortResolutions = dynamicVideoPortConfigs->pKVideoPortResolutionsSettings;
        isDynamic = true;
    } else {
        configSize = dsUTL_DIM(kConfigs);
        portSize = dsUTL_DIM(kPorts);
        resolutionSize = dsUTL_DIM(kResolutions);
        defaultResIndex = kDefaultResIndex;
        videoPortConfigs = kConfigs;
        videoPortPorts = kPorts;
        videoPortResolutions = kResolutions;
        isDynamic = false;
    }

    // Allocate and copy video port resolutions FIRST (needed by configs)
    if (allocateAndCopyVideoPortResolutions(videoPortResolutions, resolutionSize, isDynamic) == -1) {
        INT_ERROR("Failed to allocate video port resolutions\n");
        return dsERR_GENERAL;
    }

    // Allocate and copy video port type configs
    if (allocateAndCopyVideoPortConfigs(videoPortConfigs, configSize, isDynamic) == -1) {
        INT_ERROR("Failed to allocate video port configs\n");
        // Clean up previously allocated memory
        if (videoPortConfiguration.pKVideoPortResolutionsSettings != NULL) {
            free(videoPortConfiguration.pKVideoPortResolutionsSettings);
            videoPortConfiguration.pKVideoPortResolutionsSettings = NULL;
        }
        return dsERR_GENERAL;
    }
    
    // Update supportedResolutions pointers to point to our copied resolutions array
    for (int i = 0; i < configSize; ++i) {
        if (videoPortConfiguration.pKVideoPortConfigs[i].supportedResolutions != NULL) {
            videoPortConfiguration.pKVideoPortConfigs[i].supportedResolutions = 
                videoPortConfiguration.pKVideoPortResolutionsSettings;
            INT_INFO("Updated config[%d] supportedResolutions pointer to %p\n", 
                     i, videoPortConfiguration.pKVideoPortResolutionsSettings);
        }
    }
    
    // Allocate and copy video port ports
    if (allocateAndCopyVideoPortPorts(videoPortPorts, portSize, isDynamic) == -1) {
        INT_ERROR("Failed to allocate video port ports\n");
        // Clean up previously allocated memory
        if (videoPortConfiguration.pKVideoPortConfigs != NULL) {
            for (int i = 0; i < configSize; ++i) {
                if (videoPortConfiguration.pKVideoPortConfigs[i].name != NULL) {
                    free((void*)videoPortConfiguration.pKVideoPortConfigs[i].name);
                }
            }
            free(videoPortConfiguration.pKVideoPortConfigs);
            videoPortConfiguration.pKVideoPortConfigs = NULL;
        }
        if (videoPortConfiguration.pKVideoPortResolutionsSettings != NULL) {
            free(videoPortConfiguration.pKVideoPortResolutionsSettings);
            videoPortConfiguration.pKVideoPortResolutionsSettings = NULL;
        }
        return dsERR_GENERAL;
    }

    videoPortConfiguration.kDefaultResIndex = defaultResIndex;
    INT_INFO("Store sizes videoPortConfiguration.kDefaultResIndex = %d\n", videoPortConfiguration.kDefaultResIndex);

    INT_INFO("Store sizes configSize =%d, portSize =%d, resolutionSize = %d\n", configSize, portSize, resolutionSize);
    videoPortConfiguration.kVideoPortConfigs_size = configSize;
    videoPortConfiguration.kVideoPortPorts_size = portSize;
    videoPortConfiguration.kResolutionsSettings_size = resolutionSize;
    INT_INFO("Store sizes kVideoPortConfigs_size = %d\n", videoPortConfiguration.kVideoPortConfigs_size);
    INT_INFO("Store sizes kVideoPortPorts_size = %d\n", videoPortConfiguration.kVideoPortPorts_size);
    INT_INFO("Store sizes kResolutionsSettings_size = %d\n", videoPortConfiguration.kResolutionsSettings_size);
 
    INT_INFO("VideoPort Config[%p] ConfigSize[%d] Ports[%p] PortSize[%d] Resolutions[%p] ResolutionSize[%d] DefaultResIndex[%d]\n",
            videoPortConfiguration.pKVideoPortConfigs,
            videoPortConfiguration.kVideoPortConfigs_size,
            videoPortConfiguration.pKVideoPortPorts,
            videoPortConfiguration.kVideoPortPorts_size,
            videoPortConfiguration.pKVideoPortResolutionsSettings,
            videoPortConfiguration.kResolutionsSettings_size,
            videoPortConfiguration.kDefaultResIndex);
    videoPortDumpconfig(&videoPortConfiguration);
    return dsERR_NONE;
}

// Getter functions for use across srv code
dsError_t _dsGetVideoPortTypeConfigs(int* outConfigSize, const dsVideoPortTypeConfig_t** outConfigs)
{
    if((outConfigSize == NULL) || (outConfigs == NULL))
    {
        INT_ERROR("Invalid argument pointer\n");
        return dsERR_GENERAL;
    }
    
    *outConfigSize = videoPortConfiguration.kVideoPortConfigs_size;
    *outConfigs = videoPortConfiguration.pKVideoPortConfigs;

    return dsERR_NONE;
}

dsError_t _dsGetVideoPortPortConfigs(int* outPortSize, const dsVideoPortPortConfig_t** outPorts)
{
    if((outPortSize == NULL) || (outPorts == NULL))
    {
        INT_ERROR("Invalid argument pointer\n");
        return dsERR_GENERAL;
    }
    
    *outPortSize = videoPortConfiguration.kVideoPortPorts_size;
    *outPorts = videoPortConfiguration.pKVideoPortPorts;

    return dsERR_NONE;
}

dsError_t _dsGetVideoPortResolutions(int* outResolutionSize, dsVideoPortResolution_t** outResolutions)
{
    if((outResolutionSize == NULL) || (outResolutions == NULL))
    {
        INT_ERROR("Invalid argument pointer\n");
        return dsERR_GENERAL;
    }
    
    *outResolutionSize = videoPortConfiguration.kResolutionsSettings_size;
    *outResolutions = videoPortConfiguration.pKVideoPortResolutionsSettings;

    return dsERR_NONE;
}

dsError_t _dsGetDefaultResolutionIndex(int* outDefaultIndex)
{

    if(outDefaultIndex == NULL)
    {
        INT_ERROR("Invalid argument pointer\n");
        return dsERR_GENERAL;
    }

    *outDefaultIndex = videoPortConfiguration.kDefaultResIndex;

    return dsERR_NONE;
}

void dsVideoPortConfigFree(void)
{
    INT_INFO("Freeing VideoPort configuration resources\n");
    
    // Free video port configs and their allocated name strings
    if (videoPortConfiguration.pKVideoPortConfigs != NULL) {
        for (int i = 0; i < videoPortConfiguration.kVideoPortConfigs_size; ++i) {
            if (videoPortConfiguration.pKVideoPortConfigs[i].name != NULL) {
                free((void*)videoPortConfiguration.pKVideoPortConfigs[i].name);
                videoPortConfiguration.pKVideoPortConfigs[i].name = NULL;
            }
        }
        free(videoPortConfiguration.pKVideoPortConfigs);
        videoPortConfiguration.pKVideoPortConfigs = NULL;
        INT_INFO("Freed pKVideoPortConfigs\n");
    }
    
    // Free video port ports and their allocated defaultResolution strings
    if (videoPortConfiguration.pKVideoPortPorts != NULL) {
        for (int i = 0; i < videoPortConfiguration.kVideoPortPorts_size; ++i) {
            if (videoPortConfiguration.pKVideoPortPorts[i].defaultResolution != NULL) {
                free((void*)videoPortConfiguration.pKVideoPortPorts[i].defaultResolution);
                videoPortConfiguration.pKVideoPortPorts[i].defaultResolution = NULL;
            }
        }
        free(videoPortConfiguration.pKVideoPortPorts);
        videoPortConfiguration.pKVideoPortPorts = NULL;
        INT_INFO("Freed pKVideoPortPorts\n");
    }
    
    // Free video port resolutions (name is embedded array, not allocated)
    if (videoPortConfiguration.pKVideoPortResolutionsSettings != NULL) {
        free(videoPortConfiguration.pKVideoPortResolutionsSettings);
        videoPortConfiguration.pKVideoPortResolutionsSettings = NULL;
        INT_INFO("Freed pKVideoPortResolutionsSettings\n");
    }
    
    // Reset size variables
    videoPortConfiguration.kVideoPortConfigs_size = 0;
    videoPortConfiguration.kVideoPortPorts_size = 0;
    videoPortConfiguration.kResolutionsSettings_size = 0;
    videoPortConfiguration.kDefaultResIndex = 0;
    
    INT_INFO("VideoPort configuration freed successfully\n");
}

#ifdef __cplusplus
}
#endif

/** @} */
/** @} */
