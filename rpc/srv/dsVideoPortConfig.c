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
#include "dsVideoPortSettings.h"

typedef struct videoPortConfigs
{
	const dsVideoPortTypeConfig_t  *pKVideoPortConfigs;
	int *pKVideoPortConfigs_size;
	const dsVideoPortPortConfig_t  *pKVideoPortPorts;
	int *pKVideoPortPorts_size;
	dsVideoPortResolution_t *pKVideoPortResolutionsSettings;
	int *pKResolutionsSettings_size;
	int *pKDefaultResIndex;
}videoPortConfigs_t;

static videoPortConfigs_t videoPortConfiguration = {0};
static dsVideoPortTypeConfig_t* allocatedVideoPortConfigs = NULL;
static dsVideoPortPortConfig_t* allocatedVideoPortPorts = NULL;
static dsVideoPortResolution_t* allocatedVideoPortResolutions = NULL;
static int g_videoPortConfigSize = -1;
static int g_videoPortPortSize = -1;
static int g_videoPortResolutionSize = -1;
static int g_defaultResIndex = -1;

void dumpconfig(videoPortConfigs_t *config)
{
    if (nullptr == config) {
        INT_ERROR("Video config is NULL");
        return;
    }
    if ( -1 == access("/opt/dsMgrDumpDeviceConfigs", F_OK) ) {
        INT_INFO("Dumping of Device configs is disabled");
        return;
    }
    INT_INFO("\n=============== Starting to Dump VideoPort Configs ===============\n");

    int configSize = -1, portSize = -1, resolutionSize = -1;
    if (( NULL != config->pKVideoPortConfigs ) && ( NULL != config->pKVideoPortPorts ) && ( nullptr != config->pKVideoPortResolutionsSettings ))
    {
        configSize = (config->pKVideoPortConfigs_size) ? *(config->pKVideoPortConfigs_size) : -1;
        portSize = (config->pKVideoPortPorts_size) ? *(config->pKVideoPortPorts_size) : -1;
        resolutionSize = (config->pKResolutionsSettings_size) ? *(config->pKResolutionsSettings_size) : -1;
        INT_INFO("pKVideoPortConfigs = %p", config->pKVideoPortConfigs);
        INT_INFO("pKConfigSize pointer %p = %d", config->pKVideoPortConfigs_size, configSize);
        INT_INFO("pKVideoPortPorts = %p", config->pKVideoPortPorts);
        INT_INFO("pKPortSize pointer %p = %d", config->pKVideoPortPorts_size, portSize);
        INT_INFO("pKResolutionsSettings = %p", config->pKVideoPortResolutionsSettings);
        INT_INFO("pKResolutionsSettingsSize pointer %p = %d", config->pKResolutionsSettings_size, resolutionSize);

        INT_INFO("\n\n############### Dumping Video Resolutions Settings ############### \n\n");

        for (int i = 0; i < resolutionSize; i++) {
            dsVideoPortResolution_t *resolution = &(config->pKVideoPortResolutionsSettings[i]);
            INT_INFO("resolution->name = %s", resolution->name);
            INT_INFO("resolution->pixelResolution= %d", resolution->pixelResolution);
            INT_INFO("resolution->aspectRatio= %d", resolution->aspectRatio);
            INT_INFO("resolution->stereoScopicMode= %d", resolution->stereoScopicMode);
            INT_INFO("resolution->frameRate= %d", resolution->frameRate);
            INT_INFO("resolution->interlaced= %d", resolution->interlaced);
        }

        INT_INFO("\n ############### Dumping Video Port Configurations ############### \n");

        for (int i = 0; i < configSize; i++)
        {
            const dsVideoPortTypeConfig_t *typeCfg = &(config->pKVideoPortConfigs[i]);
            INT_INFO("typeCfg->typeId = %d", typeCfg->typeId);
            INT_INFO("typeCfg->name = %s", (typeCfg->name) ? typeCfg->name : "NULL");
            INT_INFO("typeCfg->dtcpSupported= %d", typeCfg->dtcpSupported);
            INT_INFO("typeCfg->hdcpSupported = %d", typeCfg->hdcpSupported);
            INT_INFO("typeCfg->restrictedResollution = %d", typeCfg->restrictedResollution);
            INT_INFO("typeCfg->numSupportedResolutions= %lu", typeCfg->numSupportedResolutions);

            INT_INFO("typeCfg->supportedResolutions = %p", typeCfg->supportedResolutions);
            INT_INFO("typeCfg->supportedResolutions->name = %s", (typeCfg->supportedResolutions->name) ? typeCfg->supportedResolutions->name : "NULL");
            INT_INFO("typeCfg->supportedResolutions->pixelResolution= %d", typeCfg->supportedResolutions->pixelResolution);
            INT_INFO("typeCfg->supportedResolutions->aspectRatio= %d", typeCfg->supportedResolutions->aspectRatio);
            INT_INFO("typeCfg->supportedResolutions->stereoScopicMode= %d", typeCfg->supportedResolutions->stereoScopicMode);
            INT_INFO("typeCfg->supportedResolutions->frameRate= %d", typeCfg->supportedResolutions->frameRate);
            INT_INFO("typeCfg->supportedResolutions->interlaced= %d", typeCfg->supportedResolutions->interlaced);
        }
        INT_INFO("\n############### Dumping Video Port Connections ###############\n");

        for (int i = 0; i < portSize; i++) {
            const dsVideoPortPortConfig_t *portCfg = &(config->pKVideoPortPorts[i]);
            INT_INFO("portCfg->id.type = %d", portCfg->id.type);
            INT_INFO("portCfg->id.index = %d", portCfg->id.index);
            INT_INFO("portCfg->connectedAOP.type = %d", portCfg->connectedAOP.type);
            INT_INFO("portCfg->connectedAOP.index = %d", portCfg->connectedAOP.index);
            INT_INFO("portCfg->defaultResolution = %s", (portCfg->defaultResolution) ? portCfg->defaultResolution : "NULL");
        }
    }
    else
    {
        INT_ERROR("pKConfigs or pKPorts or pKResolutionsSettings is NULL");
    }
    INT_INFO("\n=============== Dump VideoPort Configs done ===============\n");
    INT_INFO("Exit function");
}

