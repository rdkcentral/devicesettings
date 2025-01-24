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


#include "dsVideoDevice.h"

#include <sys/types.h>
#include <stdint.h>
#include <iostream>
#include <string.h>
#include <pthread.h>
#include "dsError.h"
#include "dsUtl.h"
#include "dsTypes.h"
#include "pthread.h"
#include "libIARM.h"
#include "iarmUtil.h"
#include "libIBus.h"
#include "dsRpc.h"
#include "dsMgr.h"
#include "dsserverlogger.h"
#include "hostPersistence.hpp"
#include <dlfcn.h>
#include "safec_lib.h"
#include "dsHdmiIn.h"

#include "dsInternal.h"

static int m_isInitialized = 0;
static int m_isPlatInitialized = 0;
static pthread_mutex_t dsLock = PTHREAD_MUTEX_INITIALIZER;
static dsVideoZoom_t srv_dfc = dsVIDEO_ZOOM_FULL;
static bool force_disable_hdr = true;

#define IARM_BUS_Lock(lock) pthread_mutex_lock(&dsLock)
#define IARM_BUS_Unlock(lock) pthread_mutex_unlock(&dsLock)

IARM_Result_t _dsVideoDeviceInit(void *arg);
IARM_Result_t _dsGetVideoDevice(void *arg);
IARM_Result_t _dsSetDFC(void *arg);
IARM_Result_t _dsGetDFC(void *arg);
IARM_Result_t _dsVideoDeviceTerm(void *arg);
IARM_Result_t _dsGetHDRCapabilities(void *arg);
IARM_Result_t _dsGetSupportedVideoCodingFormats(void *arg);
IARM_Result_t _dsGetVideoCodecInfo(void *arg);
IARM_Result_t _dsForceDisableHDR(void *arg);

IARM_Result_t _dsSetFRFMode(void *arg);
IARM_Result_t _dsGetFRFMode(void *arg);
IARM_Result_t _dsGetCurrentDisframerate(void *arg);
IARM_Result_t _dsSetDisplayframerate(void *arg);

static int _dsSendDisplayFrameRateStatusChangeEventCallBack(dsFramerateParam_t *displayframerate, IARM_Bus_DSMgr_EventId_t _eventId, dsError_t result);

void _dsFramerateStatusPostChangeCB(unsigned int inputStatus);
void _dsFramerateStatusPreChangeCB(unsigned int inputStatus);

IARM_Result_t dsVideoDeviceMgr_init()
{
   IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsVideoDeviceInit, _dsVideoDeviceInit);
   IARM_BUS_Lock(lock);

	try
	{
		 std::string _ZoomSettings("Full");
		/* Get the Zoom from Persistence */
		_ZoomSettings = device::HostPersistence::getInstance().getProperty("VideoDevice.DFC", _ZoomSettings);
		if (_ZoomSettings.compare("None") == 0)
		{
			srv_dfc = dsVIDEO_ZOOM_NONE;
		}
        if (PROFILE_TV == profileType)
        {
            INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
            dsHdmiInSelectZoomMode(srv_dfc);
        }
	}
	catch(...) 
	{
		INT_INFO("Exception in Getting the Zoom settings on Startup..... \r\n");
	}

	try
	{
		std::string _hdr_setting("false");
		_hdr_setting = device::HostPersistence::getInstance().getProperty("VideoDevice.forceHDRDisabled", _hdr_setting);
		if (_hdr_setting.compare("false") == 0)
		{
			force_disable_hdr = false;
		}
		else
		{
			force_disable_hdr = true;
			INT_INFO("HDR support in disabled configuration.\n");
		}
	}
	catch(...) 
	{
		INT_INFO("Exception in getting force-disable-HDR setting at start up.\r\n");
	}



    if (!m_isPlatInitialized) {
    	dsVideoDeviceInit();
    }
    /*coverity[missing_lock]  CID-19380 using Coverity Annotation to ignore error*/
    m_isPlatInitialized++;
    IARM_BUS_Unlock(lock);   //CID:136385 - Data race condition
    
   return IARM_RESULT_SUCCESS;
}

