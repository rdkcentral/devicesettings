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


#define BOOST_TEST_MODULE Logger 
#define BOOST_TEST_MAIN
#include "boost/test/included/unit_test.hpp"
#include <iostream>

#include "logger.hpp"

using namespace rdkutil;


BOOST_AUTO_TEST_CASE(test_Logger)
{
	try {
        rdkutil::Logger &testLog1 = rdkutil::Logger::getLogger("");
        testLog1.fatal("Test fatal");
        testLog1.error("Test error");
        testLog1.warn("Test warn");
        testLog1.info("Test info");
        testLog1.debug("Test debug");
        testLog1.trace("Test trace");
        testLog1.isErrorEnabled ();
        testLog1.isWarnEnabled();
        testLog1.isInfoEnabled();
        testLog1.isDebugEnabled();
        testLog1.isTraceEnabled();
        testLog1.enableError();
        testLog1.enableInfo();
        testLog1.enableDebug();
        testLog1.enableTrace();

        rdkutil::Logger &testLog2 = rdkutil::Logger::getLogger("Test 1");
        testLog2.fatal("Test fatal");
        testLog2.error("Test error");
        testLog2.warn("Test warn");
        testLog2.info("Test info");
        testLog2.debug("Test debug");
        testLog2.trace("Test trace");
        testLog2.isErrorEnabled ();
        testLog2.isWarnEnabled();
        testLog2.isInfoEnabled();
        testLog2.isDebugEnabled();
        testLog2.isTraceEnabled();
        testLog2.enableError();
        testLog2.enableInfo();
        testLog2.enableDebug();
        testLog2.enableTrace();

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