static int allocateAndCopyVideoPortConfigs(const dsVideoPortTypeConfig_t* source, int size, const char* configType)
{
    if (size <= 0) {
        INT_ERROR("Invalid %s video port config size: %d\n", configType, size);
        return -1;
    }
    
    allocatedVideoPortConfigs = (dsVideoPortTypeConfig_t*)malloc(size * sizeof(dsVideoPortTypeConfig_t));
    if (allocatedVideoPortConfigs == NULL) {
        INT_ERROR("Failed to allocate memory for %s video port configs\n", configType);
        return -1;
    }
    
    memcpy(allocatedVideoPortConfigs, source, size * sizeof(dsVideoPortTypeConfig_t));
    videoPortConfiguration.pKVideoPortConfigs = allocatedVideoPortConfigs;
    INT_INFO("Allocated and copied %d video port configs (%s)", size, configType);
    return size;
}

static int allocateAndCopyVideoPortPorts(const dsVideoPortPortConfig_t* source, int size, const char* configType)
{
    if (size <= 0) {
        INT_ERROR("Invalid %s video port port size: %d\n", configType, size);
        return -1;
    }
    
    allocatedVideoPortPorts = (dsVideoPortPortConfig_t*)malloc(size * sizeof(dsVideoPortPortConfig_t));
    if (allocatedVideoPortPorts == NULL) {
        INT_ERROR("Failed to allocate memory for %s video port ports\n", configType);
        return -1;
    }
    
    memcpy(allocatedVideoPortPorts, source, size * sizeof(dsVideoPortPortConfig_t));
    videoPortConfiguration.pKVideoPortPorts = allocatedVideoPortPorts;
    INT_INFO("Allocated and copied %d video port ports (%s)", size, configType);
    return size;
}

static int allocateAndCopyVideoPortResolutions(const dsVideoPortResolution_t* source, int size, const char* configType)
{
    if (size <= 0) {
        INT_ERROR("Invalid %s video port resolution size: %d\n", configType, size);
        return -1;
    }
    
    allocatedVideoPortResolutions = (dsVideoPortResolution_t*)malloc(size * sizeof(dsVideoPortResolution_t));
    if (allocatedVideoPortResolutions == NULL) {
        INT_ERROR("Failed to allocate memory for %s video port resolutions\n", configType);
        return -1;
    }
    
    memcpy(allocatedVideoPortResolutions, source, size * sizeof(dsVideoPortResolution_t));
    videoPortConfiguration.pKVideoPortResolutionsSettings = allocatedVideoPortResolutions;
    INT_INFO("Allocated and copied %d video port resolutions (%s)", size, configType);
    return size;
}

