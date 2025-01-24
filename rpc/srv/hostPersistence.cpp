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


#include <iostream>
#include <fstream>
#include <map>
#include <exception>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#include "hostPersistence.hpp"
#include "exception.hpp"
#include "illegalArgumentException.hpp"


#include "dsserverlogger.h"

#define HOSTPERSIST_ERROR       -1
#define HOSTPERSIST_SUCCESS      0


using namespace std;

namespace device {

/**
 * <code>HostPersistence</code> is an simple class that provides
 * for robust serialization of (key,value) values stored in a Hashmap.
 *
 * The local file system is used as the database, with different sets
 * of persistent objects may occupy individual files in the file system.
 *
 * <p>
 * In order to provide for more robust updates, we never overwrite a file
 * directly. Instead we write to a secondary file, then delete the original, and
 * finally rename the secondary file. When reading files in we look for the
 * secondary file first. If no secondary file is found or it is corrupt, we fall
 * back to the original filename.
 * <p>
 *
 * <p>
 * For this implementation, where the <i>key</i> is a string constant and <i>value</i> is
 * either a string constant or a primitive data type, we use a HashMap to store the properties.
 * <p>
 *
 * @TODO: If necessary, we will later add capability to configure the file path where the persistent data
 * is to be stored.
 *
 */

HostPersistence::HostPersistence() {
	/*
		* TBD This need to be removed and 
		* Persistent path shall be set  from startup script
		* To do this Host Persistent need to be part of DS Manager
		* TBD
	*/

	#if defined(HAS_HDD_PERSISTENT)
		/*Product having HDD Persistent*/
		filePath = "/tmp/mnt/diska3/persistent/ds/hostData";
		{
		}
	#elif defined(HAS_FLASH_PERSISTENT)
		/*Product having Flash Persistent*/
		filePath = "/opt/persistent/ds/hostData";
		{
		}
	#else
		filePath = "/opt/ds/hostData";
		{
		}
		/*Default case*/
	#endif
		defaultFilePath = "/etc/hostDataDefault";


}

HostPersistence::HostPersistence( const std::string &storeFileName) {
	// TODO Auto-generated constructor stub
    filePath = storeFileName; 
}
HostPersistence::~HostPersistence() {
	// TODO Auto-generated destructor stub
}

/**
 * Provides a Host Persistenc Instance.
 * 
 */
  HostPersistence& HostPersistence::getInstance()
    {
        static HostPersistence instance;
        return instance;
    }

/**
 * Provides a simple utility method for load host persisted values.
 * from file system. Exception is thrown if the persisted file is
 * not readable, and property map is loaded with default values.
 */
void HostPersistence::load() {
    try
    {
       loadFromFile(filePath, _properties);
    }
    catch (exception& e)
    {
        cout << "Backup file is currupt or not available.." << endl;
        try {
            loadFromFile(filePath + "tmpDB", _properties);
        }
        catch (...) {
        	/* Remove all properties, and start with default values */
        }
    }

    try
    {
        loadFromFile(defaultFilePath, _defaultProperties);
    }
    catch (exception& e)
    {
        cout << "System file "<< defaultFilePath <<" is currupt or not available.." << endl;
    }

    return;
}

/**
 * Provides a simple utility method for accessing host persisted values.
 * Exception is thrown if the asked property is not available.
 *
 * @param key
 *            property key
 * @param defValue
 *            default value
 * @return {@link Integer#getInteger(String)} for the given <i>key</i>,
 *         <i>defValue</i>
 */
std::string HostPersistence::getProperty(const string &key)
{
    /* Check the validness of the key */
    if( key.empty()) 
    {
        cout << "The KEY is empty..." << endl;
        throw IllegalArgumentException();
    }

    std::map <std::string, std::string> :: const_iterator eFound = _properties.find (key);
    if (eFound == _properties.end())
    {
        cout << "The Item  IS NOT FOUND " << endl;

        throw IllegalArgumentException();
    }
    else
    {
        /*cout << "The Item " << eFound->first << " is found & the value is " << eFound->second << endl;*/
        return eFound->second;
    }
}

/**
 * Provides a simple utility method for accessing host persisted values
 * Default value is returned if the asked property is not available.
 *
 * @param key
 *            property key
 * @param defValue
 *            default value
 * @return {@link Integer#getInteger(String)} for the given <i>key</i>,
 *         <i>defValue</i>
 */
std::string HostPersistence::getProperty(const std::string &key, const std::string &defValue) 
{
    /* Check the validness of the key */
    if( key.empty()) 
    {
        cout << "The KEY is empty..." << endl;
        throw IllegalArgumentException();
    }

    /*cout << "Looking for " << key << " and " << defValue << endl;*/

    std::map <std::string, std::string> :: const_iterator eFound = _properties.find (key);

    if (eFound == _properties.end())
    {
        return defValue;
    }
    else
    {
        /*cout << "The Item " << eFound->first << " is found & the value is " << eFound->second << endl;*/
        return eFound->second;
    }
}

/**
 * Provides a simple utility method for accessing host persisted values.
 * Exception is thrown if the asked property is not available.
 *
 * @param key
 *            property key
 * @return {@link Integer#getInteger(String)} for the given <i>key</i>,
 *         <i>defValue</i>
 */
std::string HostPersistence::getDefaultProperty(const string &key)
{
    /* Check the validness of the key */
    if( key.empty())
    {
        cout << "The KEY is empty..." << endl;
        throw IllegalArgumentException();
    }

    std::map <std::string, std::string> :: const_iterator eFound = _defaultProperties.find (key);
    if (eFound == _defaultProperties.end())
    {
        cout << "The Item  IS NOT FOUND " << endl;

        throw IllegalArgumentException();
    }
    else
    {
        cout << "The Item " << eFound->first << " is found & the value is " << eFound->second << endl;
        return eFound->second;
    }
}

/**
 * Provides a simple utility method for persist host values to a file.
 * Default value is returned if the asked property is not available.
 *
 * @param key
 *            property key
 * @param defValue
 *            default value
 * @return {@link Integer#getInteger(String)} for the given <i>key</i>,
 *         <i>defValue</i>
 */
void HostPersistence::persistHostProperty(const std::string &key, const std::string &value)
{
    if( key.empty() || value.empty())
    {
        cout << "Given KEY or VALUE is empty..." << endl;
        throw IllegalArgumentException();
    }


    try {
		string eRet = getProperty (key);

		

		if (eRet.compare(value) == 0) {
			/* Same value. No need to do anything */
			return;
		}
			
		 /* Save a current copy before modifying */
	    writeToFile(filePath + "tmpDB");
			
		/* First of all check whether the entry is already present in the hashtable */
		/*cout << "Entry Already present. Erase it before Writting..!\n";*/
		_properties.erase (key);

    }
    catch (const IllegalArgumentException &e) {
		cout << "Entry  Not found..!\n";
    }
    catch (...) {

    }

    _properties.insert ({key, value });

    writeToFile(filePath);

    return;
}

/**
 * Provides a simple utility method for loading the key and defvalue from the backup file.
 *
 * @param filename
 *            property file name
 *
 * @param std::map <std::string, std::string> map
 *            properties map
 * @return {@link Integer#getInteger(String)} 0 for Success and -1 for Failure
 */
void HostPersistence::loadFromFile (const string &fileName, std::map <std::string, std::string> &map)
{
    char keyValue[1024]  = "";
    char key[1024] = "";
    FILE *filePtr = NULL;
    
    filePtr = fopen (fileName.c_str(), "r");
    if (filePtr != NULL) {
        while (!feof(filePtr))
        {
            /* RDKSEC-811 Coverity fix - CHECKED_RETURN */
            if (fscanf (filePtr, "%1023s\t%1023s\n", key, keyValue) <= 0 )
            {
                cout << "fscanf failed !\n";
            }
            else
            {
                /* Check the TypeOfInput variable and then call the appropriate insert function */
                map.insert ({key, keyValue });
            }
        }
        fclose (filePtr);
    }
    else {
    	throw Exception(-1);
    }
}

/**
 * Provides a simple utility method for dumping all the data that are in hash to tmp file.
 *
 * @return {@link Integer#getInteger(String)} 0 for Success and -1 for Failure
 */
void HostPersistence::writeToFile (const string &fileName)
{
	unlink(fileName.c_str());
	
	if(_properties.size() > 0)
    {
		/*
			* Replacing the ofstream to fwrite
			* Because the ofstream.close or ofstream.flush or ofstream.rdbuf->sync
			* does not sync the data onto disk.
			* TBD - This need to be changed to C++ APIs in future.
		*/
		
		FILE *file = fopen (fileName.c_str(),"w");
		if (file != NULL)
		{
			for ( auto it = _properties.begin(); it != _properties.end(); ++it )  {
				string dataToWrite = it->first + "\t" + it->second + "\n";
				unsigned int size = dataToWrite.length();
				fwrite(dataToWrite.c_str(),1,size,file);
				/*cout << "Size " << size <<  endl;*/
				/*cout << "Item " << it->first << " Value" << it->second << endl;*/
			}
		
			fflush (file); //Flush buffers to FS
			fsync(fileno(file)); // Flush file to HDD
			fclose (file);
		}
	}
}

}



/** @} */
/** @} */
