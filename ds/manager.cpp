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
#include <functional>
#include <dlfcn.h>
#include "dsHALConfig.h"
#include "frontPanelConfig.hpp"
#include "dsMgr.h"       /* IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_RESTARTED */
#include "libIBus.h"     /* IARM_Bus_RegisterEventHandler / UnRegisterEventHandler */
#include <thread>           /* std::thread for deferred handle refresh */
#include <chrono>           /* std::chrono::milliseconds */

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
static dsError_t initializeFunctionWithRetry(const char* functionName, std::function<dsError_t()> initFunc);

/**
 * @brief IARM_BUS_DSMGR_EVENT_RESTARTED handler registered by Manager::Initialize().
 *
 * Fired by dsmgr after it has fully re-initialised its HAL — i.e. after
 * dsAudioPortInit / dsVideoPortInit / dsVideoDeviceInit have all completed
 * in the new dsmgr process.
 *
 * All three libds config singletons hold stale intptr_t handles that point
 * into the address space of the *previous* dsmgr process. This handler
 * refreshes them unconditionally so that subsequent API calls use handles
 * valid in the new dsmgr process.
 *
 * Registered in Manager::Initialize() and unregistered in
 * Manager::DeInitialize() so its lifetime exactly matches the Manager's.
 */
static void dsMgrRestartedHandler(const char* owner, IARM_EventId_t eventId,
                                   void* /*data*/, size_t /*len*/)
{
    INT_INFO("[Manager] IARM_BUS_DSMGR_EVENT_RESTARTED received owner=%s eventId=%d",
             owner, eventId);

    if (std::string(IARM_BUS_DSMGR_NAME) != std::string(owner)) {
        INT_ERROR("unexpected owner '%s', ignoring", owner);
        return;
    }

    std::lock_guard<std::mutex> lock(gManagerInitMutex);
    if (Manager::IsInitialized == 0) {
        /* Manager has been torn down; ignore the event */
        INT_WARN("Manager not initialized, skipping refresh");
        return;
    }

    INT_INFO("Refreshing ALL libds handles (deferred — spawning retry thread)");

    /*
     * IMPORTANT: Do NOT call refreshAllHandles() synchronously here.
     *
     * This handler is dispatched while dsmgr is still inside
     * IARM_Bus_BroadcastEvent(). Any IARM_Bus_Call() back into DSMgr
     * from this context hits the RPC dispatcher while it is busy and
     * returns IARM_RESULT_IPCCORE_FAIL — leaving every handle stale.
     *
     * Fix: spawn a detached thread that sleeps before calling back into
     * dsmgr (letting BroadcastEvent return) and retries up to 3 times
     * with escalating delays covering ~3× the observed 600 ms restart time.
     */
    std::thread([]() {
        /* Delays tuned to the ~600 ms restart time observed on device.
         * RESTARTED fires at end of DSMgr_Start() so dsmgr is already
         * RPC-ready; only BroadcastEvent dispatch (~2 ms) needs to clear.
         *   200 ms → fast path  |  600 ms → full restart  |  1200 ms → 2×
         * Total window: 2.0 s. Tune DELAYS_MS[] to device RestartSec= if needed. */
        static const int DELAYS_MS[] = { 200, 600, 1200 };
        static const int MAX_ATTEMPTS = static_cast<int>(
                             sizeof(DELAYS_MS) / sizeof(DELAYS_MS[0]));

        int cumulativeWaitMs = 0;
        for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
            const int delayMs = DELAYS_MS[attempt - 1];
            cumulativeWaitMs += delayMs;

            /* Sleep so BroadcastEvent has returned before we call back. */
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

            {
                std::lock_guard<std::mutex> lk(gManagerInitMutex);
                if (Manager::IsInitialized == 0) {
                    INT_WARN("[refreshThread] Manager deinitialized, aborting");
                    return;
                }
            }

            INT_INFO("[refreshThread] attempt %d/%d (after %d ms delay)",
                     attempt, MAX_ATTEMPTS, delayMs);

            dsError_t audioRet    = AudioOutputPortConfig::getInstance().refreshAllHandles();
            dsError_t videoPortRet = static_cast<dsError_t>(
                                        VideoOutputPortConfig::getInstance().refreshAllHandles());
            dsError_t videoDevRet  = static_cast<dsError_t>(
                                        VideoDeviceConfig::getInstance().refreshAllHandles());

            INT_INFO("[refreshThread] attempt %d audioRet=%d videoPortRet=%d videoDevRet=%d",
                     attempt, audioRet, videoPortRet, videoDevRet);

            if (audioRet == dsERR_NONE && videoPortRet == dsERR_NONE && videoDevRet == dsERR_NONE) {
                INT_INFO("[refreshThread] ALL handles refreshed OK on attempt %d "
                         "(total wait %d ms)", attempt, cumulativeWaitMs);
                return;
            }

            if (attempt < MAX_ATTEMPTS) {
                INT_WARN("[refreshThread] attempt %d failed (a=%d vp=%d vd=%d), "
                         "retrying in %d ms",
                         attempt, audioRet, videoPortRet, videoDevRet,
                         DELAYS_MS[attempt]);   /* next delay */
            }
        }

        /* Last-resort: all refreshAllHandles() attempts failed after ~2 s.
         * Do a full DeInitialize/Initialize to rebuild all libds singletons
         * and HAL bindings from scratch against the running dsmgr.
         * IsInitialized is a ref-count — force it to 1 so one DeInitialize()
         * reaches 0 (triggers real teardown) and one Initialize() reaches 1
         * (triggers real re-init); restore original count afterwards. */
        INT_ERROR("[refreshThread] FAILED to refresh handles after %d attempts "
                  "(~2 s window exhausted) — initiating full DeInitialize/Initialize",
                  MAX_ATTEMPTS);

        int savedRefCount = 0;
        {
            std::lock_guard<std::mutex> lk(gManagerInitMutex);
            savedRefCount = Manager::IsInitialized;
        }

        if (savedRefCount <= 0) {
            INT_WARN("[refreshThread] IsInitialized=%d — skipping force re-init",
                     savedRefCount);
            return;
        }

        INT_INFO("[refreshThread] force re-init: savedRefCount=%d", savedRefCount);

        {
            std::lock_guard<std::mutex> lk(gManagerInitMutex);
            Manager::IsInitialized = 1;   /* arm: next DeInitialize() will reach 0 */
        }

        INT_INFO("[refreshThread] calling DeInitialize (will trigger real teardown)");
        Manager::DeInitialize();   /* count 1 → 0 → dsAudioPortTerm + release() */

        INT_INFO("[refreshThread] calling Initialize (will trigger real HAL re-init)");
        try {
            Manager::Initialize(); /* count 0 → 1 → dsAudioPortInit + load() + handler re-reg */
        }
        catch (const std::exception& ex) {
            INT_ERROR("[refreshThread] Initialize threw: %s — aborting", ex.what());
            return;
        }

        /* Restore count so callers still have a matching DeInitialize(). */
        {
            std::lock_guard<std::mutex> lk(gManagerInitMutex);
            Manager::IsInitialized = savedRefCount;
        }

        INT_INFO("[refreshThread] force re-init complete — IsInitialized restored to %d",
                 savedRefCount);
    }).detach();
}

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
    
    if (nullptr == pDLHandle) {
        const char* dlError = dlerror();
        INT_WARN("Failed to dlopen '%s': %s - Falling back to static configurations", 
                 RDK_DSHAL_NAME, dlError ? dlError : "Unknown error");
    } else {
        INT_INFO("Successfully opened dynamic library '%s'", RDK_DSHAL_NAME);
    }
    
    INT_INFO("DL Instance '%s'", (nullptr == pDLHandle ? "NULL" : "Valid"));

    // initialize the frame rates map with supported frame rates. This is required to be done before loading the video port configs as some of the frame rate related APIs depend on this map to return the supported frame rates and its values.
    initializeFrameRates();

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
 * @brief Retry initialization function with configurable retry logic.
 * 
 * This helper function attempts to initialize a device settings component by calling
 * the provided initialization function. It retries the operation with a delay between
 * attempts until either the operation succeeds, the maximum retry count is reached,
 * or (optionally) a specific error state is encountered.
 *
 * @param[in] functionName Name of the initialization function being called. Used for logging
 *                         purposes to identify which component is being initialized.
 * @param[in] initFunc Lambda or function object that performs the actual initialization.
 *                     Should return dsError_t indicating success (dsERR_NONE) or an error code.
 *
 * @return dsERR_NONE on successful initialization, or the last error code encountered after
 *         all retry attempts are exhausted. When checkInvalidState is true, also returns
 *         immediately with the error code if a non-dsERR_INVALID_STATE error occurs.
 */
