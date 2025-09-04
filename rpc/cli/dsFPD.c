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
#include "dsFPD.h"
#include "dsRpc.h"
#include "dsMgr.h"
#include "dsclientlogger.h"
#include "iarmUtil.h"
#include "libIARM.h"
#include "libIBus.h"
#include "dsInternal.h"



dsError_t dsFPInit (void)
{
    printf("<<<<< Front Panel is initialized in Multi-App Mode >>>>>>>>\r\n");

    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                            (char *)IARM_BUS_DSMGR_API_dsFPInit,
                            NULL,
                            0);
  
	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}

	return dsERR_GENERAL;
}


dsError_t dsFPTerm(void)
{
    _DEBUG_ENTER();

   IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
   rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                            (char *)IARM_BUS_DSMGR_API_dsFPTerm,
                            NULL,
                            0);
	
	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}

	return dsERR_GENERAL ;
}

dsError_t dsSetFPText(const char* pszChars)
{
    _DEBUG_ENTER();
    _RETURN_IF_ERROR(pszChars != NULL, dsERR_INVALID_PARAM);

    int len = strlen(pszChars),i=0;
	unsigned char text[128]={'\0'};

	for (i=0;i<len ;i++ )
	{
		text[i] = pszChars[i];
	}
	text[i] = '\0';
 
   IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

   rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsSetFPText,
							&text[0],
							len+1);

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}
 
	return dsERR_GENERAL ;
 
 }

dsError_t dsSetFPTime (dsFPDTimeFormat_t eTime, const unsigned int uHour, const unsigned int uMinutes)
{
    _DEBUG_ENTER();
   
	dsFPDTimeParam_t param ;

    param.eTime = eTime;
    param.nHours = uHour;
    param.nMinutes = uMinutes;

	
   IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

   rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsSetFPTime,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}
return dsERR_GENERAL ;
}

dsError_t dsSetFPScroll(unsigned int nScrollHoldOnDur, unsigned int nHorzScrollIterations, unsigned int nVertScrollIterations)
{
    _DEBUG_ENTER();

	dsFPDScrollParam_t param ;

    
    param.nScrollHoldOnDur = nScrollHoldOnDur;
    param.nHorzScrollIterations = nHorzScrollIterations;
    param.nVertScrollIterations = nVertScrollIterations;
  
   IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsSetFPScroll,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}

	return dsERR_GENERAL ;
}

dsError_t dsSetFPBlink (dsFPDIndicator_t eIndicator, unsigned int nBlinkDuration, unsigned int nBlinkIterations)
{
    _DEBUG_ENTER();

    dsFPDBlinkParam_t param ;

    
    param.eIndicator = eIndicator;
    param.nBlinkDuration = nBlinkDuration;
    param.nBlinkIterations = nBlinkIterations;

    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsSetFPBlink,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}

	return dsERR_GENERAL ;
}

dsError_t dsGetFPBrightness (dsFPDIndicator_t eIndicator, dsFPDBrightness_t *pBrightness)
{
    _DEBUG_ENTER();

	dsFPDBrightParam_t param ;

    param.eIndicator = eIndicator;
    param.eBrightness = 0;
	
	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsGetFPBrightness,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		*pBrightness = param.eBrightness;
		return dsERR_NONE;
	}

	return dsERR_GENERAL ;
}

dsError_t dsGetFPDBrightness (dsFPDIndicator_t eIndicator, dsFPDBrightness_t *pBrightness, bool persist)
{
    _DEBUG_ENTER();
        dsFPDBrightParam_t param ;
    param.eIndicator = eIndicator;
    param.eBrightness = 0;
    param.toPersist = persist;
        IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
        rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                                                        (char *)IARM_BUS_DSMGR_API_dsGetFPBrightness,
                                                        (void *)&param,
                                                        sizeof(param));
        if (IARM_RESULT_SUCCESS == rpcRet)
        {
                *pBrightness = param.eBrightness;
                return dsERR_NONE;
        }
        return dsERR_GENERAL ;
}

dsError_t dsSetFPBrightness (dsFPDIndicator_t eIndicator, dsFPDBrightness_t eBrightness)
{
    _DEBUG_ENTER();
    dsFPDBrightParam_t param ;
   
    if(eIndicator >= dsFPD_INDICATOR_MAX || eBrightness > 100)
    {
        return dsERR_INVALID_PARAM;
    }
    param.eIndicator = eIndicator;
    param.eBrightness = eBrightness;
	param.toPersist	= true;

	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsSetFPBrightness,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}
	return dsERR_GENERAL ;
}



dsError_t dsSetFPDBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t eBrightness,bool toPersist)
{
    _DEBUG_ENTER();
    dsFPDBrightParam_t param ;
   
    if(eIndicator >= dsFPD_INDICATOR_MAX || eBrightness > 100)
    {
        return dsERR_INVALID_PARAM;
    }

    param.eIndicator = eIndicator;
    param.eBrightness = eBrightness;
	param.toPersist	= toPersist;

	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsSetFPBrightness,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}
	return dsERR_GENERAL ;
}