IARM_Result_t dsVideoDeviceMgr_term()
{
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsVideoDeviceInit(void *arg)
{
    IARM_BUS_Lock(lock);
   
	if (!m_isInitialized) {

		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetVideoDevice,_dsGetVideoDevice);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetDFC,_dsSetDFC);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetDFC,_dsGetDFC);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsVideoDeviceTerm,_dsVideoDeviceTerm);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetHDRCapabilities,_dsGetHDRCapabilities); 
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetSupportedVideoCodingFormats, _dsGetSupportedVideoCodingFormats); 
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetVideoCodecInfo, _dsGetVideoCodecInfo); 
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetForceDisableHDR, _dsForceDisableHDR); 
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetFRFMode, _dsSetFRFMode);
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetFRFMode, _dsGetFRFMode);
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetCurrentDisframerate, _dsGetCurrentDisframerate);
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetDisplayframerate, _dsSetDisplayframerate);

		typedef dsError_t (*_dsFramerateStatusPreChangeCB_t)(dsRegisterFrameratePreChangeCB_t CBFunc);
                static _dsFramerateStatusPreChangeCB_t frameratePreChangeCB = 0;
                if (frameratePreChangeCB  == 0) {
                        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                        if (dllib) {
                                frameratePreChangeCB = (_dsFramerateStatusPreChangeCB_t) dlsym(dllib, "dsRegisterFrameratePreChangeCB");
                                if(frameratePreChangeCB == 0) {
                                        INT_DEBUG("dsRegisterFrameratePreChangeCB(dsHdmiInStatusChangeCB_t) is not defined\r\n");
                                }
                                dlclose(dllib);
                        }
                        else {
                                INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                        }
                }

                if(frameratePreChangeCB) {
                        frameratePreChangeCB(_dsFramerateStatusPreChangeCB);
                }

		typedef dsError_t (*dsRegisterFrameratePostChangeCB_t)(dsRegisterFrameratePostChangeCB_t CBFunc);
                static dsRegisterFrameratePostChangeCB_t frameratePostChangeCB = 0;
                if (frameratePostChangeCB == 0) {
                        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                        if (dllib) {
                                frameratePostChangeCB = (dsRegisterFrameratePostChangeCB_t) dlsym(dllib, "dsRegisterFrameratePostChangeCB");
                                if(frameratePostChangeCB == 0) {
                                        INT_DEBUG("dsRegisterFrameratePostChangeCB(dsHdmiInStatusChangeCB_t) is not defined\r\n");
                                }
                                dlclose(dllib);
                        }
                        else {
                                INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                        }
                }

                if(frameratePostChangeCB) {
                        frameratePostChangeCB(_dsFramerateStatusPostChangeCB);
                }

		m_isInitialized = 1;
    }

    if (!m_isPlatInitialized) {
    	dsVideoDeviceInit();
    }
    
	m_isPlatInitialized++;

	IARM_BUS_Unlock(lock);
	
	return IARM_RESULT_SUCCESS;

}

IARM_Result_t _dsGetVideoDevice(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);
    
	dsVideoDeviceGetHandleParam_t *param = (dsVideoDeviceGetHandleParam_t *)arg;
    dsGetVideoDevice(param->index, &param->handle);
    
	
    IARM_BUS_Unlock(lock);

	return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsSetDFC(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);
    
	IARM_Bus_DSMgr_EventData_t eventData;
	dsVideoDeviceSetDFCParam_t *param = (dsVideoDeviceSetDFCParam_t *)arg;

	if (param != NULL)
	{
		try
		{
			if(param->dfc == dsVIDEO_ZOOM_NONE)
			{
				INT_DEBUG("\n Call Zoom setting NONE\n");
				dsSetDFC(param->handle,param->dfc);
				eventData.data.dfc.zoomsettings = dsVIDEO_ZOOM_NONE;
				srv_dfc = param->dfc;
				IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)IARM_BUS_DSMGR_EVENT_ZOOM_SETTINGS,(void *)&eventData, sizeof(eventData));
				device::HostPersistence::getInstance().persistHostProperty("VideoDevice.DFC","None");
			}
			else if(param->dfc == dsVIDEO_ZOOM_FULL)
			{
				INT_DEBUG("\n Call Zoom setting FULL\n");
				dsSetDFC(param->handle,param->dfc);
				eventData.data.dfc.zoomsettings =  dsVIDEO_ZOOM_FULL;
				srv_dfc = param->dfc;
				IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)IARM_BUS_DSMGR_EVENT_ZOOM_SETTINGS,(void *)&eventData, sizeof(eventData));
				device::HostPersistence::getInstance().persistHostProperty("VideoDevice.DFC","Full");

			}
			else if(param->dfc == dsVIDEO_ZOOM_16_9_ZOOM)
			{
				INT_DEBUG("\n Call Zoom setting dsVIDEO_ZOOM_16_9_ZOOM\n");
				dsSetDFC(param->handle,param->dfc);
				eventData.data.dfc.zoomsettings =  dsVIDEO_ZOOM_FULL;
				srv_dfc = param->dfc;
				IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)IARM_BUS_DSMGR_EVENT_ZOOM_SETTINGS,(void *)&eventData, sizeof(eventData));
				device::HostPersistence::getInstance().persistHostProperty("VideoDevice.DFC","Full");
			}
			else
			{
				INT_INFO("\n ERROR: unsupported Zoom setting %d\n", param->dfc);
			}

            if (PROFILE_TV == profileType)
            {
                INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
                dsHdmiInSelectZoomMode(srv_dfc);
            }
		}
		catch(...) 
		{
			INT_ERROR("Error in Setting the Video Resolution..... \r\n");
		}
		
	}
  
    IARM_BUS_Unlock(lock);
	return IARM_RESULT_SUCCESS;
}




