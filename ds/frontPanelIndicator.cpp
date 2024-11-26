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
 * @file frontPanelIndicator.cpp
 * @brief Configuration of individual indicators are managed here. The blink rate, color,
 * and maximum cycle rate of the front panel indicator can be configured.
 */



/**
* @defgroup devicesettings
* @{
* @defgroup ds
* @{
**/


#include <iostream>
#include <sstream>
#include "frontPanelIndicator.hpp"
#include "frontPanelConfig.hpp"
#include "illegalArgumentException.hpp"
#include "unsupportedOperationException.hpp"
#include "host.hpp"

#include "dslogger.h"
#include "dsError.h"
#include "dsTypes.h"
#include "dsInternal.h"

using namespace std;

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


namespace {
	const char *_colorNames[] = {
			"Blue",
			"Green",
			"Red",
			"Yellow",
			"Orange",
			"White",
	};

	const int _colorsValues[] = {
			dsFPD_COLOR_BLUE,
			dsFPD_COLOR_GREEN,
			dsFPD_COLOR_RED,
			dsFPD_COLOR_YELLOW,
			dsFPD_COLOR_ORANGE,
			dsFPD_COLOR_WHITE,
	};

	inline bool isColorValid(int id) {
		return ((id >= 0) && (id < dsUTL_DIM(_colorNames)));
	}

	const char *_indicatorNames[] = {
			"Message",
			"Power",
			"Record",
			"Remote",
			"RfByPass",

	};


	inline bool isIndicatorValid(int id) {
		return dsFPDIndicator_isValid(id);
	}
}

