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
/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright ARRIS Enterprises, Inc. 2015.
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
 * limitations under
*/


/**
* @defgroup devicesettings
* @{
* @defgroup rpc
* @{
**/




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
#include <dlfcn.h>
#include "dsHdmiIn.h"
#include "dsRpc.h"
#include "dsTypes.h"
#include "dsserverlogger.h"
#include "dsMgr.h"

#include "iarmUtil.h"
#include "libIARM.h"
#include "libIBus.h"
#include "rfcapi.h"
#include "safec_lib.h"

#include "dsInternal.h"

#define direct_list_top(list) ((list))
#define IARM_BUS_Lock(lock) pthread_mutex_lock(&fpLock)
#define IARM_BUS_Unlock(lock) pthread_mutex_unlock(&fpLock)
#define TVSETTINGS_DALS_RFC_PARAM "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TvSettings.DynamicAutoLatency"

static bool isDalsEnabled = false;
static int m_isInitialized = 0;
static int m_isPlatInitialized=0;
static pthread_mutex_t fpLock = PTHREAD_MUTEX_INITIALIZER;
static tv_hdmi_edid_version_t m_edidversion[dsHDMI_IN_PORT_MAX];
static bool m_edidallmsupport[dsHDMI_IN_PORT_MAX];
static bool m_vrrsupport[dsHDMI_IN_PORT_MAX];
static bool m_hdmiPortVrrCaps[dsHDMI_IN_PORT_MAX];
IARM_Result_t dsHdmiInMgr_init();
IARM_Result_t dsHdmiInMgr_term();
IARM_Result_t _dsHdmiInInit(void *arg);
IARM_Result_t _dsHdmiInTerm(void *arg);
IARM_Result_t _dsHdmiInLoadKsvs(void *arg);
IARM_Result_t _dsHdmiInGetNumberOfInputs(void *arg);
IARM_Result_t _dsHdmiInGetStatus(void *arg);
IARM_Result_t _dsHdmiInSelectPort(void *arg);
IARM_Result_t _dsHdmiInToggleHotPlug(void *arg);
IARM_Result_t _dsHdmiInLoadEdidData(void *arg);
IARM_Result_t _dsHdmiInSetRepeater(void *arg);
IARM_Result_t _dsHdmiInScaleVideo(void *arg);
IARM_Result_t _dsHdmiInSelectZoomMode(void *arg);
IARM_Result_t _dsHdmiInGetCurrentVideoMode(void *arg);
IARM_Result_t _dsGetEDIDBytesInfo (void *arg);
IARM_Result_t _dsGetHDMISPDInfo (void *arg);
IARM_Result_t _dsSetEdidVersion (void *arg);
IARM_Result_t _dsGetEdidVersion (void *arg);
IARM_Result_t _dsGetAllmStatus (void *arg);
IARM_Result_t _dsGetSupportedGameFeaturesList (void *arg);
IARM_Result_t _dsGetAVLatency (void *arg);
IARM_Result_t _dsSetEdid2AllmSupport (void *arg);
IARM_Result_t _dsGetEdid2AllmSupport (void *arg);
IARM_Result_t _dsSetVRRSupport (void *arg);
IARM_Result_t _dsGetVRRSupport (void *arg);
IARM_Result_t _dsGetVRRStatus (void *arg);
IARM_Result_t _dsGetHdmiVersion (void *arg);

static dsError_t setEdid2AllmSupport (dsHdmiInPort_t iHdmiPort, bool allmSupport);
static dsError_t setVRRSupport (dsHdmiInPort_t iHdmiPort, bool vrrSupport);
static dsError_t getVRRSupport (dsHdmiInPort_t iHdmiPort, bool *vrrSupport);
void _dsHdmiInConnectCB(dsHdmiInPort_t port, bool isPortConnected);
void _dsHdmiInSignalChangeCB(dsHdmiInPort_t port, dsHdmiInSignalStatus_t sigStatus);
void _dsHdmiInStatusChangeCB(dsHdmiInStatus_t inputStatus);
void _dsHdmiInVideoModeUpdateCB(dsHdmiInPort_t port, dsVideoPortResolution_t videoResolution);
void _dsHdmiInAllmChangeCB(dsHdmiInPort_t port, bool allm_mode);
void _dsHdmiInVRRChangeCB(dsHdmiInPort_t port, dsVRRType_t vrr_type);
void _dsHdmiInAviContentTypeChangeCB(dsHdmiInPort_t port, dsAviContentType_t content_type);
void _dsHdmiInAVLatencyChangeCB(int audio_latency, int video_latency);

static dsHdmiInCap_t hdmiInCap_gs;

#include <iostream>
#include "hostPersistence.hpp"
#include <sstream>


using namespace std;

void getDynamicAutoLatencyConfig()
{
     RFC_ParamData_t param = {0};
     WDMP_STATUS status = getRFCParameter((char*)"dssrv", TVSETTINGS_DALS_RFC_PARAM, &param);
     INT_DEBUG("DALS Feature Enable = [ %s ] \n", param.value);
     if(WDMP_SUCCESS == status && (strncasecmp(param.value,"true",4) == 0)) {
         isDalsEnabled = true;
         INT_INFO("Value of isDalsEnabled = [ %d ] \n", isDalsEnabled);
     }
     else {
         INT_ERROR("Fetching RFC for DALS failed or DALS is disabled\n");
     }
}

static dsError_t isHdmiARCPort (int iPort, bool* isArcEnabled) {
    dsError_t eRet = dsERR_GENERAL; 

    typedef bool (*dsIsHdmiARCPort_t)(int iPortArg, bool *boolArg);
    static dsIsHdmiARCPort_t dsIsHdmiARCPortFunc = 0;
    if (dsIsHdmiARCPortFunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
            dsIsHdmiARCPortFunc = (dsIsHdmiARCPort_t) dlsym(dllib, "dsIsHdmiARCPort");
            if(dsIsHdmiARCPortFunc == 0) {
                INT_INFO("%s:%d dsIsHdmiARCPort (int) is not defined %s\r\n", __FUNCTION__,__LINE__, dlerror());
                eRet = dsERR_GENERAL;
            }
            else {
                INT_DEBUG("%s:%d dsIsHdmiARCPort dsIsHdmiARCPortFunc loaded\r\n", __FUNCTION__,__LINE__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s:%d dsIsHdmiARCPort  Opening RDK_DSHAL_NAME[%s] failed %s\r\n", 
                   __FUNCTION__,__LINE__, RDK_DSHAL_NAME, dlerror());  //CID 168096 - Print Args
            eRet = dsERR_GENERAL;
        }
    }
    if (0 != dsIsHdmiARCPortFunc) { 
        dsIsHdmiARCPortFunc (iPort, isArcEnabled);
        INT_INFO("%s: dsIsHdmiARCPort port %d isArcEnabled:%d\r\n", __FUNCTION__, iPort, *isArcEnabled);
    }
    else {
        INT_INFO("%s: dsIsHdmiARCPort  dsIsHdmiARCPortFunc = %p\n", __FUNCTION__, dsIsHdmiARCPortFunc);
    }
    return eRet;
}

static dsError_t getEDIDBytesInfo (dsHdmiInPort_t iHdmiPort, unsigned char *edid, int *length) {
    dsError_t eRet = dsERR_GENERAL;
    typedef dsError_t (*dsGetEDIDBytesInfo_t)(dsHdmiInPort_t iHdmiPort, unsigned char *edid, int *length);
    static dsGetEDIDBytesInfo_t dsGetEDIDBytesInfoFunc = 0;
    if (dsGetEDIDBytesInfoFunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
            dsGetEDIDBytesInfoFunc = (dsGetEDIDBytesInfo_t) dlsym(dllib, "dsGetEDIDBytesInfo");
            if(dsGetEDIDBytesInfoFunc == 0) {
                INT_INFO("%s:%d dsGetEDIDBytesInfo (int) is not defined %s\r\n", __FUNCTION__,__LINE__, dlerror());
                eRet = dsERR_GENERAL;
            }
            else {
                INT_INFO("%s:%d dsGetEDIDBytesInfoFunc loaded\r\n", __FUNCTION__,__LINE__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s:%d dsGetEDIDBytesInfo  Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__,__LINE__, RDK_DSHAL_NAME, dlerror());
            eRet = dsERR_GENERAL;
        }
    }
    if (0 != dsGetEDIDBytesInfoFunc) {
        INT_INFO("%s:%d Entering dsGetEDIDBytesInfoFunc\r\n", __FUNCTION__,__LINE__);
        eRet = dsGetEDIDBytesInfoFunc (iHdmiPort, edid, length);
        INT_INFO("[srv] %s: dsGetEDIDBytesInfoFunc eRet: %d data len: %d \r\n", __FUNCTION__,eRet, *length);
    }
    return eRet;
}

