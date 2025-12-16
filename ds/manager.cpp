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


#include "manager.hpp"
#include <iostream>
#include "audioOutputPortConfig.hpp"
#include "videoOutputPortConfig.hpp"
#include "host.hpp"
#include "videoDeviceConfig.hpp"
#include "dsVideoPort.h"
#include "dsVideoDevice.h"
#include "dsAudio.h"
#include "dsDisplay.h"
#include "dsFPD.h"
#include "dslogger.h"
#include <mutex>
#include "exception.hpp"
#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fstream>
#include "dsHALConfig.h"
#include "frontPanelConfig.hpp"

//static pthread_mutex_t dsLock = PTHREAD_MUTEX_INITIALIZER;


/**
 * @file manager.cpp
 * @brief RDK Device Settings module is a cross-platform device for controlling the following hardware configurations:
 *  Audio Output Ports (Volume, Mute, etc.)
 *  Video Ouptut Ports (Resolutions, Aspect Ratio, etc.)
 *  Front Panel Indicators
 *	DFC[zoom] Settings
 *	Display (Aspect Ratio, EDID data etc.)
 *	General Host configuration (Power managements, event management etc.)
 *
 */

using namespace std;

namespace device {

int Manager::IsInitialized = 0;   //!< Indicates the application has initialized with devicettings modules.
static std::mutex gManagerInitMutex;

bool LoadDLSymbols(void* pDLHandle, const dlSymbolLookup* symbols, int numberOfSymbols)
{
    int currentSymbols = 0;
    bool isAllSymbolsLoaded = false;
    if ((nullptr == pDLHandle) || (nullptr == symbols)) {
        INT_ERROR("Invalid DL Handle or symbolsPtr");
    }
    else {
        INT_INFO("numberOfSymbols = %d",numberOfSymbols);
        for (int i = 0; i < numberOfSymbols; i++) {
            if (( nullptr == symbols[i].dataptr) || ( nullptr == symbols[i].name)) {
                INT_ERROR("Invalid symbol entry at index [%d]", i);
                continue;
            }
            *(symbols[i].dataptr) = dlsym(pDLHandle, symbols[i].name);
            if (nullptr == *(symbols[i].dataptr)) {
                INT_ERROR("[%s] is not defined", symbols[i].name);
            }
            else {
                currentSymbols++;
                INT_INFO("[%s] is defined and loaded, data[%p]", symbols[i].name, *(symbols[i].dataptr));
            }
        }
        isAllSymbolsLoaded = (numberOfSymbols) ? (currentSymbols == numberOfSymbols) : false;
    }
    return isAllSymbolsLoaded;
}

void loadDeviceCapabilities(unsigned int capabilityType)
{
    void* pDLHandle = nullptr;
    bool isSymbolsLoaded = false;

    INT_INFO("Entering capabilityType = 0x%08X", capabilityType);
    dlerror(); // clear old error
    pDLHandle = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
    INT_INFO("DL Instance '%s'", (nullptr == pDLHandle ? "NULL" : "Valid"));

    // Audio Port Config
    if (DEVICE_CAPABILITY_AUDIO_PORT & capabilityType) {
        audioConfigs_t dynamicAudioConfigs = {0 };
        dlSymbolLookup audioConfigSymbols[] = {
            {"kAudioConfigs", (void**)&dynamicAudioConfigs.pKConfigs},
            {"kAudioPorts", (void**)&dynamicAudioConfigs.pKPorts},
            {"kAudioConfigs_size", (void**)&dynamicAudioConfigs.pKConfigSize},
            {"kAudioPorts_size", (void**)&dynamicAudioConfigs.pKPortSize}
        };

        isSymbolsLoaded = false;
        if (nullptr != pDLHandle) {
            isSymbolsLoaded = LoadDLSymbols(pDLHandle, audioConfigSymbols, sizeof(audioConfigSymbols)/sizeof(dlSymbolLookup));
        }
        AudioOutputPortConfig::getInstance().load(isSymbolsLoaded ? &dynamicAudioConfigs : nullptr);
    }

    // Video Port Config
    if (DEVICE_CAPABILITY_VIDEO_PORT & capabilityType) {
        videoPortConfigs_t dynamicVideoPortConfigs = {0};
        dlSymbolLookup videoPortConfigSymbols[] = {
            {"kVideoPortConfigs", (void**)&dynamicVideoPortConfigs.pKConfigs},
            {"kVideoPortConfigs_size", (void**)&dynamicVideoPortConfigs.pKVideoPortConfigs_size},
            {"kVideoPortPorts", (void**)&dynamicVideoPortConfigs.pKPorts},
            {"kVideoPortPorts_size", (void**)&dynamicVideoPortConfigs.pKVideoPortPorts_size},
            {"kResolutionsSettings", (void**)&dynamicVideoPortConfigs.pKResolutionsSettings},
            {"kResolutionsSettings_size", (void**)&dynamicVideoPortConfigs.pKResolutionsSettings_size}
        };

        isSymbolsLoaded = false;
        if (nullptr != pDLHandle) {
            isSymbolsLoaded = LoadDLSymbols(pDLHandle, videoPortConfigSymbols, sizeof(videoPortConfigSymbols)/sizeof(dlSymbolLookup));
        }
        VideoOutputPortConfig::getInstance().load(isSymbolsLoaded ? &dynamicVideoPortConfigs : nullptr);
    }

    // Video Device Config
    if (DEVICE_CAPABILITY_VIDEO_DEVICE & capabilityType) {
        videoDeviceConfig_t dynamicVideoDeviceConfigs = {0};
        dlSymbolLookup videoDeviceConfigSymbols[] = {
            {"kVideoDeviceConfigs", (void**)&dynamicVideoDeviceConfigs.pKVideoDeviceConfigs},
            {"kVideoDeviceConfigs_size", (void**)&dynamicVideoDeviceConfigs.pKVideoDeviceConfigs_size}
        };
        isSymbolsLoaded = false;
        if (nullptr != pDLHandle) {
            isSymbolsLoaded = LoadDLSymbols(pDLHandle, videoDeviceConfigSymbols, sizeof(videoDeviceConfigSymbols)/sizeof(dlSymbolLookup));
        }
        VideoDeviceConfig::getInstance().load(isSymbolsLoaded ? &dynamicVideoDeviceConfigs : nullptr);
    }

    // Front Panel Config
    if (DEVICE_CAPABILITY_FRONT_PANEL & capabilityType) {
        fpdConfigs_t dynamicFPDConfigs = {0};
        dlSymbolLookup fpdConfigSymbols[] = {
            {"kFPDIndicatorColors", (void**)&dynamicFPDConfigs.pKFPDIndicatorColors},
            {"kFPDIndicatorColors_size", (void**)&dynamicFPDConfigs.pKFPDIndicatorColors_size},
            {"kIndicators", (void**)&dynamicFPDConfigs.pKIndicators},
            {"kIndicators_size", (void**)&dynamicFPDConfigs.pKIndicators_size},
            {"kFPDTextDisplays", (void**)&dynamicFPDConfigs.pKTextDisplays},
            {"kFPDTextDisplays_size", (void**)&dynamicFPDConfigs.pKTextDisplays_size}
        };
        isSymbolsLoaded = false;
        if (nullptr != pDLHandle) {
            isSymbolsLoaded = LoadDLSymbols(pDLHandle, fpdConfigSymbols, sizeof(fpdConfigSymbols)/sizeof(dlSymbolLookup));
        }
        FrontPanelConfig::getInstance().load(isSymbolsLoaded ? &dynamicFPDConfigs : nullptr);
    }

    if (nullptr != pDLHandle) {
        dlclose(pDLHandle);
        pDLHandle = nullptr;
    }
    INT_INFO("Exiting ...");
}

Manager::Manager() {
	// TODO Auto-generated constructor stub

}

Manager::~Manager() {
	// TODO Auto-generated destructor stub
}

#define CHECK_RET_VAL(ret) {\
	if (ret != dsERR_NONE) {\
		cout << "**********Failed to Initialize device port*********" << endl;\
		throw Exception(ret, "Error Initialising platform ports");\
	}\
	}

