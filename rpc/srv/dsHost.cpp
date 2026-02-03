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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <iostream>
#include "hostPersistence.hpp"
#include <sstream>
#include "iarmUtil.h"
#include "libIARM.h"
#include "libIBus.h"
#include "dsRpc.h"
#include "dsError.h"
#include "dsTypes.h"
#include "dsHost.h"
#include "dsserverlogger.h"
#include <dlfcn.h>
#include "dsInternal.h"

#include "safec_lib.h"
#include "dsMgr.h"

static int m_isInitialized = 0;
static pthread_mutex_t hostLock = PTHREAD_MUTEX_INITIALIZER;

#define IARM_BUS_Lock(lock) pthread_mutex_lock(&hostLock)
#define IARM_BUS_Unlock(lock) pthread_mutex_unlock(&hostLock)


IARM_Result_t _dsGetPreferredSleepMode(void *arg);
IARM_Result_t _dsSetPreferredSleepMode(void *arg);
IARM_Result_t _dsGetCPUTemperature(void *arg);
IARM_Result_t _dsGetVersion(void *arg);
IARM_Result_t _dsGetSocIDFromSDK(void *arg);
IARM_Result_t _dsGetHostEDID(void *arg);
IARM_Result_t _dsGetMS12ConfigType(void *arg);

static dsSleepMode_t _SleepMode = dsHOST_SLEEP_MODE_LIGHT;

using namespace std;

static string enumToString( dsSleepMode_t mode );
static dsSleepMode_t stringToEnum ( string mode );
#define DSHAL_API_VERSION_MAJOR_DEFAULT     1 
#define DSHAL_API_VERSION_MINOR_DEFAULT     0


IARM_Result_t dsHostMgr_init()
{
  
   try{
        string mode="LIGHT_SLEEP";
        mode = device::HostPersistence::getInstance().getProperty("Power.Mode",mode);
        INT_INFO("Sleep mode Persistent value is -> %s\n",mode.c_str());
        _SleepMode = stringToEnum(std::move(mode));        
        IARM_Bus_DSMgr_EventData_t eventData = {0};
        eventData.data.sleepModeInfo.sleepMode = _SleepMode;
        IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)IARM_BUS_DSMGR_EVENT_SLEEP_MODE_CHANGED,(void *)&eventData, sizeof(eventData));
        INT_INFO("Mani %d %s: Exit - Broadcast Event IARM_BUS_DSMGR_EVENT_SLEEP_MODE_CHANGED :%d. \n",__LINE__, __FUNCTION__, _SleepMode);
    }
    catch(...)
    {
        INT_INFO("Error in Reading the Power Mode Persisent \r\n");
    }

    if (!m_isInitialized) {
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetPreferredSleepMode,_dsGetPreferredSleepMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetPreferredSleepMode,_dsSetPreferredSleepMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetCPUTemperature,_dsGetCPUTemperature);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetVersion,_dsGetVersion);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetSocIDFromSDK,_dsGetSocIDFromSDK);
	IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetHostEDID,_dsGetHostEDID);
	IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetMS12ConfigType,_dsGetMS12ConfigType);

        
        uint32_t  halVersion = 0x10000;
        halVersion =  dsHAL_APIVER(DSHAL_API_VERSION_MAJOR_DEFAULT, DSHAL_API_VERSION_MINOR_DEFAULT);
        INT_INFO("DS HAL Version is - %d.%d \r\n",dsHAL_APIVER_MAJOR(halVersion),dsHAL_APIVER_MINOR(halVersion));
      
        m_isInitialized = 1;
    }
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t dsHostMgr_term()
{
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsSetPreferredSleepMode(void *arg)
{
    IARM_Result_t ret =  IARM_RESULT_SUCCESS;
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    INT_INFO("Mani %d %s: Entry - _dsSetPreferredSleepMode \n",__LINE__, __FUNCTION__);

    dsPreferredSleepMode *param = (dsPreferredSleepMode *)arg;
    INT_DEBUG("_dsSetPreferredSleepMode called with the mode - %s \r\n", enumToString(param->mode).c_str());
    try{
        
        device::HostPersistence::getInstance().persistHostProperty("Power.Mode",enumToString(param->mode));
        _SleepMode  = param->mode;
        IARM_Bus_DSMgr_EventData_t eventData;
        eventData.data.sleepModeInfo.sleepMode = _SleepMode;
        IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)IARM_BUS_DSMGR_EVENT_SLEEP_MODE_CHANGED,(void *)&eventData, sizeof(eventData));
        INT_INFO("Mani %d %s: Exit - callaing IARM_BUS_DSMGR_EVENT_SLEEP_MODE_CHANGED :%d \n",__LINE__, __FUNCTION__, _SleepMode);
        ret = IARM_RESULT_SUCCESS;
    }
    catch(...)
    {
        INT_INFO("Error in Persisting the Power Mode\r\n");
    }
    IARM_BUS_Unlock(lock);
    INT_INFO("Mani %d %s: Exit - _dsSetPreferredSleepMode \n",__LINE__, __FUNCTION__);
    return ret;
}

IARM_Result_t _dsGetPreferredSleepMode(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;

   _DEBUG_ENTER();
    
    IARM_BUS_Lock(lock);

    dsPreferredSleepMode *param = (dsPreferredSleepMode *)arg;
    param->mode =_SleepMode;
    
    INT_INFO("_dsGetPreferredSleepMode: Mode  - %s \r\n", enumToString(param->mode).c_str());
    
    IARM_BUS_Unlock(lock);
    return ret;
}

