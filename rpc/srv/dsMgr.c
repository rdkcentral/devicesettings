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


#include "dsMgr.h"
#include "libIARM.h"
#include "dsError.h"
#include "libIARM.h"
#include "dsHost.h"
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h>
#include "dsserverlogger.h"

#include <iostream>
#include "hostPersistence.hpp"

#include "dsInternal.h"
#include "dsConfigs.h"

profile_t profileType = PROFILE_INVALID;

extern IARM_Result_t dsAudioMgr_init();
extern IARM_Result_t dsVideoPortMgr_init();
extern IARM_Result_t dsVideoDeviceMgr_init();
extern IARM_Result_t dsFPDMgr_init();
extern IARM_Result_t dsDisplayMgr_init();
extern IARM_Result_t dsHostMgr_init();
extern IARM_Result_t dsAudioMgr_term();
extern IARM_Result_t dsVideoPortMgr_term();
extern IARM_Result_t dsVideoDeviceMgr_term();
extern IARM_Result_t dsFPDMgr_term();
extern IARM_Result_t dsDisplayMgr_term();
extern IARM_Result_t dsHostMgr_term();
extern IARM_Result_t dsHdmiInMgr_init();
extern IARM_Result_t dsHdmiInMgr_term();
extern IARM_Result_t dsCompositeInMgr_init();
extern IARM_Result_t dsCompositeInMgr_term();

profile_t searchRdkProfile(void) {
    INT_DEBUG("Entering [%s]\r\n", __FUNCTION__);
    const char* devPropPath = "/etc/device.properties";
    char line[256], *rdkProfile = NULL;
    profile_t ret = PROFILE_INVALID;
    FILE* file;

    file = fopen(devPropPath, "r");
    if (file == NULL) {
        INT_ERROR("[%s]: File not found.\n", __FUNCTION__);
        return PROFILE_INVALID;
    }

    while (fgets(line, sizeof(line), file)) {
        rdkProfile = strstr(line, RDK_PROFILE);
        if (rdkProfile != NULL) {
            rdkProfile = strchr(line, '=');
            INT_DEBUG("[%s]: Found RDK_PROFILE\r\n", __FUNCTION__);
            break;
        }
    }
    if(rdkProfile != NULL)
    {
        rdkProfile++; // Move past the '=' character
        if(0 == strncmp(rdkProfile, PROFILE_STR_TV, strlen(PROFILE_STR_TV)))
        {
            ret = PROFILE_TV;
        }
        else if (0 == strncmp(rdkProfile, PROFILE_STR_STB, strlen(PROFILE_STR_STB)))
        {
            ret = PROFILE_STB;
        }
    }
    else
    {
        INT_ERROR("[%s]: NOT FOUND RDK_PROFILE in device properties file\r\n", __FUNCTION__);
        ret = PROFILE_INVALID;
    }

    fclose(file);
    INT_INFO("Exit [%s]: RDK_PROFILE = %d\r\n", __FUNCTION__, ret);
    return ret;
}

IARM_Result_t dsMgr_init()
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;

	INT_INFO("[%s]: Calling searchRdkProfile()\r\n", __FUNCTION__);
    profileType = searchRdkProfile();
    INT_INFO("[%s]: profileType=%d\r\n", __FUNCTION__, profileType);

	INT_INFO("[%s]: Loading device configurations\r\n", __FUNCTION__);
	dsLoadConfigs();
    device::HostPersistence::getInstance().load();
	dsServer_Rdklogger_Init();
	dsHostInit();
	dsDisplayMgr_init();
	dsAudioMgr_init();
	dsVideoPortMgr_init();
	dsVideoDeviceMgr_init();
	dsFPDMgr_init();
	dsHostMgr_init();
	dsHdmiInMgr_init();
	dsCompositeInMgr_init();

	return ret;
}

IARM_Result_t dsMgr_term()
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
   
	dsAudioMgr_term();
	dsVideoPortMgr_term();
	dsVideoDeviceMgr_term();
	dsFPDMgr_term();
	dsDisplayMgr_term();
	dsHostMgr_term();
	dsHdmiInMgr_term();
	dsCompositeInMgr_term();
	return ret;
}


/** @} */
/** @} */