/**
 * @addtogroup dssettingsmanagerapi
 * @{
 */
/**
 * @fn Manager::Initialize()
 * @brief This API is used to initialize the Device Setting module.
 * Each API should be called by any client of device settings before it start device settings service.
 * <ul>
 * <li> dsDisplayInit() function must initialize all underlying the video display modules and associated data structures.
 * <li> dsAudioPortInit() function must be used to initialize all the audio output ports.
 * Must return NOT_SUPPORTED when no audio port present in the device (ex., Gateway)
 * <li> dsVideoPortInit() function must initialize all the video specific output ports.
 * <li> dsVideoDeviceInit() function must initialize all the video devices that exists in the system.
 * <li> AudioOutputPortConfig::getInstance().load() function will load constants first and initialize Audio portTypes (encodings, compressions etc.)
 * and its port instances (db, level etc).
 * <li> VideoOutputPortConfig::getInstance().load() function will load constants first and initialize a set of supported resolutions.
 * Initialize Video portTypes (Only Enable Ports) and its port instances (current resolution)
 * <li> VideoDeviceConfig::getInstance().load() function will load constants first and initialize Video Devices (supported DFCs etc.).
 * </ul>
 * IllegalStateException will be thrown by the APIs if the module is not yet initialized.
 *
 * @return None
 */
