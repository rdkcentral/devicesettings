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
#include <condition_variable>
#include "dsHALConfig.h"


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
static std::mutex gSearchMutex;
static void *dllib = NULL;
int activeLoads = 0;            // reference counter
static std::mutex gMtx;
std::condition_variable cv;
const int REQUIRED = 4;       // wait for 4 loads to finish

Manager::Manager() {
	// TODO Auto-generated constructor stub

}

Manager::~Manager() {
	// TODO Auto-generated destructor stub
	    // Ensure close if not already
    if (dllib) {
        dlclose(dllib);
        INT_INFO("Destructor dlclose\n");
    }
}

#define CHECK_RET_VAL(ret) {\
	if (ret != dsERR_NONE) {\
		cout << "**********Failed to Initialize device port*********" << endl;\
		throw Exception(ret, "Error Initialising platform ports");\
	}\
	}

std::string parse_opt_flag( std::string file_name , bool integer_check , bool debugStats )
{
    std::string return_buffer = "";
    std::ifstream parse_opt_flag_file( file_name.c_str());

    if (!parse_opt_flag_file)
    {
        if ( debugStats ){
            INT_INFO("Failed to open [%s] file",file_name.c_str());
        }
    }
    else
    {
        std::string line = "";
        if (std::getline(parse_opt_flag_file, line))
        {
            if ( debugStats ){
                INT_INFO("Content in [%s] is [%s]",file_name.c_str(),line.c_str());
            }
        }
        else
        {
            if ( debugStats ){
                INT_INFO("No Content in [%s]",file_name.c_str());
            }
        }
        parse_opt_flag_file.close();

        return_buffer = line;

        if (integer_check)
        {
            if (line.empty())
            {
                integer_check = false;
            }
            else
            {
                for (char c : line) {
                    if (!isdigit(c))
                    {
                        integer_check = false;
                        break;
                    }
                }
            }

            if ( false == integer_check )
            {
                return_buffer = "";
            }
        }
    }
    return return_buffer;
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
	
	int delay = 1;	
	bool ret = false;
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
			#if 0
			std::string delaystr = parse_opt_flag("/opt/delay", true, true);
            if (!delaystr.empty())
            {
                INT_INFO("dealy: [%s]", delaystr.c_str());
                delay = std::stoi(delaystr);
			}
			#endif
			INT_INFO("Open the hal file\n");
	    	ret = openDLFile();
			if(ret == true)
			{
				INT_INFO("File opened successfully\n");
			}
			else
			{
				INT_ERROR("Failed to open the hal file\n");
			}
			AudioOutputPortConfig::getInstance().load();
			//sleep(delay);
	    	VideoOutputPortConfig::getInstance().load();
	    	//sleep(delay);
			VideoDeviceConfig::getInstance().load();
			INT_INFO("disabled waitAndClose()\n");
			//waitAndClose();
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
	printf("%d:%s load start\n", __LINE__, __FUNCTION__);
	device::AudioOutputPortConfig::getInstance().load();
	device::VideoOutputPortConfig::getInstance().load();
	device::VideoDeviceConfig::getInstance().load();
	printf("%d:%s load completed\n", __LINE__, __FUNCTION__);
}

bool openDLFile()
{
	bool ret = false;

	INT_INFO("%d:%s: Entering function\n", __LINE__, __func__);
	INT_INFO("%d:%s: RDK_DSHAL_NAME = %s\n", __LINE__, __func__, RDK_DSHAL_NAME);

	dlerror(); // clear old error
	dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
	if (dllib) {
		INT_INFO("open %s success", RDK_DSHAL_NAME);
		ret = true;
	}
	else {
		const char* err = dlerror();
		INT_ERROR("%d:%s: Open %s failed with error err= %s\n", __LINE__, __func__, RDK_DSHAL_NAME, err ? err: "unknown");
		ret = false;
	}
	INT_INFO("%d:%s: Exiting function\n", __LINE__, __func__);
	return ret;
}

#if 0
void EE::requestClose() {
    std::lock_guard<std::mutex> lock(m);
    closeRequested = true;
    if (active == 0 && handle) {
        dlclose(handle);
        handle = nullptr;
    }
}
void startLoad() 
{
    std::lock_guard<std::mutex> lock(gMtx);
	INT_INFO("%d:%s: Entering function\n", __LINE__, __func__);
    activeLoads++;
	INT_INFO("%d:%s: Exit function. activeLoads = %d\n", __LINE__, __func__, activeLoads);

}


void finishLoad() 
{
    std::lock_guard<std::mutex> lock(gMtx);
	INT_INFO("%d:%s: Entering function\n", __LINE__, __func__);

    activeLoads++;
	INT_INFO("%d:%s: activeLoads = %d\n", __LINE__, __func__, activeLoads);
    if (activeLoads == 0) {
		if (dllib) {
			dlclose(dllib);
			INT_INFO("%d:%s: Closed the hal file\n", __LINE__, __func__);
			dllib = NULL;
		}
		else {
			INT_ERROR("dllib is NULL\n");
		}
	}
	INT_INFO("%d:%s: Exiting function\n", __LINE__, __func__);
}
#endif

bool searchConfigs(const char *searchConfigStr, void **pConfigVar)
{
	INT_INFO("%d:%s: Entering function\n", __LINE__, __func__);
	INT_INFO("%d:%s: searchConfigStr = %s\n", __LINE__, __func__, searchConfigStr);
	INT_INFO("%d:%s: RDK_DSHAL_NAME = %s\n", __LINE__, __func__, RDK_DSHAL_NAME);

	std::lock_guard<std::mutex> lock(gSearchMutex);
	INT_INFO("%d:%s: using lock_guard() instead pthread_mutex_lock \n", __LINE__, __func__);
	if (dllib) {
		*pConfigVar = (void *) dlsym(dllib, searchConfigStr);
		if (*pConfigVar != NULL) {
			INT_INFO("%s is defined and loaded  pConfigVar= %p\r\n", searchConfigStr, *pConfigVar);
		}
		else {
			INT_ERROR("%d:%s: %s is not defined\n", __LINE__, __func__, searchConfigStr);
		}
	}
	else {
		INT_ERROR("dllib is NULL\n");
	}
	INT_INFO("%d:%s: Exit function\n", __LINE__, __func__);
	return (*pConfigVar != NULL);
}

void notifyLoadComplete() {
	INT_INFO("%d:%s: Entering function\n", __LINE__, __func__);
    std::unique_lock<std::mutex> lock(gMtx);

    activeLoads++;
    // Wake waiting thread
    cv.notify_all();
    INT_INFO("%d:%s: Exit function\n", __LINE__, __func__);
}

void waitAndClose() {
    std::unique_lock<std::mutex> lock(gMtx);
    INT_INFO("%d:%s: Entering function activeLoads = %d\n", __LINE__, __func__, activeLoads);

    cv.wait(lock, [&] { return activeLoads >= REQUIRED; }); // wait for 4 loads complete

    if (dllib) {
        dlclose(dllib);
        dllib = nullptr;
    }
    INT_INFO("%d:%s: Exit function\n", __LINE__, __func__);
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
