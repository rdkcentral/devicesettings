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
 
// TODO: Include your class to test here.


/**
* @defgroup devicesettings
* @{
* @defgroup test
* @{
**/


#define BOOST_TEST_MODULE FrontPannel 
#define BOOST_TEST_MAIN
#include "boost/test/included/unit_test.hpp"
#include <iostream>
#include "frontPanelConfig.hpp"

BOOST_AUTO_TEST_CASE(test_hostPersistence)
{
	try {
            device::FrontPanelIndicator::Blink p(1, 2);
            device::FrontPanelTextDisplay::Scroll s(2,4,6);
            device::FrontPanelIndicator::Color c(rpFP_COLOR_GREEN);

            device::FrontPanelConfig::getInstance().getIndicator("Power").setBrightness(25);
            device::FrontPanelConfig::getInstance().getIndicator("Message").setBrightness(95);
            device::FrontPanelConfig::getInstance().getIndicator("Power").setBlink(p);;
            device::FrontPanelConfig::getInstance().getIndicator("Record").setColor(c);
            device::FrontPanelConfig::getInstance().getIndicator("Remote").getColor();
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").setText("Hello World...");
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").setBrightness(65);
    
            device::FrontPanelConfig::getInstance().getIndicators();
            device::FrontPanelConfig::getInstance().getIndicator("RfByPass").setBrightness(65);
            device::FrontPanelConfig::getInstance().getIndicator(rpFP_INDICATOR_RFBYPASS).setBrightness(95);
            device::FrontPanelConfig::getInstance().getIndicator("Power").getId();
            device::FrontPanelConfig::getInstance().getIndicator("Power").getName();
            device::FrontPanelConfig::getInstance().getIndicator("Power").getBrightness();
            device::FrontPanelConfig::getInstance().getIndicator("Power").getBlink();
            device::FrontPanelConfig::getInstance().getIndicator("Power").getMaxCycleRate();
            device::FrontPanelConfig::getInstance().getIndicator("Power").getSupportedColors();

            device::FrontPanelConfig::getInstance().getTextDisplays();
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").setBrightness(65);
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").setBrightness(95);
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").getId();
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").getName();
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").getBrightness();
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").getBlink();
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").getMaxCycleRate();
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").getSupportedColors();
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").getScroll();
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").getCurrentTimeFormat();
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").setTimeFormat(1);
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").setTime(14,35);


        }
	catch(...) {
		BOOST_CHECK(0);
	}
}

BOOST_AUTO_TEST_CASE(testDummy)
{
	BOOST_CHECK(1 == 1);
}


/** @} */
/** @} */
