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


#include "dsVideoPort.h"
#include "dsDisplay.h"

#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <dlfcn.h>
#include "dsError.h"
#include "dsUtl.h"
#include "dsTypes.h"
#include "pthread.h"
#include <pthread.h>
#include "libIARM.h"
#include "iarmUtil.h"
#include "libIBusDaemon.h"
#include "libIBus.h"
#include "dsRpc.h"
#include "dsMgr.h"
#include <iostream>
#include <string.h>
#include "hostPersistence.hpp"
#include "dsserverlogger.h"
#include "dsTypes.h"
#include "dsVideoPortSettings.h"
#include "dsInternal.h"
#include "safec_lib.h"
#include <vector>
#include <fstream>
#include <string>

#define DEFAULT_RESOLUTION  "720p"
#define DEFAULT_RESOLUTION_1080P "1080p"
#define DEFAULT_RESOLUTION_2160P "2160p"

#define DEFAULT_SD_RESOLUTION "480i"

static int m_isInitialized = 0;
static int m_isPlatInitialized = 0;
static pthread_mutex_t dsLock = PTHREAD_MUTEX_INITIALIZER;
static std::string _dsHDMIResolution(DEFAULT_RESOLUTION);
static std::string _dsCompResolution(DEFAULT_RESOLUTION);
static std::string _dsBBResolution(DEFAULT_SD_RESOLUTION);
static std::string _dsRFResolution(DEFAULT_SD_RESOLUTION);
static dsHdcpStatus_t _hdcpStatus = dsHDCP_STATUS_UNAUTHENTICATED;
static bool force_disable_4K = false;
static const dsDisplayColorDepth_t DEFAULT_COLOR_DEPTH = dsDISPLAY_COLORDEPTH_AUTO;
static dsDisplayColorDepth_t hdmiColorDept = DEFAULT_COLOR_DEPTH;
#define NULL_HANDLE 0
#define IARM_BUS_Lock(lock) pthread_mutex_lock(&dsLock)
#define IARM_BUS_Unlock(lock) pthread_mutex_unlock(&dsLock)

IARM_Result_t _dsVideoPortInit(void *arg);
IARM_Result_t _dsGetVideoPort(void *arg);
IARM_Result_t _dsIsVideoPortEnabled(void *arg);
IARM_Result_t _dsIsDisplayConnected(void *arg);
IARM_Result_t _dsIsDisplaySurround(void *arg);
IARM_Result_t _dsGetSurroundMode(void *arg);
IARM_Result_t _dsEnableVideoPort(void *arg);
IARM_Result_t _dsSetResolution(void *arg);
IARM_Result_t _dsGetResolution(void *arg);
IARM_Result_t _dsColorDepthCapabilities(void *arg);
IARM_Result_t _dsGetPreferredColorDepth(void *arg);
IARM_Result_t _dsSetPreferredColorDepth(void *arg);
IARM_Result_t _dsVideoPortTerm(void *arg);
IARM_Result_t _dsEnableHDCP(void *arg);
IARM_Result_t _dsIsHDCPEnabled(void *arg);
IARM_Result_t _dsGetHDCPStatus(void *arg);
IARM_Result_t _dsGetHDCPProtocol(void *arg);
IARM_Result_t _dsGetHDCPReceiverProtocol(void *arg);
IARM_Result_t _dsGetHDCPCurrentProtocol(void *arg);
IARM_Result_t _dsIsVideoPortActive(void *arg);
IARM_Result_t _dsGetTVHDRCapabilities(void *arg);
IARM_Result_t _dsSupportedTvResolutions(void *arg);
IARM_Result_t _dsSetForceDisable4K(void *arg);
IARM_Result_t _dsGetForceDisable4K(void *arg);
IARM_Result_t _dsIsOutputHDR(void *arg);
IARM_Result_t _dsResetOutputToSDR(void *arg);
IARM_Result_t _dsSetHdmiPreference(void *arg);
IARM_Result_t _dsGetHdmiPreference(void *arg);
IARM_Result_t _dsGetVideoEOTF(void *arg);
IARM_Result_t _dsGetMatrixCoefficients(void* arg);
IARM_Result_t _dsGetColorDepth(void* arg);
IARM_Result_t _dsGetColorSpace(void* arg);
IARM_Result_t _dsGetQuantizationRange(void* arg);
IARM_Result_t _dsGetCurrentOutputSettings(void* arg);
IARM_Result_t _dsSetBackgroundColor(void *arg);
IARM_Result_t _dsSetForceHDRMode(void *arg);
IARM_Result_t _dsGetIgnoreEDIDStatus(void *arg);


void _dsVideoFormatUpdateCB(dsHDRStandard_t videoFormat);
static dsVideoPortType_t _GetVideoPortType(intptr_t handle);
static int  _dsVideoPortPreResolutionCall(dsVideoPortResolution_t *resolution);
static int  _dsSendVideoPortPostResolutionCall(dsVideoPortResolution_t *resolution);
static dsError_t _dsVideoFormatUpdateRegisterCB (dsVideoFormatUpdateCB_t cbFun);
void _dsHdcpCallback(intptr_t handle, dsHdcpStatus_t event);
static void persistResolution(dsVideoPortSetResolutionParam_t *param);
void resetColorDepthOnHdmiReset(intptr_t handle);
static dsDisplayColorDepth_t getPersistentColorDepth ();
static dsDisplayColorDepth_t getBestSupportedColorDepth (intptr_t handle, dsDisplayColorDepth_t inColorDepth);
static IARM_Result_t dsGetHDCPCurrentProtocolInternal (dsVideoPortGetHDCPProtocolVersion_t *param);

//Call this functions from locked function calls in srv
static IARM_Result_t setPreferredColorDepth(void *arg);
static dsError_t handleDsColorDepthCapabilities(intptr_t handle, unsigned int *colorDepthCapability );
static dsError_t handleDsGetPreferredColorDepth(intptr_t handle, dsDisplayColorDepth_t *colorDepth, bool persist);
static dsError_t handleDsSetPreferredColorDepth(intptr_t handle,dsDisplayColorDepth_t colorDepth, bool persist);

#define IsHDCompatible(p)  (((p) >= dsVIDEO_PIXELRES_1280x720 ) && ((p) < dsVIDEO_PIXELRES_MAX))
static  std::string getCompatibleResolution(dsVideoPortResolution_t *SrcResn);
static bool    IsCompatibleResolution(dsVideoResolution_t pixelResolution1,dsVideoResolution_t pixelResolution2);
static dsVideoResolution_t getPixelResolution(std::string &resolution);
void _dsSyncHdmiStatus(const std::string& key, int value);
static intptr_t dsGetDefaultPortHandle();
static std::string getHdmiConnectionName(const int key);
static std::string getHdcpStatusName(const int key);
static std::string getHdcpVersionName(const int key);

void VideoConfigInit()
{
	intptr_t handle = 0;
	dsError_t eRet = dsGetVideoPort(dsVIDEOPORT_TYPE_HDMI,0,&handle);
	if (dsERR_NONE == eRet) {
		resetColorDepthOnHdmiReset(handle);
	}else {
		INT_INFO("HDMI get port handle failed %d \r\n", eRet);
	}
}

IARM_Result_t dsVideoPortMgr_init()
{
   IARM_BUS_Lock(lock);

   std::string _Resolution(DEFAULT_RESOLUTION);

   if (PROFILE_TV == profileType)
   {
        INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
        _Resolution = DEFAULT_RESOLUTION_2160P;
        _dsHDMIResolution = DEFAULT_RESOLUTION_2160P;
        _dsCompResolution = DEFAULT_RESOLUTION_2160P;
   }
   else
   {
        INT_INFO("[%d][%s]: its Other Profile\r\n", __LINE__, __FUNCTION__);
        _Resolution= DEFAULT_RESOLUTION_1080P;
        _dsHDMIResolution = DEFAULT_RESOLUTION_1080P;
        _dsCompResolution = DEFAULT_RESOLUTION_1080P;
   }
	
	try
	{
		/*TBD - Get the Device type Dynamically*/
		/*
			* Read the HDMI,Component or Composite
			* Next is to browse through all supported ports  i.e kPorts to differentiate between 
			* Component and Composite. TBD - Remove HAS_ONLY_COMPOSITE
		*/
		_dsHDMIResolution = device::HostPersistence::getInstance().getProperty("HDMI0.resolution",_Resolution);
		INT_INFO("The Persistent HDMI resolution read is %s \r\n",_dsHDMIResolution.c_str());
		#ifdef HAS_ONLY_COMPOSITE
           if (PROFILE_TV == profileType)
           {
               INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
               _Resolution = DEFAULT_RESOLUTION_2160P;
           }
           else
           {
                INT_INFO("[%d][%s]: its Other Profile\r\n", __LINE__, __FUNCTION__);
                _Resolution = DEFAULT_RESOLUTION_1080P;
           }
			_dsCompResolution = device::HostPersistence::getInstance().getProperty("Baseband0.resolution",_Resolution);
		#else
           if (PROFILE_TV == profileType)
           {
               INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
               _Resolution = DEFAULT_RESOLUTION_2160P;
           }
           else
           {
                INT_INFO("[%d][%s]: its Other Profile\r\n", __LINE__, __FUNCTION__);
                _Resolution = DEFAULT_RESOLUTION_1080P;
           }
			_dsCompResolution = device::HostPersistence::getInstance().getProperty("COMPONENT0.resolution",_Resolution);
		#endif
		INT_INFO("The Persistent Component/Composite resolution read is %s \r\n",_dsCompResolution.c_str());
                 _dsRFResolution = device::HostPersistence::getInstance().getProperty("RF0.resolution",_Resolution);
                INT_INFO("The Persistent RF resolution read is %s \r\n",_dsRFResolution.c_str());
                _dsBBResolution = device::HostPersistence::getInstance().getProperty("Baseband0.resolution",_Resolution);
                INT_INFO("The Persistent BB resolution read is %s \r\n",_dsBBResolution.c_str());
					
		if (!m_isPlatInitialized) 
		{
			/*Initialize the Video Ports */
			dsVideoPortInit();
			VideoConfigInit();
		}
		/*coverity[missing_lock]  CID-18497 using Coverity Annotation to ignore error*/
		m_isPlatInitialized ++;
	}
	catch(...) 
	{
		INT_INFO("Error in Getting the Video Resolution on Startup..... \r\n");
	}
	try
	{
		std::string _4K_setting("false");
		_4K_setting = device::HostPersistence::getInstance().getProperty("VideoDevice.force4KDisabled", _4K_setting);
		if (_4K_setting.compare("true") == 0)
		{
			force_disable_4K = true;
			INT_INFO("4K support in disabled configuration.\n");
                }
		else
		{
			force_disable_4K = false;
		}
	}
	catch(...) 
	{
		INT_INFO("Exception in getting force-disable-4K setting at start up.\r\n");
	}
	IARM_BUS_Unlock(lock);  //CID:136282 - Data race condition
	IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsVideoPortInit, _dsVideoPortInit);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t dsVideoPortMgr_term()
{
   return IARM_RESULT_SUCCESS;
}


