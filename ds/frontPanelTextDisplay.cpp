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
 * @file frontPanelTextDisplay.cpp
 * @brief Configuration of individual text display sub-panel to display system time or text is managed here.
 * The scroll speed, time format (12Hour or 24 Hour format) and a string to display can be configured.
 */



/**
* @defgroup devicesettings
* @{
* @defgroup ds
* @{
**/


#include <iostream>
#include <sstream>

#include "frontPanelTextDisplay.hpp"
#include "frontPanelConfig.hpp"
#include "illegalArgumentException.hpp"
#include "host.hpp"

#include "dslogger.h"
#include "dsError.h"
#include "dsTypes.h"

using namespace std;

std::string numToStr (int number)
{
    stringstream convert;
    convert << number;
    return convert.str();
}

int strToNum (std::string text)
{
    int number;
    stringstream convert (text);

    if (!(convert >> number) )
                number = 0;

    return number;
}


namespace {
	const char *_names[] = {
			"Text",
	};

	inline bool isValid(int id) {
		return dsFPDTextDisplay_isValid(id);
	}
}


namespace device {

const int FrontPanelTextDisplay::kModeClock12Hr = dsFPD_TIME_12_HOUR;
const int FrontPanelTextDisplay::kModeClock24Hr = dsFPD_TIME_24_HOUR;
const char * FrontPanelTextDisplay::kPropertyBrightness = ".brightness";

/**
 * @addtogroup dssettingsfptextdisplayapi
 * @{
 */ 

/**
 * @fn FrontPanelTextDisplay::getInstance(int id)
 * @brief This API gets the FrontPanelIndicator instance corresponding to the specified id, only if the id
 * passed is valid.
 *
 * @param[in] id Indicates the id of the FrontPanelTextDisplay whose instance has to be returned.
 *
 * @return Returns an instance of FrontPanelTextDisplay corresponding to the id parameter or else throws an
 * IllegalArgumentException indicating that the instance corresponding to the id was not found.
 */
FrontPanelTextDisplay & FrontPanelTextDisplay::getInstance(int id)
{
	if (::isValid(id)) {
		return FrontPanelConfig::getInstance().getTextDisplay(id);
	}
	else {
		throw IllegalArgumentException();
	}

}


/**
 * @fn FrontPanelTextDisplay::getInstance(const std::string &name)
 * @brief This API gets the FrontPanelTextDisplay instance based on the name parameter passed as input.
 * <ul>
 * <li> Text is the valid name for front panel text display.
 * </ul>
 *
 * @param[in] name Indicates the name against which the FrontPanelTextDisplay instance has to be returned.
 *
 * @return Returns the FrontPanelTextDisplay instance corresponding to the name parameter else throws
 * an IllegalArgumentException if the instance corresponding to the name parameter is not found.
 */
FrontPanelTextDisplay & FrontPanelTextDisplay::getInstance(const std::string &name)
{
	for (size_t i = 0; i < dsUTL_DIM(_names); i++) {
		if (name.compare(_names[i]) == 0) {
			return FrontPanelConfig::getInstance().getTextDisplay(i);
		}
	}

	throw IllegalArgumentException();
}


/**
 * @fn FrontPanelTextDisplay(int id, int maxBrightness, int maxCycleRate, int levels,
 * int maxHorizontalIterations, int maxVerticalIterations, const string &supportedCharacters,int colorMode)
 * @brief This function is the default constructor for FrontPanelTextDisplay. It initializes the data members
 * of FrontPanelTextDisplay instance with the parameters passed.
 *
 * @param[in] id Indicates the id for the FrontPanelTextDisplay instance.
 * @param[in] maxBrightness Indicates maximum brightness value for the text display which is 100.
 * @param[in] maxCycleRate Indicates maximum cycle rate for the text display which is 2 by default.
 * @param[in] levels Indicates brightness level for the text display.
 * @param[in] maxHorizontalIterations Indicates maximum horizontal iterations for scrolling.
 * @param[in] maxVerticalIterations Indicates maximum vertical iterations for scrolling.
 * @param[in] supportedCharacters Indicates supported characters for text display. The default supported
 * supported characters are ABCDEF.
 * @param[in] colorMode Indicates color mode for the text display.
 *
 * @return None
 */
FrontPanelTextDisplay::FrontPanelTextDisplay(int id, int maxBrightness, int maxCycleRate, int levels,
                                             int maxHorizontalIterations, int maxVerticalIterations,
                                             const string &supportedCharacters,int colorMode):
                                             FrontPanelIndicator(id, maxBrightness, maxCycleRate, levels, colorMode)
{
  _TextBrightness = 0;   //CID:80988 - Uninit_ctor
  _timeFormat = kModeClock24Hr;
  _scroll = Scroll(maxVerticalIterations, maxHorizontalIterations);
	if (::isValid(id)) {
		_id = id;
		_name = std::string(_names[id]);
	}
	else {
		throw IllegalArgumentException();
	}
}

FrontPanelTextDisplay::~FrontPanelTextDisplay()
{
}


/**
 * @fn FrontPanelTextDisplay::setText(const std::string &text)
 * @brief This API sets the text LED display, by switching the text display to text mode.
 *
 * @param[in] text Indicates the text to be displayed.
 *
 * @return None
 */
void FrontPanelTextDisplay::setText(const std::string &text)
{
    dsError_t ret = dsSetFPText(text.c_str());
    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }
}