static dsError_t getHDMISPDInfo (dsHdmiInPort_t iHdmiPort, unsigned char *spd) {
    dsError_t eRet = dsERR_GENERAL;
    typedef dsError_t (*dsGetHDMISPDInfo_t)(dsHdmiInPort_t iHdmiPort, unsigned char *data);
    static dsGetHDMISPDInfo_t dsGetHDMISPDInfoFunc = 0;
    if (dsGetHDMISPDInfoFunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
            dsGetHDMISPDInfoFunc = (dsGetHDMISPDInfo_t) dlsym(dllib, "dsGetHDMISPDInfo");
            if(dsGetHDMISPDInfoFunc == 0) {
                INT_INFO("%s:%d dsGetHDMISPDInfo (int) is not defined %s\r\n", __FUNCTION__,__LINE__, dlerror());
                eRet = dsERR_GENERAL;
            }
            else {
                INT_DEBUG("%s:%d dsGetHDMISPDInfoFunc loaded\r\n", __FUNCTION__,__LINE__);
            }
            dlclose(dllib);
        }
        else {
            INT_INFO("%s:%d dsGetHDMISPDInfo  Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__,__LINE__, RDK_DSHAL_NAME, dlerror());
            eRet = dsERR_GENERAL;
        }
    }
    if (0 != dsGetHDMISPDInfoFunc) {
        eRet = dsGetHDMISPDInfoFunc (iHdmiPort, spd);
        INT_INFO("[srv] %s: dsGetHDMISPDInfoFunc eRet: %d \r\n", __FUNCTION__,eRet);
    }
    else {
        INT_INFO("%s:  dsGetHDMISPDInfoFunc = %p\n", __FUNCTION__, dsGetHDMISPDInfoFunc);
    }
    return eRet;
}

static dsError_t setEdidVersion (dsHdmiInPort_t iHdmiPort, tv_hdmi_edid_version_t iEdidVersion) {
    dsError_t eRet = dsERR_GENERAL;
    typedef dsError_t (*dsSetEdidVersion_t)(dsHdmiInPort_t iHdmiPort, tv_hdmi_edid_version_t iEdidVersion);
    static dsSetEdidVersion_t dsSetEdidVersionFunc = 0;
    char edidVer[2];
    sprintf(edidVer,"%d\0",iEdidVersion);

    if (dsSetEdidVersionFunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
            dsSetEdidVersionFunc = (dsSetEdidVersion_t) dlsym(dllib, "dsSetEdidVersion");
            if(dsSetEdidVersionFunc == 0) {
                INT_INFO("%s:%d dsSetEdidVersion (int) is not defined %s\r\n", __FUNCTION__, __LINE__, dlerror());
            }
            else {
                INT_DEBUG("%s:%d dsSetEdidVersionFunc loaded\r\n", __FUNCTION__, __LINE__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s:%d dsSetEdidVersion  Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__, __LINE__, RDK_DSHAL_NAME, dlerror());
        }
    }

    if (0 != dsSetEdidVersionFunc) {
        eRet = dsSetEdidVersionFunc (iHdmiPort, iEdidVersion);
        if (eRet == dsERR_NONE) {
            switch (iHdmiPort) {
                case dsHDMI_IN_PORT_0:
                    device::HostPersistence::getInstance().persistHostProperty("HDMI0.edidversion", edidVer);
                    INT_DEBUG("Port %s: Persist EDID Version: %d\n", "HDMI0", iEdidVersion);
                    break;
                case dsHDMI_IN_PORT_1:
                    device::HostPersistence::getInstance().persistHostProperty("HDMI1.edidversion", edidVer);
                    INT_DEBUG("Port %s: Persist EDID Version: %d\n", "HDMI1", iEdidVersion);
                    break;
                case dsHDMI_IN_PORT_2:
                    device::HostPersistence::getInstance().persistHostProperty("HDMI2.edidversion", edidVer);
                    INT_DEBUG("Port %s: Persist EDID Version: %d\n", "HDMI2", iEdidVersion);
                    break;
            }
	    // Whenever there is a change in edid version to 2.0, ensure the edid allm support and edid vrr support is updated with latest value
	if(iEdidVersion == HDMI_EDID_VER_20)
    	{
		INT_INFO("As the version is changed to 2.0, we are updating the allm bit and the vrr bit in edid\n");
		setEdid2AllmSupport(iHdmiPort,m_edidallmsupport[iHdmiPort]);
                setVRRSupport(iHdmiPort,m_vrrsupport[iHdmiPort]);
        }
        }
        INT_INFO("[srv] %s: dsSetEdidVersionFunc eRet: %d \r\n", __FUNCTION__, eRet);
    }
    else {
        INT_INFO("%s:  dsSetEdidVersionFunc = %p\n", __FUNCTION__, dsSetEdidVersionFunc);
    }
    return eRet;
}

static dsError_t getEdidVersion (dsHdmiInPort_t iHdmiPort, int *iEdidVersion) {
    dsError_t eRet = dsERR_GENERAL;
    typedef dsError_t (*dsGetEdidVersion_t)(dsHdmiInPort_t iHdmiPort, tv_hdmi_edid_version_t *iEdidVersion);
    static dsGetEdidVersion_t dsGetEdidVersionFunc = 0;
    if (dsGetEdidVersionFunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
            dsGetEdidVersionFunc = (dsGetEdidVersion_t) dlsym(dllib, "dsGetEdidVersion");
            if(dsGetEdidVersionFunc == 0) {
                INT_INFO("%s:%d dsGetEdidVersion (int) is not defined %s\r\n", __FUNCTION__, __LINE__, dlerror());
            }
            else {
                INT_DEBUG("%s:%d dsGetEdidVersionFunc loaded\r\n", __FUNCTION__, __LINE__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s:%d dsGetEdidVersion  Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__, __LINE__, RDK_DSHAL_NAME, dlerror());
        }
    }
    if (0 != dsGetEdidVersionFunc) {
        tv_hdmi_edid_version_t EdidVersion;
        eRet = dsGetEdidVersionFunc (iHdmiPort, &EdidVersion);
        int tmp = static_cast<int>(EdidVersion);
        *iEdidVersion = tmp;
        INT_INFO("[srv] %s: dsGetEdidVersionFunc eRet: %d \r\n", __FUNCTION__, eRet);
    }
    else {
        INT_INFO("%s:  dsGetEdidVersionFunc = %p\n", __FUNCTION__, dsGetEdidVersionFunc);
    }
    return eRet;
}

static dsError_t getAllmStatus (dsHdmiInPort_t iHdmiPort, bool *allmStatus) {
    dsError_t eRet = dsERR_GENERAL;
    typedef dsError_t (*dsGetAllmStatus_t)(dsHdmiInPort_t iHdmiPort, bool *allmStatus);
    static dsGetAllmStatus_t dsGetAllmStatusFunc = 0;
    if (dsGetAllmStatusFunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
            dsGetAllmStatusFunc = (dsGetAllmStatus_t) dlsym(dllib, "dsGetAllmStatus");
            if(dsGetAllmStatusFunc == 0) {
                INT_INFO("%s:%d dsGetAllmStatus (int) is not defined %s\r\n", __FUNCTION__, __LINE__, dlerror());
            }
            else {
                INT_DEBUG("%s:%d dsGetAllmStatusFunc loaded\r\n", __FUNCTION__, __LINE__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s:%d dsGetAllmStatus  Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__, __LINE__, RDK_DSHAL_NAME, dlerror());
        }
    }
    if (0 != dsGetAllmStatusFunc) {
        eRet = dsGetAllmStatusFunc (iHdmiPort, allmStatus);
        INT_INFO("[srv] %s: dsGetAllmStatusFunc eRet: %d \r\n", __FUNCTION__, eRet);
    }
    else {
        INT_INFO("%s:  dsGetAllmStatusFunc = %p\n", __FUNCTION__, dsGetAllmStatusFunc);
    }
    return eRet;
}

