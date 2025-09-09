/* If not stated otherwise in this file or this component's LICENSE file the
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
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include "dsError.h"
#include "dsUtl.h"
#include "dsRpc.h"
#include "dsMgr.h"
#include "iarmUtil.h"
#include "libIBus.h"
#include "libIARM.h"
#include "dsTypes.h"
#include "dsclientlogger.h"
#include "dsInternal.h"

#include "safec_lib.h"

dsError_t dsVideoPortInit()
{
    printf("<<<<< VOP is initialized in Multi-App Mode >>>>>>>>\r\n");

	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                            (char *)IARM_BUS_DSMGR_API_dsVideoPortInit,
                            NULL,
                            0);
  
	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}

	return dsERR_GENERAL;
}

dsError_t dsGetVideoPort(dsVideoPortType_t type, int index, intptr_t *handle)
{
    _DEBUG_ENTER();
    _RETURN_IF_ERROR(dsVideoPortType_isValid(type), dsERR_INVALID_PARAM);
    _RETURN_IF_ERROR((handle) != NULL, dsERR_INVALID_PARAM);

	dsVideoPortGetHandleParam_t param;
    param.type = type;
    param.index = index;
    param.handle = NULL;



    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsGetVideoPort,
							 &param,
							sizeof(param));

	printf("%s..%d-%d\n",__func__,param.type,param.handle);

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		*handle = param.handle;
		 return dsERR_NONE;
	}
 
	return dsERR_GENERAL ;
}


dsError_t dsIsHDCPEnabled(intptr_t handle, bool *enabled)
{
    _DEBUG_ENTER();
    
    _RETURN_IF_ERROR(enabled != NULL, dsERR_INVALID_PARAM);

    dsVideoPortIsHDCPEnabledParam_t param;
   
    param.handle = handle;
    param.enabled = false;
	


	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
    
	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsIsHDCPEnabled,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		 *enabled = param.enabled;
		 return dsERR_NONE;
	}

	return dsERR_GENERAL ;
}

dsError_t dsIsVideoPortEnabled(intptr_t handle, bool *enabled)
{
    _DEBUG_ENTER();
    
    _RETURN_IF_ERROR(enabled != NULL, dsERR_INVALID_PARAM);

    dsVideoPortIsEnabledParam_t param;
   
    param.handle = handle;
    param.enabled = false;
	


	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
    
	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsIsVideoPortEnabled,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		 *enabled = param.enabled;
		 return dsERR_NONE;
	}

	return dsERR_GENERAL ;
}

dsError_t dsGetHDCPStatus (intptr_t handle, dsHdcpStatus_t *status)
{
    _DEBUG_ENTER();
    _RETURN_IF_ERROR(status != NULL, dsERR_INVALID_PARAM);

    dsVideoPortGetHDCPStatus_t param;
    param.handle = handle;
    param.hdcpStatus = dsHDCP_STATUS_UNAUTHENTICATED; 

    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsGetHDCPStatus,
							(void *)&param,
							sizeof(param));
    if (IARM_RESULT_SUCCESS == rpcRet)
    {
     	*status = param.hdcpStatus;
	 	return dsERR_NONE;
    }

    return dsERR_GENERAL ;
}

dsError_t dsGetHDCPProtocol (intptr_t handle, dsHdcpProtocolVersion_t *version)
{
    _DEBUG_ENTER();
    _RETURN_IF_ERROR(version != NULL, dsERR_INVALID_PARAM);

    dsVideoPortGetHDCPProtocolVersion_t param;
    param.handle = handle;
    param.protocolVersion = dsHDCP_VERSION_MAX;

    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
					(char *)IARM_BUS_DSMGR_API_dsGetHDCPProtocol,
					(void *)&param,
					sizeof(param));
    if (IARM_RESULT_SUCCESS == rpcRet)
    {
     	*version = param.protocolVersion;
     	return dsERR_NONE;
    }

    return dsERR_GENERAL ;
}

dsError_t dsGetHDCPReceiverProtocol (intptr_t handle, dsHdcpProtocolVersion_t *version)
{
    _DEBUG_ENTER();
    _RETURN_IF_ERROR(version != NULL, dsERR_INVALID_PARAM);

    dsVideoPortGetHDCPProtocolVersion_t param;
    param.handle = handle;
    param.protocolVersion = dsHDCP_VERSION_MAX;

    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
					(char *)IARM_BUS_DSMGR_API_dsGetHDCPReceiverProtocol,
					(void *)&param,
					sizeof(param));
    if (IARM_RESULT_SUCCESS == rpcRet)
    {
     	*version = param.protocolVersion;
     	return dsERR_NONE;
    }

    return dsERR_GENERAL ;
}

dsError_t dsGetHDCPCurrentProtocol (intptr_t handle, dsHdcpProtocolVersion_t *version)
{
    _DEBUG_ENTER();
    _RETURN_IF_ERROR(version != NULL, dsERR_INVALID_PARAM);

    dsVideoPortGetHDCPProtocolVersion_t param;
    param.handle = handle;
    param.protocolVersion = dsHDCP_VERSION_MAX;

    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
					(char *)IARM_BUS_DSMGR_API_dsGetHDCPCurrentProtocol,
					(void *)&param,
					sizeof(param));
    if (IARM_RESULT_SUCCESS == rpcRet)
    {
     	*version = param.protocolVersion;
     	return dsERR_NONE;
    }

    return dsERR_GENERAL ;
}
dsError_t  dsIsDisplayConnected(intptr_t handle, bool *connected)
{
    _DEBUG_ENTER();
    _RETURN_IF_ERROR(connected != NULL, dsERR_INVALID_PARAM);
   
	dsVideoPortIsDisplayConnectedParam_t param;
    
	param.handle = handle;
    param.connected = false;

	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
	

	
	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsIsDisplayConnected,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		 *connected = param.connected;
		 return dsERR_NONE;
	}

	return dsERR_GENERAL ;
}

dsError_t  dsIsDisplaySurround(intptr_t handle, bool *surround)
{
    _DEBUG_ENTER();
    _RETURN_IF_ERROR(surround != NULL, dsERR_INVALID_PARAM);
   
	dsVideoPortIsDisplaySurroundParam_t param;
    
	param.handle = handle;
	param.surround = false;

	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
	

	
	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsIsDisplaySurround,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		 *surround = param.surround;
		 return dsERR_NONE;
	}

	return dsERR_GENERAL ;
}

dsError_t  dsGetSurroundMode(intptr_t handle, int *surround)
{
    _DEBUG_ENTER();
    _RETURN_IF_ERROR(surround != NULL, dsERR_INVALID_PARAM);
   
	dsVideoPortGetSurroundModeParam_t param;
    
	param.handle = handle;
	param.surround = dsSURROUNDMODE_NONE;

	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
	

	
	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsGetSurroundMode,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		 *surround = param.surround;
		 return dsERR_NONE;
	}

	return dsERR_GENERAL ;
}
dsError_t  dsEnableVideoPort(intptr_t handle, bool enabled)
{
    _DEBUG_ENTER();

	dsVideoPortSetEnabledParam_t param;
    param.handle = handle;
    param.enabled = enabled;


    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsEnableVideoPort,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}

	return dsERR_GENERAL ;
}


dsError_t  dsGetResolution(intptr_t handle, dsVideoPortResolution_t *resolution)
{
    _DEBUG_ENTER();
    _RETURN_IF_ERROR(resolution != NULL, dsERR_INVALID_PARAM);
 
	dsVideoPortGetResolutionParam_t param;
    
    param.handle = handle;
    param.toPersist = false;
    param.resolution = *resolution;

	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsGetResolution,
							(void *)&param,
							sizeof(param));

	*resolution = param.resolution;

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}
	return dsERR_GENERAL ;
}

dsError_t  dsVideoPortSetResolution(intptr_t handle, dsVideoPortResolution_t *resolution, bool persist)
{
    _DEBUG_ENTER();
    _RETURN_IF_ERROR(resolution != NULL, dsERR_INVALID_PARAM);
 
	dsVideoPortSetResolutionParam_t param;
    
    param.handle = handle;
    param.toPersist = persist;
    param.forceCompatible = true;
    param.resolution = *resolution;

	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsSetResolution,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet && (dsERR_NONE == param.result))
	{
		return dsERR_NONE;
	}
	
	return dsERR_GENERAL ;
}

dsError_t  dsVideoPortGetPreferredColorDepth (intptr_t handle, dsDisplayColorDepth_t *colorDepth, bool persist)
{
    _DEBUG_ENTER();
    _RETURN_IF_ERROR(colorDepth != NULL, dsERR_INVALID_PARAM);

    dsPreferredColorDepthParam_t param;

    param.handle = handle;
    param.toPersist = persist;
    param.colorDepth = *colorDepth;

    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsGetPreferredColorDepth,
							(void *)&param,
							sizeof(param));

    *colorDepth = param.colorDepth;

    if (IARM_RESULT_SUCCESS == rpcRet)
    {
        return dsERR_NONE;
    }
    return dsERR_GENERAL ;
}

dsError_t  dsVideoPortSetPreferredColorDepth (intptr_t handle, dsDisplayColorDepth_t colorDepth, bool persist)
{
    _DEBUG_ENTER();

    dsPreferredColorDepthParam_t param;

    param.handle = handle;
    param.toPersist = persist;
    param.colorDepth = colorDepth;

    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsSetPreferredColorDepth,
							(void *)&param,
							sizeof(param));

    if (IARM_RESULT_SUCCESS == rpcRet && (dsERR_NONE == param.result))
    {
        return dsERR_NONE;
    }

    return dsERR_GENERAL ;
}

dsError_t dsColorDepthCapabilities(intptr_t handle, unsigned int *capabilities)
{
    _DEBUG_ENTER();

    dsColorDepthCapabilitiesParam_t param;
    memset(&param, 0, sizeof(param));
    param.handle = handle;

    IARM_Result_t rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
			(char *) IARM_BUS_DSMGR_API_dsColorDepthCapabilities,
			(void *) &param,
			sizeof(param));

    if (IARM_RESULT_SUCCESS == rpcRet)
    {
        *capabilities = param.colorDepthCapability;
        return param.result;
    }

    return dsERR_GENERAL ;
}


dsError_t dsVideoPortTerm(void)
{
    _DEBUG_ENTER();

   IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

   rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                            (char *)IARM_BUS_DSMGR_API_dsVideoPortTerm,
                            NULL,
                            0);
	
	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}

	return dsERR_GENERAL ;
}

dsError_t  dsEnableHDCP(intptr_t handle, bool contentProtect, char *hdcpKey, size_t keySize)
{
    errno_t rc = -1;
    _DEBUG_ENTER();

//    if ((keySize <= 0) || (keySize > HDCP_KEY_MAX_SIZE) )
    if (((unsigned int) keySize > HDCP_KEY_MAX_SIZE) )
    {
        return dsERR_INVALID_PARAM;
    }

//    if (contentProtect && !hdcpKey) {
//        return dsERR_INVALID_PARAM;
//    }

    dsEnableHDCPParam_t param;
    
    memset(&param, 0, sizeof(param));
    param.handle = handle;
    param.contentProtect = contentProtect;
    param.keySize = keySize;
    param.rpcResult = dsERR_NONE;

    if (contentProtect && hdcpKey && keySize && keySize <= HDCP_KEY_MAX_SIZE) {
            rc = memcpy_s(param.hdcpKey,sizeof(param.hdcpKey), hdcpKey, keySize);
            if(rc!=EOK)
            {
                    ERR_CHK(rc);
            }
    } 
    
    printf("IARM:CLI:dsEnableHDCP %d, %p, %d\r\n", contentProtect, hdcpKey, keySize);

    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
		    (char *)IARM_BUS_DSMGR_API_dsEnableHDCP,
		    (void *)&param,
		    sizeof(param));

	if( (IARM_RESULT_SUCCESS == rpcRet) && (dsERR_NONE == param.rpcResult))
	{
		return dsERR_NONE;
	}

	return dsERR_GENERAL ;
}

dsError_t dsIsVideoPortActive(intptr_t handle, bool *active)
{
    _DEBUG_ENTER();
    
    _RETURN_IF_ERROR(active != NULL, dsERR_INVALID_PARAM);

    dsVideoPortIsActiveParam_t param;
   
    param.handle = handle;
    param.active = false;
    param.result = dsERR_NONE;
	

	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsIsVideoPortActive,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		*active = param.active;
		return param.result;
	}

	return dsERR_GENERAL ;
}

dsError_t dsGetTVHDRCapabilities(intptr_t handle, int *capabilities)
{
	_DEBUG_ENTER();

	dsGetHDRCapabilitiesParam_t param;
	memset(&param, 0, sizeof(param));
	param.handle = handle;

	IARM_Result_t rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
			(char *) IARM_BUS_DSMGR_API_dsGetTVHDRCapabilities,
			(void *) &param,
			sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		*capabilities = param.capabilities;
		return param.result;
	}

	return dsERR_GENERAL ;
}

dsError_t dsSupportedTvResolutions(intptr_t handle, int *resolutions)
{
	_DEBUG_ENTER();

	dsSupportedResolutionParam_t param;
	memset(&param, 0, sizeof(param));
	param.handle = handle;

	IARM_Result_t rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
			(char *) IARM_BUS_DSMGR_API_dsGetSupportedTVResolution,
			(void *) &param,
			sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		*resolutions = param.resolutions;
		return param.result;
	}

	return dsERR_GENERAL ;
}

dsError_t dsSetForceDisable4KSupport(intptr_t handle, bool disable)
{
	_DEBUG_ENTER();

	dsForceDisable4KParam_t param;
	memset(&param, 0, sizeof(param));
	param.handle = handle;
	param.disable = disable;

	IARM_Result_t rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
			(char *) IARM_BUS_DSMGR_API_dsSetForceDisable4K,
			(void *) &param,
			sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return param.result;
	}

	return dsERR_GENERAL ;
}

dsError_t dsGetForceDisable4KSupport(intptr_t handle, bool *disable)
{
	_DEBUG_ENTER();

	dsForceDisable4KParam_t param;
	memset(&param, 0, sizeof(param));
	param.handle = handle;

	IARM_Result_t rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
			(char *) IARM_BUS_DSMGR_API_dsGetForceDisable4K,
			(void *) &param,
			sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		*disable = param.disable;
		return param.result;
	}

	return dsERR_GENERAL ;
}

dsError_t dsIsOutputHDR(intptr_t handle, bool *hdr)
{
    _DEBUG_ENTER();

    _RETURN_IF_ERROR(hdr != NULL, dsERR_INVALID_PARAM);

    dsIsOutputHDRParam_t param;

    param.handle = handle;
    param.hdr = false;
    param.result = dsERR_NONE;


    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                                                        (char *)IARM_BUS_DSMGR_API_dsIsOutputHDR,
                                                        (void *)&param,
                                                        sizeof(param));

        if (IARM_RESULT_SUCCESS == rpcRet)
        {
                *hdr = param.hdr;
                return param.result;
        }

        return dsERR_GENERAL ;
}

dsError_t dsResetOutputToSDR()
{
    _DEBUG_ENTER();

    bool param =true;

    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                                                        (char *)IARM_BUS_DSMGR_API_dsResetOutputToSDR,
                                                        (void *)&param,
                                                        sizeof(param));

        if (IARM_RESULT_SUCCESS != rpcRet)
        {
                return dsERR_GENERAL;
        }

        return dsERR_NONE ;
}

dsError_t dsSetHdmiPreference(intptr_t handle, dsHdcpProtocolVersion_t *hdcpProtocol)
{
    _DEBUG_ENTER();

    _RETURN_IF_ERROR(hdcpProtocol != NULL, dsERR_INVALID_PARAM);

    dsSetHdmiPreferenceParam_t param;

    param.handle = handle;
    param.hdcpCurrentProtocol = *hdcpProtocol;
    param.result = dsERR_NONE;


    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                                                        (char *)IARM_BUS_DSMGR_API_dsSetHdmiPreference,
                                                        (void *)&param,
                                                        sizeof(param));

        if (IARM_RESULT_SUCCESS == rpcRet)
        {
                return param.result;
        }

        return dsERR_GENERAL ;
}

dsError_t dsGetHdmiPreference(intptr_t handle, dsHdcpProtocolVersion_t *hdcpProtocol)
{
    _DEBUG_ENTER();

    _RETURN_IF_ERROR(hdcpProtocol != NULL, dsERR_INVALID_PARAM);

    dsGetHdmiPreferenceParam_t param;

    param.handle = handle;
    param.hdcpCurrentProtocol = dsHDCP_VERSION_MAX;
    param.result = dsERR_NONE;


    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                                                        (char *)IARM_BUS_DSMGR_API_dsGetHdmiPreference,
                                                        (void *)&param,
                                                        sizeof(param));

        if (IARM_RESULT_SUCCESS == rpcRet)
        {
                *hdcpProtocol = param.hdcpCurrentProtocol;
                return param.result;
        }

        return dsERR_GENERAL ;
}

dsError_t dsGetVideoEOTF(intptr_t handle, dsHDRStandard_t* video_eotf)
{
  _DEBUG_ENTER();

  if (video_eotf == NULL) {
      return dsERR_INVALID_PARAM;
  }

  dsEot_t param;

  memset(&param, 0, sizeof(param));
  param.handle = handle;
  param.result = dsERR_NONE;

  IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
  rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                        (char *)IARM_BUS_DSMGR_API_dsGetVideoEOTF,
                        (void *)&param,
                        sizeof(param));

  if( (IARM_RESULT_SUCCESS == rpcRet) && (dsERR_NONE == param.result))
  {
    *video_eotf = param.video_eotf;
    return dsERR_NONE;
  }

  return dsERR_GENERAL ;
}

dsError_t dsGetMatrixCoefficients(intptr_t handle, dsDisplayMatrixCoefficients_t *matrix_coefficients)
{
  _DEBUG_ENTER();

  if (matrix_coefficients == NULL) {
      return dsERR_INVALID_PARAM;
  }

  dsMatrixCoefficients_t param;

  memset(&param, 0, sizeof(param));
  param.handle = handle;
  param.result = dsERR_NONE;

  IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
  rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                        (char *)IARM_BUS_DSMGR_API_dsGetMatrixCoefficients,
                        (void *)&param,
                        sizeof(param));

  if( (IARM_RESULT_SUCCESS == rpcRet) && (dsERR_NONE == param.result))
  {
    *matrix_coefficients = param.matrix_coefficients;
    return dsERR_NONE;
  }

  return dsERR_GENERAL ;
}

dsError_t dsGetColorDepth(intptr_t handle, unsigned int* color_depth)
{
  _DEBUG_ENTER();

  if (color_depth == NULL) {
      return dsERR_INVALID_PARAM;
  }

  dsColorDepth_t param;

  memset(&param, 0, sizeof(param));
  param.handle = handle;
  param.result = dsERR_NONE;

  IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
  rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                        (char *)IARM_BUS_DSMGR_API_dsGetColorDepth,
                        (void *)&param,
                        sizeof(param));

  if( (IARM_RESULT_SUCCESS == rpcRet) && (dsERR_NONE == param.result))
  {
    *color_depth = param.color_depth;
    return dsERR_NONE;
  }

  return dsERR_GENERAL ;
}

dsError_t dsGetColorSpace(intptr_t handle, dsDisplayColorSpace_t* color_space)
{
  _DEBUG_ENTER();

  if (color_space == NULL) {
      return dsERR_INVALID_PARAM;
  }

  dsColorSpace_t param;

  memset(&param, 0, sizeof(param));
  param.handle = handle;
  param.result = dsERR_NONE;

  IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
  rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                        (char *)IARM_BUS_DSMGR_API_dsGetColorSpace,
                        (void *)&param,
                        sizeof(param));

  if( (IARM_RESULT_SUCCESS == rpcRet) && (dsERR_NONE == param.result))
  {
    *color_space = param.color_space;
    return dsERR_NONE;
  }

  return dsERR_GENERAL ;
}

dsError_t dsGetQuantizationRange(intptr_t handle, dsDisplayQuantizationRange_t* quantization_range)
{
  _DEBUG_ENTER();

  if (quantization_range == NULL) {
      return dsERR_INVALID_PARAM;
  }

  dsQuantizationRange_t param;

  memset(&param, 0, sizeof(param));
  param.handle = handle;
  param.result = dsERR_NONE;

  IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
  rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                        (char *)IARM_BUS_DSMGR_API_dsGetQuantizationRange,
                        (void *)&param,
                        sizeof(param));

  if( (IARM_RESULT_SUCCESS == rpcRet) && (dsERR_NONE == param.result))
  {
    *quantization_range = param.quantization_range;
    return dsERR_NONE;
  }

  return dsERR_GENERAL ;
}

dsError_t dsGetCurrentOutputSettings(intptr_t handle, dsHDRStandard_t* video_eotf, dsDisplayMatrixCoefficients_t* matrix_coefficients, dsDisplayColorSpace_t* color_space, unsigned int* color_depth, dsDisplayQuantizationRange_t* quantization_range)
{
  _DEBUG_ENTER();

  if (video_eotf == NULL || matrix_coefficients == NULL || color_space == NULL || color_depth == NULL || quantization_range == NULL) {
      return dsERR_INVALID_PARAM;
  }

  dsCurrentOutputSettings_t param;

  memset(&param, 0, sizeof(param));
  param.handle = handle;
  param.result = dsERR_NONE;

  IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
  rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                        (char *)IARM_BUS_DSMGR_API_dsGetCurrentOutputSettings,
                        (void *)&param,
                        sizeof(param));

  if( (IARM_RESULT_SUCCESS == rpcRet) && (dsERR_NONE == param.result))
  {
    *video_eotf = param.video_eotf;
    *matrix_coefficients = param.matrix_coefficients;
    *color_space = param.color_space;
    *color_depth = param.color_depth;
    *quantization_range = param.quantization_range;
    return dsERR_NONE;
  }

  return dsERR_GENERAL ;
}

dsError_t dsSetForceHDRMode(intptr_t handle, dsHDRStandard_t mode)
{
        _DEBUG_ENTER();

        dsForceHDRModeParam_t param;
        memset(&param, 0, sizeof(param));
        param.handle = handle;
        param.hdrMode = mode;

        IARM_Result_t rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                        (char *) IARM_BUS_DSMGR_API_dsSetForceHDRMode,
                        (void *) &param,
                        sizeof(param));

        if (IARM_RESULT_SUCCESS == rpcRet)
        {
                return param.result;
        }

        return dsERR_GENERAL ;
}

dsError_t dsSetStandbyVideoState(string port , const bool enable)
{
    dsError_t dsErr = dsERR_GENERAL;
    _DEBUG_ENTER();

    if(!port.empty())
    {
        dsMgrStandbyVideoStateParam_t param = {0};
        int n = port.length();
        strcpy(param.port, port.c_str());
        param.port[n+1] = '\n';
        param.isEnabled = enable;
        param->result = -1;

        IARM_Result_t rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                                (char *)IARM_BUS_DSMGR_API_SetStandbyVideoState,
                                (void *)&param, sizeof(param));

        if (IARM_RESULT_SUCCESS == rpcRet && param.result == 0)
        {
            dsErr = dsERR_NONE;
        }
    }
    return dsErr;
}

dsError_t dsGetStandbyVideoState(string port , bool &enable)
{
    dsError_t dsErr = dsERR_GENERAL;
    _DEBUG_ENTER();

    if(!port.empty())
    {
        dsMgrStandbyVideoStateParam_t param = {0};

        int n = port.length();
        strcpy(param.port, port.c_str());
        param.port[n+1] = '\n';
        param->result = -1;

        IARM_Result_t rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                (char *)IARM_BUS_DSMGR_API_GetStandbyVideoState,
                (void *)&param,
                sizeof(param));

        if (IARM_RESULT_SUCCESS == rpcRet && param.result == 0)
        {
            enable = param.isEnabled;
            dsErr = dsERR_NONE;
        }
    }
    return dsErr ;
}

/** @} */
/** @} */