string enumToString( dsSleepMode_t mode )
{
	string ret;
	switch(mode)
	{
		case dsHOST_SLEEP_MODE_LIGHT:
			ret = "LIGHT_SLEEP";
		break;
		case dsHOST_SLEEP_MODE_DEEP:
			ret = "DEEP_SLEEP";
		break;
		default:
		ret = "LIGHT_SLEEP";
	}
	return ret;
}

dsSleepMode_t stringToEnum (string mode )
{
    if(mode == "LIGHT_SLEEP")
    {
        return dsHOST_SLEEP_MODE_LIGHT;
    }
    else if(mode == "DEEP_SLEEP")
    {
        return dsHOST_SLEEP_MODE_DEEP;
    }
    return dsHOST_SLEEP_MODE_LIGHT; 
}

IARM_Result_t _dsGetCPUTemperature(void *arg)
{
   
    _DEBUG_ENTER();
    IARM_Result_t ret = IARM_RESULT_SUCCESS;


    #ifdef HAS_THERMAL_API 
    IARM_BUS_Lock(lock);
   
    dsCPUThermalParam *param = (dsCPUThermalParam *)arg;
    float temperature = 45.0;
    dsError_t retValue = dsERR_NONE;

    retValue = dsGetCPUTemperature(&temperature);
    if (retValue == dsERR_NONE)
    {
        param->temperature = temperature;
    }

    INT_INFO("Current temperature in SRV is: %+7.2fC\n", param->temperature);

    IARM_BUS_Unlock(lock);
    #endif

    return ret;
}


IARM_Result_t _dsGetVersion(void *arg)
{
   
    _DEBUG_ENTER();
   
    IARM_BUS_Lock(lock);
   
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    dsError_t retValue = dsERR_NONE;
    dsVesrionParam *param = (dsVesrionParam *)arg;
    uint32_t  ver = 0x10000;
 
    param->versionNumber =  dsHAL_APIVER(DSHAL_API_VERSION_MAJOR_DEFAULT, DSHAL_API_VERSION_MINOR_DEFAULT);
          
    INT_INFO("DS HAL Version - %d.%d \r\n",dsHAL_APIVER_MAJOR(param->versionNumber),dsHAL_APIVER_MINOR(param->versionNumber));
   
    IARM_BUS_Unlock(lock);

    return ret;
}

IARM_Result_t _dsGetMS12ConfigType(void *arg)
{
   _DEBUG_ENTER();
   IARM_Result_t ret = IARM_RESULT_INVALID_STATE;
   dsMS12ConfigTypeParam_t *param = (dsMS12ConfigTypeParam_t*) arg;
   dsError_t retValue = dsERR_NONE;
   string ms12ConfigType;

   printf("Read default platform ms12 config type\n");
   try {
       ms12ConfigType = device::HostPersistence::getInstance().getDefaultProperty("MS12.Config.Type");
       printf("MS12 config type %s\n", ms12ConfigType.c_str());
   }
   catch(...){
	printf("Failed to retrieve config from default persistace, default to NONE");
	ms12ConfigType = "CONFIG_NONE";
   }
   if (retValue == dsERR_NONE) {
       strncpy(param->configType, ms12ConfigType.c_str(), MS12_CONFIG_BUF_SIZE);
       param->configType[MS12_CONFIG_BUF_SIZE-1] = '\0';
       
        param->result = dsERR_NONE;
        ret = IARM_RESULT_SUCCESS;
    }
    return ret; 
}
IARM_Result_t _dsGetSocIDFromSDK(void *arg)
{
    dsGetSocIDFromSDKParam_t *param = (dsGetSocIDFromSDKParam_t*) arg;
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    INT_DEBUG("dsSRV:_dsGetSocIDFromSDK\r\n");
    typedef dsError_t (*dsGetSocIDFromSDK_t)(char* socID);
    static dsGetSocIDFromSDK_t  func = NULL;
    if (func == NULL) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);

        if (dllib != NULL) {
            func = (dsGetSocIDFromSDK_t) dlsym(dllib, "dsGetSocIDFromSDK");
            if (func != NULL) {
                INT_DEBUG("dsSRV: dsGetSocIDFromSDK(char* socID) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSRV: dsGetSocIDFromSDK(char* socID) is not defined\r\n");
            }
	    dlclose(dllib);   //CID:80469,163396,163400,163407 - Resource leak
        }
        else {
            INT_ERROR("dsSRV: Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    if (param != NULL) {
        param->result = dsERR_GENERAL;

        if (func != NULL) {
            param->result = func(param->socID);
        }
    }
    INT_INFO("dsSRV: _dsGetSocIDFromSDK SocID : %s\n",param->socID);

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetHostEDID(void *arg)
{

#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    errno_t rc = -1;
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    INT_DEBUG("dsSRV::getHostEDID \r\n");

    typedef dsError_t (*dsGetHostEDID_t)(unsigned char *edid, int *length);
    static dsGetHostEDID_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetHostEDID_t) dlsym(dllib, "dsGetHostEDID");
            if (func) {
                INT_DEBUG("dsGetHostEDID(void) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetHostEDID(void) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsDisplayGetEDIDBytesParam_t *param = (dsDisplayGetEDIDBytesParam_t *)arg;

    if (func != 0) {
        unsigned char edidBytes[EDID_MAX_DATA_SIZE] ;
        int length = 0;
        dsError_t ret = func(edidBytes, &length);
        if (ret == dsERR_NONE && length <= 1024) {
            INT_INFO("dsSRV ::getHostEDID returns %d bytes\r\n", length);
            rc = memcpy_s(param->bytes,sizeof(param->bytes),edidBytes,EDID_MAX_DATA_SIZE);
            if(rc!=EOK){
                    ERR_CHK(rc);
            }
            param->length = length;
        }
        param->result = ret;
    }
    else {
        param->result = dsERR_OPERATION_NOT_SUPPORTED;
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}


/** @} */
/** @} */