namespace device {

typedef int _SafetyCheck[(dsUTL_DIM(_colorNames) == dsFPD_COLOR_MAX) ? 1 : -1];
typedef int _SafetyCheck[(dsUTL_DIM(_indicatorNames) == dsFPD_INDICATOR_MAX) ? 1 : -1];

const int FrontPanelIndicator::Color::kBlue   = dsFPD_COLOR_BLUE;
const int FrontPanelIndicator::Color::kGreen  = dsFPD_COLOR_GREEN;
const int FrontPanelIndicator::Color::kRed    = dsFPD_COLOR_RED;
const int FrontPanelIndicator::Color::kYellow = dsFPD_COLOR_YELLOW;
const int FrontPanelIndicator::Color::kOrange = dsFPD_COLOR_ORANGE;
const int FrontPanelIndicator::Color::kWhite  = dsFPD_COLOR_WHITE;
const int FrontPanelIndicator::Color::kMax    = dsFPD_COLOR_MAX;


const int FrontPanelIndicator::kMessage       = dsFPD_INDICATOR_MESSAGE;
const int FrontPanelIndicator::kPower    	  = dsFPD_INDICATOR_POWER;
const int FrontPanelIndicator::kRecord   	  = dsFPD_INDICATOR_RECORD;
const int FrontPanelIndicator::kRemote   	  = dsFPD_INDICATOR_REMOTE;
const int FrontPanelIndicator::kRFBypass 	  = dsFPD_INDICATOR_RFBYPASS;
const int FrontPanelIndicator::kMax      	  = dsFPD_INDICATOR_MAX;

/**
 * @addtogroup dssettingsfpindicatorapi
 * @{
 */

/**
 * @fn FrontPanelIndicator::Color::getInstance(int id)
 * @brief This function gets an instance of the Color with the specified id, only if the id
 * passed is valid.
 *
 * @param[in] id Indicates the id of the color against which the Color instance is requested.
 *
 * @return Returns Color instance corresponding to the id parameter else throws an
 * IllegalArgumentException indicating that the instance corresponding to the id
 * parameter is not found
 */
const FrontPanelIndicator::Color & FrontPanelIndicator::Color::getInstance(int id)
{
	if (::isColorValid(id)) {
		return FrontPanelConfig::getInstance().getColor(id);
	}
	else {
		throw IllegalArgumentException("Bad color id");
	}
}


/**
 * @fn FrontPanelIndicator::Color::getInstance(const std::string &name)
 * @brief This function gets an instance of the Color corresponding to the specified name,
 * only if the name passed is valid.
 *
 * @param[in] name Indicates the name against which the color instance is required.
 *
 * @return  Returns Color instance corresponding to the name parameter else throws an
 * IllegalArgumentException indicating that the instance corresponding to the name
 * parameter is not found
 */
const FrontPanelIndicator::Color & FrontPanelIndicator::Color::getInstance(const std::string &name)
{
	for (size_t i = 0; i < dsUTL_DIM(_colorNames); i++) {
		if (name.compare(_colorNames[i]) == 0) {
			return FrontPanelConfig::getInstance().getColor(i);
		}
	}
	throw IllegalArgumentException("Bad color name");
}

FrontPanelIndicator::Color::Color(int id)
{
		
	if (::isColorValid(id)) {
		 _id = id;
		 _name = std::string(_colorNames[id]);
	}
	else {
		throw IllegalArgumentException("Constructor for color: bad color id");
	}
}


/**
 * @fn FrontPanelIndicator::Blink::Blink(int interval, int iteration)
 * @brief This API is a parameterized constructor for the nested class Blink
 * <ul>
 * <li> It initializes iteration and interval parameters. By default they are set to Zero
 * </ul>
 *
 * @param[in] interval Specifies the blink intervals for front panel indicator
 * @param[in] iteration Specifies the number of blink iterations for front panel indicator
 *
 * @return None
 */
FrontPanelIndicator::Blink::Blink(int interval, int iteration)
{
	if(interval < 0 || iteration < 0)
		throw IllegalArgumentException();
        _interval = interval;
        _iteration = iteration;
};


/**
 * @fn FrontPanelIndicator::getInstance(int id)
 * @brief This function gets the FrontPanelIndicator instance corresponding to the id parameter,
 * only if the id passed is valid.
 *
 * @param[in] id Indicates the front panel indicator id whose instance has to be returned.
 *
 * @return Returns FrontPanelIndicator instance corresponding to the name parameter else throws
 * an IllegalArgumentException indicating that the instance corresponding to id parameter was
 * not found.
 */
FrontPanelIndicator & FrontPanelIndicator::getInstance(int id)
{
	if (::isIndicatorValid(id)) {
		return FrontPanelConfig::getInstance().getIndicator(id);
	}
	else {
		throw IllegalArgumentException("Bad front panel indicator id");
	}

}


/**
 * @fn FrontPanelIndicator::getInstance(const std::string &name)
 * @brief This API gets the FrontPanelIndicator instance corresponding to the name parameter.
 * <ul>
 * <li> It gets the id corresponding to the front panel indicator name.
 * <li> Then it gets the FrontPanelIndicator instance corresponding to the id.
 * </ul>
 *
 * @param[in] name Indicates the front panel indicator name whose instance has to be returned.
 *
 * @return Returns FrontPanelIndicator instance corresponding to the name parameter else
 * throws an IllegalArgumentException.
 */
FrontPanelIndicator & FrontPanelIndicator::getInstance(const std::string &name)
{
	for (size_t i = 0; i < dsUTL_DIM(_indicatorNames); i++) {
		if (name.compare(_indicatorNames[i]) == 0) {
			return FrontPanelConfig::getInstance().getIndicator(i);
		}
	}

	throw IllegalArgumentException("Bad frontpanel indicator name");
}


/**
 * @fn FrontPanelIndicator::FrontPanelIndicator(int id, int maxBrightness, int maxCycleRate, int levels , int colorMode)
 * @brief This function is a parameterised constructor of FrontPanelIndicator. It creates and initializes the
 * FrontPanelIndicator instance with the configurations/values specified as input parameters.
 *
 * @param[in] id Indicates the id for the FrontPanelIndicator instance created.
 * @param[in] maxBrightness Indicates maximum brightness value for the front panel indicator.
 * @param[in] maxCycleRate Indicates maximum cycle rate for the front panel indicator.
 * @param[in] levels Indicates brightness level for the front panel indicator.
 * @param[in] colorMode Indicates the color mode for the front panel indicator like single or multi color mode.
 *
 * @return None
 */
FrontPanelIndicator::FrontPanelIndicator(int id, int maxBrightness, int maxCycleRate, int levels , int colorMode):
                                         _maxBrightness(maxBrightness), _maxCycleRate(maxCycleRate), _levels(levels),_colorMode(colorMode)
{
	_brightness = 0;
	_state = 0;
	_color = 0;
	_color_rgb32 = 0;  //CID:88908 - Uninit_ctor
	if (::isIndicatorValid(id)) {
		_id = id;
		_name = std::string(_indicatorNames[id]);
	}
}


/**
 * @fn FrontPanelIndicator::~FrontPanelIndicator()
 * @brief This function is the default constructor for FrontPanelIndicator.
 *
 * @return None
 */
FrontPanelIndicator::~FrontPanelIndicator()
{
}


/**
 * @fn FrontPanelIndicator::setBrightness(const int &brightness,bool toPersist)
 * @brief This API sets the brightness or intensity of the front panel indicators.
 *
 * @param[in] brightness Indicates brightness value to be set.
 * @param[in] toPersist If true, the value will also return in persistence memory.
 *
 * @return None
 */
void FrontPanelIndicator::setBrightness(const int &brightness,bool toPersist)
{
	bool IsPersist = toPersist;

	if (dsERR_NONE != dsSetFPDBrightness((dsFPDIndicator_t)_id, brightness,IsPersist) )
    {
    	throw IllegalArgumentException();
    } 
	_brightness = brightness;
}



/**
 * @fn FrontPanelIndicator::getState()
 * @brief This API gets the State of the specified LED indicators.
 *
 * @return _state State of the specified LED indicators.
 */
bool FrontPanelIndicator::getState() 
{
	dsFPDState_t state;
	
	if (dsERR_NONE == dsGetFPState((dsFPDIndicator_t)_id,&state))
	{
		return state == dsFPD_STATE_ON;
	}
        else
        {
                throw IllegalArgumentException();
        }
	return false;   //CID:82714 - Missing return
}



/**
 * @fn FrontPanelIndicator::setState(const bool &enable)
 * @brief This API is used to enable or disable the front panel indicator.
 *
 * @param[in] enable True to enable front panel indicator else false.
 *
 * @return None
 */
void FrontPanelIndicator::setState(const bool &enable)
{
    _state = enable;
    dsError_t ret = dsERR_NONE;
	if (_state)
    {
		ret =  dsSetFPState((dsFPDIndicator_t)_id,dsFPD_STATE_ON);
	}
	else
	{
		ret = dsSetFPState((dsFPDIndicator_t)_id,dsFPD_STATE_OFF);
	}
        if (ret != dsERR_NONE) {
            throw Exception(ret);
        }
}


/**
 * @fn FrontPanelIndicator::getBrightness()
 * @brief This API gets the brightness of the specified LED indicators.
 *
 * @return _brightness Brightness value of the specified LED indicators.
 */
int FrontPanelIndicator::getBrightness() 
{
	dsFPDBrightness_t brightness;

	dsError_t ret = dsGetFPBrightness((dsFPDIndicator_t)_id,&brightness);
        if (ret != dsERR_NONE) {
            throw Exception(ret);
        }
        else
        {
	    _brightness = brightness;
        }
	return _brightness;
}


/**
 * @fn FrontPanelIndicator::getBrightnessLevels(int &levels, int &min, int &max)
 * @brief This function gets the maximum brightness, minimum brightness and the brightness level
 * set for the front panel indicator.
 * <ul>
 * <li> Note :
 * <li> Brightness level indicates a step value at which the brightness/intensity of the indicator
 * could be changed.
 * <ul/>
 *
 * @param[out] levels Indicates brightness level set for the front panel indicator.
 * @param[out] min Indicates minimum brightness of the front panel indicator.
 * @param[out] max Indicates maximum brightness of the front panel indicator.
 *
 * @return None
 */
void FrontPanelIndicator::getBrightnessLevels(int &levels, int &min, int &max) 
{
    max   = _maxBrightness;
	levels = _levels;
    min    = 0;
}


/**
 * @fn FrontPanelIndicator::getColorMode()
 * @brief This function is used to get the color mode of the front panel indicator.
 * The color mode is device specific and can be single or multi color mode (RGB colors).
 * By default the color mode is set to 0 to indicate single color mode.
 *
 * @return None
 */
int  FrontPanelIndicator::getColorMode() 
{
    return _colorMode;
}


/**
 * @fn FrontPanelIndicator::setBlink(const Blink &blink)
 * @brief This API sets the blink iteration and blink interval for the LED.
 *
 * @param[in] blink Indicates the blink iteration and interval value to be set.
 *
 * @return None
 */
void FrontPanelIndicator::setBlink(const Blink &blink)
{
    dsError_t ret = dsSetFPBlink((dsFPDIndicator_t)_id, blink.getIteration(), blink.getInterval());
    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }
    else
    {
        _blink = blink;
    }
}