/**
 * @fn FrontPanelTextDisplay::enableDisplay(const int enable)
 * @brief This function is used to enable or disable the display of clock on front panel.
 *
 * @param[in] enable Indicates enable or disable value.
 *
 * @return None
 */
void FrontPanelTextDisplay::enableDisplay(const int enable)
{
	dsError_t ret = dsFPEnableCLockDisplay(enable);
        if (ret != dsERR_NONE) {
            throw Exception(ret);
        }
}

/**
 * @fn FrontPanelTextDisplay::SetLEDStatus
 * @brief This function is used to Set LED status on front panel.
 *
 * @param[in] fpdIndicator.
 * @param[in] fpdState.
 *
 * @return None
 */
    void SetLEDStatus(const dsFPDIndicator_t fpdIndicator, const dsFPDState_t fpdState);
    {
        dsError_t ret = dsSetLEDStatus(fpdIndicator, fpdState);
        if (ret != dsERR_NONE) {
            throw Exception(ret);
        }
    }
/**
 * @fn FrontPanelTextDisplay::setTextBrightness(const int &brightness)
 * @brief This API sets the brightness value for the front panel LED.
 *
 * @param[in] brightness Indicates the brightness value to be set for text LED.
 *
 * @return None
 */
void FrontPanelTextDisplay::setTextBrightness(const int &brightness)
{
    dsError_t ret = dsSetFPTextBrightness((dsFPDTextDisplay_t)_id, brightness);
    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }
    else
    {
        _TextBrightness = brightness;
    }
}


/**
 * @fn FrontPanelTextDisplay::getTextBrightness()
 * @brief This API gets the text LED brightness value.
 *
 * @return _TextBrightness Indicates brightness value for text LED.
 */
int FrontPanelTextDisplay::getTextBrightness()
{
	dsFPDBrightness_t brightness;
	dsError_t ret = dsGetFPTextBrightness((dsFPDTextDisplay_t)_id,&brightness);
        if (ret != dsERR_NONE) {
            throw Exception(ret);
        }
        else
        {
	    _TextBrightness = brightness;
        }
	return _TextBrightness;
}


/**
 * @fn FrontPanelTextDisplay::getTextBrightnessLevels(int &levels, int &min, int &max)
 * @brief This function is used to get maximum brightness, minimum brightness and brightness level
 * of the front panel LED display.
 * <ul>
 * <li> Note:
 * <li> Brightness level - Indicates the step value at which the brightness could be changed.
 * <ul/>
 *
 * @param[out] max Indicates maximum brightness for the front panel text display.
 * @param[out] min Indicates minimum brightness for the front panel text display.
 * @param[out] levels Indicates brightness level of the front panel text display.
 *
 * @return None
 */
void FrontPanelTextDisplay::getTextBrightnessLevels(int &levels, int &min, int &max)
{
    max   = _maxBrightness;
	levels = _levels;
    min    = 0;
}


