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
#include "dsFPD.h"
#include "dsRpc.h"
#include "dsTypes.h"
#include "dsserverlogger.h"
#include "dsMgr.h"
	
#include "iarmUtil.h"
#include "libIARM.h"
#include "libIBus.h"

#include <iostream>
#include "hostPersistence.hpp"
#include <sstream>


#define direct_list_top(list) ((list))
#define IARM_BUS_Lock(lock) pthread_mutex_lock(&fpLock)
#define IARM_BUS_Unlock(lock) pthread_mutex_unlock(&fpLock)

/* Below block allows the default brightness of a device (after a reset) to be set from 
 * device-specific recipes. If recipes say nothing, it should use max brightness supported. */
#ifndef dsFPD_BRIGHTNESS_DEFAULT
#define dsFPD_BRIGHTNESS_DEFAULT dsFPD_BRIGHTNESS_MAX 
#endif

static int m_isInitialized = 0;
static int m_isPlatInitialized=0;
static pthread_mutex_t fpLock = PTHREAD_MUTEX_INITIALIZER;

IARM_Result_t dsFPDMgr_init();
IARM_Result_t dsFPDMgr_term();
IARM_Result_t _dsFPInit(void *arg);
IARM_Result_t _dsFPTerm(void *arg);
IARM_Result_t _dsSetFPText(void *arg);
IARM_Result_t _dsSetFPTime(void *arg);
IARM_Result_t _dsSetFPScroll(void *arg);
IARM_Result_t _dsSetFPBlink(void *arg);
IARM_Result_t _dsGetFPBrightness(void *arg);
IARM_Result_t _dsSetFPBrightness(void *arg);
IARM_Result_t _dsSetFPState(void *arg);
IARM_Result_t _dsGetFPState(void *arg);
IARM_Result_t _dsSetFPColor(void *arg);
IARM_Result_t _dsGetFPColor(void *arg);
IARM_Result_t _dsSetFPTextBrightness(void *arg);
IARM_Result_t _dsGetFPTextBrightness(void *arg);
IARM_Result_t _dsFPEnableCLockDisplay(void *arg);
IARM_Result_t _dsGetTimeFormat(void *arg);
IARM_Result_t _dsSetTimeFormat(void *arg);
IARM_Result_t _dsSetFPDMode(void *arg);

/*TBD - Only Text and Power Brigghtness settings for the time being
 * Create an Array of all inidcator and test display
*/
static  dsFPDBrightness_t _dsPowerBrightness = dsFPD_BRIGHTNESS_MAX ;
static  dsFPDBrightness_t _dsTextBrightness  = dsFPD_BRIGHTNESS_MAX ;
static  dsFPDColor_t     _dsPowerLedColor   = dsFPD_COLOR_BLUE;
static  dsFPDTimeFormat_t _dsTextTimeFormat	= dsFPD_TIME_12_HOUR;
static  dsFPDMode_t _dsFPDMode  = dsFPD_MODE_ANY;




/** Structure that defines internal data base for the FP */
typedef struct _dsFPDSettings_t_
{   
    dsFPDBrightness_t brightness;
    dsFPDState_t state;
    dsFPDColor_t color;
}_FPDSettings_t;

/** Variable that stores the brightness and State for FP */
static _FPDSettings_t srvFPDSettings[dsFPD_INDICATOR_MAX];



using namespace std;

std::string numberToString (int number);
int stringToNumber (std::string text);
std::string enumToColor (dsFPDColor_t enumColor);

std::string numberToString (int number)
{
    stringstream convert;
    convert << number;
    return convert.str();
}


int stringToNumber (std::string text)
{
    int number;
    stringstream convert (text);

    if (!(convert >> number) )
    number = 0;

	return number;
}

std::string enumToColor (dsFPDColor_t enumColor)
{
	std::string color;
    stringstream convert;

    switch (enumColor)
    {
		
		case dsFPD_COLOR_BLUE:
			color = "BLUE";
		break;
		
		case dsFPD_COLOR_GREEN:
			color = "GREEN";
		break;

		case dsFPD_COLOR_RED:
			color = "RED";
		break;

		case dsFPD_COLOR_YELLOW:
			color = "YELLOW";
		break;

		case dsFPD_COLOR_ORANGE:
			color = "RED";
		break;

		default:
			convert << enumColor;
			return convert.str();
	}
	return color;
}


