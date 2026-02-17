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
#include <dlfcn.h>
#include "dsserverlogger.h"
#include "dsAudioConfig.h"
#include "dsVideoPortConfig.h"
#include "dsVideoDeviceConfig.h"

#ifndef RDK_DSHAL_NAME
#define RDK_DSHAL_NAME "libdshal.so"
#endif

#define DEVICE_CAPABILITY_VIDEO_PORT    0x01
#define DEVICE_CAPABILITY_AUDIO_PORT    0x02
#define DEVICE_CAPABILITY_VIDEO_DEVICE  0x04
#define DEVICE_CAPABILITY_FRONT_PANEL   0x08

typedef struct {
    const char* name;
    void** dataptr;
} dlSymbolLookup_t;


static int LoadDLSymbols(void* pDLHandle, const dlSymbolLookup_t* symbols, int numberOfSymbols)
{
    int currentSymbols = 0;
    int isAllSymbolsLoaded = 0;
    
    if ((NULL == pDLHandle) || (NULL == symbols)) {
        INT_ERROR("Invalid DL Handle or symbolsPtr");
        return 0;
    }
    
    INT_INFO("numberOfSymbols = %d", numberOfSymbols);
    for (int i = 0; i < numberOfSymbols; i++) {
        if ((NULL == symbols[i].dataptr) || (NULL == symbols[i].name)) {
            INT_ERROR("Invalid symbol entry at index [%d]", i);
            continue;
        }
        *(symbols[i].dataptr) = dlsym(pDLHandle, symbols[i].name);
        if (NULL == *(symbols[i].dataptr)) {
            INT_ERROR("[%s] is not defined", symbols[i].name);
        }
        else {
            currentSymbols++;
            INT_INFO("[%s] is defined and loaded, data[%p]", symbols[i].name, *(symbols[i].dataptr));
        }
    }
    isAllSymbolsLoaded = (numberOfSymbols) ? (currentSymbols == numberOfSymbols) : 0;
    return isAllSymbolsLoaded;
}

static void loadDeviceCapabilities(unsigned int capabilityType)
{
    void* pDLHandle = NULL;
    int isSymbolsLoaded = 0;

    INT_INFO("Entering capabilityType = 0x%08X", capabilityType);
    dlerror(); /* clear old error */
    pDLHandle = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
    INT_INFO("DL Instance '%s'", (NULL == pDLHandle ? "NULL" : "Valid"));

    /* Audio Port Config */
    if (DEVICE_CAPABILITY_AUDIO_PORT & capabilityType) {
        audioConfigs_t dynamicAudioConfigs;
        memset(&dynamicAudioConfigs, 0, sizeof(audioConfigs_t));
        
        dlSymbolLookup_t audioConfigSymbols[] = {
            {"kAudioConfigs", (void**)&dynamicAudioConfigs.pKAudioConfigs},
            {"kAudioPorts", (void**)&dynamicAudioConfigs.pKAudioPorts},
            {"kAudioConfigs_size", (void**)&dynamicAudioConfigs.pKConfigSize},
            {"kAudioPorts_size", (void**)&dynamicAudioConfigs.pKPortSize}
        };

        isSymbolsLoaded = 0;
        if (NULL != pDLHandle) {
            isSymbolsLoaded = LoadDLSymbols(pDLHandle, audioConfigSymbols, 
                                           sizeof(audioConfigSymbols)/sizeof(dlSymbolLookup_t));
        }
        dsLoadAudioOutputPortConfig(isSymbolsLoaded ? &dynamicAudioConfigs : NULL);
    }

    /* Video Port Config */
    if (DEVICE_CAPABILITY_VIDEO_PORT & capabilityType) {
        videoPortConfigs_t dynamicVideoPortConfigs;
        memset(&dynamicVideoPortConfigs, 0, sizeof(videoPortConfigs_t));
        
        dlSymbolLookup_t videoPortConfigSymbols[] = {
            {"kVideoPortConfigs", (void**)&dynamicVideoPortConfigs.pKVideoPortConfigs},
            {"kVideoPortConfigs_size", (void**)&dynamicVideoPortConfigs.pKVideoPortConfigs_size},
            {"kVideoPortPorts", (void**)&dynamicVideoPortConfigs.pKVideoPortPorts},
            {"kVideoPortPorts_size", (void**)&dynamicVideoPortConfigs.pKVideoPortPorts_size},
            {"kResolutionsSettings", (void**)&dynamicVideoPortConfigs.pKVideoPortResolutionsSettings},
            {"kResolutionsSettings_size", (void**)&dynamicVideoPortConfigs.pKResolutionsSettings_size},
            {"kDefaultResIndex", (void**)&dynamicVideoPortConfigs.pKDefaultResIndex}
        };

        isSymbolsLoaded = 0;
        if (NULL != pDLHandle) {
            isSymbolsLoaded = LoadDLSymbols(pDLHandle, videoPortConfigSymbols, 
                                           sizeof(videoPortConfigSymbols)/sizeof(dlSymbolLookup_t));
        }
        dsLoadVideoOutputPortConfig(isSymbolsLoaded ? &dynamicVideoPortConfigs : NULL);
    }

    /* Video Device Config */
    if (DEVICE_CAPABILITY_VIDEO_DEVICE & capabilityType) {
        videoDeviceConfig_t dynamicVideoDeviceConfigs;
        memset(&dynamicVideoDeviceConfigs, 0, sizeof(videoDeviceConfig_t));
        
        dlSymbolLookup_t videoDeviceConfigSymbols[] = {
            {"kVideoDeviceConfigs", (void**)&dynamicVideoDeviceConfigs.pKVideoDeviceConfigs},
            {"kVideoDeviceConfigs_size", (void**)&dynamicVideoDeviceConfigs.pKVideoDeviceConfigs_size}
        };
        
        isSymbolsLoaded = 0;
        if (NULL != pDLHandle) {
            isSymbolsLoaded = LoadDLSymbols(pDLHandle, videoDeviceConfigSymbols, 
                                           sizeof(videoDeviceConfigSymbols)/sizeof(dlSymbolLookup_t));
        }
        dsLoadVideoDeviceConfig(isSymbolsLoaded ? &dynamicVideoDeviceConfigs : NULL);
    }

    if (NULL != pDLHandle) {
        dlclose(pDLHandle);
        pDLHandle = NULL;
    }
    INT_INFO("Exiting ...");
}

void dsLoadConfigs(void)
{
    INT_INFO("Enter function");
    loadDeviceCapabilities(DEVICE_CAPABILITY_VIDEO_PORT | 
                          DEVICE_CAPABILITY_AUDIO_PORT |
                          DEVICE_CAPABILITY_VIDEO_DEVICE);
    INT_INFO("Exit function");
}

/** @} */
/** @} */