/**
 * @fn FrontPanelTextDisplay::getTextColorMode()
 * @brief This function is used to get the color mode of the front panel text display.
 *
 * @return _colorMode Color mode of the front panel text display is returned.
 */
int  FrontPanelTextDisplay::getTextColorMode()
{
    return _colorMode;
}


/**
 * @fn FrontPanelTextDisplay::setScroll(const Scroll & scroll)
 * @brief This API sets the scroll parameters for text LED display
 * like hold duration, vertical iterations and horizontal iterations.
 *
 * @param[in] scroll Contains scroll parameters to be set.
 *
 * @return None
 */
void FrontPanelTextDisplay::setScroll(const Scroll & scroll)
{
    dsError_t ret = dsSetFPScroll(scroll.getHoldDuration(), scroll.getHorizontalIteration(), scroll.getVerticalIteration());
    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }
    else
    {
        _scroll = scroll;
    }
}


/**
 * @fn int FrontPanelTextDisplay::getCurrentTimeFormat()
 * @brief This API Get the time format of the LED display 
 *
 * @param[in] none
 * @return Current Persisted /Set Time Zone format
 */

int FrontPanelTextDisplay::getCurrentTimeFormat ()  
{
	dsFPDTimeFormat_t timeFormat;
	dsError_t ret = dsGetFPTimeFormat(&timeFormat);
        if (ret != dsERR_NONE) {
            throw Exception(ret);
        }
        else
        {
	    _timeFormat = timeFormat;
        }
	return timeFormat;
};

/**
 * @fn FrontPanelTextDisplay::setTimeFormat(const int iTimeFormat)
 * @brief This API sets the time format of the LED display to either 12hr or 24hr format.
 *
 * @param[in] iTimeFormat Indicates time format.
 * <ul>
 * <li>  Zero indicates 12hr format.
 * <li>  1 indicates 24hr format.
 * </ul>
 *
 * @return None
 */
void FrontPanelTextDisplay::setTimeFormat(const int iTimeFormat)
{
   
    if ((iTimeFormat == kModeClock24Hr) || (iTimeFormat == kModeClock12Hr))
    {

        dsError_t ret = dsSetFPTimeFormat((dsFPDTimeFormat_t)iTimeFormat);
		if (ret != dsERR_NONE) {
			throw Exception(ret);
		}
		else
		{
			_timeFormat = iTimeFormat;
		}
    }
    else
    {
    	throw IllegalArgumentException();
    }
}


/**
 * @fn FrontPanelTextDisplay::setTime(const int uiHours, const int uiMinutes)
 * @brief This API sets the time of the LED display by switching the text display to time mode.
 *
 * @param[in] uiHours Indicates hour parameter in time.
 * @param[in] uiMinutes Indicates minutes parameter in time.
 *
 * @return None
 */
void FrontPanelTextDisplay::setTime(const int uiHours, const int uiMinutes)
{
    if ((uiHours < 0 ) || (uiHours > 23) || (uiMinutes < 0) || (uiMinutes > 59)) {
        throw IllegalArgumentException();
    }

    dsError_t ret = dsSetFPTime ((dsFPDTimeFormat_t)_timeFormat, uiHours, uiMinutes);
    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }
}

/**
 * @fn FrontPanelTextDisplay:setMode(int mode)
 * @brief This API sets the display mode of the LED display to any, text only or clock only.
 *
 * @param[in] mode Indicates display mode.
 * <ul>
 * <li>  0 indicates both text and clock are supported (default mode).
 * <li>  1 indicates only text mode is supported (trying to set clock results in no change).
 * <li>  2 indicates only clock mode is supported (trying to set text results in no change).
 * </ul>
 *
 * @return None
 */
void FrontPanelTextDisplay::setMode(int mode)
{
    if ((mode == 0) || (mode == 1) || (mode == 2)) {
        dsError_t ret = dsSetFPDMode ((dsFPDMode_t)mode);
        if (ret != dsERR_NONE) {
            throw Exception(ret);
        }
    }
    else
    {
        throw IllegalArgumentException();
    }
}


}

/** @} */

/** @} */
/** @} */