IARM_Result_t dsFPDMgr_init()
{


	IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsFPInit,_dsFPInit);
	
	try
	{
		/* Init the Power and Clock Brightness */
		string value;
		int maxBrightness = dsFPD_BRIGHTNESS_DEFAULT;	

		value = device::HostPersistence::getInstance().getProperty("Power.brightness", numberToString(maxBrightness));
		_dsPowerBrightness = stringToNumber(value);
		
		value = device::HostPersistence::getInstance().getProperty("Text.brightness", numberToString(maxBrightness));
		_dsTextBrightness = stringToNumber(std::move(value));

#if(dsFPD_BRIGHTNESS_DEFAULT != dsFPD_BRIGHTNESS_MAX)
		/* If we're applying a default brightness that's not MAX, check for currently persisted values. If any of them = MAX,
		 * update those to the current default. If the persisted values != MAX, leave them alone because they're likely to have been
		 * set by the user. */
		if(dsFPD_BRIGHTNESS_MAX == _dsPowerBrightness)
		{
			INT_INFO("Applying new default brightness to power indicator. Changing from %d to %d.\n", _dsPowerBrightness, dsFPD_BRIGHTNESS_DEFAULT);
			_dsPowerBrightness = dsFPD_BRIGHTNESS_DEFAULT;
		}
		if(dsFPD_BRIGHTNESS_MAX == _dsTextBrightness)
		{
			INT_INFO("Applying new default brightness to text indicator. Changing from %d to %d.\n", _dsTextBrightness, dsFPD_BRIGHTNESS_DEFAULT);
			_dsTextBrightness = dsFPD_BRIGHTNESS_DEFAULT;
		}
#endif
		
		INT_INFO("Power Brightness Read from Persistent is %d \r\n",_dsPowerBrightness);
		INT_INFO("Text Brightness Read from Persistent is %d \r\n",_dsTextBrightness);

		#ifdef HAS_CLOCK_DISPLAY
			string _TimeFormat("12_HOUR");
			/* Get the Time Format from Persistence */
			_TimeFormat = device::HostPersistence::getInstance().getProperty("Text.timeformat",_TimeFormat);
			if (_TimeFormat.compare("12_HOUR") == 0)
			{
				_dsTextTimeFormat = dsFPD_TIME_12_HOUR;
				device::HostPersistence::getInstance().persistHostProperty("Text.timeformat","12_HOUR");
			}
			else if (_TimeFormat.compare("24_HOUR") == 0)
			{
				_dsTextTimeFormat = dsFPD_TIME_24_HOUR;
				device::HostPersistence::getInstance().persistHostProperty("Text.timeformat","24_HOUR");
			}
			INT_INFO("Clock Time Format Read from Persistent is %s \r\n",_TimeFormat.c_str());
		#endif

	}
	catch(...)
	{
		INT_ERROR("Error in Reading Brightness Value On Startup.. Use Default value \r\n");
	}

	return IARM_RESULT_SUCCESS;
}

