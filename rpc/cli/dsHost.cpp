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
#include "dsRpc.h"
#include "dsMgr.h"
#include "dsError.h"
#include "dsTypes.h"
#include "iarmUtil.h"
#include "libIARM.h"
#include "libIBus.h"
#include "dsHost.h"
#include "dsInternal.h"

#include "safec_lib.h"

dsError_t dsSetPreferredSleepMode(dsSleepMode_t mode)
{
    _DEBUG_ENTER();

    dsPreferredSleepMode param;
    param.mode = mode;
    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                (char *)IARM_BUS_DSMGR_API_dsSetPreferredSleepMode,
                (void *)&param,
                sizeof(param));
    if (IARM_RESULT_SUCCESS == rpcRet)
    {
        return dsERR_NONE;
    }

    return dsERR_GENERAL ;
 
}

dsError_t dsGetPreferredSleepMode(dsSleepMode_t *mode)
{
    _DEBUG_ENTER();

    dsPreferredSleepMode param;
 
    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                (char *)IARM_BUS_DSMGR_API_dsGetPreferredSleepMode,
                (void *)&param,
                sizeof(param));

    if (IARM_RESULT_SUCCESS != rpcRet)
    {
        return dsERR_GENERAL ;
    }

    *mode = param.mode;
   
    return dsERR_NONE;
 
}


dsError_t dsGetCPUTemperature(float *cpuTemperature)
{
    _DEBUG_ENTER();
    
    dsCPUThermalParam param;
   
    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

    param.temperature = 0.0;
    
    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                (char *)IARM_BUS_DSMGR_API_dsGetCPUTemperature,
                (void *)&param,
                sizeof(param));

    if (IARM_RESULT_SUCCESS != rpcRet)
    {
        return dsERR_GENERAL ;
    }

    *cpuTemperature = param.temperature;
   
    return dsERR_NONE;
}


dsError_t dsGetVersion(uint32_t *versionNumber)
{
    _DEBUG_ENTER();
    
    dsVesrionParam param;
   
    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

    param.versionNumber = 0x10000;
    
    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                (char *)IARM_BUS_DSMGR_API_dsGetVersion,
                (void *)&param,
                sizeof(param));

    if (IARM_RESULT_SUCCESS != rpcRet)
    {
        return dsERR_GENERAL ;
    }

    *versionNumber = param.versionNumber;
   
    return dsERR_NONE;
}

dsError_t dsGetSocIDFromSDK(char *socID)
{
    _DEBUG_ENTER();

    dsGetSocIDFromSDKParam_t param;

    param.result = dsERR_NONE;

    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                (char *)IARM_BUS_DSMGR_API_dsGetSocIDFromSDK,
                (void *)&param,
                sizeof(param));

   if (IARM_RESULT_SUCCESS != rpcRet)
   {
      return dsERR_GENERAL ;
   }
   strncpy(socID, param.socID, DS_DEVICEID_LEN_MAX);
   return dsERR_NONE;

}

dsError_t dsGetHostEDID( unsigned char *edid, int *length)
{
   errno_t rc = -1;
   IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

   _DEBUG_ENTER();

   dsDisplayGetEDIDBytesParam_t param;
   printf("dsCLI::getHostEDID \r\n");

   rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                                                        (char *)IARM_BUS_DSMGR_API_dsGetHostEDID,
                                                        (void *)&param,
                                                        sizeof(param));

   if (IARM_RESULT_SUCCESS == rpcRet)
   {
        if (param.result == dsERR_NONE) {
            printf("dsCLI ::getHostEDID returns %d bytes\r\n", param.length);
                rc = memcpy_s((void *)edid,param.length, param.bytes, param.length);
                if(rc!=EOK)
                {
                        ERR_CHK(rc);
                }
                *length = param.length;
                return dsERR_NONE;
        }
        else {
            return (dsError_t)param.result;
        }
    }
    else {
        return dsERR_GENERAL;
    }
}

dsError_t dsGetMS12ConfigType(const char *ms12ConfigType)
{
   IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
   dsError_t ret = dsERR_NONE;
   _DEBUG_ENTER();

   dsMS12ConfigTypeParam_t param;

   rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                                                        (char *)IARM_BUS_DSMGR_API_dsGetMS12ConfigType,
                                                        (void *)&param,
                                                        sizeof(param));
   if (IARM_RESULT_SUCCESS == rpcRet)
   {
       if (param.result == dsERR_NONE) {
	        strncpy((char*)ms12ConfigType, param.configType, MS12_CONFIG_BUF_SIZE);

       } else {
            printf("%s: Fecthing the MS12 config failed\n", __FUNCTION__);
            ret = dsERR_GENERAL;
       }
   } else {
        ret = dsERR_GENERAL;
   }

   return ret;
}

dsError_t dsSetRebootConfig(string rebootReasonCustom, uint8_t powerState)
{
    dsError_t ret = dsERR_GENERAL;
    _DEBUG_ENTER();

    if (!rebootReasonCustom.empty())
    {
        dsMgrRebootConfigParam_t param;
        param.powerState = powerState;
        param.reboot_reason_custom = rebootReasonCustom;
        param.result = 0;

        IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
        rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                    (char *)IARM_BUS_DSMGR_API_dsSetRebootConfig,
                    (void *)&param,
                    sizeof(param));
        if (IARM_RESULT_SUCCESS == rpcRet)
        {
            ret = dsERR_NONE;
        }
    }
    return ret;
}


/** @} */
/** @} */
