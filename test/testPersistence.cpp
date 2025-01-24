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


#define BOOST_TEST_MODULE HostPersistence 
#define BOOST_TEST_MAIN
#include "boost/test/included/unit_test.hpp"
#include <iostream>
#include "hostPersistence.hpp"

BOOST_AUTO_TEST_CASE(test_hostPersistence)
{
	try {
        device::HostPersistence *pClass = new device::HostPersistence();
        pClass->load();
        pClass->persistHostProperty ("Testing1", "10");
        pClass->persistHostProperty ("Testing2", "16");
        pClass->persistHostProperty ("Testing3", "987654321");
        pClass->persistHostProperty ("Testing3", "111111111");
        pClass->persistHostProperty ("Testing5", "");
        pClass->persistHostProperty ("Testing6", "0");
        pClass->getProperty("Testing2");
        pClass->getProperty("Testing1");
        pClass->getProperty("Testing1", "g");
        pClass->getProperty("Testing4", "lol");
        pClass->getProperty("Testing4");
        pClass->getProperty("Testing5");
        pClass->getProperty("Testing5", "lol");
        pClass->getProperty("Testing6", "123");
        pClass->getProperty("Testing3");
        pClass->getProperty("Testing11", "123");

        pClass->load();
        pClass->persistHostProperty ("Testing1", "10");
        pClass->persistHostProperty ("Testing2", "16");
        pClass->getProperty("Testing3");
       	
        device::HostPersistence *pClass2 = new device::HostPersistence("oNe");
        pClass2->load();
        pClass2->persistHostProperty ("Hello1", "10");
        pClass2->persistHostProperty ("Hello2", "16");
        pClass2->persistHostProperty ("Hello3", "987654321");
        pClass2->persistHostProperty ("Hello3", "111111111");
        pClass2->persistHostProperty ("Hello5", "");
        pClass2->persistHostProperty ("Hello6", "0");
        pClass2->getProperty("Hello2");
        pClass2->getProperty("Hello1");
        pClass2->getProperty("Hello1", "g");
        pClass2->getProperty("Hello4", "lol");
        pClass2->getProperty("Hello4");
        pClass2->getProperty("Hello5");
        pClass2->getProperty("Hello5", "lol");
        pClass2->getProperty("Hello6", "123");
        pClass2->getProperty("Hello3");
        pClass2->getProperty("Hello11", "123");

        pClass2->load();
        pClass2->persistHostProperty ("Hello1", "10");
        pClass2->persistHostProperty ("Hello2", "16");
        pClass2->getProperty("Hello3");

    
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
