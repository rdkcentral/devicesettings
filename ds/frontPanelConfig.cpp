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
#include <unistd.h>
#include "dsError.h"
#include "dsUtl.h"
#include "frontPanelConfig.hpp"
#include "frontPanelSettings.hpp"
#include "illegalArgumentException.hpp"
#include "dslogger.h"
#include "manager.hpp"

#define DEBUG 1 // Using for dumpconfig 

using namespace std;

typedef struct fpdConfigs
{
	const dsFPDColorConfig_t    *pKFPDIndicatorColors;
	const dsFPDIndicatorConfig_t   *pKIndicators;
	const dsFPDTextDisplayConfig_t   *pKTextDisplays;
	int *pKFPDIndicatorColors_size;
	int *pKIndicators_size;
	int *pKTextDisplays_size;
}fpdConfigs_t;

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
    m_isFPInitialized = false;
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
    m_isFPInitialized = false;
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
    if (!_singleton.m_isFPInitialized)
    {
        dsError_t errorCode = dsERR_NONE;
        unsigned int retryCount = 1;
        do
        {
            errorCode = dsFPInit();
            if (dsERR_NONE == errorCode)
            {
                _singleton.load();
                _singleton.m_isFPInitialized = true;
                INT_INFO("dsFPInit success\n");
            }
            else{
                INT_ERROR("dsFPInit failed with error[%d]. Retrying... (%d/20)\n", errorCode, retryCount);
                usleep(50000); // Sleep for 50ms before retrying
            }
        }
        while ((!_singleton.m_isFPInitialized) && (retryCount++ < 20));
    }
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