void Manager::Initialize()
{
	{std::lock_guard<std::mutex> lock(gManagerInitMutex);
	
	INT_INFO("Entering ... count %d with thread id %lu\n",IsInitialized,pthread_self());
	
	try {
	    if (0 == IsInitialized) {	
        
	    	dsError_t err = dsERR_GENERAL;
	    	unsigned int retryCount = 0;
	    	// This retry logic will wait for the device manager initialization from the client side
	    	// until the device manager service initialization is completed. The retry mechanism checks
	    	// only for dsERR_INVALID_STATE, which is reported if the underlying service is not ready.
	    	// Once the service is ready, other port initializations can be called directly without any delay.
	    	// That's why the retry logic is applied only for dsDisplayInit.
	    	do {
	    		err = dsDisplayInit();
	    		INT_INFO("dsDisplayInit returned %d, retryCount %d", err, retryCount);
	    		if (dsERR_NONE == err) break;
	    		usleep(100000);
	    	} while(( dsERR_INVALID_STATE == err) && (retryCount++ < 25));
            CHECK_RET_VAL(err);
	    	err = dsAudioPortInit();
            CHECK_RET_VAL(err);
	    	err = dsVideoPortInit();
            CHECK_RET_VAL(err);
	    	err = dsVideoDeviceInit();
	    	CHECK_RET_VAL(err);

            loadDeviceCapabilities(device::DEVICE_CAPABILITY_VIDEO_PORT |
                                    device::DEVICE_CAPABILITY_AUDIO_PORT |
                                    device::DEVICE_CAPABILITY_VIDEO_DEVICE |
                                    device::DEVICE_CAPABILITY_FRONT_PANEL);
	    }
        IsInitialized++;
    }
    catch(const Exception &e) {
		cout << "Caught exception during Initialization" << e.what() << endl;
		throw e;
	}
	}
	INT_INFO("Exiting ... with thread id %lu",pthread_self());
}

void Manager::load()
{
    INT_INFO("Enter function");
	loadDeviceCapabilities( device::DEVICE_CAPABILITY_VIDEO_PORT | 
                            device::DEVICE_CAPABILITY_AUDIO_PORT |
                            device::DEVICE_CAPABILITY_VIDEO_DEVICE |
                            device::DEVICE_CAPABILITY_FRONT_PANEL);
    INT_INFO("Exit function");
}

/**
 * @fn void Manager::DeInitialize()
 * @brief This API is used to deinitialize the device settings module.
 * DeInitialize() must be called to release all resource used by the Device Setting module.
 * After DeInitialize(), the device will return to a state as if the Initialize() was never called.
 * <ul>
 * <li> dsVideoDeviceTerm() function will reset the data structures used within this module and release the handles specific to video devices.
 * <li> dsVideoPortTerm() function will reset the data structures used within this module and release the video port specific handles.
 * <li> dsAudioPortTerm() function will terminate the usage of audio output ports by resetting the data structures used within this module
 * and release the audio port specific handles.
 * <li> dsDisplayTerm() function will reset the data structures used within this module and release the video display specific handles.
 * <li> VideoDeviceConfig::getInstance().release() function releases the DFCs and video device instance.
 * <li> VideoOutputPortConfig::getInstance().release() function clears the instance of video pixel resolution, aspect ratio, stereoscopic mode, frameRate,
 * supported Resolution and video port types.
 * <li> AudioOutputPortConfig::getInstance().release() function clears the instance of audio encoding, compression, stereo modes and audio port types.
 * </ul>
 *
 * @return None
 */
void Manager::DeInitialize()
{
	{std::lock_guard<std::mutex> lock(gManagerInitMutex);
	INT_INFO("Entering ... count %d with thread id %lu",IsInitialized,pthread_self());
	if(IsInitialized>0)IsInitialized--;
	if (0 == IsInitialized) {	
	
		VideoDeviceConfig::getInstance().release();
		VideoOutputPortConfig::getInstance().release();
		AudioOutputPortConfig::getInstance().release();

		dsVideoDeviceTerm();
		dsVideoPortTerm();
		dsAudioPortTerm();
		dsDisplayTerm();
	}
	}
	INT_INFO("Exiting ... with thread %lu",pthread_self());
}

}

/** @} */

/** @} */
/** @} */
