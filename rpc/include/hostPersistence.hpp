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


#ifndef _DS_MGR_HOSTPERSISTENCE_HPP_
#define _DS_MGR_HOSTPERSISTENCE_HPP_
#include <map>
/* This namespace is used to defined custom Hash table which has provision
   to write to local storage for a back up and recover it upon request */
namespace device {

class HostPersistence {
	std::map <std::string, std::string> _properties;
	std::map <std::string, std::string> _defaultProperties;
	std::string filePath;
	std::string defaultFilePath;
	
	void loadFromFile (const std::string &file, std::map <std::string, std::string> &map);
	void writeToFile (const std::string &file);

public:
	HostPersistence();
	HostPersistence (const std::string &storeFolder);
	static HostPersistence& getInstance(void);
	virtual ~HostPersistence();
	void load();
	std::string getProperty(const std::string &key);
	std::string getProperty(const std::string &key, const std::string &defValue);
	std::string getDefaultProperty(const std::string &key);
	void persistHostProperty(const std::string &key, const std::string &value);
};

}

#endif /* _DS_HOSTPERSISTENCE_HPP_ */




/** @} */
/** @} */