static dsError_t getSupportedGameFeaturesList (dsSupportedGameFeatureList_t *fList) {
    dsError_t eRet = dsERR_GENERAL;
    typedef dsError_t (*dsGetSupportedGameFeaturesList_t)(dsSupportedGameFeatureList_t *fList);
    static dsGetSupportedGameFeaturesList_t dsGetSupportedGameFeaturesListFunc = 0;
    if (dsGetSupportedGameFeaturesListFunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
            dsGetSupportedGameFeaturesListFunc = (dsGetSupportedGameFeaturesList_t) dlsym(dllib, "dsGetSupportedGameFeaturesList");
            if(dsGetSupportedGameFeaturesListFunc == 0) {
                INT_INFO("%s:%d dsGetSupportedGameFeaturesList (int) is not defined %s\r\n", __FUNCTION__, __LINE__, dlerror());
            }
            else {
                INT_DEBUG("%s:%d dsGetSupportedGameFeaturesList loaded\r\n", __FUNCTION__, __LINE__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s:%d dsGetSupportedGameFeaturesList  Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__, __LINE__, RDK_DSHAL_NAME, dlerror());
        }
    }
    if (0 != dsGetSupportedGameFeaturesListFunc) {
        eRet = dsGetSupportedGameFeaturesListFunc (fList);
        INT_INFO("[srv] %s: dsGetSupportedGameFeaturesListFunc eRet: %d \r\n", __FUNCTION__, eRet);
    }
    else {
        INT_INFO("%s:  dsGetSupportedGameFeaturesListFunc = %p\n", __FUNCTION__, dsGetSupportedGameFeaturesListFunc);
    }
    return eRet;
}

static dsError_t getAVLatency_hal (int *audio_latency, int *video_latency)
{
   dsError_t eRet = dsERR_GENERAL;
    typedef dsError_t (*dsGetAVLatency_t)(int *audio_latency, int *video_latency);
    static dsGetAVLatency_t dsGetAVLatencyFunc = 0;
    if (dsGetAVLatencyFunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
            dsGetAVLatencyFunc = (dsGetAVLatency_t) dlsym(dllib, "dsGetAVLatency");
            if(dsGetAVLatencyFunc == 0) {
                INT_INFO("%s:%d dsGetAVLatency (int) is not defined %s\r\n", __FUNCTION__, __LINE__, dlerror());
            }
            else {
                INT_DEBUG("%s:%d dsGetAVLatencyFunc loaded\r\n", __FUNCTION__, __LINE__);
            }
            dlclose(dllib);
        }
        else {
            INT_INFO("%s:%d dsGetAVLatency  Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__, __LINE__, RDK_DSHAL_NAME, dlerror());
        }
    }
    if (0 != dsGetAVLatencyFunc) {
        eRet = dsGetAVLatencyFunc (audio_latency, video_latency);
        INT_INFO("[srv] %s: dsGetAVLatencyFunc eRet: %d \r\n", __FUNCTION__, eRet);
    }
    else {
        INT_INFO("%s:  dsGetAVLatencyFunc = %p\n", __FUNCTION__, dsGetAVLatencyFunc);
               }
       return eRet;
}

static dsError_t getHdmiVersion (dsHdmiInPort_t iHdmiPort, dsHdmiMaxCapabilityVersion_t  *capversion) {
    dsError_t eRet = dsERR_GENERAL;
    typedef dsError_t (*dsGetHdmiVersion_t)(dsHdmiInPort_t iHdmiPort, dsHdmiMaxCapabilityVersion_t  *capversion);
    static dsGetHdmiVersion_t dsGetHdmiVersionFunc = 0;
    if (dsGetHdmiVersionFunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
            dsGetHdmiVersionFunc = (dsGetHdmiVersion_t) dlsym(dllib, "dsGetHdmiVersion");
            if(dsGetHdmiVersionFunc == 0) {
                INT_INFO("%s:%d dsGetHdmiVersion (int) is not defined %s\r\n", __FUNCTION__,__LINE__, dlerror());
                eRet = dsERR_GENERAL;
            }
            else {
                INT_INFO("%s:%d dsGetHdmiVersionFunc loaded\r\n", __FUNCTION__,__LINE__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s:%d dsGetHdmiVersion  Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__,__LINE__, RDK_DSHAL_NAME, dlerror());
            eRet = dsERR_GENERAL;
        }
    }
    if (0 != dsGetHdmiVersionFunc) {
        eRet = dsGetHdmiVersionFunc (iHdmiPort, capversion);
        INT_INFO("[srv] %s: dsGetHdmiVersionFunc eRet: %d \r\n", __FUNCTION__,eRet);
    }
    return eRet;
}