IARM_Result_t _dsVideoPortInit(void *arg)
{
    IARM_BUS_Lock(lock);
    bool isConnected = false;
    if (!m_isInitialized) {

		_dsSyncHdmiStatus(DS_HDMI_TAG_HDCPSTATUS, dsHDCP_STATUS_UNAUTHENTICATED);
		_dsSyncHdmiStatus(DS_HDMI_TAG_HDCPVERSION, dsHDCP_VERSION_1X);
		dsIsDisplayConnected(dsGetDefaultPortHandle(),&isConnected);
		_dsSyncHdmiStatus(DS_HDMI_TAG_HOTPLUP, (isConnected?dsDISPLAY_EVENT_CONNECTED:dsDISPLAY_EVENT_DISCONNECTED));
		#ifdef HAS_HDCP_CALLBACK
			dsRegisterHdcpStatusCallback(NULL,_dsHdcpCallback);
		#endif
		
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetVideoPort,_dsGetVideoPort);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsIsVideoPortEnabled,_dsIsVideoPortEnabled);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsIsDisplayConnected,_dsIsDisplayConnected);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsIsDisplaySurround,_dsIsDisplaySurround);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetSurroundMode,_dsGetSurroundMode);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsEnableVideoPort,_dsEnableVideoPort);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetResolution,_dsSetResolution);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetResolution,_dsGetResolution);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsVideoPortTerm,_dsVideoPortTerm);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsEnableHDCP ,_dsEnableHDCP);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsIsHDCPEnabled,_dsIsHDCPEnabled);
	    IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetHDCPStatus ,_dsGetHDCPStatus); 
	    IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetHDCPProtocol ,_dsGetHDCPProtocol);
	    IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetHDCPReceiverProtocol ,_dsGetHDCPReceiverProtocol);
	    IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetHDCPCurrentProtocol ,_dsGetHDCPCurrentProtocol);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsIsVideoPortActive ,_dsIsVideoPortActive); 
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetTVHDRCapabilities,_dsGetTVHDRCapabilities);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetSupportedTVResolution,_dsSupportedTvResolutions);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetForceDisable4K, _dsSetForceDisable4K); 
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetForceDisable4K, _dsGetForceDisable4K); 
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsIsOutputHDR,_dsIsOutputHDR);
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsResetOutputToSDR,_dsResetOutputToSDR);
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetHdmiPreference,_dsSetHdmiPreference);
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetHdmiPreference,_dsGetHdmiPreference);
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetVideoEOTF,_dsGetVideoEOTF);
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetMatrixCoefficients,_dsGetMatrixCoefficients);
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetColorDepth,_dsGetColorDepth);
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetColorSpace,_dsGetColorSpace);
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetQuantizationRange,_dsGetQuantizationRange);
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetCurrentOutputSettings,_dsGetCurrentOutputSettings);
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetBackgroundColor,_dsSetBackgroundColor);
                IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetForceHDRMode,_dsSetForceHDRMode);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsColorDepthCapabilities,_dsColorDepthCapabilities);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetPreferredColorDepth,_dsGetPreferredColorDepth);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetPreferredColorDepth,_dsSetPreferredColorDepth);
	
        dsError_t eRet = _dsVideoFormatUpdateRegisterCB (_dsVideoFormatUpdateCB) ;
        if (dsERR_NONE != eRet) {
            INT_DEBUG("%s: _dsVideoFormatUpdateRegisterCB eRet:%04x", __FUNCTION__, eRet);
        }
        m_isInitialized = 1;
    }

    if (!m_isPlatInitialized) {
    	/* Nexus init, if any here */
        dsVideoPortInit();
        VideoConfigInit();
    }
    m_isPlatInitialized++;

    IARM_BUS_Unlock(lock);
	
	return IARM_RESULT_SUCCESS;

}

