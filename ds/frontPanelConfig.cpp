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
 * @file frontPanelConfig.cpp
 * @brief A manager module for the front panel.
 */



/**
* @defgroup devicesettings
* @{
* @defgroup ds
* @{
**/


#include <iostream>
#include "dsError.h"
#include "dsUtl.h"
#include "frontPanelConfig.hpp"
#include "frontPanelSettings.hpp"
#include "illegalArgumentException.hpp"
#include "dslogger.h"

using namespace std;

namespace device {

/**
 * @fn FrontPanelConfig::FrontPanelConfig()
 * @brief This function initializes the underlying front panel sub-system. It loads
 * the platform supported configurations of all the front panel indicator and text display.
 *
 * @return None
 */
FrontPanelConfig::FrontPanelConfig()
{
    dsFPInit();
    load();
}


/**
 * @fn FrontPanelConfig::~FrontPanelConfig()
 * @brief This function is the default destructor for FrontPanelConfig.
 *
 * @return None
 */
FrontPanelConfig::~FrontPanelConfig()
{
    //dsFPTerm();
}


/**
 * @fn FrontPanelConfig::getInstance()
 * @brief This API gets the instance of the FrontPanelConfig. When called for the first time,
 * it creates an instance of FrontPanelConfig where the front panel indicators
 * and text display are initialized and loaded with supported colours and text by the default constructor.
 *
 * @return _singleton An instance of FrontPanelConfig is returned.
 */
FrontPanelConfig & FrontPanelConfig::getInstance()
{
    static FrontPanelConfig _singleton;
	return _singleton;
}


/**
 * @fn FrontPanelConfig::getIndicator(const string &name)
 * @brief This API gets the FrontPanelndicator instance corresponding to the name parameter returned by the get supported frontpanel indicator device.
 * <ul>
 * <li> The valid indicator names are Message, Power, Record, Remote and RfByPass.
 * </ul>
 *
 * @param[in] name Indicates the name of the FrontPanelIndicator whose instance should be returned.
 *
 * @return Returns an instance of FrontPanelIndicator corresponding to the name parameter else throws
 * an IllegalArgumentException if the instance corresponding to the name parameter was not found.
 */
FrontPanelIndicator &FrontPanelConfig::getIndicator(const string &name)
{
	std::vector<FrontPanelIndicator>::iterator it = _indicators.begin();
	while (it != _indicators.end()) {
		if (it->getName() == name) {
			return *it;
		}
		it++;
	}

	throw IllegalArgumentException("Bad indicator name");
}

/**
 * @fn FrontPanelConfig::fPInit()
 * @brief This API is used to Initialize front panel.
 *
 * @return None
 */
void FrontPanelConfig::fPInit()
{
	dsFPInit();
}


/**
 * @fn FrontPanelConfig::fPTerm()
 * @brief This API is used to terminate front panel.
 *
 * @return None
 */
void FrontPanelConfig::fPTerm()
{
	dsFPTerm();
}

/**
 * @fn FrontPanelConfig::getIndicator(int id)
 * @brief This function gets an instance of the FrontPanelndicator with the specified id, only if the id
 * passed is valid.
 *
 * @param[in] id Indicates the id of front panel indicator whose instance is required.
 *
 * @return Returns an instance of FrontPanelIndicator corresponding to the id parameter else throws an
 * IllegalArgumentException indicating that the instance corresponding to the id parameter was not found.
 */
FrontPanelIndicator &FrontPanelConfig::getIndicator(int id)
{
	std::vector<FrontPanelIndicator>::iterator it = _indicators.begin();
	while (it != _indicators.end()) {
		if (it->getId() == id) {
			return *it;
		}
		it++;
	}

	throw IllegalArgumentException();


}


/**
 * @fn FrontPanelConfig::getColor(const string &name)
 * @brief This function gets an instance of the front panel indicator Color with the specified name,
 * only if the name passed is valid.
 *
 * @param[in] name Indicates the name of the color whose instance is required.
 *
 * @return Returns an instance of Color corresponding to the name parameter else throws an
 * IllegalArgumentException indicating that the instance corresponding to the name parameter was
 * not found.
 */
FrontPanelIndicator::Color &FrontPanelConfig::getColor(const string &name)
{
	std::vector<FrontPanelIndicator::Color>::iterator it = _colors.begin();
	while (it != _colors.end()) {
		if (it->getName() == name) {
			return *it;
		}
		it++;
	}

	throw IllegalArgumentException("Bad color name");
}


/**
 * @fn FrontPanelConfig::getColor(int id)
 * @brief This function gets an instance of the front panel indicator Color with the specified id,
 * only if the id passed is valid.
 *
 * @param[in] id Indicates the id of the color whose instance is required.
 *
 * @return Returns an instance of Color corresponding to the id parameter else throws an
 * IllegalArgumentException indicating that the instance corresponding to the id parameter was
 * not found.
 */
FrontPanelIndicator::Color &FrontPanelConfig::getColor(int id)
{
	std::vector<FrontPanelIndicator::Color>::iterator it = _colors.begin();
	while (it != _colors.end()) {
		if (it->getId() == id) {
			return *it;
		}
		it++;
	}

	throw IllegalArgumentException("Bad color id");
}


/**
 * @fn FrontPanelConfig::getTextDisplay(int id)
 * @brief This function gets the FrontPanelTextDisplay instance corresponding to the specified id,
 * only if the id passed is valid.
 *
 * @param[in] id Indicates the id of the front panel display whose instance is required.
 *
 * @return Returns FrontPanelTextDisplay instance corresponding to the id parameter else throws an
 * IllegalArgumentException indicating that the instance corresponding to the id parameter is not
 * found
 */
FrontPanelTextDisplay &FrontPanelConfig::getTextDisplay(int id)
{
	std::vector<FrontPanelTextDisplay>::iterator it = _textDisplays.begin();
	while (it != _textDisplays.end()) {
		if (it->getId() == id) {
			return *it;
		}
		it++;
	}

	throw IllegalArgumentException();

}


/**
 * @fn FrontPanelConfig::getTextDisplay(const string &name)
 * @brief This API gets the FrontPanelTextDisplay instance corresponding to the name parameter, only
 * if the name passed is valid.
 * <ul>
 * <li> Valid name parameter is Text.
 * </ul>
 *
 * @param[in] name Indicates the name of FrontPanelTextDisplay whose instance has to be returned.
 *
 * @return Returns FrontPanelTextDisplay instance corresponding to the name else throws an
 * IllegalArgumentException if the instance corresponding to the name parameter is not found.
 */
FrontPanelTextDisplay &FrontPanelConfig::getTextDisplay(const string &name)
{
	std::vector<FrontPanelTextDisplay>::iterator it = _textDisplays.begin();
	while (it != _textDisplays.end()) {
		if (it->getName() == name) {
			return *it;
		}
		it++;
	}

	throw IllegalArgumentException();

}


/**
 * @fn FrontPanelConfig::getColors()
 * @brief This API gets the list of colors supported by front panel indicators.
 *
 * @return rColors List of colors supported by the indicators.
 */
List<FrontPanelIndicator::Color>  FrontPanelConfig::getColors()
{
	List <FrontPanelIndicator::Color> rColors;

	for (size_t i = 0; i < _colors.size(); i++) {
		rColors.push_back(_colors.at(i));
	}

	return rColors;
}


/**
 * @fn FrontPanelConfig::getIndicators()
 * @brief This API gets a list of indicators on the front panel.
 *
 * @return rIndicators Contains list indicators on the front panel.
 */
List<FrontPanelIndicator>  FrontPanelConfig::getIndicators()
{
	List <FrontPanelIndicator> rIndicators;

	for (size_t i = 0; i < _indicators.size(); i++) {
		rIndicators.push_back(_indicators.at(i));
	}

	return rIndicators;
}


/**
 * @fn FrontPanelConfig::getTextDisplays()
 * @brief This API gets a list of text display supported by the front panels.
 *
 * @return rIndicators Contains the list of text supported by the front panel display.
 */
List<FrontPanelTextDisplay>  FrontPanelConfig::getTextDisplays()
{
	List <FrontPanelTextDisplay> rTexts;

	for (size_t i = 0; i < _textDisplays.size(); i++) {
		rTexts.push_back(_textDisplays.at(i));
	}

	return rTexts;
}


/**
 * @fn FrontPanelConfig::load()
 * @brief This function creates instances of the front panel indicators and text display.
 * It also loads the platform supported configurations.
 *
 * @return None
 */
void FrontPanelConfig::load()
{
	/*
	 * Create Indicators
	 * 1. Create Supported Colors.
	 * 2. Create Indicators.
	 */
	{
		for (size_t i = 0; i < dsUTL_DIM(kIndicatorColors); i++) {
			_colors.push_back(FrontPanelIndicator::Color(kIndicatorColors[i].id));
		}

		for (size_t i = 0; i < dsUTL_DIM(kIndicators); i++) {
			/* All indicators support a same set of colors */
			_indicators.push_back(FrontPanelIndicator(kIndicators[i].id,
													  kIndicators[i].maxBrightness,
													  kIndicators[i].maxCycleRate,
													  kIndicators[i].levels,
													  kIndicators[i].colorMode));
		}

	}

	{
		/*
		 * Create TextDisplays
		 * 1. Use Supported Colors created for indicators.
		 * 2. Create Text Displays.
		 */
		for (size_t i = 0; i < dsUTL_DIM(kTextDisplays); i++) {
			_textDisplays.push_back(
					FrontPanelTextDisplay(kTextDisplays[i].id,
										  kTextDisplays[i].maxBrightness,
										  kTextDisplays[i].maxCycleRate,
                                          kTextDisplays[i].levels,
										  kTextDisplays[i].maxHorizontalIterations,
										  kTextDisplays[i].maxVerticalIterations,
										  kTextDisplays[i].supportedCharacters,
										  kTextDisplays[i].colorMode));
		}
	}
}

}


/** @} */
/** @} */
