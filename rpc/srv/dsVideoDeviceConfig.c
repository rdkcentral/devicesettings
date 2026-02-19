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
#include "dsVideoDeviceSettings.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct videoDeviceConfig
{
	dsVideoConfig_t *pKVideoDeviceConfigs;
	int *pKVideoDeviceConfigs_size;
}videoDeviceConfig_t;

static videoDeviceConfig_t videoDeviceConfiguration;

void videoDeviceDumpconfig(videoDeviceConfig_t *config)
{
    if (NULL == config) {
        INT_ERROR("Video config is NULL");
        return;
    }
    if ( -1 == access("/opt/dsMgrDumpDeviceConfigs", F_OK) ) {
        INT_INFO("Dumping of Device configs is disabled");
        return;
    }

    INT_INFO("\n=============== Starting to Dump VideoDevice Configs ===============\n");

	if( NULL != config->pKVideoDeviceConfigs )
	{
        int configSize = (config->pKVideoDeviceConfigs_size) ? *(config->pKVideoDeviceConfigs_size) : -1;
		INT_INFO("pKVideoDeviceConfigs = %p", config->pKVideoDeviceConfigs);
		INT_INFO("videoDeviceConfigs_size = %d", configSize);
		for (int i = 0; i < configSize; i++) {
            dsVideoConfig_t* videoDeviceConfig = &config->pKVideoDeviceConfigs[i];
            INT_INFO("pKVideoDeviceConfigs[%d].numSupportedDFCs = %lu ", i, videoDeviceConfig->numSupportedDFCs);
			for (int j = 0; j < videoDeviceConfig->numSupportedDFCs; j++) {
				INT_INFO(" Address of pKVideoDeviceConfigs[%d].supportedDFCs[%d] = %d", i, j, videoDeviceConfig->supportedDFCs[j]);
			}
		}
	}
	else
	{
		INT_ERROR(" kVideoDeviceConfigs is NULL");
	}

	INT_INFO("\n=============== Dump VideoDevice Configs done ===============\n");
}

static int allocateAndCopyVideoDeviceConfigs(const dsVideoConfig_t* source, int numElements, bool isDynamic)
{
    const char* configType = isDynamic ? "dynamic" : "static";
    
    if (numElements <= 0) {
        INT_ERROR("Invalid %s video device config numElements: %d\n", configType, numElements);
        return -1;
    }
    
    videoDeviceConfiguration.pKVideoDeviceConfigs = (dsVideoConfig_t*)malloc(numElements * sizeof(dsVideoConfig_t));
    if (videoDeviceConfiguration.pKVideoDeviceConfigs == NULL) {
        INT_ERROR("Failed to allocate memory for %s video device configs\n", configType);
        return -1;
    }
    
    memcpy(videoDeviceConfiguration.pKVideoDeviceConfigs, source, numElements * sizeof(dsVideoConfig_t));
    INT_INFO("Allocated and copied %d video device configs (%s)", numElements, configType);
    return numElements;
}

void dsLoadVideoDeviceConfig(const videoDeviceConfig_t* dynamicVideoDeviceConfigs)
{
    int configSize = -1, ret = -1;

    INT_INFO("Using '%s' config", dynamicVideoDeviceConfigs ? "dynamic" : "static");
    
    if (NULL != dynamicVideoDeviceConfigs)
    {
        // Reading Video Device configs
        configSize = (dynamicVideoDeviceConfigs->pKVideoDeviceConfigs_size) ? *(dynamicVideoDeviceConfigs->pKVideoDeviceConfigs_size) : -1;
        ret = allocateAndCopyVideoDeviceConfigs(dynamicVideoDeviceConfigs->pKVideoDeviceConfigs, configSize, true);
        if(ret == -1)
        {
            INT_ERROR("failed to create dynamic memory\n");
        }
    }
    else {
        // Using static configuration
        configSize = dsUTL_DIM(kConfigs);
        ret = allocateAndCopyVideoDeviceConfigs(kConfigs, configSize, false);
        if(ret == -1)
        {
            INT_ERROR("failed to create dynamic memory\n");
        }
    }

    INT_INFO("Store sizes configSize =%d\n", configSize);
    videoDeviceConfiguration.pKVideoDeviceConfigs_size = (int*)malloc(sizeof(int));
    if (videoDeviceConfiguration.pKVideoDeviceConfigs_size == NULL) {
        INT_ERROR("Failed to allocate memory for pKConfigSize\n");
    }
    else {
        *(videoDeviceConfiguration.pKVideoDeviceConfigs_size) = configSize;
        INT_INFO("Store sizes *(videoDeviceConfiguration.pKVideoDeviceConfigs_size)  =%d\n", *(videoDeviceConfiguration.pKVideoDeviceConfigs_size));
    }

    INT_INFO("VideoDevice Config[%p] ConfigSize[%d]",
            videoDeviceConfiguration.pKVideoDeviceConfigs,
            *(videoDeviceConfiguration.pKVideoDeviceConfigs_size));
    videoDeviceDumpconfig(&videoDeviceConfiguration);
}

// Getter functions for use across srv code
void dsGetVideoDeviceConfigs(int* outConfigSize, dsVideoConfig_t** outConfigs)
{
    if (outConfigSize != NULL) {
        *outConfigSize = *(videoDeviceConfiguration.pKVideoDeviceConfigs_size);
    }
    if (outConfigs != NULL) {
        *outConfigs = videoDeviceConfiguration.pKVideoDeviceConfigs;
    }
}

#ifdef __cplusplus
}
#endif

/** @} */
/** @} */