IARM_Result_t dsHdmiInMgr_init()
{
    _dsHdmiInInit(NULL);

    IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsHdmiInInit, _dsHdmiInInit);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t dsHdmiInMgr_term()
{
    _dsHdmiInTerm(NULL);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsHdmiInInit(void *arg)
{
    INT_INFO("%s:%d ---> m_isInitialized=%d, m_isPlatInitialized=%d \n",
                   __PRETTY_FUNCTION__,__LINE__, m_isInitialized, m_isPlatInitialized);


    IARM_BUS_Lock(lock);
    getDynamicAutoLatencyConfig();

    if (PROFILE_TV == profileType)
    {
        INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
        if (!m_isPlatInitialized)
        {
            /* Nexus init, if any here */
            dsError_t eError = dsHdmiInInit();
        }
        m_isPlatInitialized++;
    }

    if (!m_isInitialized)
    {
        if (PROFILE_TV == profileType)
        {
            INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
            dsHdmiInRegisterConnectCB(_dsHdmiInConnectCB);

            typedef dsError_t (*dsHdmiInRegisterSignalChangeCB_t)(dsHdmiInSignalChangeCB_t CBFunc);
            static dsHdmiInRegisterSignalChangeCB_t signalChangeCBFunc = 0;
            if (signalChangeCBFunc == 0) {
                void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    signalChangeCBFunc = (dsHdmiInRegisterSignalChangeCB_t) dlsym(dllib, "dsHdmiInRegisterSignalChangeCB");
                    if(signalChangeCBFunc == 0) {
                        INT_INFO("dsHdmiInRegisterSignalChangeCB(dsHdmiInSignalChangeCB_t) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
    
            if(signalChangeCBFunc) {
                 signalChangeCBFunc(_dsHdmiInSignalChangeCB);
            }
    
            typedef dsError_t (*dsHdmiInRegisterStatusChangeCB_t)(dsHdmiInStatusChangeCB_t CBFunc);
            static dsHdmiInRegisterStatusChangeCB_t statusChangeCBFunc = 0;
            if (statusChangeCBFunc == 0) {
                void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    statusChangeCBFunc = (dsHdmiInRegisterStatusChangeCB_t) dlsym(dllib, "dsHdmiInRegisterStatusChangeCB");
                    if(statusChangeCBFunc == 0) {
                        INT_INFO("dsHdmiInRegisterStatusChangeCB(dsHdmiInStatusChangeCB_t) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
    
            if(statusChangeCBFunc) {
                 statusChangeCBFunc(_dsHdmiInStatusChangeCB);
            }
    
            typedef dsError_t (*dsHdmiInRegisterVideoModeUpdateCB_t)(dsHdmiInVideoModeUpdateCB_t CBFunc);
            static dsHdmiInRegisterVideoModeUpdateCB_t videoModeUpdateCBFunc = 0;
            if (videoModeUpdateCBFunc == 0) {
                void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    videoModeUpdateCBFunc = (dsHdmiInRegisterVideoModeUpdateCB_t) dlsym(dllib, "dsHdmiInRegisterVideoModeUpdateCB");
                    if(statusChangeCBFunc == 0) {
                        INT_INFO("dsHdmiInRegisterStatusChangeCB(dsHdmiInStatusChangeCB_t) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
    
            if(videoModeUpdateCBFunc) {
                 videoModeUpdateCBFunc(_dsHdmiInVideoModeUpdateCB);
            }
    
            typedef dsError_t (*dsHdmiInRegisterAllmChangeCB_t)(dsHdmiInAllmChangeCB_t CBFunc);
            static dsHdmiInRegisterAllmChangeCB_t allmChangeCBFunc = 0;
            if (allmChangeCBFunc == 0) {
                void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    allmChangeCBFunc = (dsHdmiInRegisterAllmChangeCB_t) dlsym(dllib, "dsHdmiInRegisterAllmChangeCB");
                    if(statusChangeCBFunc == 0) {
                        INT_INFO("dsHdmiInRegisterAllmChangeCB(dsHdmiInAllmChangeCB_t) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
    
            if(allmChangeCBFunc) {
                allmChangeCBFunc(_dsHdmiInAllmChangeCB);
            }
	    typedef dsError_t (*dsHdmiInRegisterVRRChangeCB_t)(dsHdmiInVRRChangeCB_t CBFunc);
            static dsHdmiInRegisterVRRChangeCB_t vrrChangeCBFunc = 0;
            if (vrrChangeCBFunc == 0) {
                void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    vrrChangeCBFunc = (dsHdmiInRegisterVRRChangeCB_t) dlsym(dllib, "dsHdmiInRegisterVRRChangeCB");
                    if(vrrChangeCBFunc == 0) {
                        INT_INFO("dsHdmiInRegisterVRRChangeCB(dsHdmiInVRRChangeCB_t) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }

            if(vrrChangeCBFunc) {
                vrrChangeCBFunc(_dsHdmiInVRRChangeCB);
            }
            typedef dsError_t (*dsHdmiInRegisterAviContentTypeChangeCB_t)(dsHdmiInAviContentTypeChangeCB_t CBFunc);
            static dsHdmiInRegisterAviContentTypeChangeCB_t contentTypeChangeCBFunc = 0;
            if (contentTypeChangeCBFunc == 0) {
                void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    contentTypeChangeCBFunc = (dsHdmiInRegisterAviContentTypeChangeCB_t) dlsym(dllib, "dsHdmiInRegisterAviContentTypeChangeCB");
                    if(contentTypeChangeCBFunc == 0) {
                        INT_INFO("dsHdmiInRegisterContentTypeChangeCB(dsHdmiInContentTypeChangeCB_t) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
    
            if(contentTypeChangeCBFunc) {
                contentTypeChangeCBFunc(_dsHdmiInAviContentTypeChangeCB);
            }
    
    	typedef dsError_t (*dsHdmiInRegisterAVLatencyChangeCB_t)(dsAVLatencyChangeCB_t CBFunc);
            static dsHdmiInRegisterAVLatencyChangeCB_t AVLatencyCBFunc = 0;
            if (AVLatencyCBFunc == 0) {
                void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    AVLatencyCBFunc = (dsHdmiInRegisterAVLatencyChangeCB_t) dlsym(dllib, "dsHdmiInRegisterAVLatencyChangeCB");
                    if(AVLatencyCBFunc == 0) {
                        INT_INFO("dsRegisterAVLatencyChangeCB(dsAVLatencyChangeCB_t) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
    
            if(AVLatencyCBFunc && isDalsEnabled) {
                AVLatencyCBFunc(_dsHdmiInAVLatencyChangeCB);
           }
    
        }

        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsHdmiInTerm,                  _dsHdmiInTerm);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsHdmiInGetNumberOfInputs,     _dsHdmiInGetNumberOfInputs);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsHdmiInGetStatus,             _dsHdmiInGetStatus);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsHdmiInSelectPort,            _dsHdmiInSelectPort);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsHdmiInScaleVideo,            _dsHdmiInScaleVideo);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsHdmiInSelectZoomMode,        _dsHdmiInSelectZoomMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsHdmiInGetCurrentVideoMode,   _dsHdmiInGetCurrentVideoMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetEDIDBytesInfo,              _dsGetEDIDBytesInfo);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetHDMISPDInfo,              _dsGetHDMISPDInfo);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetEdidVersion,              _dsSetEdidVersion);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetEdidVersion,              _dsGetEdidVersion);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetAllmStatus,               _dsGetAllmStatus);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetSupportedGameFeaturesList,_dsGetSupportedGameFeaturesList);
	IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetAVLatency,  _dsGetAVLatency);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetEdid2AllmSupport,  _dsSetEdid2AllmSupport);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetEdid2AllmSupport,  _dsGetEdid2AllmSupport);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetVRRSupport,  _dsSetVRRSupport);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetVRRSupport,  _dsGetVRRSupport);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetVRRStatus,               _dsGetVRRStatus);
	IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetHdmiVersion,  _dsGetHdmiVersion);

        int itr = 0;
        bool isARCCapable = false;
        for (itr = 0; itr < dsHDMI_IN_PORT_MAX; itr++) {
            isARCCapable = false;
            isHdmiARCPort (itr, &isARCCapable);
            hdmiInCap_gs.isPortArcCapable[itr] = isARCCapable; 
        }

        // Getting the edidallmEnable value from persistence upon bootup
        std::string _EdidAllmSupport("TRUE");
        try {
            _EdidAllmSupport = device::HostPersistence::getInstance().getProperty("HDMI0.edidallmEnable");
            if(_EdidAllmSupport == "TRUE")
                m_edidallmsupport[dsHDMI_IN_PORT_0] =  true;
            else
                m_edidallmsupport[dsHDMI_IN_PORT_0] =  false;

            INT_INFO("Port %s: _EdidAllmSupport: %s , m_edidallmsupport: %d\n", "HDMI0", _EdidAllmSupport.c_str(), m_edidallmsupport[0]);
        }
        catch(...) {
            INT_INFO("Port %s: Exception in Getting the HDMI0 EDID allm support from persistence storage..... \r\n", "HDMI0");
            m_edidallmsupport[dsHDMI_IN_PORT_0] = true;
        }
       	
	try {
            _EdidAllmSupport = device::HostPersistence::getInstance().getProperty("HDMI1.edidallmEnable");
            if(_EdidAllmSupport == "TRUE")
                m_edidallmsupport[dsHDMI_IN_PORT_1] =  true;
            else
                m_edidallmsupport[dsHDMI_IN_PORT_1] =  false;

            INT_INFO("Port %s: _EdidAllmSupport: %s , m_edidallmsupport: %d\n", "HDMI1", _EdidAllmSupport.c_str(), m_edidallmsupport[1]);
        }
        catch(...) {
            INT_INFO("Port %s: Exception in Getting the HDMI1 EDID allm support from persistence storage..... \r\n", "HDMI1");
            m_edidallmsupport[dsHDMI_IN_PORT_1] = true;
        }

        try {
            _EdidAllmSupport = device::HostPersistence::getInstance().getProperty("HDMI2.edidallmEnable");
            if(_EdidAllmSupport == "TRUE")
                m_edidallmsupport[dsHDMI_IN_PORT_2] =  true;
            else
                m_edidallmsupport[dsHDMI_IN_PORT_2] =  false;

            INT_INFO("Port %s: _EdidAllmSupport: %s , m_edidallmsupport: %d\n", "HDMI2", _EdidAllmSupport.c_str(), m_edidallmsupport[2]);
        }
        catch(...) {
            INT_INFO("Port %s: Exception in Getting the HDMI2 EDID allm support from persistence storage..... \r\n", "HDMI2");
            m_edidallmsupport[dsHDMI_IN_PORT_2] = true;
        }
       // Getting the edidvrrEnable value from persistence upon bootup
        std::string _VRRSupport("TRUE");
        try {
            _VRRSupport = device::HostPersistence::getInstance().getProperty("HDMI0.vrrEnable");
            if(_VRRSupport == "TRUE")
                m_vrrsupport[dsHDMI_IN_PORT_0] =  true;
            else
                m_vrrsupport[dsHDMI_IN_PORT_0] =  false;

            INT_INFO("Port %s: _VRRSupport: %s , m_vrrsupport: %d\n", "HDMI0", _VRRSupport.c_str(), m_vrrsupport[0]);
        }
        catch(...) {
            INT_INFO("Port %s: Exception in Getting the HDMI0 EDID VRR support from persistence storage..... \r\n", "HDMI0");
            m_vrrsupport[dsHDMI_IN_PORT_0] = false;
        }

        try {
            _VRRSupport = device::HostPersistence::getInstance().getProperty("HDMI1.vrrEnable");
            if(_VRRSupport == "TRUE")
                m_vrrsupport[dsHDMI_IN_PORT_1] =  true;
            else
                m_vrrsupport[dsHDMI_IN_PORT_1] =  false;

            INT_INFO("Port %s: _VRRSupport: %s , m_edidallmsupport: %d\n", "HDMI1", _VRRSupport.c_str(), m_vrrsupport[1]);
        }
        catch(...) {
            INT_INFO("Port %s: Exception in Getting the HDMI1 EDID VRR support from persistence storage..... \r\n", "HDMI1");
            m_vrrsupport[dsHDMI_IN_PORT_1] = false;
        }

        try {
            _VRRSupport = device::HostPersistence::getInstance().getProperty("HDMI2.vrrEnable");
            if(_VRRSupport == "TRUE")
                m_vrrsupport[dsHDMI_IN_PORT_2] =  true;
            else
                m_vrrsupport[dsHDMI_IN_PORT_2] =  false;

            INT_INFO("Port %s: _vrrSupport: %s , m_vrrsupport: %d\n", "HDMI2", _VRRSupport.c_str(), m_vrrsupport[2]);
        }
        catch(...) {
            INT_INFO("Port %s: Exception in Getting the HDMI2 EDID VRR support from persistence storage..... \r\n", "HDMI2");
            m_vrrsupport[dsHDMI_IN_PORT_2] = true;
        }
        try {
            _VRRSupport = device::HostPersistence::getInstance().getProperty("HDMI3.vrrEnable");
            if(_VRRSupport == "TRUE")
                m_vrrsupport[dsHDMI_IN_PORT_3] =  true;
            else
                m_vrrsupport[dsHDMI_IN_PORT_3] =  false;

            INT_INFO("Port %s: _vrrSupport: %s , m_vrrsupport: %d\n", "HDMI3", _VRRSupport.c_str(), m_vrrsupport[3]);
        }
        catch(...) {
            INT_INFO("Port %s: Exception in Getting the HDMI3 EDID VRR support from persistence storage..... \r\n", "HDMI3");
            m_vrrsupport[dsHDMI_IN_PORT_3] = true;
        }
       	
	std::string _EdidVersion("1");
        try {
            _EdidVersion = device::HostPersistence::getInstance().getProperty("HDMI0.edidversion");
            m_edidversion[dsHDMI_IN_PORT_0] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
        }
        catch(...) {
            try {
                INT_INFO("Port %s: Exception in Getting the HDMI0 EDID version from persistence storage. Try system default...\r\n", "HDMI0");
                _EdidVersion = device::HostPersistence::getInstance().getDefaultProperty("HDMI0.edidversion");
                m_edidversion[dsHDMI_IN_PORT_0] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
            }
            catch(...) {
                INT_INFO("Port %s: Exception in Getting the HDMI0 EDID version from system default..... \r\n", "HDMI0");
                m_edidversion[dsHDMI_IN_PORT_0] = HDMI_EDID_VER_20;
            }
        }
        try {
            _EdidVersion = device::HostPersistence::getInstance().getProperty("HDMI1.edidversion");
            m_edidversion[dsHDMI_IN_PORT_1] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
        }
        catch(...) {
            try {
                INT_INFO("Port %s: Exception in Getting the HDMI1 EDID version from persistence storage. Try system default...\r\n", "HDMI1");
                _EdidVersion = device::HostPersistence::getInstance().getDefaultProperty("HDMI1.edidversion");
                m_edidversion[dsHDMI_IN_PORT_1] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
            }
            catch(...) {
                INT_INFO("Port %s: Exception in Getting the HDMI1 EDID version from system default..... \r\n", "HDMI1");
                m_edidversion[dsHDMI_IN_PORT_1] = HDMI_EDID_VER_20;
            }
        }
        try {
            _EdidVersion = device::HostPersistence::getInstance().getProperty("HDMI2.edidversion");
            m_edidversion[dsHDMI_IN_PORT_2] = static_cast<tv_hdmi_edid_version_t>((atoi (_EdidVersion.c_str())));
        }
        catch(...) {
            try {
                INT_INFO("Port %s: Exception in Getting the HDMI2 EDID version from persistence storage. Try system default...\r\n", "HDMI2");
                _EdidVersion = device::HostPersistence::getInstance().getDefaultProperty("HDMI2.edidversion");
                m_edidversion[dsHDMI_IN_PORT_2] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
            }
            catch(...) {
                INT_INFO("Port %s: Exception in Getting the HDMI2 EDID version from system default..... \r\n", "HDMI2");
                m_edidversion[dsHDMI_IN_PORT_2] = HDMI_EDID_VER_20;
            }
        }
	
        for (itr = 0; itr < dsHDMI_IN_PORT_MAX; itr++) {
            if (getVRRSupport(static_cast<dsHdmiInPort_t>(itr), &m_hdmiPortVrrCaps[itr]) >= 0) {
                INT_INFO("Port HDMI%d: VRR capability : %d\n", itr, m_hdmiPortVrrCaps[itr]);
            }
        }
        for (itr = 0; itr < dsHDMI_IN_PORT_MAX; itr++) {
            if (setEdidVersion (static_cast<dsHdmiInPort_t>(itr), m_edidversion[itr]) >= 0) {
                INT_INFO("Port HDMI%d: Initialized EDID Version : %d\n", itr, m_edidversion[itr]);
            }
        }
        m_isInitialized = 1;
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}


IARM_Result_t _dsHdmiInTerm(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);
    if (PROFILE_TV == profileType)
    {
        INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
        if (m_isPlatInitialized)
        {
            m_isPlatInitialized--;
            if (!m_isPlatInitialized)
            {
                dsHdmiInTerm();
            }
        }
    }
    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}


IARM_Result_t _dsHdmiInGetNumberOfInputs(void *arg)
{
    _DEBUG_ENTER();

    dsHdmiInGetNumberOfInputsParam_t *param = (dsHdmiInGetNumberOfInputsParam_t *)arg;

    IARM_BUS_Lock(lock);

    if (PROFILE_TV == profileType)
    {
        INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
        param->result = dsHdmiInGetNumberOfInputs(&param->numHdmiInputs);
    }
    else
    {
        INT_INFO("[%d][%s]: its Other Profile\r\n", __LINE__, __FUNCTION__);
        param->result = dsERR_GENERAL;
    }
    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsHdmiInGetStatus(void *arg)
{
    _DEBUG_ENTER();

    dsHdmiInGetStatusParam_t *param= (dsHdmiInGetStatusParam_t *)arg;

    IARM_BUS_Lock(lock);

    if (PROFILE_TV == profileType)
    {
        INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
        param->result = dsHdmiInGetStatus(&param->status);
    }
    else
    {
        INT_INFO("[%d][%s]: its Other Profile\r\n", __LINE__, __FUNCTION__);
        param->result = dsERR_GENERAL;
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsHdmiInSelectPort(void *arg)
{
    _DEBUG_ENTER();

    dsHdmiInSelectPortParam_t *param = (dsHdmiInSelectPortParam_t *)arg;

    IARM_BUS_Lock(lock);

    if (PROFILE_TV == profileType)
    {
        INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
        param->result = dsHdmiInSelectPort(param->port,param->requestAudioMix, param->videoPlaneType,param->topMostPlane);
    }
    else
    {
        INT_INFO("[%d][%s]: its Other Profile\r\n", __LINE__, __FUNCTION__);
        param->result = dsERR_GENERAL;
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsHdmiInScaleVideo(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);
    dsHdmiInScaleVideoParam_t *param = (dsHdmiInScaleVideoParam_t *)arg;

    if (PROFILE_TV == profileType)
    {
        INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
        param->result = dsHdmiInScaleVideo(param->videoRect.x, param->videoRect.y, param->videoRect.width, param->videoRect.height);
    }
    else
    {
        INT_INFO("[%d][%s]: its Other Profile\r\n", __LINE__, __FUNCTION__);
        param->result = dsERR_GENERAL;
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsHdmiInSelectZoomMode(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);
    dsHdmiInSelectZoomModeParam_t *param = (dsHdmiInSelectZoomModeParam_t *)arg;

    if (PROFILE_TV == profileType)
    {
        INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
        param->result = dsHdmiInSelectZoomMode(param->zoomMode);
    }
    else
    {
        INT_INFO("[%d][%s]: its Other Profile\r\n", __LINE__, __FUNCTION__);
        param->result = dsERR_GENERAL;
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsHdmiInGetCurrentVideoMode(void *arg)
{
    _DEBUG_ENTER();

    dsHdmiInGetResolutionParam_t *param = (dsHdmiInGetResolutionParam_t *)arg;

    IARM_BUS_Lock(lock);

    if (PROFILE_TV == profileType)
    {
        INT_INFO("[%d][%s]: its TV Profile\r\n", __LINE__, __FUNCTION__);
        param->result = dsHdmiInGetCurrentVideoMode(&param->resolution);
    }
    else
    {
        INT_INFO("[%d][%s]: its Other Profile\r\n", __LINE__, __FUNCTION__);
        param->result = dsERR_GENERAL;
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

void _dsHdmiInConnectCB(dsHdmiInPort_t port, bool isPortConnected)
{
    IARM_Bus_DSMgr_EventData_t hdmi_in_hpd_eventData;
 
    INT_INFO("%s:%d - HDMI In hotplug update!!!!!!..Port: %d, isPort: %d\r\n",__PRETTY_FUNCTION__,__LINE__, port, isPortConnected);
    hdmi_in_hpd_eventData.data.hdmi_in_connect.port = port;
    hdmi_in_hpd_eventData.data.hdmi_in_connect.isPortConnected = isPortConnected;
			
    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
	                        (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG,
	                        (void *)&hdmi_in_hpd_eventData, 
	                        sizeof(hdmi_in_hpd_eventData));
           
}

void _dsHdmiInSignalChangeCB(dsHdmiInPort_t port, dsHdmiInSignalStatus_t sigStatus)
{
    IARM_Bus_DSMgr_EventData_t hdmi_in_sigStatus_eventData;

    INT_INFO("%s:%d - HDMI In signal status change update!!!!!! Port: %d, Signal Status: %d\r\n", __PRETTY_FUNCTION__,__LINE__,port, sigStatus);
    hdmi_in_sigStatus_eventData.data.hdmi_in_sig_status.port = port;
    hdmi_in_sigStatus_eventData.data.hdmi_in_sig_status.status = sigStatus;

    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
			        (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS,
			        (void *)&hdmi_in_sigStatus_eventData,
			        sizeof(hdmi_in_sigStatus_eventData));

}

void _dsHdmiInStatusChangeCB(dsHdmiInStatus_t inputStatus)
{
    IARM_Bus_DSMgr_EventData_t hdmi_in_status_eventData;

    INT_INFO("%s:%d - HDMI In status change update!!!!!! Port: %d, isPresented: %d\r\n", __PRETTY_FUNCTION__,__LINE__, inputStatus.activePort, inputStatus.isPresented);
    hdmi_in_status_eventData.data.hdmi_in_status.port = inputStatus.activePort;
    hdmi_in_status_eventData.data.hdmi_in_status.isPresented = inputStatus.isPresented;

    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                                (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS,
                                (void *)&hdmi_in_status_eventData,
                                sizeof(hdmi_in_status_eventData));

}

void _dsHdmiInVideoModeUpdateCB(dsHdmiInPort_t port, dsVideoPortResolution_t videoResolution)
{
    IARM_Bus_DSMgr_EventData_t hdmi_in_videoMode_eventData;

    INT_INFO("%s:%d - HDMI In video mode info  update, Port: %d, Pixel Resolution: %d, Interlaced: %d, Frame Rate: %d \n", __PRETTY_FUNCTION__,__LINE__,port, videoResolution.pixelResolution, videoResolution.interlaced, videoResolution.frameRate);
    hdmi_in_videoMode_eventData.data.hdmi_in_video_mode.port = port;
    hdmi_in_videoMode_eventData.data.hdmi_in_video_mode.resolution.pixelResolution = videoResolution.pixelResolution;
    hdmi_in_videoMode_eventData.data.hdmi_in_video_mode.resolution.interlaced = videoResolution.interlaced;
    hdmi_in_videoMode_eventData.data.hdmi_in_video_mode.resolution.frameRate = videoResolution.frameRate;


    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                                (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE,
                                (void *)&hdmi_in_videoMode_eventData,
                                sizeof(hdmi_in_videoMode_eventData));

}

void _dsHdmiInAllmChangeCB(dsHdmiInPort_t port, bool allm_mode)
{
    IARM_Bus_DSMgr_EventData_t hdmi_in_allmMode_eventData;

    INT_INFO("%s:%d - HDMI In ALLM Mode update!!!!!! Port: %d, ALLM Mode: %d\r\n", __FUNCTION__,__LINE__,port, allm_mode);
    hdmi_in_allmMode_eventData.data.hdmi_in_allm_mode.port = port;
    hdmi_in_allmMode_eventData.data.hdmi_in_allm_mode.allm_mode = allm_mode;

    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                                (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_HDMI_IN_ALLM_STATUS,
                                (void *)&hdmi_in_allmMode_eventData,
                                sizeof(hdmi_in_allmMode_eventData));

}

void _dsHdmiInVRRChangeCB(dsHdmiInPort_t port, dsVRRType_t vrr_type)
{
    IARM_Bus_DSMgr_EventData_t hdmi_in_vrrMode_eventData;

    INT_INFO("%s:%d - HDMI In VRR Mode update!!!!!! Port: %d, VRR TYPE: %d\r\n", __FUNCTION__,__LINE__,port, vrr_type);
    hdmi_in_vrrMode_eventData.data.hdmi_in_vrr_mode.port = port;
    hdmi_in_vrrMode_eventData.data.hdmi_in_vrr_mode.vrr_type = vrr_type;

    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                                (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_HDMI_IN_VRR_STATUS,
                                (void *)&hdmi_in_vrrMode_eventData,
                                sizeof(hdmi_in_vrrMode_eventData));

}

void _dsHdmiInAviContentTypeChangeCB(dsHdmiInPort_t port, dsAviContentType_t avi_content_type)
{
    IARM_Bus_DSMgr_EventData_t hdmi_in_contentType_eventData;

    INT_INFO("%s:%d - HDMI In Content Type update!!!!!! Port: %d, content type: %d\r\n", __FUNCTION__,__LINE__,port, avi_content_type);
    hdmi_in_contentType_eventData.data.hdmi_in_content_type.port = port;
    hdmi_in_contentType_eventData.data.hdmi_in_content_type.aviContentType = avi_content_type;

    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                                (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_HDMI_IN_AVI_CONTENT_TYPE,
                                (void *)&hdmi_in_contentType_eventData,
                                sizeof(hdmi_in_contentType_eventData));
}

void _dsHdmiInAVLatencyChangeCB(int audio_latency, int video_latency)
{
    IARM_Bus_DSMgr_EventData_t hdmi_in_av_latency_eventData;

    hdmi_in_av_latency_eventData.data.hdmi_in_av_latency.audio_output_delay = audio_latency;
    hdmi_in_av_latency_eventData.data.hdmi_in_av_latency.video_latency = video_latency;
    INT_INFO("%s:%d - HDMI In AV Latency update!!!!!! audio_latency: %d, video latency: %d\r\n", __FUNCTION__,__LINE__,audio_latency,video_latency);
    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                                (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_HDMI_IN_AV_LATENCY,
                                (void *)&hdmi_in_av_latency_eventData,
                                sizeof(hdmi_in_av_latency_eventData));

}

IARM_Result_t _dsGetEDIDBytesInfo (void *arg) 
{
    errno_t rc = -1;
    dsError_t eRet = dsERR_GENERAL;

    dsGetEDIDBytesInfoParam_t *param = (dsGetEDIDBytesInfoParam_t *) arg;
    memset (param->edid, '\0', MAX_EDID_BYTES_LEN);
    unsigned char edidArg[MAX_EDID_BYTES_LEN] = {0};
    IARM_BUS_Lock(lock);
    eRet = getEDIDBytesInfo (param->iHdmiPort, edidArg, &(param->length));
    param->result = eRet;
    INT_INFO("[srv] %s: getEDIDBytesInfo eRet: %d\r\n", __FUNCTION__, param->result);
    if (eRet == dsERR_NONE && param->length > 0 && param->length <= MAX_EDID_BYTES_LEN) {//Make sure the result was true, and there is a valid length.
        rc = memcpy_s(param->edid,sizeof(param->edid), edidArg, param->length);
        if(rc!=EOK)
        {
		ERR_CHK(rc);
        }
    }
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetHDMISPDInfo(void *arg)
{
    errno_t rc = -1;
    _DEBUG_ENTER();
    INT_DEBUG("%s:%d [srv] _dsGetHDMISPDInfo \n", __PRETTY_FUNCTION__,__LINE__);

    dsGetHDMISPDInfoParam_t *param = (dsGetHDMISPDInfoParam_t *)arg;

    IARM_BUS_Lock(lock);

    memset (param->spdInfo, '\0', sizeof(struct dsSpd_infoframe_st));
    unsigned char spdArg[sizeof(struct dsSpd_infoframe_st)] = {0};
    param->result = getHDMISPDInfo(param->iHdmiPort, spdArg);
    INT_INFO("[srv] %s: dsGetHDMISPDInfo eRet: %d\r\n", __FUNCTION__, param->result);
    if (param->result == dsERR_NONE) {
            rc = memcpy_s(param->spdInfo,sizeof(param->spdInfo), spdArg, sizeof(struct dsSpd_infoframe_st));
            if(rc!=EOK)
            {
                    ERR_CHK(rc);
            }
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsSetEdidVersion (void *arg)
{
    _DEBUG_ENTER();

    dsEdidVersionParam_t *param = (dsEdidVersionParam_t *) arg;
    IARM_BUS_Lock(lock);
    param->result = setEdidVersion (param->iHdmiPort, param->iEdidVersion);
    m_edidversion[param->iHdmiPort]=param->iEdidVersion;
    INT_INFO("[srv] %s: dsSetEdidVersion Port: %d EDID: %d eRet: %d\r\n", __FUNCTION__, param->iHdmiPort,  param->iEdidVersion, param->result);
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetEdidVersion (void *arg)
{
    int edidVer = -1;
    _DEBUG_ENTER();

    dsEdidVersionParam_t *param = (dsEdidVersionParam_t *) arg;
    IARM_BUS_Lock(lock);
    param->result = getEdidVersion (param->iHdmiPort, &edidVer);
    param->iEdidVersion = static_cast<tv_hdmi_edid_version_t>(edidVer);
    INT_INFO("[srv] %s: dsGetEdidVersion edidVer: %d\r\n", __FUNCTION__, param->iEdidVersion);
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetAllmStatus (void *arg)
{
    bool allmStatus = false;
    _DEBUG_ENTER();

    dsAllmStatusParam_t *param = (dsAllmStatusParam_t *) arg;
    IARM_BUS_Lock(lock);
    param->result = getAllmStatus (param->iHdmiPort, &allmStatus);
    param->allmStatus = allmStatus;
    INT_INFO("[srv] %s: dsGetAllmStatus allmStatus: %d\r\n", __FUNCTION__, param->allmStatus);
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetSupportedGameFeaturesList (void *arg)
{
    dsSupportedGameFeatureList_t    fList;
    _DEBUG_ENTER();

    dsSupportedGameFeatureListParam_t *param = (dsSupportedGameFeatureListParam_t *) arg;
    IARM_BUS_Lock(lock);
    param->result = getSupportedGameFeaturesList (&fList);
    param->featureList.gameFeatureCount = fList.gameFeatureCount;
    strncpy(param->featureList.gameFeatureList,fList.gameFeatureList,MAX_PROFILE_LIST_BUFFER_LEN);

    INT_INFO("%s: Total number of supported game features: %d\n",__FUNCTION__, fList.gameFeatureCount);
    INT_INFO("%s: Supported Game Features List: %s\n",__FUNCTION__, fList.gameFeatureList);

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetAVLatency (void *arg)
{
    _DEBUG_ENTER();
    int audio_latency;
    int video_latency;
    dsTVAudioVideoLatencyParam_t *param = (dsTVAudioVideoLatencyParam_t *) arg;
    IARM_BUS_Lock(lock);

    param->result = getAVLatency_hal(&audio_latency,&video_latency);
    param->video_latency = video_latency;
    param->audio_output_delay = audio_latency;
    INT_INFO("[srv] %s: _dsGetAVLatency AVLatency_params: %d : %d\r\n", __FUNCTION__, param->video_latency, param->audio_output_delay);
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

void updateEdidAllmBitValuesInPersistence(dsHdmiInPort_t iHdmiPort, bool allmSupport)
{
      INT_INFO("[srv]: Updating values of edid allm bit in persistence\n");
      switch(iHdmiPort){
                case dsHDMI_IN_PORT_0:
                    device::HostPersistence::getInstance().persistHostProperty("HDMI0.edidallmEnable", allmSupport ? "TRUE" : "FALSE");
                    INT_INFO("Port %s: Persist EDID Allm Bit: %d\n", "HDMI0", allmSupport);
                    break;
                case dsHDMI_IN_PORT_1:
                    device::HostPersistence::getInstance().persistHostProperty("HDMI1.edidallmEnable", allmSupport ? "TRUE" : "FALSE");
                    INT_INFO("Port %s: Persist EDID Allm Bit: %d\n", "HDMI1", allmSupport);
                    break;
                case dsHDMI_IN_PORT_2:
                    device::HostPersistence::getInstance().persistHostProperty("HDMI2.edidallmEnable", allmSupport ? "TRUE" : "FALSE");
                    INT_INFO("Port %s: Persist EDID Allm Bit: %d\n", "HDMI2", allmSupport);
                    break;
      }
}

static dsError_t setEdid2AllmSupport (dsHdmiInPort_t iHdmiPort, bool allmSupport) {
    dsError_t eRet = dsERR_GENERAL;
    typedef dsError_t (*dsSetEdid2AllmSupport_t)(dsHdmiInPort_t iHdmiPort, bool allmSupport);
    static dsSetEdid2AllmSupport_t dsSetEdid2AllmSupportFunc = 0;

    if (dsSetEdid2AllmSupportFunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
            dsSetEdid2AllmSupportFunc = (dsSetEdid2AllmSupport_t) dlsym(dllib, "dsSetEdid2AllmSupport");
            if(dsSetEdid2AllmSupportFunc == 0) {
                INT_INFO("%s:%d dsSetEdid2AllmSupport (int,bool) is not defined %s\r\n", __FUNCTION__, __LINE__, dlerror());
            }
            else {
                INT_DEBUG("%s:%d dsSetEdid2AllmSupport loaded\r\n", __FUNCTION__, __LINE__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s:%d dsSetEdid2AllmSupport  Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__, __LINE__, RDK_DSHAL_NAME, dlerror());
        }
    }
    INT_INFO("setEdid2AllmSupport to ds-hal:  EDID Allm Bit: %d\n", allmSupport);
    if (0 != dsSetEdid2AllmSupportFunc) {
        eRet = dsSetEdid2AllmSupportFunc (iHdmiPort, allmSupport);
        INT_INFO("[srv] %s: dsSetEdid2AllmSupportFunc eRet: %d \r\n", __FUNCTION__, eRet);
    }
    else {
        INT_INFO("%s:  dsSetEdid2AllmSupportFunc = %p\n", __FUNCTION__, dsSetEdid2AllmSupportFunc);
    }
    return eRet;
}

IARM_Result_t _dsSetEdid2AllmSupport (void *arg)
{
    _DEBUG_ENTER();

    dsEdidAllmSupportParam_t *param = (dsEdidAllmSupportParam_t *) arg;
    IARM_BUS_Lock(lock);
    param->result = dsERR_NONE;
    INT_INFO("[srv] :  In _dsSetEdid2AllmSupport, checking m_ediversion of port %d : %d\n",param->iHdmiPort,m_edidversion[param->iHdmiPort]);
    if(m_edidversion[param->iHdmiPort] == HDMI_EDID_VER_20)//if the edidver is 2.0, then only set the allm bit in edid
    {
        param->result = setEdid2AllmSupport (param->iHdmiPort, param->allmSupport);
    }
    INT_INFO("[srv] %s: dsSetEdid2AllmSupport Port: %d AllmSupport: %d eRet: %d\r\n", __FUNCTION__, param->iHdmiPort,  param->allmSupport, param->result);
    if(param->result == dsERR_NONE) 
    {
        updateEdidAllmBitValuesInPersistence(param->iHdmiPort,param->allmSupport);
        m_edidallmsupport[param->iHdmiPort] = param->allmSupport;
    }   
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetEdid2AllmSupport (void *arg)
{
    _DEBUG_ENTER();
    bool allmSupport = false;
    dsEdidAllmSupportParam_t *param = (dsEdidAllmSupportParam_t *) arg;
    IARM_BUS_Lock(lock);
    param->result =  dsERR_NONE;
    // getEdid2AllmSupport will return the latest allm bit value of the specified port(which is written to persistence)
    // irrespective of the edid version, the latest value is returned.
    param->allmSupport = m_edidallmsupport[param->iHdmiPort];
    INT_INFO("[srv] %s: dsGetEdid2AllmSupport : %d for port %d\r\n", __FUNCTION__, param->allmSupport,param->iHdmiPort);
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

void updateVRRBitValuesInPersistence(dsHdmiInPort_t iHdmiPort, bool vrrSupport)
{
      INT_INFO("[srv]: Updating values of vrr bit in persistence\n");
      switch(iHdmiPort){
                case dsHDMI_IN_PORT_0:
                    device::HostPersistence::getInstance().persistHostProperty("HDMI0.vrrEnable", vrrSupport ? "TRUE" : "FALSE");
                    INT_INFO("Port %s: Persist EDID VRR Bit: %d\n", "HDMI0", vrrSupport);
                    break;
                case dsHDMI_IN_PORT_1:
                    device::HostPersistence::getInstance().persistHostProperty("HDMI1.vrrEnable", vrrSupport ? "TRUE" : "FALSE");
                    INT_INFO("Port %s: Persist EDID VRR Bit: %d\n", "HDMI1", vrrSupport);
                    break;
                case dsHDMI_IN_PORT_2:
                    device::HostPersistence::getInstance().persistHostProperty("HDMI2.vrrEnable", vrrSupport ? "TRUE" : "FALSE");
                    INT_INFO("Port %s: Persist EDID VRR Bit: %d\n", "HDMI2", vrrSupport);
                    break;
                case dsHDMI_IN_PORT_3:
                    device::HostPersistence::getInstance().persistHostProperty("HDMI3.vrrEnable", vrrSupport ? "TRUE" : "FALSE");
                    INT_INFO("Port %s: Persist EDID VRR Bit: %d\n", "HDMI3", vrrSupport);
                    break;
      }
}

static dsError_t setVRRSupport (dsHdmiInPort_t iHdmiPort, bool vrrSupport) {
    dsError_t eRet = dsERR_GENERAL;
    if (!m_hdmiPortVrrCaps[iHdmiPort]) {
	    return dsERR_OPERATION_NOT_SUPPORTED;
    }
    typedef dsError_t (*dsHdmiInSetVRRSupport_t)(dsHdmiInPort_t iHdmiPort, bool vrrSupport);
    static dsHdmiInSetVRRSupport_t dsHdmiInSetVRRSupportFunc = 0;

    if (dsHdmiInSetVRRSupportFunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
            dsHdmiInSetVRRSupportFunc = (dsHdmiInSetVRRSupport_t) dlsym(dllib, "dsHdmiInSetVRRSupport");
            if(dsHdmiInSetVRRSupportFunc == 0) {
                INT_INFO("%s:%d dsHdmiInSetVRRSupport (int,bool) is not defined %s\r\n", __FUNCTION__, __LINE__, dlerror());
            }
            else {
                INT_DEBUG("%s:%d dsHdmiInSetVRRSupport loaded\r\n", __FUNCTION__, __LINE__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s:%d dsHdmiInSetVRRSupport  Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__, __LINE__, RDK_DSHAL_NAME, dlerror());
        }
    }
    INT_INFO("setVRRSupport to ds-hal:  EDID VRR Bit: %d\n", vrrSupport);
    if (0 != dsHdmiInSetVRRSupportFunc) {
        eRet = dsHdmiInSetVRRSupportFunc (iHdmiPort, vrrSupport);
        INT_INFO("[srv] %s: dsHdmiInSetVRRSupportFunc eRet: %d \r\n", __FUNCTION__, eRet);
    }
    else {
        INT_INFO("%s:  dsHdmiInSetVRRSupportFunc = %p\n", __FUNCTION__, dsHdmiInSetVRRSupportFunc);
    }
    return eRet;
}

IARM_Result_t _dsSetVRRSupport (void *arg)
{
    _DEBUG_ENTER();

    dsVRRSupportParam_t *param = (dsVRRSupportParam_t *) arg;
    param->result = dsERR_NONE;
    INT_INFO("[srv] :  In _dsSetVRRSupport, checking m_ediversion of port %d : %d\n",param->iHdmiPort,m_edidversion[param->iHdmiPort]);
    IARM_BUS_Lock(lock);
    if(m_edidversion[param->iHdmiPort] == HDMI_EDID_VER_20)//if the edidver is 2.0, then only set the VRR bit in edid
    {
        param->result = setVRRSupport (param->iHdmiPort, param->vrrSupport);
    }
    INT_INFO("[srv] %s: dsSetVRRSupport Port: %d vrrSupport: %d eRet: %d\r\n", __FUNCTION__, param->iHdmiPort,  param->vrrSupport, param->result);
    if(param->result == dsERR_NONE)
    {
        updateVRRBitValuesInPersistence(param->iHdmiPort,param->vrrSupport);
        m_vrrsupport[param->iHdmiPort] = param->vrrSupport;
    }
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

static dsError_t getVRRSupport (dsHdmiInPort_t iHdmiPort, bool *vrrSupport) {
    dsError_t eRet = dsERR_GENERAL;
    typedef dsError_t (*dsHdmiInGetVRRSupport_t)(dsHdmiInPort_t iHdmiPort, bool *vrrSupport);
    static dsHdmiInGetVRRSupport_t dsHdmiInGetVRRSupportFunc = 0;

    if (dsHdmiInGetVRRSupportFunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
            dsHdmiInGetVRRSupportFunc = (dsHdmiInGetVRRSupport_t) dlsym(dllib, "dsHdmiInGetVRRSupport");
            if(dsHdmiInGetVRRSupportFunc == 0) {
                INT_INFO("%s:%d dsHdmiInGetVRRSupport (int,bool) is not defined %s\r\n", __FUNCTION__, __LINE__, dlerror());
            }
            else {
                INT_DEBUG("%s:%d dsHdmiInGetVRRSupport loaded\r\n", __FUNCTION__, __LINE__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s:%d dsHdmiInGetVRRSupport  Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__, __LINE__, RDK_DSHAL_NAME, dlerror());
        }
    }
    if (0 != dsHdmiInGetVRRSupportFunc) {
        eRet = dsHdmiInGetVRRSupportFunc (iHdmiPort, vrrSupport);
        INT_INFO("[srv] %s: dsHdmiInGetVRRSupportFunc eRet: %d \r\n", __FUNCTION__, eRet);
    }
    else {
        INT_INFO("%s:  dsHdmiInGetVRRSupportFunc = %p\n", __FUNCTION__, dsHdmiInGetVRRSupportFunc);
    }
    return eRet;
}

IARM_Result_t _dsGetVRRSupport (void *arg)
{
    _DEBUG_ENTER();
    bool vrrSupport = false;
    dsVRRSupportParam_t *param = (dsVRRSupportParam_t *) arg;
    IARM_BUS_Lock(lock);
    param->result =  dsERR_NONE;
    // getVRRSupport will return the latest vrr bit value of the specified port(which is written to persistence)
    // irrespective of the edid version, the latest value is returned.
    param->vrrSupport = m_vrrsupport[param->iHdmiPort];
    INT_INFO("[srv] %s: dsGetVRRSupport : %d for port %d\r\n", __FUNCTION__, param->vrrSupport,param->iHdmiPort);
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}


static dsError_t getVRRStatus (dsHdmiInPort_t iHdmiPort, dsVRRType_t *vrrStatus) {
    dsError_t eRet = dsERR_GENERAL;
    typedef dsError_t (*dsHdmiInGetVRRStatus_t)(dsHdmiInPort_t iHdmiPort, dsVRRType_t *vrrStatus);
    static dsHdmiInGetVRRStatus_t dsHdmiInGetVRRStatusFunc = 0;
    if (dsHdmiInGetVRRStatusFunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
            dsHdmiInGetVRRStatusFunc = (dsHdmiInGetVRRStatus_t) dlsym(dllib, "dsHdmiInGetVRRStatus");
            if(dsHdmiInGetVRRStatusFunc == 0) {
                INT_INFO("%s:%d dsHdmiInGetVRRStatus (int) is not defined %s\r\n", __FUNCTION__, __LINE__, dlerror());
            }
            else {
                INT_DEBUG("%s:%d dsHdmiInGetVRRStatusFunc loaded\r\n", __FUNCTION__, __LINE__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s:%d dsHdmiInGetVRRStatus  Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__, __LINE__, RDK_DSHAL_NAME, dlerror());
        }
    }
    if (0 != dsHdmiInGetVRRStatusFunc) {
        eRet = dsHdmiInGetVRRStatusFunc (iHdmiPort, vrrStatus);
        INT_INFO("[srv] %s: dsHdmiInGetVRRStatusFunc eRet: %d \r\n", __FUNCTION__, eRet);
    }
    else {
        INT_INFO("%s:  dsHdmiInGetVRRStatusFunc = %p\n", __FUNCTION__, dsHdmiInGetVRRStatusFunc);
    }
    return eRet;
}

IARM_Result_t _dsGetVRRStatus (void *arg)
{
    dsVRRType_t vrrStatus = dsVRR_NONE;
    _DEBUG_ENTER();

    dsVRRStatusParam_t *param = (dsVRRStatusParam_t *) arg;
    IARM_BUS_Lock(lock);
    param->result = getVRRStatus (param->iHdmiPort, &vrrStatus);
    param->vrrStatus = vrrStatus;
    INT_INFO("[srv] %s: dsGetVRRStatus vrrStatus: %d\r\n", __FUNCTION__, param->vrrStatus);
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetHdmiVersion (void *arg)
{
    dsError_t eRet = dsERR_GENERAL;
    dsHdmiVersionParam_t *param = (dsHdmiVersionParam_t *) arg;
    dsHdmiMaxCapabilityVersion_t capVersion;
    IARM_BUS_Lock(lock);
    eRet = getHdmiVersion (param->iHdmiPort, &capVersion);
    param->iCapVersion = capVersion;
    param->result = eRet;
    INT_INFO("[srv] %s: getHdmiVersion is %d, eRet: %d\r\n", __FUNCTION__,param->iCapVersion, param->result);
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}
/** @} */
/** @} */