IARM_Result_t dsFPDMgr_term()
{
   return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsFPInit(void *arg)
{
    IARM_BUS_Lock(lock);


    if (!m_isInitialized) {

    	INT_INFO("<<<<< called _dsFPInit >>>>>>>>\r\n");

		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsFPTerm,_dsFPTerm);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetFPText,_dsSetFPText);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetFPTime,_dsSetFPTime);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetFPScroll,_dsSetFPScroll);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetFPBlink,_dsSetFPBlink);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetFPBrightness,_dsGetFPBrightness);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetFPState,_dsSetFPState);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetFPState,_dsGetFPState);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetFPBrightness,_dsSetFPBrightness);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetFPColor,_dsSetFPColor);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetFPColor,_dsGetFPColor);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetFPTextBrightness,_dsGetFPTextBrightness);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetFPTextBrightness,_dsSetFPTextBrightness);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsFPEnableCLockDisplay,_dsFPEnableCLockDisplay);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetTimeFormat,_dsGetTimeFormat);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetTimeFormat,_dsSetTimeFormat);
		IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetFPDMode,_dsSetFPDMode);
		
        
		memset (srvFPDSettings, 0, sizeof (srvFPDSettings));
		
		for (int i = dsFPD_INDICATOR_MESSAGE; i < dsFPD_INDICATOR_MAX; i++)
		{
			srvFPDSettings[i].brightness = dsFPD_BRIGHTNESS_MAX;
			srvFPDSettings[i].state = dsFPD_STATE_OFF;
                        srvFPDSettings[i].color = dsFPD_COLOR_BLUE;
		}

        m_isInitialized = 1;
    }

    if (!m_isPlatInitialized) {
        dsFPInit();
        m_isPlatInitialized = 1;
    }

    IARM_BUS_Unlock(lock);
	
    return IARM_RESULT_SUCCESS;
}
IARM_Result_t _dsFPTerm(void *arg)
{
    /*@TODO: Deregister the calls, if necessary */
    _DEBUG_ENTER();


	IARM_BUS_Lock(lock);

	if (m_isPlatInitialized) {
        dsFPTerm();
        m_isPlatInitialized = 0;
    }

    IARM_BUS_Unlock(lock);
	
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsSetFPText(void *arg)
{
    _DEBUG_ENTER();
   
    IARM_Result_t ret = IARM_RESULT_SUCCESS; 
     #ifdef HAS_CLOCK_DISPLAY
    IARM_BUS_Lock(lock);

    if ((_dsFPDMode  == dsFPD_MODE_ANY) || (_dsFPDMode  == dsFPD_MODE_TEXT)) {
	dsError_t dsStatus = dsSetFPText((char *) arg);
        if(dsStatus != dsERR_NONE)
        {
            ret = IARM_RESULT_INVALID_PARAM;
        }
    }
    else {
       INT_INFO("_dsSetFPText: Not setting Text, Clock mode enabled \r\n");
    }

    IARM_BUS_Unlock(lock);
    #endif
	
	return ret;
}

IARM_Result_t _dsSetFPTime(void *arg)
{
    _DEBUG_ENTER();
   
    IARM_Result_t ret = IARM_RESULT_SUCCESS; 
    #ifdef HAS_CLOCK_DISPLAY
    IARM_BUS_Lock(lock);

    if ((_dsFPDMode  == dsFPD_MODE_ANY) || (_dsFPDMode  == dsFPD_MODE_CLOCK)) {
	dsFPDTimeParam_t *param = (dsFPDTimeParam_t *)arg;
        dsError_t dsStatus = dsSetFPTime(_dsTextTimeFormat, param->nHours, param->nMinutes);
        if(dsStatus != dsERR_NONE)
        {
            ret = IARM_RESULT_INVALID_PARAM;
        }
    }
    else {
       INT_INFO("_dsSetFPTime: Not setting Clock, Text mode enabled \r\n");
    }

    IARM_BUS_Unlock(lock);
    #endif
	
	return ret;
}

IARM_Result_t _dsSetFPScroll(void *arg)
{
    _DEBUG_ENTER();
   	
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
   	#ifdef HAS_CLOCK_DISPLAY
    IARM_BUS_Lock(lock);

	dsFPDScrollParam_t *param = (dsFPDScrollParam_t *)arg;
    dsError_t dsStatus = dsSetFPScroll(param->nScrollHoldOnDur, param->nHorzScrollIterations, param->nVertScrollIterations);
    if(dsStatus != dsERR_NONE)
    {
        ret = IARM_RESULT_INVALID_PARAM;
    }

    IARM_BUS_Unlock(lock);
    #endif

	return ret;
}

IARM_Result_t _dsSetFPBlink(void *arg)
{
    _DEBUG_ENTER();
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    IARM_BUS_Lock(lock);

	dsFPDBlinkParam_t *param = (dsFPDBlinkParam_t *)arg;
    dsError_t dsStatus = dsSetFPBlink(param->eIndicator, param->nBlinkDuration, param->nBlinkIterations);
    if(dsStatus != dsERR_NONE)
    {
        ret = IARM_RESULT_INVALID_PARAM;
    }

    IARM_BUS_Unlock(lock);
	return ret;
}

IARM_Result_t _dsGetFPBrightness(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

	dsFPDBrightParam_t *param = (dsFPDBrightParam_t *)arg;
    dsGetFPBrightness(param->eIndicator, &param->eBrightness);

	/*
		* Power LED Indicator Brightness is the Global LED brightness
		* for all indicators
	*/

	if(param->toPersist)
	{
	     param->eBrightness = _dsPowerBrightness;
	}

	INT_INFO("_dsGetFPBrightness Power  Brightness is %d _dsPowerBrightness: %d from persist: %d   \r\n",param->eBrightness,_dsPowerBrightness,param->toPersist);

	IARM_BUS_Unlock(lock);
	return IARM_RESULT_SUCCESS;

}



IARM_Result_t _dsSetFPBrightness(void *arg)
{
    _DEBUG_ENTER();
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    IARM_BUS_Lock(lock);

	dsFPDBrightParam_t *param = (dsFPDBrightParam_t *)arg;

	
	if (param->eBrightness <= dsFPD_BRIGHTNESS_MAX)
    {
		dsError_t dsStatus = dsSetFPBrightness(param->eIndicator, param->eBrightness);
                if(dsStatus == dsERR_NONE)
                {
		    srvFPDSettings[param->eIndicator].brightness = param->eBrightness;

		    try{
			switch (param->eIndicator)
			{
				case dsFPD_INDICATOR_POWER:
				{	
					 INT_INFO("_dsSetFPBrightness Power Brightness From  App is %d \r\n",param->eBrightness);
					
					if(param->toPersist)
					{	
						_dsPowerBrightness =  param->eBrightness;
						device::HostPersistence::getInstance().persistHostProperty("Power.brightness", numberToString(_dsPowerBrightness));
					}
					break;
				}

				case dsFPD_INDICATOR_MESSAGE:
				case dsFPD_INDICATOR_RECORD:
				case dsFPD_INDICATOR_REMOTE:
				case dsFPD_INDICATOR_RFBYPASS:
				default:
				{	
					break;
				}
			}
		    }    
		    catch(...)
		    {
			INT_ERROR("Error in Persisting the Power Brightness Value \r\n");
		    }
                }
                else
                {
                    ret = IARM_RESULT_INVALID_PARAM;
                }

	}
        else
        {
            ret = IARM_RESULT_INVALID_PARAM;
        }
	//INT_DEBUG("_dsSetFPBrightness Power Brighnes is %d \r\n",_dsPowerBrightness);
	//INT_DEBUG("_dsSetFPBrightness Text Brighnes is %d \r\n",_dsTextBrightness);

    IARM_BUS_Unlock(lock);
	return ret;
}


IARM_Result_t _dsGetFPTextBrightness(void *arg)
{
    _DEBUG_ENTER();

	#ifdef HAS_CLOCK_DISPLAY
		IARM_BUS_Lock(lock);
		
		dsFPDTextBrightParam_t *param = (dsFPDTextBrightParam_t *)arg;
		/*
			* Text Display Indicator Brightness
		*/
		param->eBrightness = _dsTextBrightness;
		INT_INFO("_dsGetFPTextBrightness Brightness is %d \r\n",param->eBrightness);

		IARM_BUS_Unlock(lock);
	#endif
	return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsSetFPTextBrightness(void *arg)
{
    _DEBUG_ENTER();
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
	#ifdef HAS_CLOCK_DISPLAY
    IARM_BUS_Lock(lock);

	dsFPDTextBrightParam_t *param = (dsFPDTextBrightParam_t *)arg;

	
	if (param->eBrightness <= dsFPD_BRIGHTNESS_MAX)
    {
		dsError_t dsStatus = dsSetFPTextBrightness(param->eIndicator, param->eBrightness);
                if(dsStatus == dsERR_NONE)
                {
		    try
		    {
			switch (param->eIndicator)
			{
				case dsFPD_TEXTDISP_TEXT:
				{	
					 INT_INFO("_dsSetFPTextBrightness Brightness frm App is %d \r\n",param->eBrightness);
					_dsTextBrightness = param->eBrightness;
					device::HostPersistence::getInstance().persistHostProperty("Text.brightness",numberToString(_dsTextBrightness));
					break;
				}
				default:
				{	
					break;
				}
			}
		    }
		    catch(...)
		    {
			INT_ERROR("Error in Persisting the Text Brightness Value \r\n");
		    }
                }
                else
                {
                    ret = IARM_RESULT_INVALID_PARAM;
                }     
		
	}
        else
        {
            ret = IARM_RESULT_INVALID_PARAM;
        }
	
	IARM_BUS_Unlock(lock);

	#endif
	return ret;
}


IARM_Result_t _dsGetFPColor(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

	dsFPDColorParam_t *param = (dsFPDColorParam_t *)arg;
        param->eColor = srvFPDSettings[param->eIndicator].color;
	IARM_BUS_Unlock(lock);
	return IARM_RESULT_SUCCESS;

}

IARM_Result_t _dsSetFPColor(void *arg)
{
    _DEBUG_ENTER();
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    IARM_BUS_Lock(lock);

	dsFPDColorParam_t *param = (dsFPDColorParam_t *)arg;
    dsError_t dsStatus = dsSetFPColor(param->eIndicator, param->eColor);
    if(dsStatus == dsERR_NONE)
    {
    param->eColor &= 0x00FFFFFF;
    srvFPDSettings[param->eIndicator].color = param->eColor;
    INT_INFO("_dsSetFPColor Value  From  App is %d for Indicator %d \r\n",param->eColor,param->eIndicator);
    try{
			switch (param->eIndicator)
			{
				case dsFPD_INDICATOR_POWER:
				{	
					if(param->toPersist)
					{	
                                                _dsPowerLedColor = param->eColor;
						device::HostPersistence::getInstance().persistHostProperty("Power.Color",enumToColor(param->eColor));
					}
					break;
				}
				case dsFPD_INDICATOR_MESSAGE:
				case dsFPD_INDICATOR_RECORD:
				case dsFPD_INDICATOR_REMOTE:
				case dsFPD_INDICATOR_RFBYPASS:
				default:
				{	
					break;
				}
			}
		}
		catch(...)
		{
			INT_ERROR("Error in Persisting the Color  Value \r\n");
		}
    }
    else
    {
       INT_ERROR("Error in dsSetFPColor dsStatus:%d \r\n",dsStatus);
       ret = IARM_RESULT_INVALID_PARAM;
    }
    IARM_BUS_Unlock(lock);
	return ret;
}



IARM_Result_t _dsFPEnableCLockDisplay(void *arg)
{

    IARM_Result_t ret = IARM_RESULT_SUCCESS;
#ifdef HAS_CLOCK_DISPLAY
	IARM_BUS_Lock(lock);
	int *enable = (int *)arg;
	int lenable = *enable;
    dsError_t dsStatus = dsFPEnableCLockDisplay(lenable);
    if(dsStatus != dsERR_NONE)
    {
        ret = IARM_RESULT_INVALID_PARAM;
    }
	IARM_BUS_Unlock(lock);
#endif
	return ret;
}


IARM_Result_t _dsSetFPState(void *arg)
{
    _DEBUG_ENTER();
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    IARM_BUS_Lock(lock);

	dsFPDStateParam_t *param = (dsFPDStateParam_t *)arg;
        dsError_t dsStatus = dsERR_NONE;

	if (param->state == dsFPD_STATE_ON)
    {
		/*
			* Power LED Indicator Brightness is the Global LED brightness
			* for all indicators
		*/
		dsStatus = dsSetFPBrightness(param->eIndicator,_dsPowerBrightness);
                if(dsStatus == dsERR_NONE)
                {
		    if(param->eIndicator == dsFPD_INDICATOR_POWER)
			INT_INFO("_dsSetFPState Setting Power LED to ON with Brightness %d \r\n",_dsPowerBrightness);
	
		    srvFPDSettings[param->eIndicator].state = param->state;
                }
                else
                {
                    ret = IARM_RESULT_INVALID_PARAM;
                }
	}
	else if (param->state == dsFPD_STATE_OFF)
	{
		dsStatus = dsSetFPBrightness(param->eIndicator,0);
                if(dsStatus == dsERR_NONE)
                {
		    if(param->eIndicator == dsFPD_INDICATOR_POWER)
			INT_INFO("_dsSetFPState Setting Power LED to OFF with Brightness 0 \r\n");  //CID:127891 - Print_args

		    srvFPDSettings[param->eIndicator].state = param->state;
                }
                else
                {
                    ret = IARM_RESULT_INVALID_PARAM;
                }
	}

    IARM_BUS_Unlock(lock);
	return ret;
}




IARM_Result_t _dsGetTimeFormat(void *arg)
{
    _DEBUG_ENTER();
    
#ifdef HAS_CLOCK_DISPLAY
    IARM_BUS_Lock(lock);

   	dsFPDTimeFormatParam_t *param = (dsFPDTimeFormatParam_t *)arg;

	if( param == NULL)
	{
		INT_INFO("Error:_dsGetTimeFormat : NULL Param ... \r\n");
		IARM_BUS_Unlock(lock);
		return IARM_RESULT_INVALID_PARAM;
	}	

   	param->eTime = _dsTextTimeFormat;
	
    IARM_BUS_Unlock(lock);
  #endif 
	return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsSetTimeFormat(void *arg)
{
    _DEBUG_ENTER();
 	
	#ifdef HAS_CLOCK_DISPLAY
	    
	    IARM_BUS_Lock(lock);

	    dsFPDTimeFormatParam_t *param = (dsFPDTimeFormatParam_t *)arg;

	    if( param == NULL)
	    {
	    	INT_INFO("Error:_dsSetTimeFormat : NULL Param ... \r\n");
	    	IARM_BUS_Unlock(lock);
	    	return IARM_RESULT_INVALID_PARAM;
	    }	

	    if (param->eTime != _dsTextTimeFormat)
	    {
	    	_dsTextTimeFormat = param->eTime;
			
			if (param->eTime == dsFPD_TIME_12_HOUR)
			{
				device::HostPersistence::getInstance().persistHostProperty("Text.timeformat","12_HOUR");
				INT_INFO("Clock Time Format Updated to 12_HOUR ... \r\n");
			}
			else if (param->eTime == dsFPD_TIME_24_HOUR)
			{
				device::HostPersistence::getInstance().persistHostProperty("Text.timeformat","24_HOUR");
				INT_INFO("Clock Time Format Updated to 24_HOUR ... \r\n");
			}
	    
		   	/* Send Time Format Event */
		   	IARM_Bus_DSMgr_EventData_t _eventData;
	    	IARM_Bus_DSMgr_EventId_t _eventId;

	    	_eventData.data.FPDTimeFormat.eTimeFormat =  _dsTextTimeFormat;
	    	_eventId = IARM_BUS_DSMGR_EVENT_TIME_FORMAT_CHANGE;
	    	
	    	IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)_eventId,(void *)&_eventData, sizeof(_eventData));
		    
		    INT_INFO("Sent Clock IARM_BUS_DSMGR_EVENT_TIME_FORMAT_CHANGE event ... \r\n");
		}

	    IARM_BUS_Unlock(lock);

    #endif
	
	return IARM_RESULT_SUCCESS;
}


IARM_Result_t _dsGetFPState(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

	dsFPDStateParam_t *param = (dsFPDStateParam_t *)arg;

	if(param->eIndicator < dsFPD_INDICATOR_MAX)
    {
   		param->state = srvFPDSettings[param->eIndicator].state;
	}
	
    IARM_BUS_Unlock(lock);
	return IARM_RESULT_SUCCESS;
}


IARM_Result_t _dsSetFPDMode(void *arg)
{
    _DEBUG_ENTER();

                IARM_BUS_Lock(lock);

                dsFPDModeParam_t *param = (dsFPDModeParam_t *)arg;
                if ((param->eMode == dsFPD_MODE_ANY) || (param->eMode == dsFPD_MODE_TEXT) || (param->eMode == dsFPD_MODE_CLOCK)) {
                    _dsFPDMode = param->eMode;
                    INT_INFO("_dsSetFPDMode: Mode set to %d \r\n",param->eMode);
                }
                else {
                    INT_INFO("Error:_dsSetFPDMode : Invalid Param ... \r\n");
                    IARM_BUS_Unlock(lock);
                    return IARM_RESULT_INVALID_PARAM;
                }

                IARM_BUS_Unlock(lock);
        return IARM_RESULT_SUCCESS;
}


/** @} */
/** @} */