dsError_t initializeFunctionWithRetry(const char* functionName, 
                                   std::function<dsError_t()> initFunc) 
{
	dsError_t err = dsERR_GENERAL;
	unsigned int retryCount = 0;
	unsigned int maxRetries = 25;

	do {
		err = initFunc();
		printf("Manager::Initialize:%s result :%d retryCount :%d\n", 
				functionName, err, retryCount);
		if (dsERR_NONE == err) break;
		usleep(100000);
	} while (retryCount++ < maxRetries);
	
	return err;
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
    bool needInit = false;

    {std::lock_guard<std::mutex> lock(gManagerInitMutex);
        printf("Entering %s count %d with thread id %lu\n",__FUNCTION__,IsInitialized,pthread_self());
        if (IsInitialized == 0) {
            needInit = true;
        }
        IsInitialized++;
	}
	
    try {
        if (needInit) {
            dsError_t err = dsERR_GENERAL;

            err = initializeFunctionWithRetry("dsDisplayInit", dsDisplayInit);
            CHECK_RET_VAL(err);
            
            err = initializeFunctionWithRetry("dsAudioPortInit", dsAudioPortInit);
            CHECK_RET_VAL(err);
            
            err = initializeFunctionWithRetry("dsVideoPortInit", dsVideoPortInit);
            CHECK_RET_VAL(err);
            
            err = initializeFunctionWithRetry("dsVideoDeviceInit", dsVideoDeviceInit);
            CHECK_RET_VAL(err);
            
            loadDeviceCapabilities(device::DEVICE_CAPABILITY_VIDEO_PORT |
                                    device::DEVICE_CAPABILITY_AUDIO_PORT |
                                    device::DEVICE_CAPABILITY_VIDEO_DEVICE |
                                    device::DEVICE_CAPABILITY_FRONT_PANEL);

            /* Register the dsmgr-restart handler so that libds handles are
             * refreshed automatically whenever dsmgr crashes and restarts.
             * Registered here (after all HAL inits) so handles are valid
             * before the first event can arrive.  Unregistered in
             * DeInitialize() to match the Manager lifetime exactly. */
            IARM_Result_t iarmRet = IARM_Bus_RegisterEventHandler(
                IARM_BUS_DSMGR_NAME,
                IARM_BUS_DSMGR_EVENT_RESTARTED,
                dsMgrRestartedHandler);
            if (IARM_RESULT_SUCCESS != iarmRet) {
                INT_ERROR("[Manager] Failed to register IARM_BUS_DSMGR_EVENT_RESTARTED handler (ret=%d)",
                          iarmRet);
            } else {
                INT_INFO("[Manager] IARM_BUS_DSMGR_EVENT_RESTARTED handler registered OK");
            }
        }
    }
    catch(const Exception &e) {
        cout << "Caught exception during Initialization" << e.what() << endl;
		std::lock_guard<std::mutex> lock(gManagerInitMutex);
        IsInitialized--;
        throw e;
    }

	INT_INFO("Exiting ... with thread id %lu",pthread_self());
}

void Manager::load()
{
    INT_INFO("Enter function");
	loadDeviceCapabilities( device::DEVICE_CAPABILITY_VIDEO_PORT | 
                            device::DEVICE_CAPABILITY_AUDIO_PORT |
                            device::DEVICE_CAPABILITY_VIDEO_DEVICE);
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

            // deinitialize the frame rates map to release the resources allocated for the map and its contents, as some of the frame rate APIs depend on this map to return the supported frame rates and its values.
            deinitializeFrameRates();

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