/**
 * @fn FrontPanelIndicator::getColor()
 * @brief This API gets the color of the front panel indicator/LED.
 *
 * @return _color_rgb32 Indicates the color of the LED in RGB format.
 */
uint32_t FrontPanelIndicator::getColor()  
{
	dsFPDColor_t color;

        if (dsERR_NONE != dsGetFPColor((dsFPDIndicator_t)_id,&color) )
        {
            throw IllegalArgumentException("dsGetFPColor failed.");
        }
	_color_rgb32 = color;

	return _color_rgb32;
};


/**
 * @fn FrontPanelIndicator::setColor(const FrontPanelIndicator::Color & color,bool toPersist)
 * @brief This API sets the color of the front panel indicator.
 *
 * @param[in] color This parameter provides the color to be set in RGB format.
 * @param[in] toPersist If true, the value will also return in persistence memory.
 *
 * @return None
 */
void FrontPanelIndicator::setColor(const FrontPanelIndicator::Color & color,bool toPersist)
{
   
    bool IsPersist = toPersist;
    if(_colorMode == 0 || _colorMode == 1)
    {
        throw UnsupportedOperationException("This API not supported for the color mode");
    }
    else if(_colorMode == 2)
    {
        bool isValidColor = false;
        const List<FrontPanelIndicator::Color> supportedColors = FrontPanelConfig::getInstance().getColors();
        for (uint j = 0; j < supportedColors.size(); j++)
        {
            if( supportedColors.at(j).getId() == color.getId())
            {
               isValidColor = true;
               break;
            }
        }
        if(!isValidColor)
        {
           throw IllegalArgumentException("Invalid color object");
        }
    }
    dsFPDColor_t colorValue = _colorsValues[color.getId()];

    if (dsERR_NONE != dsSetFPDColor((dsFPDIndicator_t)_id, (dsFPDColor_t)colorValue,IsPersist) )
    {
        throw IllegalArgumentException("dsSetFPDColor failed");
    }
	_color_rgb32 = colorValue;
	//std::cout << "_color_rgb32 = " << _color_rgb32 << std::endl;
}


