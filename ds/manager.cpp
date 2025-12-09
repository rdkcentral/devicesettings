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
#include "dsHALConfig.h"


static pthread_mutex_t dsLock = PTHREAD_MUTEX_INITIALIZER;


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
	printf("Entering %s count %d with thread id %lu\n",__FUNCTION__,IsInitialized,pthread_self());
	
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
	    		printf ("Manager::Initialize: result :%d retryCount :%d\n", err, retryCount);
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
			std::lock_guard<std::mutex> lock(mutex);
	    	AudioOutputPortConfig::getInstance().load();
	    	VideoOutputPortConfig::getInstance().load();
	    	VideoDeviceConfig::getInstance().load();
	    }
        IsInitialized++;
    }
    catch(const Exception &e) {
		cout << "Caught exception during Initialization" << e.what() << endl;
		throw e;
	}
	}
	printf("Exiting %s with thread %lu\n",__FUNCTION__,pthread_self());
}

void Manager::load()
{
	std::lock_guard<std::mutex> lock(mutex);
	printf("%d:%s load start\n", __LINE__, __FUNCTION__);
	device::AudioOutputPortConfig::getInstance().load();
	device::VideoOutputPortConfig::getInstance().load();
	device::VideoDeviceConfig::getInstance().load();
	printf("%d:%s load completed\n", __LINE__, __FUNCTION__);
}

bool searchConfigs(const char *searchConfigStr, void **pConfigVar)
{
	INT_INFO("%d:%s: Entering function\n", __LINE__, __func__);
	INT_INFO("%d:%s: searchConfigStr = %s\n", __LINE__, __func__, searchConfigStr);

	pthread_mutex_lock(&dsLock);

		void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
		if (dllib) {
			*pConfigVar = (void *) dlsym(dllib, searchConfigStr);
			if (*pConfigVar != NULL) {
				INT_INFO("%s is defined and loaded  pConfigVar= %p\r\n", searchConfigStr, *pConfigVar);
			}
			else {
				INT_ERROR("%d:%s: %s is not defined\n", __LINE__, __func__, searchConfigStr);
			}

			dlclose(dllib);
		}
		else {
			INT_ERROR("%d:%s: Open %s failed\n", __LINE__, __func__, RDK_DSHAL_NAME);
		}
	pthread_mutex_unlock(&dsLock);
	INT_INFO("%d:%s: Exit function\n", __LINE__, __func__);
	return (*pConfigVar != NULL);
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
	printf("Entering %s count %d with thread id: %lu\n",__FUNCTION__,IsInitialized,pthread_self());
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
	printf("Exiting %s with thread %lu\n",__FUNCTION__,pthread_self());

}

}

/** @} */

/** @} */
/** @} */