void dsLoadVideoOutputPortConfig(const videoPortConfigs_t* dynamicVideoPortConfigs)
{
    int configSize = -1, portSize = -1, resolutionSize = -1;

    INT_INFO("Using '%s' config", dynamicVideoPortConfigs ? "dynamic" : "static");
    
    if (NULL != dynamicVideoPortConfigs)
    {
        // Reading Video Port Type configs
        configSize = (dynamicVideoPortConfigs->pKVideoPortConfigs_size) ? *(dynamicVideoPortConfigs->pKVideoPortConfigs_size) : -1;
        configSize = allocateAndCopyVideoPortConfigs(dynamicVideoPortConfigs->pKVideoPortConfigs, configSize, "dynamic");
        
        // Reading Video Port Port configs
        portSize = (dynamicVideoPortConfigs->pKVideoPortPorts_size) ? *(dynamicVideoPortConfigs->pKVideoPortPorts_size) : -1;
        portSize = allocateAndCopyVideoPortPorts(dynamicVideoPortConfigs->pKVideoPortPorts, portSize, "dynamic");
        
        // Reading Video Port Resolutions
        resolutionSize = (dynamicVideoPortConfigs->pKResolutionsSettings_size) ? *(dynamicVideoPortConfigs->pKResolutionsSettings_size) : -1;
        resolutionSize = allocateAndCopyVideoPortResolutions(dynamicVideoPortConfigs->pKVideoPortResolutionsSettings, resolutionSize, "dynamic");
        
        // Reading Default Resolution Index
        if (dynamicVideoPortConfigs->pKDefaultResIndex != NULL) {
            g_defaultResIndex = *(dynamicVideoPortConfigs->pKDefaultResIndex);
            videoPortConfiguration.pKDefaultResIndex = &g_defaultResIndex;
            INT_INFO("Read default resolution index: %d (dynamic)", g_defaultResIndex);
        } else {
            INT_INFO("Default resolution index not available in dynamic config");
        }
    }
    else {
        // Using static configuration
        configSize = dsUTL_DIM(kConfigs);
        configSize = allocateAndCopyVideoPortConfigs(kConfigs, configSize, "static");
        
        portSize = dsUTL_DIM(kPorts);
        portSize = allocateAndCopyVideoPortPorts(kPorts, portSize, "static");
        
        resolutionSize = dsUTL_DIM(kResolutions);
        resolutionSize = allocateAndCopyVideoPortResolutions(kResolutions, resolutionSize, "static");
        
        // Using static default resolution index
        g_defaultResIndex = kDefaultResIndex;
        videoPortConfiguration.pKDefaultResIndex = &g_defaultResIndex;
        INT_INFO("Using static default resolution index: %d", g_defaultResIndex);
    }

    // Store sizes for getter functions
    g_videoPortConfigSize = configSize;
    g_videoPortPortSize = portSize;
    g_videoPortResolutionSize = resolutionSize;

    INT_INFO("VideoPort Config[%p] ConfigSize[%d] Ports[%p] PortSize[%d] Resolutions[%p] ResolutionSize[%d] DefaultResIndex[%d]",
            videoPortConfiguration.pKVideoPortConfigs,
            configSize,
            videoPortConfiguration.pKVideoPortPorts,
            portSize,
            videoPortConfiguration.pKVideoPortResolutionsSettings,
            resolutionSize,
            g_defaultResIndex);
    dumpconfig(&videoPortConfiguration);
}

// Getter functions for use across srv code
void dsGetVideoPortTypeConfigs(int* outConfigSize, const dsVideoPortTypeConfig_t** outConfigs)
{
    if (outConfigSize != NULL) {
        *outConfigSize = g_videoPortConfigSize;
    }
    if (outConfigs != NULL) {
        *outConfigs = videoPortConfiguration.pKVideoPortConfigs;
    }
}

void dsGetVideoPortPortConfigs(int* outPortSize, const dsVideoPortPortConfig_t** outPorts)
{
    if (outPortSize != NULL) {
        *outPortSize = g_videoPortPortSize;
    }
    if (outPorts != NULL) {
        *outPorts = videoPortConfiguration.pKVideoPortPorts;
    }
}

void dsGetVideoPortResolutions(int* outResolutionSize, dsVideoPortResolution_t** outResolutions)
{
    if (outResolutionSize != NULL) {
        *outResolutionSize = g_videoPortResolutionSize;
    }
    if (outResolutions != NULL) {
        *outResolutions = videoPortConfiguration.pKVideoPortResolutionsSettings;
    }
}

int dsGetDefaultResolutionIndex(void)
{
    return g_defaultResIndex;
}

/** @} */
/** @} */