/**
 * @fn FrontPanelIndicator::setColor(uint32_t color,bool toPersist)
 * @brief This API sets the color of the front panel indicator
 *
 * @param[in] color Indicates the color to be set in RGB format.
 * @param[in] toPersist If true, the value will also return in persistence memory.
 *
 * @return None
 */
void FrontPanelIndicator::setColor(uint32_t color,bool toPersist)
{
    bool IsPersist = toPersist;
    bool isValidColor = false;
    if(_colorMode == 0 || _colorMode == 2)
    {
        throw UnsupportedOperationException("This API not supported for the color mode");
    }
    if (dsERR_NONE != dsSetFPDColor((dsFPDIndicator_t)_id, (dsFPDColor_t) color,IsPersist) )
    {
        throw IllegalArgumentException();
    }
	_color_rgb32 = (dsFPDColor_t) color;
    //std::cout << "UINT _color_rgb32 = " << _color_rgb32 << std::endl;
}


/**
 * @fn FrontPanelIndicator::getSupportedColors() const
 * @brief This API gets the list of supported colors for front panel indicator.
 * <ul>
 * <li> The colors supported are platform specific and can be RGB combinations.
 * </ul>
 *
 * @return Returns a list of colors suppported.
 */
const List<FrontPanelIndicator::Color> FrontPanelIndicator::getSupportedColors() const
{
	return FrontPanelConfig::getInstance().getColors();
}

}

/** @} */

/** @} */
/** @} */