IARM_Result_t _dsGetVideoPort(void *arg)
{
    _DEBUG_ENTER();
    
	IARM_BUS_Lock(lock);
    
	dsVideoPortGetHandleParam_t *param = (dsVideoPortGetHandleParam_t *)arg;
    dsGetVideoPort(param->type, param->index, &param->handle);

	IARM_BUS_Unlock(lock);
	return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsIsVideoPortEnabled(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);
    
	dsVideoPortIsEnabledParam_t *param = (dsVideoPortIsEnabledParam_t *)arg;
    dsIsVideoPortEnabled(param->handle, &param->enabled);
   
    IARM_BUS_Unlock(lock);
	
	return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsIsVideoPortActive(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);
    
	dsVideoPortIsActiveParam_t *param = (dsVideoPortIsActiveParam_t *)arg;
    
    dsVideoPortType_t _VPortType = _GetVideoPortType(param->handle);

	if (_VPortType == dsVIDEOPORT_TYPE_HDMI ||
         _VPortType == dsVIDEOPORT_TYPE_INTERNAL)
	{
		param->result = dsIsVideoPortActive(param->handle, &param->active);
	}
	else if (_VPortType == dsVIDEOPORT_TYPE_COMPONENT || _VPortType == dsVIDEOPORT_TYPE_RF)
	{
		param->active = true;
        param->result =  dsERR_NONE;
	}

    IARM_BUS_Unlock(lock);
	
	return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetVideoEOTF(void* arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetVideoEOTF_t)(intptr_t handle, dsHDRStandard_t* video_eotf);
    static dsGetVideoEOTF_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetVideoEOTF_t) dlsym(dllib, "dsGetVideoEOTF");
            if (func) {
                INT_DEBUG("dsGetVideoEOTF_t(int, dsHDRStandard_t*) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetVideoEOTF_t(int, dsHDRStandard_t*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsEot_t* param = (dsEot_t*)arg;

    if (func != 0) {
        param->result = func(param->handle, &param->video_eotf);
    }
    else {
        param->video_eotf = dsHDRSTANDARD_NONE;
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetMatrixCoefficients(void* arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetMatrixCoefficients_t)(intptr_t handle, dsDisplayMatrixCoefficients_t* matrix_coefficients);
    static dsGetMatrixCoefficients_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetMatrixCoefficients_t) dlsym(dllib, "dsGetMatrixCoefficients");
            if (func) {
                INT_DEBUG("dsGetMatrixCoefficients_t(int, dsDisplayMatrixCoefficients_t*) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetMatrixCoefficients_t(int, dsDisplayMatrixCoefficients_t*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsMatrixCoefficients_t* param = (dsMatrixCoefficients_t*)arg;

    if (func != 0) {
        param->result = func(param->handle, &param->matrix_coefficients);
    }
    else {
        param->matrix_coefficients = dsDISPLAY_MATRIXCOEFFICIENT_UNKNOWN;
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetColorDepth(void* arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetColorDepth_t)(intptr_t handle, unsigned int* color_depth);
    static dsGetColorDepth_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetColorDepth_t) dlsym(dllib, "dsGetColorDepth");
            if (func) {
                INT_DEBUG("dsGetColorDepth_t(int, unsigned int*) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetColorDepth_t(int, unsigned int*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsColorDepth_t* param = (dsColorDepth_t*)arg;

    if (func != 0) {
        param->result = func(param->handle, &param->color_depth);
    }
    else {
        param->color_depth = 0;
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetColorSpace(void* arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetColorSpace_t)(intptr_t handle, dsDisplayColorSpace_t* color_space);
    static dsGetColorSpace_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetColorSpace_t) dlsym(dllib, "dsGetColorSpace");
            if (func) {
                INT_DEBUG("dsGetColorSpace_t(int, dsDisplayColorSpace_t*) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetColorSpace_t(int, dsDisplayColorSpace_t*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsColorSpace_t* param = (dsColorSpace_t*)arg;

    if (func != 0) {
        param->result = func(param->handle, &param->color_space);
    }
    else {
        param->color_space = dsDISPLAY_COLORSPACE_UNKNOWN;
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetQuantizationRange(void* arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetQuantizationRange_t)(intptr_t handle, dsDisplayQuantizationRange_t* quantization_range);
    static dsGetQuantizationRange_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetQuantizationRange_t) dlsym(dllib, "dsGetQuantizationRange");
            if (func) {
                INT_DEBUG("dsGetQuantizationRange_t(int, dsDisplayQuantizationRange_t*) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetQuantizationRange_t(int, dsDisplayQuantizationRange_t*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsQuantizationRange_t* param = (dsQuantizationRange_t*)arg;

    if (func != 0) {
        param->result = func(param->handle, &param->quantization_range);
    }
    else {
        param->quantization_range = dsDISPLAY_QUANTIZATIONRANGE_UNKNOWN;
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetCurrentOutputSettings(void* arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetCurrentOutputSettings_t)(intptr_t handle, dsHDRStandard_t* video_eotf, dsDisplayMatrixCoefficients_t* matrix_coefficients, dsDisplayColorSpace_t* color_space, unsigned int* color_depth, dsDisplayQuantizationRange_t* quantization_range);
    static dsGetCurrentOutputSettings_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetCurrentOutputSettings_t) dlsym(dllib, "dsGetCurrentOutputSettings");
            if (func) {
                INT_DEBUG("dsGetCurrentOutputSettings_t(int, dsHDRStandard_t*, dsDisplayMatrixCoefficients_t*, dsDisplayColorSpace_t*, unsigned int*, dsDisplayQuantizationRange_t*) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetCurrentOutputSettings_t(int, dsHDRStandard_t*, dsDisplayMatrixCoefficients_t*, dsDisplayColorSpace_t*, unsigned int*, dsDisplayQuantizationRange_t*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsCurrentOutputSettings_t* param = (dsCurrentOutputSettings_t*)arg;

    if (func != 0) {
        param->result = func(param->handle, &param->video_eotf, &param->matrix_coefficients, &param->color_space, &param->color_depth, &param->quantization_range);
    }
    else {
        param->color_space = dsDISPLAY_COLORSPACE_UNKNOWN;
        param->color_depth = 0;
        param->matrix_coefficients = dsDISPLAY_MATRIXCOEFFICIENT_UNKNOWN;
        param->video_eotf = dsHDRSTANDARD_NONE;
        param->quantization_range = dsDISPLAY_QUANTIZATIONRANGE_UNKNOWN;
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsIsHDCPEnabled(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);
    
    dsVideoPortIsHDCPEnabledParam_t *param = (dsVideoPortIsHDCPEnabledParam_t *)arg;
    dsIsHDCPEnabled(param->handle, &param->enabled);

#if 0
    if(param->enabled){
    	INT_DEBUG("isHDCP =true !!!!!!..\r\n");
	}
    else{
    	INT_DEBUG("isHDCP =false !!!!!!..\r\n");
    }
#endif

    IARM_BUS_Unlock(lock);
	
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsIsDisplayConnected(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);
   
	dsVideoPortIsDisplayConnectedParam_t *param = (dsVideoPortIsDisplayConnectedParam_t *)arg;
    dsIsDisplayConnected(param->handle,&param->connected);

	IARM_BUS_Unlock(lock);
	
	return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsIsDisplaySurround(void *arg)
{

#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    INT_DEBUG("dsSRV::_dsIsDisplaySurround \r\n");

    typedef dsError_t (*dsIsDisplaySurround_t)(intptr_t handle, bool *surround);
    static dsIsDisplaySurround_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsIsDisplaySurround_t) dlsym(dllib, "dsIsDisplaySurround");
            if (func) {
                INT_DEBUG("dsIsDisplaySurround_t(int, bool*) is defined and loaded\r\n");
            }   
            else {
                INT_INFO("dsIsDisplaySurround_t(int, bool*) is not defined\r\n");
            }   
            dlclose(dllib);
        }   
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }   
    }   

	dsVideoPortIsDisplaySurroundParam_t *param = (dsVideoPortIsDisplaySurroundParam_t *)arg;

    if (func != 0) {
        dsError_t ret = func(param->handle, &param->surround);
        INT_INFO("dsSRV ::isDisplaySurround() returns %d %d\r\n", ret, param->surround);
    }
    else {
        param->surround = false;
    }

	IARM_BUS_Unlock(lock);
	
	return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetSurroundMode(void *arg)
{

#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    INT_DEBUG("dsSRV::_dsGetSurroundMode \r\n");

    typedef dsError_t (*dsGetSurroundMode_t)(intptr_t handle, int *surround);
    static dsGetSurroundMode_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetSurroundMode_t) dlsym(dllib, "dsGetSurroundMode");
            if (func) {
                INT_DEBUG("dsGetSurroundMode_t(int, int*) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetSurroundMode_t(int, int*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

	dsVideoPortGetSurroundModeParam_t *param = (dsVideoPortGetSurroundModeParam_t *)arg;

    if (func != 0) {
        dsError_t ret = func(param->handle, &param->surround);
        INT_INFO("dsSRV ::_dsGetSurroundMode() returns %d %d\r\n", ret, param->surround);
    }
    else {
        param->surround = dsSURROUNDMODE_NONE;
    }

	IARM_BUS_Unlock(lock);
	return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsEnableVideoPort(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);
   
	dsVideoPortSetEnabledParam_t *param = (dsVideoPortSetEnabledParam_t *)arg;
    dsEnableVideoPort(param->handle, param->enabled);
   
    IARM_BUS_Unlock(lock);
	
	return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetResolution(void *arg)
{
    _DEBUG_ENTER();
   
	IARM_BUS_Lock(lock);

   std::string _Resolution(DEFAULT_RESOLUTION);
   if (PROFILE_TV == profileType)
   {
        INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
        _Resolution = DEFAULT_RESOLUTION_2160P;
   }
   else
   {
        INT_INFO("[%d][%s]: its Other Profile\r\n", __LINE__, __FUNCTION__);
        _Resolution = DEFAULT_RESOLUTION_1080P;    
   }
	dsVideoPortGetResolutionParam_t *param = (dsVideoPortGetResolutionParam_t *)arg;
	dsVideoPortResolution_t *resolution = &param->resolution;	

	dsVideoPortType_t _VPortType = _GetVideoPortType(param->handle);

	if (_VPortType == dsVIDEOPORT_TYPE_HDMI ||
         _VPortType == dsVIDEOPORT_TYPE_INTERNAL)
	{
		if(param->toPersist)
		{
			_Resolution = device::HostPersistence::getInstance().getProperty("HDMI0.resolution",_Resolution);
			INT_INFO("Reading HDMI  persistent resolution %s\r\n",_Resolution.c_str());
		}
		else{
			dsError_t error = dsGetResolution(param->handle,resolution);
			if(error == dsERR_NONE) {
				_Resolution = resolution->name;
				INT_DEBUG("ResOverride platform reported resolution is: %s. Cached resolution is: %s\r\n",_Resolution.c_str(), _dsHDMIResolution.c_str());
			}
			else {
				_Resolution = _dsHDMIResolution;
                        }
		}
	}
	else if (_VPortType == dsVIDEOPORT_TYPE_COMPONENT)
	{
		if(param->toPersist){
			#ifdef HAS_ONLY_COMPOSITE
				_Resolution = device::HostPersistence::getInstance().getProperty("Baseband0.resolution",_Resolution);
			#else
				_Resolution = device::HostPersistence::getInstance().getProperty("COMPONENT0.resolution",_Resolution);
			#endif
			INT_DEBUG("Reading Component persistent resolution %s\r\n",_Resolution.c_str());
		}
		else{
			_Resolution = _dsCompResolution; 
		}
	}
        else if (_VPortType == dsVIDEOPORT_TYPE_BB )
        {
                if(param->toPersist){
                        _Resolution = device::HostPersistence::getInstance().getProperty("Baseband0.resolution",_Resolution);
                        INT_DEBUG("Reading BB persistent resolution %s\r\n",_Resolution.c_str());
                }
                else{
                        _Resolution = _dsBBResolution;
                }
        }
        else if (_VPortType == dsVIDEOPORT_TYPE_RF )
        {
                if(param->toPersist){
                        _Resolution = device::HostPersistence::getInstance().getProperty("RF0.resolution",_Resolution);
                        INT_DEBUG("Reading RF persistent resolution %s\r\n",_Resolution.c_str());
                }
                else{
                        _Resolution = _dsRFResolution;
                }
        }
        strncpy(resolution->name, _Resolution.c_str(), sizeof(resolution->name));
     	INT_INFO("%s _VPortType:%d  resolution::%s \n",__FUNCTION__,_VPortType,resolution->name);
	IARM_BUS_Unlock(lock);
	
	return IARM_RESULT_SUCCESS;
}


IARM_Result_t _dsSetResolution(void *arg)
{
    _DEBUG_ENTER();
	dsError_t ret = dsERR_NONE;
    
	IARM_BUS_Lock(lock);

	dsVideoPortSetResolutionParam_t *param = (dsVideoPortSetResolutionParam_t *)arg;
	if (param != NULL)   //CID:82753 - Reverse_inull
        {
		dsVideoPortType_t _VPortType = _GetVideoPortType(param->handle);
		bool isConnected = 0;
		dsIsDisplayConnected(param->handle,&isConnected);
		if(!isConnected)
		{
		    INT_INFO("Port _VPortType:%d  not connected..Ignoring Resolution Request------\r\n",_VPortType);
		    ret = dsERR_OPERATION_NOT_SUPPORTED;
		    param->result = ret;
		    IARM_BUS_Unlock(lock);
		    return IARM_RESULT_SUCCESS;
		}
	
		dsVideoPortResolution_t resolution = param->resolution;
		std::string resolutionName(resolution.name);
                INT_DEBUG("Resolution Requested ..%s \r\n",resolution.name);

		if(force_disable_4K)
		{
			std::size_t location = resolutionName.find("2160");
			if(std::string::npos != location)
			{
				//Trying to set 4K resolution when it's disabled. This cannot be allowed.
				INT_INFO("Error! Cannot set 4K resolution. Support for 4K is disabled.\n");
                                ret = dsERR_OPERATION_NOT_SUPPORTED;
                                param->result = ret;
				IARM_BUS_Unlock(lock);
				return IARM_RESULT_SUCCESS;
			}
		}
		/* * Check the Platform Resolution 
		   * If the platform Resolution is same as requested , Do not set new resolution
		   * Needed to avoid setting resolution during multiple hot plug  
		 */
		IARM_BUS_Unlock(lock);
                dsEdidIgnoreParam_t ignoreEdidParam;
                memset(&ignoreEdidParam,0,sizeof(ignoreEdidParam));
                ignoreEdidParam.handle = _VPortType;
                _dsGetIgnoreEDIDStatus(&ignoreEdidParam);
		bool IsIgnoreEdid = ignoreEdidParam.ignoreEDID;
		IARM_BUS_Lock(lock);
		INT_DEBUG("ResOverride _dsSetResolution IsIgnoreEdid:%d\n", IsIgnoreEdid);
		//IsIgnoreEdid is true platform will take care of current resolution cache.
		if (!IsIgnoreEdid) {
			dsVideoPortResolution_t platresolution;
			memset(platresolution.name,'\0',sizeof(platresolution.name));
			dsGetResolution(param->handle,&platresolution);
			INT_INFO("Resolution Requested ..%s Platform Resolution - %s\r\n",resolution.name,platresolution.name);
			if ((strcmp(resolution.name,platresolution.name) == 0 ))
			{
			
				INT_INFO("Same Resolution ..Ignoring Resolution Request------\r\n");
				_dsHDMIResolution = platresolution.name;
				/*!< Persist Resolution Settings */
				persistResolution(param);
				param->result = ret;
				IARM_BUS_Unlock(lock);
				return IARM_RESULT_SUCCESS;
			}
		}
		/*!< Resolution Pre Change Event  - IARM_BUS_DSMGR_EVENT_RES_POSTCHANGE */
		_dsVideoPortPreResolutionCall(&param->resolution);

		/*!< Set Platform Resolution  */
		ret = dsSetResolution(param->handle, &param->resolution);
		
		/*!< Resolution Post Change Event  - IARM_BUS_DSMGR_EVENT_RES_POSTCHANGE */
		_dsSendVideoPortPostResolutionCall(&param->resolution);
		
		if (ret == dsERR_NONE)
		{
			/*!< Persist Resolution Settings */
			persistResolution(param);
		}
		param->result = ret;
	}
		
    IARM_BUS_Unlock(lock);
	
	return IARM_RESULT_SUCCESS;
}

dsDisplayColorDepth_t getPersistentColorDepth ()
{
    dsDisplayColorDepth_t _colorDepth = DEFAULT_COLOR_DEPTH;
    std::string strColorDept = std::to_string (_colorDepth);
    strColorDept = device::HostPersistence::getInstance().getProperty("HDMI0.colorDepth", strColorDept);
    try {
       _colorDepth = (dsDisplayColorDepth_t)stoi (strColorDept);
    }
    catch (...) {
        INT_INFO("Reading HDMI  persistent color dept %s conversion failed\r\n", strColorDept.c_str());
    }
    INT_INFO("Reading HDMI  persistent color dept %d\r\n", _colorDepth);
    return _colorDepth;
}

dsError_t handleDsGetPreferredColorDepth(intptr_t handle, dsDisplayColorDepth_t *colorDepth, bool persist)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif

    dsError_t ret = dsERR_GENERAL;
    typedef dsError_t (*dsGetPreferredColorDepth_t)(intptr_t handle, dsDisplayColorDepth_t *colorDepth);
    static dsGetPreferredColorDepth_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetPreferredColorDepth_t) dlsym(dllib, "dsGetPreferredColorDepth");
            if (func) {
                INT_DEBUG("dsGetPreferredColorDepth(intptr_t handle, dsDisplayColorDepth_t *colorDepth, bool persist ) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetPreferredColorDepth(intptr_t handle, dsDisplayColorDepth_t *colorDepth, bool persist ) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }


    if (func != 0) {
        ret = func(handle, colorDepth);
    }
    else {
        INT_INFO("%s:%d not able to load funtion func:%p\r\n",__FUNCTION__,__LINE__, func);
        ret = dsERR_GENERAL;
    }
    return ret;
}

IARM_Result_t _dsGetPreferredColorDepth(void *arg)
{
    errno_t rc = -1;
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    dsDisplayColorDepth_t _colorDepth = DEFAULT_COLOR_DEPTH;
    dsPreferredColorDepthParam_t *param = (dsPreferredColorDepthParam_t *)arg;
    dsDisplayColorDepth_t *pcolorDepth = &param->colorDepth;

    dsVideoPortType_t _VPortType = _GetVideoPortType(param->handle);

    if (_VPortType == dsVIDEOPORT_TYPE_HDMI)
    {
        if(param->toPersist)
        {
            _colorDepth = getPersistentColorDepth ();
        }
        else{
            //Get actual color depth here.
            dsError_t error = handleDsGetPreferredColorDepth (param->handle, &_colorDepth, false);
            if(error == dsERR_NONE) {
                INT_DEBUG("ColorDepthOverride platform reported color dept is: 0x%x. Cached color dept is: 0x%x\r\n", _colorDepth, hdmiColorDept);
            }
            else {
                _colorDepth = hdmiColorDept;
            }
        }
    }
    else {
        INT_INFO("%s:%d not supported for video port: %d\r\n",__FUNCTION__, __LINE__, _VPortType);
    }
    *pcolorDepth =  _colorDepth;
    INT_INFO("%s _VPortType:%d  color dept::0x%x \n",__FUNCTION__,_VPortType, *pcolorDepth);
    param->result = dsERR_NONE;
    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;

}

dsDisplayColorDepth_t getBestSupportedColorDepth (intptr_t handle, dsDisplayColorDepth_t inColorDepth)
{
    unsigned int colorDepthCapability = 0;
    //Get sink color depth capabilities.
    int ret = handleDsColorDepthCapabilities (handle,&(colorDepthCapability));
    INT_INFO("dsColorDepthCapabilities returned: %d  colorDepthCapability: 0x%x\r\n",
		    ret, colorDepthCapability);

    if ((colorDepthCapability & inColorDepth) &&
          (inColorDepth!=dsDISPLAY_COLORDEPTH_AUTO)) {
        return inColorDepth;
    } else {
        //This condition happens if inColorDepth not supported by edid
	//Or it is in auto mode.
        INT_INFO("getBestSupportedColorDepth inColorDepth:0x%x not supported by edid searching in auto mode.\r\n", inColorDepth);
        if ((colorDepthCapability & dsDISPLAY_COLORDEPTH_12BIT)){
             return dsDISPLAY_COLORDEPTH_12BIT;
        } else if ((colorDepthCapability & dsDISPLAY_COLORDEPTH_10BIT)){
            return dsDISPLAY_COLORDEPTH_10BIT;
        }else if ((colorDepthCapability & dsDISPLAY_COLORDEPTH_8BIT)) {
            return dsDISPLAY_COLORDEPTH_8BIT;
        } else {
            //Not expecting here.
        }
	//if none of the edid supported color mode supports RDK defined vaules return
	//widely accepted default value.
        return dsDISPLAY_COLORDEPTH_8BIT;
    }
}

dsError_t handleDsSetPreferredColorDepth(intptr_t handle,dsDisplayColorDepth_t colorDepth, bool persist)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif

    dsError_t ret = dsERR_GENERAL;
    typedef dsError_t (*dsSetPreferredColorDepth_t)(intptr_t handle,dsDisplayColorDepth_t colorDepth);
    static dsSetPreferredColorDepth_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetPreferredColorDepth_t) dlsym(dllib, "dsSetPreferredColorDepth");
            if (func) {
                INT_DEBUG("dsSetPreferredColorDepth(intptr_t handle,dsDisplayColorDepth_t colorDepth, bool persist) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetPreferredColorDepth(intptr_t handle,dsDisplayColorDepth_t colorDepth, bool persist) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }


    if (func != 0) {
        ret = func(handle, colorDepth);
    }
    else {
        INT_INFO("%s:%d not able to load funtion func:%p\r\n",__FUNCTION__,__LINE__,  func);
        ret = dsERR_GENERAL;
    }
    return ret;
}

IARM_Result_t setPreferredColorDepth(void *arg)
{
    _DEBUG_ENTER();
    dsError_t ret = dsERR_NONE;


    dsPreferredColorDepthParam_t *param = (dsPreferredColorDepthParam_t *)arg;
    if (param != NULL)
    {
        dsVideoPortType_t _VPortType = _GetVideoPortType(param->handle);
        bool isConnected = 0;
        dsIsDisplayConnected(param->handle,&isConnected);
        if(!isConnected)
        {
            INT_INFO("Port _VPortType:%d  not connected..Ignoring Set color dept Request------\r\n",_VPortType);
            ret = dsERR_OPERATION_NOT_SUPPORTED;
            param->result = ret;
            return IARM_RESULT_SUCCESS;
        }

        dsDisplayColorDepth_t colorDepth = param->colorDepth;

	dsDisplayColorDepth_t platColorDept;

	//Get actual color depth
        handleDsGetPreferredColorDepth (param->handle,&platColorDept, false);
        INT_DEBUG("Color dept Requested ..0x%x Platform color dept - 0x%x\r\n",colorDepth,platColorDept);
        if (colorDepth == platColorDept)
        {
            INT_INFO("Same color dept ..Ignoring color dept Request------\r\n");
            hdmiColorDept = platColorDept;
            /*!< Persist Resolution Settings */
            if(param->toPersist){
                std::string strColorDept = std::to_string (param->colorDepth);
                device::HostPersistence::getInstance().persistHostProperty("HDMI0.colorDepth", strColorDept);
            }
            param->result = ret;
            return IARM_RESULT_SUCCESS;
        }

        //Getting the best supported color depth based on i/p color depth and edid support.
	dsDisplayColorDepth_t colorDepthToSet = getBestSupportedColorDepth (param->handle, param->colorDepth);

        /*!< Set Platform color depth  */
        ret = handleDsSetPreferredColorDepth (param->handle, colorDepthToSet, param->toPersist);
        if (ret == dsERR_NONE)
        {
            /*!< Persist Resolution Settings */
            hdmiColorDept = param->colorDepth;
            if(param->toPersist){
                //Persist the user selected colordepth
                std::string strColorDept = std::to_string (param->colorDepth);
                device::HostPersistence::getInstance().persistHostProperty("HDMI0.colorDepth", strColorDept);
            }
	}
        param->result = ret;
    }
    return IARM_RESULT_SUCCESS;

}

IARM_Result_t _dsSetPreferredColorDepth(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);
    IARM_Result_t ret = setPreferredColorDepth (arg);
    IARM_BUS_Unlock(lock);
    return ret;

}

dsError_t handleDsColorDepthCapabilities(intptr_t handle, unsigned int *colorDepthCapability )
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif

    dsError_t ret = dsERR_GENERAL;
    typedef dsError_t (*dsColorDepthCapabilities_t)(intptr_t handle, unsigned int *colorDepthCapability);
    static dsColorDepthCapabilities_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsColorDepthCapabilities_t) dlsym(dllib, "dsColorDepthCapabilities");
            if (func) {
                INT_DEBUG("dsColorDepthCapabilities(intptr_t handle, unsigned int *colorDepthCapability ) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsColorDepthCapabilities(intptr_t handle, unsigned int *colorDepthCapability ) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }


    if (func != 0) {
        ret = func(handle, colorDepthCapability);
    }
    else {
        INT_INFO("%s:%d not able to load funtion func:%p\r\n",__FUNCTION__,__LINE__,  func);
        ret = dsERR_GENERAL;
    }
    return ret;
}

IARM_Result_t _dsColorDepthCapabilities(void *arg)
{
    _DEBUG_ENTER();
    dsError_t ret = dsERR_NONE;
    IARM_BUS_Lock(lock);
    dsColorDepthCapabilitiesParam_t *param = (dsColorDepthCapabilitiesParam_t *)arg;
    ret = handleDsColorDepthCapabilities (param->handle,&(param->colorDepthCapability));
    INT_INFO("dsColorDepthCapabilities returned:%d  colorDepthCapability: 0x%x\r\n",
		    ret, param->colorDepthCapability);
    //Add auto by default
    param->colorDepthCapability = (param->colorDepthCapability|dsDISPLAY_COLORDEPTH_AUTO);

    param->result = ret;
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

//Call this function on hotplug scenario in dsmgr
//and resume from standby
void resetColorDepthOnHdmiReset(intptr_t handle)
{
    //get color from persist location. then set the value
    dsDisplayColorDepth_t colorDepth = getPersistentColorDepth ();
    INT_INFO("resetColorDepthOnHdmiReset: resetting colordepth:0x%x \r\n", colorDepth);

    dsPreferredColorDepthParam_t colorDepthParam;
    colorDepthParam.handle = handle;
    colorDepthParam.colorDepth = colorDepth;
    //this is just a reset no need to persist
    colorDepthParam.toPersist = false;

    //call this function outside the lock
    IARM_Result_t ret = setPreferredColorDepth ((void*)(&colorDepthParam));
}

IARM_Result_t _dsVideoPortTerm(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    m_isPlatInitialized--;
	
	if (0 == m_isPlatInitialized)
	{
		dsVideoPortTerm();
	}
    
	IARM_BUS_Unlock(lock);
 
	return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsEnableHDCP(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);
   
    dsError_t ret = dsERR_NONE;

    INT_INFO("Enable HDCP in Platform !! \r\n");

    dsEnableHDCPParam_t *param = (dsEnableHDCPParam_t *)arg;
    ret = dsEnableHDCP(param->handle, param->contentProtect, param->hdcpKey, param->keySize);
    param->rpcResult = ret;
     
    IARM_BUS_Unlock(lock);
	
	return IARM_RESULT_SUCCESS;
}


static int  _dsSendVideoPortPostResolutionCall(dsVideoPortResolution_t *resolution)
{
	dsError_t ret = dsERR_NONE;

	if (resolution == NULL)
	{
		ret = dsERR_INVALID_PARAM;
	}

	if (ret == dsERR_NONE){
		  IARM_Bus_DSMgr_EventData_t eventData;
		  IARM_Bus_CommonAPI_ResChange_Param_t param;

			switch(resolution->pixelResolution) {
			case dsVIDEO_PIXELRES_720x480:
				param.width =  720;
				param.height = 480;
			break;
			case dsVIDEO_PIXELRES_720x576:
				param.width =  720;
				param.height = 576;
			break;

			case dsVIDEO_PIXELRES_1280x720:
				param.width =  1280;
				param.height = 720;
			break;

			case dsVIDEO_PIXELRES_1366x768:
				param.width =  1366;
				param.height = 768;
			break;

			case dsVIDEO_PIXELRES_1920x1080:
				param.width =  1920;
				param.height = 1080;
			break;
            case dsVIDEO_PIXELRES_3840x2160:
                param.width =  3840;
                param.height = 2160;
                break;
            case dsVIDEO_PIXELRES_4096x2160:
                param.width =  4096;
                param.height = 2160;
                break;
			case dsVIDEO_PIXELRES_MAX: //to mute compiler warning
			default:
				param.width =  1280;
				param.height = 720;
			break;
			}
			eventData.data.resn.width = param.width;
			eventData.data.resn.height = param.height;
			IARM_BusDaemon_ResolutionPostchange(param);
			IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)IARM_BUS_DSMGR_EVENT_RES_POSTCHANGE,(void *)&eventData, sizeof(eventData));
		}
	return ret;
}
static int  _dsVideoPortPreResolutionCall(dsVideoPortResolution_t *resolution)
{
	dsError_t ret = dsERR_NONE;
	if (resolution == NULL)
	{
		ret = dsERR_INVALID_PARAM;
	}

	if (ret == dsERR_NONE){
		        IARM_Bus_DSMgr_EventData_t eventData;
			IARM_Bus_CommonAPI_ResChange_Param_t param;

			switch(resolution->pixelResolution) {
			case dsVIDEO_PIXELRES_720x480:
				param.width =  720;
				param.height = 480;
			break;
			case dsVIDEO_PIXELRES_720x576:
				param.width =  720;
				param.height = 576;
			break;

			case dsVIDEO_PIXELRES_1280x720:
				param.width =  1280;
				param.height = 720;
			break;

			case dsVIDEO_PIXELRES_1366x768:
				param.width =  1366;
				param.height = 768;
			break;

			case dsVIDEO_PIXELRES_1920x1080:
				param.width =  1920;
				param.height = 1080;
			break;
            case dsVIDEO_PIXELRES_3840x2160:
                param.width =  3840;
                param.height = 2160;
                break;
            case dsVIDEO_PIXELRES_4096x2160:
                param.width =  4096;
                param.height = 2160;
                break;
            case dsVIDEO_PIXELRES_MAX: //to mute compiler warning
			default:
				param.width =  1280;
				param.height = 720;
			break;
			}
			eventData.data.resn.width = param.width;
			eventData.data.resn.height = param.height;
			IARM_BusDaemon_ResolutionPrechange(param);
			IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)IARM_BUS_DSMGR_EVENT_RES_PRECHANGE,(void *)&eventData, sizeof(eventData));
		}
	return ret;
}

/*HDCP Status  Call back */
void _dsHdcpCallback (intptr_t handle, dsHdcpStatus_t status)
{
	IARM_Bus_DSMgr_EventData_t hdcp_eventData;

	if (handle == NULL_HANDLE)
	{
		INT_INFO("Err:HDMI Hot plug back has NULL Handle... !!!!!!..\r\n");
	}
	switch(status)
	{
		case dsHDCP_STATUS_AUTHENTICATED:
			INT_INFO("DS HDCP Authenticated Event!!!!!!..\r\n");
			hdcp_eventData.data.hdmi_hdcp.hdcpStatus =  dsHDCP_STATUS_AUTHENTICATED;
			_hdcpStatus = status;
			break;

		case dsHDCP_STATUS_AUTHENTICATIONFAILURE:
			INT_INFO("DS HDCP Failure Event!!!!!!..\r\n");
			 hdcp_eventData.data.hdmi_hdcp.hdcpStatus =  dsHDCP_STATUS_AUTHENTICATIONFAILURE;
			_hdcpStatus = status;
			break;
		/* Based on discussion with Steve, we may handle the unpowered and unauthenticated 
		cases in a different manner. Logging this events for now.*/
		case dsHDCP_STATUS_UNPOWERED:
		case dsHDCP_STATUS_UNAUTHENTICATED:
		default:
			INT_INFO("HDCP Event Status from HAL is ...%d\n",status);
			hdcp_eventData.data.hdmi_hdcp.hdcpStatus = _hdcpStatus = status;
			break;
	}

	if (dsHDCP_STATUS_INPROGRESS != _hdcpStatus) {
		INT_INFO("Hdmi sync HDCP Protocol new protocol status is : %d !!!!!!!! ..\r\n", _hdcpStatus);
		_dsSyncHdmiStatus(DS_HDMI_TAG_HDCPSTATUS, _hdcpStatus);
		if (dsHDCP_STATUS_AUTHENTICATED == _hdcpStatus) {
			dsVideoPortGetHDCPProtocolVersion_t param;
			param.handle = dsGetDefaultPortHandle();
			param.protocolVersion = dsHDCP_VERSION_1X;
			dsGetHDCPCurrentProtocolInternal (&param);
			INT_INFO("Hdmi sync HDCP Protocol new protocol verision is : %d !!!!!!!! ..\r\n", param.protocolVersion);
			_dsSyncHdmiStatus(DS_HDMI_TAG_HDCPVERSION, param.protocolVersion);
		} else {
			INT_INFO("Hdmi sync HDCP Protocol new protocol verision is : %d !!!!!!!! ..\r\n", dsHDCP_VERSION_1X);
			_dsSyncHdmiStatus(DS_HDMI_TAG_HDCPVERSION, dsHDCP_VERSION_1X);
		}
	}
	
	IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)IARM_BUS_DSMGR_EVENT_HDCP_STATUS,(void *)&hdcp_eventData, sizeof(hdcp_eventData));
}

IARM_Result_t _dsGetHDCPStatus (void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);
    
    dsVideoPortGetHDCPStatus_t *param = (dsVideoPortGetHDCPStatus_t *)arg;

    if (param != NULL) {
      param->hdcpStatus = _hdcpStatus;
    }
   
    IARM_BUS_Unlock(lock);
	
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetHDCPProtocol (void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetHDCPProtocol_t)(intptr_t handle, dsHdcpProtocolVersion_t *protocolVersion);
    static dsGetHDCPProtocol_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetHDCPProtocol_t) dlsym(dllib, "dsGetHDCPProtocol");
            if (func) {
                INT_DEBUG("dsGetHDCPProtocol_t(int, dsHdcpProtocolVersion_t *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetHDCPProtocol_t(int, dsHdcpProtocolVersion_t *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

	dsVideoPortGetHDCPProtocolVersion_t *param = (dsVideoPortGetHDCPProtocolVersion_t *)arg;

    if (func != 0) {
        dsError_t ret = func(param->handle, &param->protocolVersion);
    }
    else {
        param->protocolVersion = dsHDCP_VERSION_1X;
    }

	IARM_BUS_Unlock(lock);

	return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetHDCPReceiverProtocol (void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetHDCPReceiverProtocol_t)(intptr_t handle, dsHdcpProtocolVersion_t *protocolVersion);
    static dsGetHDCPReceiverProtocol_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetHDCPReceiverProtocol_t) dlsym(dllib, "dsGetHDCPReceiverProtocol");
            if (func) {
                INT_DEBUG("dsGetHDCPReceiverProtocol_t(int, dsHdcpProtocolVersion_t *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetHDCPReceiverProtocol_t(int, dsHdcpProtocolVersion_t *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

	dsVideoPortGetHDCPProtocolVersion_t *param = (dsVideoPortGetHDCPProtocolVersion_t *)arg;

    if (func != 0) {
        dsError_t ret = func(param->handle, &param->protocolVersion);
    }
    else {
        param->protocolVersion = dsHDCP_VERSION_1X;
    }

	IARM_BUS_Unlock(lock);

	return IARM_RESULT_SUCCESS;
}

IARM_Result_t dsGetHDCPCurrentProtocolInternal (dsVideoPortGetHDCPProtocolVersion_t *param)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();

    if (!param) {
        INT_INFO("%s argument is empty : %p\r\n", __func__, param);
        return IARM_RESULT_INVALID_PARAM;
    }

    typedef dsError_t (*dsGetHDCPCurrentProtocol_t)(intptr_t handle, dsHdcpProtocolVersion_t *protocolVersion);
    static dsGetHDCPCurrentProtocol_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetHDCPCurrentProtocol_t) dlsym(dllib, "dsGetHDCPCurrentProtocol");
            if (func) {
                INT_DEBUG("dsGetHDCPCurrentProtocol_t(int, dsHdcpProtocolVersion_t *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetHDCPCurrentProtocol_t(int, dsHdcpProtocolVersion_t *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    if (func != 0) {
        dsError_t ret = func(param->handle, &param->protocolVersion);
    }
    else {
        param->protocolVersion = dsHDCP_VERSION_1X;
    }

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetHDCPCurrentProtocol (void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    dsVideoPortGetHDCPProtocolVersion_t *param = (dsVideoPortGetHDCPProtocolVersion_t *)arg;
    IARM_Result_t ret = dsGetHDCPCurrentProtocolInternal (param);

    IARM_BUS_Unlock(lock);

    return ret;
}

IARM_Result_t _dsGetTVHDRCapabilities(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetTVHDRCapabilitiesFunc_t)(intptr_t handle, int *capabilities);
    static dsGetTVHDRCapabilitiesFunc_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetTVHDRCapabilitiesFunc_t)dlsym(dllib, "dsGetTVHDRCapabilities");
            if (func) {
                INT_DEBUG("dsGetTVHDRCapabilities() is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetTVHDRCapabilities() is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }
    dsGetHDRCapabilitiesParam_t *param = (dsGetHDRCapabilitiesParam_t *)arg;
    if(0 != func) {
        param->result = func(param->handle, &param->capabilities);
    }
    else {
        param->capabilities = dsHDRSTANDARD_NONE;
    }

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsSupportedTvResolutions(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSupportedTvResolutionsFunc_t)(intptr_t handle,int *resolutions);
    static dsSupportedTvResolutionsFunc_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSupportedTvResolutionsFunc_t)dlsym(dllib, "dsSupportedTvResolutions");
            if (func) {
                INT_DEBUG("dsSupportedTvResolutions() is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSupportedTvResolutions() is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }
    dsSupportedResolutionParam_t *param = (dsSupportedResolutionParam_t *)arg;
    if(0 != func) {
        param->result = func(param->handle, &param->resolutions);
    }
    else {
        param->resolutions = 0;
    }

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}


static dsVideoPortType_t _GetVideoPortType(intptr_t handle)
{
    int numPorts,i;
    intptr_t halhandle = 0;
    
    numPorts = dsUTL_DIM(kSupportedPortTypes);
    for(i=0; i< numPorts; i++)
    {
		dsGetVideoPort(kPorts[i].id.type, kPorts[i].id.index, &halhandle);
		if (handle == halhandle)
		{
			return kPorts[i].id.type;
		}
	}
	INT_ERROR("Error: The Requested Video Port is not part of Platform Port Configuration \r\n");
	return dsVIDEOPORT_TYPE_MAX;
}


static void persistResolution(dsVideoPortSetResolutionParam_t *param)
{
	dsVideoPortResolution_t resolution = param->resolution;
	std::string resolutionName(resolution.name);

	try
	{
		dsVideoPortType_t _VPortType = _GetVideoPortType(param->handle);
		if (_VPortType == dsVIDEOPORT_TYPE_HDMI ||
             _VPortType == dsVIDEOPORT_TYPE_INTERNAL)
		{
			if(param->toPersist){
				device::HostPersistence::getInstance().persistHostProperty("HDMI0.resolution",resolutionName);
			}
			
			INT_INFO("Set Resolution on HDMI Port!!!!!!..\r\n");
			_dsHDMIResolution = resolutionName;

			if (false == IsCompatibleResolution(resolution.pixelResolution,getPixelResolution(_dsCompResolution)))
			{
				INT_INFO("HDMI Resolution is not Compatible with Analog ports..\r\n");
				_dsCompResolution = getCompatibleResolution(&resolution);
				INT_INFO("New Compatible resolution is %s  \r\n",_dsCompResolution.c_str());
				
				if(param->forceCompatible)
				{
					#ifdef HAS_ONLY_COMPOSITE
						device::HostPersistence::getInstance().persistHostProperty("Baseband0.resolution",_dsCompResolution);
					#else
						device::HostPersistence::getInstance().persistHostProperty("COMPONENT0.resolution",_dsCompResolution);
					#endif	
				}
			}
			else
			{
				INT_INFO("HDMI and Analog Ports Resolutions are  Compatible \r\n");
			}

		}
		else if (_VPortType == dsVIDEOPORT_TYPE_COMPONENT)
		{

			if(param->toPersist){
				#ifdef HAS_ONLY_COMPOSITE
					device::HostPersistence::getInstance().persistHostProperty("Baseband0.resolution",resolutionName);
				#else
					device::HostPersistence::getInstance().persistHostProperty("COMPONENT0.resolution",resolutionName);
				#endif	
			}

			INT_DEBUG("Set Resolution on Component/Composite Ports!!!!!!..\r\n");
			_dsCompResolution = resolutionName;
			if (false == IsCompatibleResolution(resolution.pixelResolution,getPixelResolution(_dsHDMIResolution)))
			{
				INT_INFO("HDMI Resolution is not Compatible with Analog ports..\r\n");
				
				_dsHDMIResolution = getCompatibleResolution(&resolution);
				if (_dsHDMIResolution.compare("480i") == 0)
					_dsHDMIResolution = "480p";

				INT_INFO("New Compatible resolution is %s  \r\n",_dsHDMIResolution.c_str());
				if(param->forceCompatible)
				{
					device::HostPersistence::getInstance().persistHostProperty("HDMI0.resolution",_dsHDMIResolution);
				}
			}
			else
			{
				INT_DEBUG("HDMI and Analog Ports Resolutions are  Compatible \r\n");
			}
		}
                else if (_VPortType == dsVIDEOPORT_TYPE_BB)
                {

                        if(param->toPersist){
                               device::HostPersistence::getInstance().persistHostProperty("Baseband0.resolution",resolutionName);
                        }

                        INT_DEBUG("Set Resolution on Composite Ports!!!!!!..\r\n");
                        _dsBBResolution = resolutionName;
                        if (false == IsCompatibleResolution(resolution.pixelResolution,getPixelResolution(_dsHDMIResolution)))
                        {
                                INT_INFO("HDMI Resolution is not Compatible with Analog ports..\r\n");

                                _dsHDMIResolution = getCompatibleResolution(&resolution);
                                if (_dsHDMIResolution.compare("480i") == 0)
                                        _dsHDMIResolution = "480p";

                                INT_INFO("New Compatible resolution is %s  \r\n",_dsHDMIResolution.c_str());
                        }
                        else
                        {
                                INT_DEBUG("HDMI and Analog Ports Resolutions are  Compatible \r\n");
                        }
                }
                else if (_VPortType == dsVIDEOPORT_TYPE_RF)
                {

                        if(param->toPersist){
                               device::HostPersistence::getInstance().persistHostProperty("RF0.resolution",resolutionName);
                        }

                        INT_DEBUG("Set Resolution on RF Ports!!!!!!..\r\n");
                        _dsRFResolution = resolutionName;
                        if (false == IsCompatibleResolution(resolution.pixelResolution,getPixelResolution(_dsHDMIResolution)))
                        {
                                INT_INFO("HDMI Resolution is not Compatible with Analog ports..\r\n");

                                _dsHDMIResolution = getCompatibleResolution(&resolution);
                                if (_dsHDMIResolution.compare("480i") == 0)
                                        _dsHDMIResolution = "480p";

                                INT_INFO("New Compatible resolution is %s  \r\n",_dsHDMIResolution.c_str());
                        }
                        else
                        {
                                INT_DEBUG("HDMI and Analog Ports Resolutions are  Compatible \r\n");
                        }
                }
	}
	catch(...) 
	{
		INT_ERROR("Error in Persisting the Video Resolution..... \r\n");
	}
}


#ifdef HAS_INIT_RESN_SETTINGS
IARM_Result_t _dsInitResolution(void *arg)
{
	_DEBUG_ENTER();
	dsError_t ret = dsERR_NONE;
	IARM_BUS_Lock(lock);

	dsVideoPortSetResolutionParam_t *param = (dsVideoPortSetResolutionParam_t *)arg;	
	if (param == NULL)
	{
		return IARM_RESULT_INVALID_STATE;
	}

	ret = dsInitResolution(&param->resolution);
	if (ret == dsERR_NONE)
	{
		persistResolution(param);
	}

	IARM_BUS_Unlock(lock);	
	return IARM_RESULT_SUCCESS;
}
#endif


static  std::string getCompatibleResolution(dsVideoPortResolution_t *SrcResn)
{
   dsError_t ret = dsERR_NONE;
   std::string resolution("720p");

   if (SrcResn == NULL)
   {
      ret = dsERR_INVALID_PARAM;
   }

   if (ret == dsERR_NONE){
      switch(SrcResn->pixelResolution) {
      
         case dsVIDEO_PIXELRES_720x480:
          	return resolution.assign(SrcResn->name);
         break;

         case dsVIDEO_PIXELRES_1280x720:
         case dsVIDEO_PIXELRES_1366x768:
         case dsVIDEO_PIXELRES_1920x1080:           
         case dsVIDEO_PIXELRES_3840x2160:
         case dsVIDEO_PIXELRES_4096x2160:
         case dsVIDEO_PIXELRES_MAX: 
         default:
       		  return resolution.assign(kResolutions[kDefaultResIndex].name);
         break;
      }
   }
   return resolution;
}

static bool  IsCompatibleResolution(dsVideoResolution_t pixelResolution1,dsVideoResolution_t pixelResolution2)
{
   bool bret = false;

 	if( pixelResolution1 == pixelResolution2) {
            bret = true;
        }
	else if((IsHDCompatible(pixelResolution1)) && (IsHDCompatible(pixelResolution2))) {
            bret = true;
         }
    return  bret;
}

static dsVideoResolution_t getPixelResolution(std::string &resolution )
{
  	dsVideoPortResolution_t *Resn = &kResolutions[kDefaultResIndex]; 
	
	for (unsigned int i = 0; i < dsUTL_DIM(kResolutions); i++)
	{
		Resn = &kResolutions[i];
		if (resolution.compare(Resn->name) == 0 )
		{
			break;
		}
	}
	return Resn->pixelResolution;
}


IARM_Result_t _dsSetForceDisable4K(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);
    
    dsForceDisable4KParam_t *param = (dsForceDisable4KParam_t *)arg;
	param->result = dsERR_NONE;

	force_disable_4K = param->disable;
	if(force_disable_4K)
	{
		device::HostPersistence::getInstance().persistHostProperty("VideoDevice.force4KDisabled","true");
	}
	else
	{
		device::HostPersistence::getInstance().persistHostProperty("VideoDevice.force4KDisabled","false");
	}

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}


IARM_Result_t _dsSetForceHDRMode(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);
    dsForceHDRModeParam_t *param = (dsForceHDRModeParam_t *) arg;

    typedef dsError_t (*dsSetForceHDRMode_t)(intptr_t handle, dsHDRStandard_t mode);
    static dsSetForceHDRMode_t func = NULL;
    if (func == NULL) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib != NULL) {
            func = (dsSetForceHDRMode_t) dlsym(dllib, "dsSetForceHDRMode");
            if (func != NULL) {
                INT_DEBUG("dsSRV: dsSetForceHDRMode(intptr_t handle, dsHDRStandard_t mode ) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSRV: dsSetForceHDRMode(intptr_t handle, dsHDRStandard_t mode) is not defined\r\n");
            }
            dlclose(dllib);  //CID:83238 - Resource leak
        }
        else {
            INT_ERROR("dsSRV: Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    if (param != NULL) {
        param->result = dsERR_GENERAL;

        if (func != NULL) {
            param->result = func(param->handle, param->hdrMode);
        }
    }

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetForceDisable4K(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);
    
    dsForceDisable4KParam_t *param = (dsForceDisable4KParam_t *)arg;
	param->result = dsERR_NONE;

	param->disable = force_disable_4K;
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsIsOutputHDR(void *arg)
{
    dsIsOutputHDRParam_t *param = (dsIsOutputHDRParam_t*) arg;
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    INT_DEBUG("dsSRV::_dsIsOutputHDR\r\n");

    typedef dsError_t (*dsIsOutputHDR_t)(intptr_t handle, bool *hdr);
    static dsIsOutputHDR_t func = NULL;
    if (func == NULL) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib != NULL) {
            func = (dsIsOutputHDR_t) dlsym(dllib, "dsIsOutputHDR");
            if (func != NULL) {
                INT_DEBUG("dsSRV: dsIsOutputHDR(intptr_t handle, bool *hdr) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSRV: dsIsOutputHDR(intptr_t handle, bool *hdr) is not defined\r\n");
            }
	    dlclose(dllib);   //CID:83623 - Resource leak
        }
        else {
            INT_ERROR("dsSRV: Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    if (param != NULL) {
        param->result = dsERR_GENERAL;

        if (func != NULL) {
            param->result = func(param->handle, &param->hdr);
        }
    }


    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;

}

IARM_Result_t _dsResetOutputToSDR(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    INT_DEBUG("dsSRV::_dsResetOutputToSDR\r\n");

    typedef dsError_t (*dsResetOutputToSDR_t)();
    static dsResetOutputToSDR_t func = NULL;
    if (func == NULL) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib != NULL) {
            func = (dsResetOutputToSDR_t) dlsym(dllib, "dsResetOutputToSDR");
            if (func != NULL) {
                INT_DEBUG("dsSRV: dsResetOutputToSDR() is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSRV: dsResetOutputToSDR() is not defined\r\n");
            }
	    dlclose(dllib);  //CID:88069 - Resource leak
        }
        else {
            INT_ERROR("dsSRV: Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

        if (func != NULL) {
            dsError_t result = func();
        }


    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;

}

IARM_Result_t _dsSetHdmiPreference(void *arg)
{
    dsSetHdmiPreferenceParam_t *param = (dsSetHdmiPreferenceParam_t*) arg;
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    INT_DEBUG("dsSRV::_dsSetHdmiPreference\r\n");

    typedef dsError_t (*dsSetHdmiPreference_t)(intptr_t handle, dsHdcpProtocolVersion_t *hdcpCurrentProtocol);
    static dsSetHdmiPreference_t func = NULL;
    if (func == NULL) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib != NULL) {
            func = (dsSetHdmiPreference_t) dlsym(dllib, "dsSetHdmiPreference");
            if (func != NULL) {
                INT_DEBUG("dsSRV: dsSetHdmiPreference(intptr_t handle, dsHdcpProtocolVersion_t *hdcpCurrentProtocol) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSRV: dsSetHdmiPreference(intptr_t handle, dsHdcpProtocolVersion_t *hdcpCurrentProtocol) is not defined\r\n");
            }
	    dlclose(dllib);  //CID:83238 - Resource leak
        }
        else {
            INT_ERROR("dsSRV: Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    if (param != NULL) {
        param->result = dsERR_GENERAL;

        if (func != NULL) {
            param->result = func(param->handle, &param->hdcpCurrentProtocol);
        }
    }


    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;

}

IARM_Result_t _dsGetHdmiPreference(void *arg)
{
    dsGetHdmiPreferenceParam_t *param = (dsGetHdmiPreferenceParam_t*) arg;
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    INT_DEBUG("dsSRV::_dsGetHdmiPreference\r\n");

    typedef dsError_t (*dsGetHdmiPreference_t)(intptr_t handle, dsHdcpProtocolVersion_t *hdcpCurrentProtocol);
    static dsGetHdmiPreference_t func = NULL;
    if (func == NULL) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib != NULL) {
            func = (dsGetHdmiPreference_t) dlsym(dllib, "dsGetHdmiPreference");
            if (func != NULL) {
                INT_DEBUG("dsSRV: dsGetHdmiPreference(intptr_t handle, dsHdcpProtocolVersion_t *hdcpCurrentProtocol) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSRV: dsGetHdmiPreference(intptr_t handle, dsHdcpProtocolVersion_t *hdcpCurrentProtocol) is not defined\r\n");
            }
	    dlclose(dllib);   //CID:82165 - Resource leak
        }
        else {
            INT_ERROR("dsSRV: Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    if (param != NULL) {
        param->result = dsERR_GENERAL;

        if (func != NULL) {
            param->result = func(param->handle, &param->hdcpCurrentProtocol);
        }
    }


    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;

}

IARM_Result_t _dsSetBackgroundColor(void *arg)
{
    dsSetBackgroundColorParam_t *param = (dsSetBackgroundColorParam_t*) arg;
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSetBackgroundColor_t)(intptr_t handle, dsVideoBackgroundColor_t color);
    static dsSetBackgroundColor_t func = NULL;
    if (func == NULL) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib != NULL) {
            func = (dsSetBackgroundColor_t) dlsym(dllib, "dsSetBackgroundColor");
            if (func != NULL) {
                INT_DEBUG("dsSRV: dsError_t dsSetBackgroundColor(intptr_t handle, dsVideoBackgroundColor_t color)  is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSRV: dsError_t dsSetBackgroundColor(intptr_t handle, dsVideoBackgroundColor_t color) is not defined\r\n");
            }
	    dlclose(dllib);  //CID:86640 - Resource leak
        }
        else {
            INT_ERROR("dsSRV: Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    if (param != NULL && func != NULL) {
          func(param->handle, param->color);
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;

}
void _dsVideoFormatUpdateCB(dsHDRStandard_t videoFormat)
{
    IARM_Bus_DSMgr_EventData_t video_format_event_data;
    INT_INFO("%s: VideoOutPort format:%d \r\n", __FUNCTION__, videoFormat);
    video_format_event_data.data.VideoFormatInfo.videoFormat = videoFormat;

    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                           (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_VIDEO_FORMAT_UPDATE,
                           (void *)&video_format_event_data,
                           sizeof(video_format_event_data));
}

static dsError_t _dsVideoFormatUpdateRegisterCB (dsVideoFormatUpdateCB_t cbFun) {
    dsError_t eRet = dsERR_GENERAL;
    INT_DEBUG("%s: %d - Inside \n", __FUNCTION__, __LINE__);

    typedef dsError_t (*dsVideoFormatUpdateRegisterCB_t)(dsVideoFormatUpdateCB_t cbFunArg);
    static dsVideoFormatUpdateRegisterCB_t dsVideoFormatUpdateRegisterCBFun = 0;
    if (dsVideoFormatUpdateRegisterCBFun == 0) {
        INT_ERROR("%s: %d - dlerror: %s\n", __FUNCTION__, __LINE__, dlerror());
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsVideoFormatUpdateRegisterCBFun = (dsVideoFormatUpdateRegisterCB_t) dlsym(dllib, "dsVideoFormatUpdateRegisterCB");
            if(dsVideoFormatUpdateRegisterCBFun == 0) {
                INT_INFO("%s: dsVideoFormatUpdateRegisterCB is not defined %s\r\n", __FUNCTION__, dlerror());
                eRet = dsERR_GENERAL;
            }
            else {
                INT_DEBUG("%s: dsVideoFormatUpdateRegisterCB is loaded\r\n", __FUNCTION__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s: Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__, RDK_DSHAL_NAME, dlerror());
            eRet = dsERR_GENERAL;
        }
    }
    if (0 != dsVideoFormatUpdateRegisterCBFun) {
        eRet = dsVideoFormatUpdateRegisterCBFun (cbFun);
        INT_INFO("%s: dsVideoFormatUpdateRegisterCBFun registered\r\n", __FUNCTION__);
    }
    else {
        INT_INFO("%s: dsVideoFormatUpdateRegisterCBFun NULL\r\n", __FUNCTION__);
    }

    return eRet;
}

bool isComponentPortPresent()
{
    bool componentPortPresent = false;
    int numPorts,i;

    numPorts = dsUTL_DIM(kSupportedPortTypes);
    for(i=0; i< numPorts; i++)
    {
        if (kSupportedPortTypes[i] == dsVIDEOPORT_TYPE_COMPONENT)
        {
            componentPortPresent = true;;
        }
    }
    INT_INFO(" componentPortPresent :%d\n",componentPortPresent);
    return componentPortPresent;
}

IARM_Result_t _dsGetIgnoreEDIDStatus(void* arg)
{
    dsEdidIgnoreParam_t *param = (dsEdidIgnoreParam_t*) arg;
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);
    //Default status is false
    typedef dsError_t (*dsGetIgnoreEDIDStatus_t)(intptr_t handleArg, bool* statusArg);
    static dsGetIgnoreEDIDStatus_t func = NULL;
    if (func == NULL) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib != NULL) {
            func = (dsGetIgnoreEDIDStatus_t) dlsym(dllib, "dsGetIgnoreEDIDStatus");
            if (func != NULL) {
                INT_DEBUG("dsSRV: dsError_t dsGetIgnoreEDIDStatus(intptr_t handle, bool* status)  is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSRV: dsError_t dsGetIgnoreEDIDStatus(intptr_t handle, bool* status) is not defined\r\n");
            }
	    dlclose(dllib);
        }
        else {
            INT_ERROR("dsSRV: Opening RDK_DSHAL_NAME [%s] dsGetIgnoreEDIDStatus failed\r\n", RDK_DSHAL_NAME);
        }
    }

    if (func != NULL) {
          func(param->handle, &param->ignoreEDID);
    }
    INT_INFO("dsSRV: _dsGetIgnoreEDIDStatus status: %d\r\n", param->ignoreEDID);

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}


std::string getHdmiConnectionName(const int key)
{
    int i = 0;
    while (HdmiConnectionToStrMapping[i].name)
    {
        if (HdmiConnectionToStrMapping[i].tag == key)
            return HdmiConnectionToStrMapping[i].name;
        i++;
    }
    return "";
}

std::string getHdcpStatusName(const int key)
{
    int i = 0;
    while (HdmiStatusToStrMapping[i].name)
    {
        if (HdmiStatusToStrMapping[i].tag == key)
            return HdmiStatusToStrMapping[i].name;
        i++;
    }
    return "";
}

std::string getHdcpVersionName(const int key)
{
    int i = 0;
    while (HdmiVerToStrMapping[i].name)
    {
        if (HdmiVerToStrMapping[i].tag == key)
            return HdmiVerToStrMapping[i].name;
        i++;
    }
    return "";
}

void _dsSyncHdmiStatus(const std::string& key, int val) {
    std::vector<std::string> lines;
    std::ifstream statusFile(DS_HDMI_STATUS_FILE);
    std::string line;
    bool found = false;
	std::string value = "";
	
	if (0 == strncmp (key.c_str(), DS_HDMI_TAG_HOTPLUP, strlen(DS_HDMI_TAG_HOTPLUP))){
		value = getHdmiConnectionName (val);
	} else if (0 == strncmp (key.c_str(), DS_HDMI_TAG_HDCPSTATUS, strlen(DS_HDMI_TAG_HDCPSTATUS))){
		value = getHdcpStatusName (val);
	} else if (0 == strncmp (key.c_str(), DS_HDMI_TAG_HDCPVERSION, strlen(DS_HDMI_TAG_HDCPVERSION))){
		value = getHdcpVersionName (val);
	} else {
		INT_INFO("dsSRV: %s: unknown key is passed %s\r\n", __FUNCTION__, key);
	}
    // Read existing lines and remove the target line if it exists
    while (std::getline(statusFile, line)) {
        if (line.find(key + "=") == 0) {
            found = true; // Mark as found, will not add it back
        } else {
            lines.push_back(line);
        }
    }
    statusFile.close();

    // Add the new status line
    lines.push_back(key + "=" + value);

    // Write everything back to the file
    std::ofstream outFile(DS_HDMI_STATUS_FILE);
    for (const auto& l : lines) {
        outFile << l << std::endl;
    }
    outFile.close();

    if (found) {
        std::cout << "Updated " << key << " to " << value << std::endl;
    } else {
        std::cout << "Added " << key << " with value " << value << std::endl;
    }
}

intptr_t dsGetDefaultPortHandle()
{
    int numPorts,i;
    intptr_t halhandle = 0;
    numPorts = dsUTL_DIM(kSupportedPortTypes);
    for(i=0; i< numPorts; i++)
    {
        dsGetVideoPort(kPorts[i].id.type, kPorts[i].id.index, &halhandle);
        if (dsVIDEOPORT_TYPE_HDMI == kPorts[i].id.type ||
             dsVIDEOPORT_TYPE_INTERNAL == kPorts[i].id.type)
        {
            return halhandle;
        }
    }
    return halhandle;
}

/** @} */
/** @} */
