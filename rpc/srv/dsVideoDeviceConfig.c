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
#include "dsVideoDeviceConfig.h"
#include "dsVideoDeviceSettings.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct videoDeviceConfigLocal
{
	dsVideoConfig_t *pKVideoDeviceConfigs;
	int kVideoDeviceConfigs_size;
}videoDeviceConfigLocal_t;

static videoDeviceConfigLocal_t videoDeviceConfiguration = {0};

void videoDeviceDumpconfig(videoDeviceConfigLocal_t *config)
{
    if (NULL == config) {
        INT_ERROR("Video config is NULL\n");
        return;
    }
    if ( -1 == access("/opt/dsMgrDumpDeviceConfigs", F_OK) ) {
        INT_INFO("Dumping of Device configs is disabled\n");
        return;
    }

    INT_INFO("\n=============== Starting to Dump VideoDevice Configs ===============\n");

	if( NULL != config->pKVideoDeviceConfigs )
	{
        int configSize = config->kVideoDeviceConfigs_size;
		INT_INFO("pKVideoDeviceConfigs = %p\n", config->pKVideoDeviceConfigs);
		INT_INFO("videoDeviceConfigs_size = %d\n", configSize);
		for (int i = 0; i < configSize; i++) {
            dsVideoConfig_t* videoDeviceConfig = &config->pKVideoDeviceConfigs[i];
            INT_INFO("pKVideoDeviceConfigs[%d].numSupportedDFCs = %lu\n", i, videoDeviceConfig->numSupportedDFCs);
			for (int j = 0; j < videoDeviceConfig->numSupportedDFCs; j++) {
				INT_INFO(" Address of pKVideoDeviceConfigs[%d].supportedDFCs[%d] = %d\n", i, j, videoDeviceConfig->supportedDFCs[j]);
			}
		}
	}
	else
	{
		INT_ERROR(" kVideoDeviceConfigs is NULL\n");
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
    INT_INFO("Allocated and copied %d video device configs (%s)\n", numElements, configType);
    return numElements;
}

int dsLoadVideoDeviceConfig(const videoDeviceConfig_t* dynamicVideoDeviceConfigs)
{
    int configSize;
    const dsVideoConfig_t* videoConfigs;
    bool isDynamic;

    INT_INFO("Using '%s' config\n", dynamicVideoDeviceConfigs ? "dynamic" : "static");
    
    // Set up parameters based on config source
    if (NULL != dynamicVideoDeviceConfigs) {
        configSize = *(dynamicVideoDeviceConfigs->pKVideoDeviceConfigs_size);
        videoConfigs = dynamicVideoDeviceConfigs->pKVideoDeviceConfigs;
        isDynamic = true;
    } else {
        configSize = dsUTL_DIM(kConfigs);
        videoConfigs = kConfigs;
        isDynamic = false;
    }

    // Allocate and copy video device configs
    if (allocateAndCopyVideoDeviceConfigs(videoConfigs, configSize, isDynamic) == -1) {
        INT_ERROR("Failed to allocate video device configs\n");
        return -1;
    }

    INT_INFO("Store sizes configSize =%d\n", configSize);
    videoDeviceConfiguration.kVideoDeviceConfigs_size = configSize;
    INT_INFO("Store sizes videoDeviceConfiguration.kVideoDeviceConfigs_size = %d\n", videoDeviceConfiguration.kVideoDeviceConfigs_size);

    INT_INFO("VideoDevice Config[%p] ConfigSize[%d]\n",
            videoDeviceConfiguration.pKVideoDeviceConfigs,
            videoDeviceConfiguration.kVideoDeviceConfigs_size);
    videoDeviceDumpconfig(&videoDeviceConfiguration);
    return 0;
}

// Getter functions for use across srv code
void dsGetVideoDeviceConfigs(int* outConfigSize, dsVideoConfig_t** outConfigs)
{
    if (outConfigSize != NULL) {
        *outConfigSize = videoDeviceConfiguration.kVideoDeviceConfigs_size;
    }
    if (outConfigs != NULL) {
        *outConfigs = videoDeviceConfiguration.pKVideoDeviceConfigs;
    }
}

void dsVideoDeviceConfigFree(void)
{
    INT_INFO("Freeing VideoDevice configuration resources\n");
    
    // Free video device configs
    if (videoDeviceConfiguration.pKVideoDeviceConfigs != NULL) {
        free(videoDeviceConfiguration.pKVideoDeviceConfigs);
        videoDeviceConfiguration.pKVideoDeviceConfigs = NULL;
        INT_INFO("Freed pKVideoDeviceConfigs\n");
    }
    
    // Reset size variable
    videoDeviceConfiguration.kVideoDeviceConfigs_size = 0;
    
    INT_INFO("VideoDevice configuration freed successfully\n");
}

#ifdef __cplusplus
}
#endif

/** @} */
/** @} */