dsError_t dsGetFPTextBrightness (dsFPDTextDisplay_t eIndicator, dsFPDBrightness_t *pBrightness)
{
    _DEBUG_ENTER();

	dsFPDTextBrightParam_t param ;

    param.eIndicator = eIndicator;
    param.eBrightness = 0;
	
	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsGetFPTextBrightness,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		*pBrightness = param.eBrightness;
		return dsERR_NONE;
	}

	return dsERR_GENERAL ;
}

dsError_t dsSetFPTextBrightness (dsFPDTextDisplay_t eIndicator, dsFPDBrightness_t eBrightness)
{
    _DEBUG_ENTER();
    dsFPDTextBrightParam_t param ;
   
    param.eIndicator = eIndicator;
    param.eBrightness = eBrightness;
	
	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsSetFPTextBrightness,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}
	return dsERR_GENERAL ;
}

dsError_t dsGetFPColor (dsFPDIndicator_t eIndicator, dsFPDColor_t *pColor)
{
    _DEBUG_ENTER();
    dsFPDColorParam_t param ;

	param.eIndicator = eIndicator;
    param.eColor = 0;
    

	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
	
	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsGetFPColor,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		*pColor = param.eColor;
		 return dsERR_NONE;
	}
	return dsERR_GENERAL ;
}



dsError_t dsSetFPColor (dsFPDIndicator_t eIndicator, dsFPDColor_t eColor)
{
    _DEBUG_ENTER();
    dsFPDColorParam_t param ;

	param.eIndicator = eIndicator;
    param.eColor = eColor;
    param.toPersist	= true;

	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
	
	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsSetFPColor,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}
	return dsERR_GENERAL ;
}


dsError_t dsSetFPDColor (dsFPDIndicator_t eIndicator, dsFPDColor_t eColor,bool toPersist)
{
    _DEBUG_ENTER();
    dsFPDColorParam_t param ;

	param.eIndicator = eIndicator;
    param.eColor = eColor;
    param.toPersist	= toPersist;

    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
	
	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsSetFPColor,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}
	return dsERR_GENERAL ;
}

dsError_t dsFPEnableCLockDisplay (int enable)
{
    _DEBUG_ENTER();
    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;
	int ienable = enable;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsFPEnableCLockDisplay,
							(void *)&ienable,
							sizeof(ienable));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}
	return dsERR_GENERAL ;
}



dsError_t dsSetFPState (dsFPDIndicator_t eIndicator, dsFPDState_t state)
{
    _DEBUG_ENTER();
    dsFPDStateParam_t param ;
   
    param.eIndicator = eIndicator;
    param.state = state;
	
	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsSetFPState,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}
	return dsERR_GENERAL ;
}

dsError_t dsSetLEDStatus(dsFPDIndicator_t eIndicator, dsFPDState_t state)
{
    _DEBUG_ENTER();
    dsFPDStateParam_t param ;
   
    param.eIndicator = eIndicator;
    param.state = state;

    IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

    rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                (char *)IARM_BUS_DSMGR_API_dsSetLEDStatus,
                (void *)&param,
                sizeof(param));

    if (IARM_RESULT_SUCCESS == rpcRet)
    {
        return dsERR_NONE;
    }
    return dsERR_GENERAL ;
}

dsError_t dsGetFPTimeFormat (dsFPDTimeFormat_t *pTimeFormat)
{
    _DEBUG_ENTER();
	dsFPDTimeFormatParam_t  param ;
  
    param.eTime = dsFPD_TIME_12_HOUR;

	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsGetTimeFormat,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		*pTimeFormat = param.eTime;
		return dsERR_NONE;
	}
	return dsERR_GENERAL ;
}


dsError_t dsSetFPTimeFormat (dsFPDTimeFormat_t eTimeFormat)
{
    _DEBUG_ENTER();
	dsFPDTimeFormatParam_t  param ;
  
    param.eTime = eTimeFormat;
		
	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsSetTimeFormat,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		return dsERR_NONE;
	}
	return dsERR_GENERAL ;
}


dsError_t dsGetFPState(dsFPDIndicator_t eIndicator, dsFPDState_t* state)
{
    _DEBUG_ENTER();
    dsFPDStateParam_t param ;
   
    param.eIndicator = eIndicator;
    param.state = dsFPD_STATE_OFF;
    
	IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

	rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
							(char *)IARM_BUS_DSMGR_API_dsGetFPState,
							(void *)&param,
							sizeof(param));

	if (IARM_RESULT_SUCCESS == rpcRet)
	{
		*state = param.state;
		return dsERR_NONE;
	}
	return dsERR_GENERAL ;
}


dsError_t dsSetFPDMode (dsFPDMode_t eMode)
{
#ifndef USE_WPE_THUNDER_PLUGIN
    _DEBUG_ENTER();
        dsFPDModeParam_t  param ;

    param.eMode = eMode;

        IARM_Result_t rpcRet = IARM_RESULT_SUCCESS;

        rpcRet = IARM_Bus_Call(IARM_BUS_DSMGR_NAME,
                                                        (char *)IARM_BUS_DSMGR_API_dsSetFPDMode,
                                                        (void *)&param,
                                                        sizeof(param));

        if (IARM_RESULT_SUCCESS == rpcRet)
        {
                return dsERR_NONE;
        }
#else
    printf("%s currently not supported in Thunder Framework\n",__FUNCTION__);
#endif
        return dsERR_GENERAL ;
}

/** @} */
/** @} */