void dumpconfig(fpdConfigs_t *configuration)
{
	// Dump the configuration details
	INT_INFO("\n\n===========================================================================\n\n");
	INT_INFO("Start of Front Panel Configuration Details:\n");
	INT_INFO("configuration->pKFPDIndicatorColors_size: %d\n", *(configuration->pKFPDIndicatorColors_size));
	INT_INFO("configuration->pKIndicators_size: %d\n", *(configuration->pKIndicators_size));
	INT_INFO("configuration->pKTextDisplays_size: %p\n", configuration->pKTextDisplays_size);
	INT_INFO("Dumping Front Panel Configuration Details:\n");
	INT_INFO("Indicator Colors:\n");
	for (size_t i = 0; i < *(configuration->pKFPDIndicatorColors_size); i++) {
		INT_INFO("  Color ID: %d, color: %d\n",
			   configuration->pKFPDIndicatorColors[i].id,
			   configuration->pKFPDIndicatorColors[i].color);
	}

	INT_INFO("Indicators:\n");
	for (size_t i = 0; i < *(configuration->pKIndicators_size); i++) {
		INT_INFO("  Indicator ID: %d, Max Brightness: %d, Max Cycle Rate: %d, Levels: %d, Color Mode: %d\n",
			   configuration->pKIndicators[i].id,
			   configuration->pKIndicators[i].maxBrightness,
			   configuration->pKIndicators[i].maxCycleRate,
			   configuration->pKIndicators[i].levels,
			   configuration->pKIndicators[i].colorMode);
	}

	if(configuration->pKTextDisplays != NULL && configuration->pKTextDisplays_size != NULL){
		INT_INFO("Text Displays:*(configuration->pKTextDisplays_size) =%d\n", *(configuration->pKTextDisplays_size));

		for (size_t i = 0; i < *(configuration->pKTextDisplays_size); i++) {
			INT_INFO("  Text Display ID: %d, Max Brightness: %d, Max Cycle Rate: %d, Levels: %d, Max Horizontal Iterations: %d, Max Vertical Iterations: %d, Supported Characters: %s, Color Mode: %d\n",
				   configuration->pKTextDisplays[i].id,
			   	configuration->pKTextDisplays[i].maxBrightness,
			   	configuration->pKTextDisplays[i].maxCycleRate,
			   	configuration->pKTextDisplays[i].levels,
			   	configuration->pKTextDisplays[i].maxHorizontalIterations,
			   	configuration->pKTextDisplays[i].maxVerticalIterations,
			   	configuration->pKTextDisplays[i].supportedCharacters,
			   	configuration->pKTextDisplays[i].colorMode);
		}
	}
	else {
		INT_INFO("  No Text Displays configured.\n");
	}
	INT_INFO("End of Front Panel Configuration Details.\n");
	INT_INFO("\n\n===========================================================================\n\n");
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
	static int indicatorSize, indicatorColorSize, textDisplaySize, invalid_size = -1;
	fpdConfigs_t configuration = {0};

	const char* searchVaribles[] = {
        "kFPDIndicatorColors",
        "kFPDIndicatorColors_size",
        "kIndicators",
        "kIndicators_size",
		"kFPDTextDisplays",
		"kFPDTextDisplays_size"
    };
	bool ret = false;

		INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[0]);
		ret = searchConfigs(searchVaribles[0], (void **)&configuration.pKFPDIndicatorColors );
		//ret= false;
		//INT_INFO("make default ret = false to read fallback\n");
		if(ret == true)
		{
			INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[1]);
			ret = searchConfigs(searchVaribles[1], (void **)&configuration.pKFPDIndicatorColors_size);
			if(ret == false)
			{
				INT_ERROR("%s is not defined\n", searchVaribles[1]);
				configuration.pKFPDIndicatorColors_size = &invalid_size;
			}
			INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[2]);
			ret = searchConfigs(searchVaribles[2], (void **)&configuration.pKIndicators);
			if(ret == false)
			{
				INT_ERROR("%s is not defined\n", searchVaribles[2]);
			}
			INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[3]);
			ret = searchConfigs(searchVaribles[3], (void **)&configuration.pKIndicators_size);
			if(ret == false)
			{
				INT_ERROR("%s is not defined\n", searchVaribles[3]);
				configuration.pKIndicators_size = &invalid_size;
			}
			INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[4]);
			ret = searchConfigs(searchVaribles[4], (void **)&configuration.pKTextDisplays);
			if(ret == false)
			{
				INT_ERROR("%s is not defined\n", searchVaribles[4]);
			}
			INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[5]);
			ret = searchConfigs(searchVaribles[5], (void **)&configuration.pKTextDisplays_size);
			if(ret == false)
			{
				INT_ERROR("%s is not defined\n", searchVaribles[5]);
				configuration.pKTextDisplays_size = &invalid_size;
			}
		}
		else
		{
			INT_ERROR("Read Old Configs\n");
			configuration.pKFPDIndicatorColors = kIndicatorColors;
			indicatorColorSize = dsUTL_DIM(kIndicatorColors);
			configuration.pKFPDIndicatorColors_size = &indicatorColorSize;
			configuration.pKIndicators = kIndicators;
			indicatorSize = dsUTL_DIM(kIndicators);
			configuration.pKIndicators_size = &indicatorSize;
			configuration.pKTextDisplays = kTextDisplays;
			textDisplaySize = dsUTL_DIM(kTextDisplays);
			configuration.pKTextDisplays_size = &textDisplaySize;

			INT_INFO("configuration.pKFPDIndicatorColors =%p, *(configuration.pKFPDIndicatorColors_size) = %d\n", configuration.pKFPDIndicatorColors, *(configuration.pKFPDIndicatorColors_size));
			INT_INFO("configuration.pKIndicators =%p, *(configuration.pKIndicators_size) = %d\n", configuration.pKIndicators, *(configuration.pKIndicators_size));
			INT_INFO("configuration.pKTextDisplays =%p, *(configuration.pKTextDisplays_size) = %d\n", configuration.pKTextDisplays, *(configuration.pKTextDisplays_size));
		}
	if (configuration.pKFPDIndicatorColors != NULL && configuration.pKFPDIndicatorColors_size != NULL &&
		*(configuration.pKFPDIndicatorColors_size) > 0 &&
		configuration.pKIndicators != NULL && configuration.pKIndicators_size != NULL &&
		*(configuration.pKIndicators_size) > 0)
	{
		#if DEBUG
		dumpconfig(&configuration);
		#endif

		{
			for (size_t i = 0; i < *(configuration.pKFPDIndicatorColors_size); i++) {
				_colors.push_back(FrontPanelIndicator::Color(configuration.pKFPDIndicatorColors[i].id));
			}

			for (size_t i = 0; i < *(configuration.pKIndicators_size); i++) {
				/* All indicators support a same set of colors */
				_indicators.push_back(FrontPanelIndicator(configuration.pKIndicators[i].id,
														  configuration.pKIndicators[i].maxBrightness,
														  configuration.pKIndicators[i].maxCycleRate,
														  configuration.pKIndicators[i].levels,
														  configuration.pKIndicators[i].colorMode));
				}
		}

		if(configuration.pKTextDisplays != NULL && configuration.pKTextDisplays_size != NULL)
		{
			/*
		 	* Create TextDisplays
		 	* 1. Use Supported Colors created for indicators.
		 	* 2. Create Text Displays.
		 	*/
			INT_DEBUG("Text Displays \n");
			for (size_t i = 0; i < *(configuration.pKTextDisplays_size); i++) {
				_textDisplays.push_back(
						FrontPanelTextDisplay(configuration.pKTextDisplays[i].id,
											  configuration.pKTextDisplays[i].maxBrightness,
											  configuration.pKTextDisplays[i].maxCycleRate,
                            	              configuration.pKTextDisplays[i].levels,
											  configuration.pKTextDisplays[i].maxHorizontalIterations,
											  configuration.pKTextDisplays[i].maxVerticalIterations,
											  configuration.pKTextDisplays[i].supportedCharacters,
										  	  configuration.pKTextDisplays[i].colorMode));
		 }
		}
		else
		{
			INT_ERROR("No valid text display configuration found\n");
		} 
	}
	else 
	{
		INT_ERROR("No valid front panel configuration found\n");	
	}
}

}

/** @} */
/** @} */