IARM_Result_t _dsGetDFC(void *arg)
{
    _DEBUG_ENTER();
    
	IARM_BUS_Lock(lock);

	dsVideoDeviceSetDFCParam_t *param = (dsVideoDeviceSetDFCParam_t *)arg;
	
	if (param != NULL)
	{
		param->dfc = srv_dfc;
		INT_INFO("The Zoom Settings value is %d \r\n",param->dfc);
	}

	IARM_BUS_Unlock(lock);
	
	return IARM_RESULT_SUCCESS;
}


IARM_Result_t _dsVideoDeviceTerm(void *arg)
{
    _DEBUG_ENTER();
    
	IARM_BUS_Lock(lock);
    
	m_isPlatInitialized --;

	if (0 == m_isPlatInitialized)
	{
		dsVideoDeviceTerm();
	}
	
    IARM_BUS_Unlock(lock);
	
	return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetHDRCapabilities(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);
    
    typedef dsError_t (*dsGetHDRCapabilitiesFunc_t)(intptr_t handle, int *capabilities);
    static dsGetHDRCapabilitiesFunc_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetHDRCapabilitiesFunc_t)dlsym(dllib, "dsGetHDRCapabilities");
            if (func) {
                INT_DEBUG("dsGetHDRCapabilities() is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetHDRCapabilities() is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }
    dsGetHDRCapabilitiesParam_t *param = (dsGetHDRCapabilitiesParam_t *)arg;
    if((0 != func) && (false == force_disable_hdr)) {
        param->result = func(param->handle, &param->capabilities);
    }
    else {
        param->capabilities = dsHDRSTANDARD_NONE;
    }

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetSupportedVideoCodingFormats(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);
    
    typedef dsError_t (*dsGetSupportedVideoCodingFormatsFunc_t)(intptr_t handle, unsigned int *supported_formats);
    static dsGetSupportedVideoCodingFormatsFunc_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetSupportedVideoCodingFormatsFunc_t)dlsym(dllib, "dsGetSupportedVideoCodingFormats");
            if (func) {
                INT_DEBUG("dsGetSupportedVideoCodingFormats() is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetSupportedVideoCodingFormats() is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }
    dsGetSupportedVideoCodingFormatsParam_t *param = (dsGetSupportedVideoCodingFormatsParam_t *)arg;
    if(0 != func) {
        param->result = func(param->handle, &param->supported_formats);
    }
    else {
        param->supported_formats = 0x0; //Safe default: no formats supported.
    }

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetVideoCodecInfo(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetVideoCodecInfoFunc_t)(intptr_t handle, dsVideoCodingFormat_t codec, dsVideoCodecInfo_t * info);
    static dsGetVideoCodecInfoFunc_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetVideoCodecInfoFunc_t)dlsym(dllib, "dsGetVideoCodecInfo");
            if (func) {
                INT_DEBUG("dsGetVideoCodecInfo() is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetVideoCodecInfo() is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsGetVideoCodecInfoParam_t *param = (dsGetVideoCodecInfoParam_t *)arg;
    if(0 != func) {
        param->result = func(param->handle, param->format, &param->info);
    }
    else {
        param->result = dsERR_OPERATION_NOT_SUPPORTED;
    }

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsForceDisableHDR(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsDisableHDRSupportFunc_t)(intptr_t handle, bool enable);
    static dsDisableHDRSupportFunc_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsDisableHDRSupportFunc_t)dlsym(dllib, "dsForceDisableHDRSupport");
            if (func) {
                INT_DEBUG("dsForceDisableHDRSupport() is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsForceDisableHDRSupport() is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("dsForceDisableHDRSupport() Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }
    
    dsForceDisableHDRParam_t *param = (dsForceDisableHDRParam_t *)arg;
	param->result = dsERR_NONE;

    if(0 != func )
    {
        param->result = func(param->handle, param->disable);
        INT_INFO("dsForceDisableHDRSupport() enable:%d \n",param->disable);
    }
    else {
        param->disable = 0;
    }

	force_disable_hdr = param->disable;
	if(force_disable_hdr)
	{
		device::HostPersistence::getInstance().persistHostProperty("VideoDevice.forceHDRDisabled","true");
	}
	else
	{
		device::HostPersistence::getInstance().persistHostProperty("VideoDevice.forceHDRDisabled","false");
	}

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsSetFRFMode(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSetFRFModeFunc_t)(intptr_t handle, int frfmode);
    static dsSetFRFModeFunc_t func = 0;
    if (func == 0) {
            void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
            if (dllib) {
                    func = (dsSetFRFModeFunc_t)dlsym(dllib, "dsSetFRFMode");
                    if (func) {
                            INT_DEBUG("dsSetFRFMode is defined and loaded\r\n");
                    }
                    else {
                            INT_INFO("dsSetFRFMode is not defined\r\n");
                    }
                    dlclose(dllib);
            }
            else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
            }
    }
    dsFRFParam_t *param = (dsFRFParam_t *)arg;
    if(0 != func) {
            func(param->handle, param->frfmode);
    }
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetFRFMode(void *arg)
{
        _DEBUG_ENTER();

        IARM_BUS_Lock(lock);

        typedef dsError_t (*dsGetFRFModeFunc_t)(intptr_t handle, int *frfmode);
        static dsGetFRFModeFunc_t func = 0;
        if (func == 0) {
                void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                        func = (dsGetFRFModeFunc_t)dlsym(dllib, "dsGetFRFMode");
                        if (func) {
                                INT_DEBUG("dsGetFRFMode() is defined and loaded\r\n");
                        }
            else {
                INT_INFO("dsGetFRFMode() is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }
	dsFRFParam_t *param = (dsFRFParam_t *)arg;
    if(0 != func) {
        func(param->handle, &param->frfmode);
    }

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetCurrentDisframerate(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetCurrentDisframerateFunc_t)(intptr_t handle, char *framerate);
    static dsGetCurrentDisframerateFunc_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetCurrentDisframerateFunc_t)dlsym(dllib, "dsGetCurrentDisplayframerate");
            if (func) {
                INT_DEBUG("dsGetCurrentDisframerate() is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetCurrentDisframerate() is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }
    
    dsFramerateParam_t *param = (dsFramerateParam_t *)arg;
    if(0 != func) {
        func(param->handle, param->framerate);
    }

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsSetDisplayframerate(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    dsError_t result = dsERR_NONE;

    typedef dsError_t (*dsSetDisplayframerateFunc_t)(intptr_t handle, char *frfmode);
    static dsSetDisplayframerateFunc_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetDisplayframerateFunc_t)dlsym(dllib, "dsSetDisplayframerate");
            if (func) {
                INT_DEBUG("dsSetDisplayframerate() is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetDisplayframerate() is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }
 	
    dsFramerateParam_t *param = (dsFramerateParam_t *)arg;

    _dsSendDisplayFrameRateStatusChangeEventCallBack(param, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, result);

    if(0 != func) {
        result = func(param->handle, param->framerate);
    }

    _dsSendDisplayFrameRateStatusChangeEventCallBack(param, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, result);

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

static int _dsSendDisplayFrameRateStatusChangeEventCallBack(dsFramerateParam_t *displayframerate, IARM_Bus_DSMgr_EventId_t _eventId, dsError_t result)
{
    IARM_Bus_DSMgr_EventData_t _eventData;
    dsError_t ret = result;

    if ((strcmp(displayframerate->framerate,"") == 0) || ret == dsERR_INVALID_PARAM)
    {
        ret = dsERR_INVALID_PARAM;
        return ret;
    }

    memmove(_eventData.data.DisplayFrameRateChange.framerate, displayframerate->framerate, sizeof(_eventData.data.DisplayFrameRateChange));
    __TIMESTAMP();
    printf("%s:%d - Framerate status change update!!!!!! \r\n", __PRETTY_FUNCTION__,__LINE__);
    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                            (IARM_EventId_t)_eventId,
                            (void *)&_eventData,
                            sizeof(_eventData));
    return ret;
}

void _dsFramerateStatusPreChangeCB(unsigned int inputStatus)
{
    IARM_Bus_DSMgr_EventData_t _eventData;

    INT_INFO("%s:%d - Framerate status prechange update!!!!!! \r\n", __PRETTY_FUNCTION__,__LINE__);

    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                                (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE,
                                (void *)&_eventData,
                                sizeof(_eventData));

}

void _dsFramerateStatusPostChangeCB(unsigned int inputStatus)
{
    IARM_Bus_DSMgr_EventData_t _eventData;

    INT_INFO("%s:%d - Framerate status changed update!!!!!! \r\n", __PRETTY_FUNCTION__,__LINE__);

    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                                (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE,
                                (void *)&_eventData,
                                sizeof(_eventData));

}

/** @} */
/** @} */
