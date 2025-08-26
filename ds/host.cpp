/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
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
* @defgroup ds
* @{
**/


#include <iostream>
#include <algorithm>
#include <string.h>
#include "iarmProxy.hpp"
#include "audioOutputPortConfig.hpp"
#include "videoOutputPortConfig.hpp"
#include "list.hpp"
#include "host.hpp"
#include "videoDeviceConfig.hpp"
#include "dsVideoPort.h"
#include "dsVideoDevice.h"
#include "dsAudio.h"
#include "dsDisplay.h"
#include "dslogger.h"
#include "dsHost.h"
#include "dsTypes.h"
#include "unsupportedOperationException.hpp"
#include "hostEDID.hpp"
#include "dsInternal.h"

/**
 * @file host.cpp
 * @brief The host module is the central module of the Device Settings module.
 * Each devices establishes one and only one host instance that represents the entire host device.
 */
using namespace std;

namespace device 
{
   	
	const int Host::kPowerOn = dsPOWER_ON;
    const int Host::kPowerOff = dsPOWER_OFF;
    const int Host::kPowerStandby = dsPOWER_STANDBY;
 
	
	Host::Host() 
	{
		// TODO Auto-generated destructor stub
	}

    Host::~Host() {
    	if (true)
		{
			IARMProxy::getInstance().UnRegisterPowerEventHandler();
		}
    }


/**
 * @addtogroup dssettingshostapi
 * @{
 */

/**
 * @fn Host& Host::getInstance()
 * @brief This API is used to get a reference to the single instance of the Host object.
 *
 * @return reference to Host singleton instance
 */
    Host& Host::getInstance()
    {
        static Host instance; // instance is in thread-safe now.
        static bool isFirstTime = true;
        try {
			if (isFirstTime) {
				isFirstTime = false;
            }
            else {
            }
        }
        catch (...) {
    		cout << "Host Exception Thrown ...!\n";
        }
        return instance;
    }


/**
 * @fn void Host::addPowerModeListener(PowerModeChangeListener *l)
 * @brief This API is used to register listeners for Power Mode change event.
 * The listener object is created by application and should be released by the application once the listener is removed.
 * Listeners will be notified with the new mode via the listener's powerModeChanged() callback.
 *
 * @param[in] PowerModeChangeListener Pointer to Power Mode change listener
 *
 * @return None
 */
    void Host::addPowerModeListener(PowerModeChangeListener *l)
    {
        std::list < PowerModeChangeListener* > ::iterator it;
        
        it = find (powerEvntListeners.begin(),powerEvntListeners.end(), l);
        if (it == powerEvntListeners.end())
        {
            powerEvntListeners.push_back (l);
            cout << "Added Power Mode listener...!\n";
        }
        else
        {
            cout << "Already register for Power Mode Change\n";
        }
        return ;
    }


/**
 * @fn void Host::removePowerModeChangeListener(PowerModeChangeListener *l)
 * @brief This API is used to remove a listener from Power Mode change listener list.
 *
 * @param[in]PowerModeChangeListener The listener to remove.
 *
 * @return None
 */
    void Host::removePowerModeChangeListener(PowerModeChangeListener *l)
    {
        std::list < PowerModeChangeListener* > ::iterator it ;
        it = find (powerEvntListeners.begin(),powerEvntListeners.end(), l);
        if (it == powerEvntListeners.end())
        {
            cout << "Not Registered for Power Mode change yet...!\n";
        }
        else
        {
            powerEvntListeners.erase (it);
            cout << "Removed from Power Mode listener group..!\n";
        }
        return;
    }


/**
 * @fn void Host::addDisplayConnectionListener (DisplayConnectionChangeListener *l)
 * @brief This API is used to register listeners for Display connection change event.
 * The listener will be notified if Display device is connected/disconnected from the video output port.
 * The notification only carries the state change of the connection.
 * It does not carry any other system state change that may have been triggered by the connection.
 * The application is responsible to query the various parts of system to detect any such change.
 * For example, when a TV device is replaced, the application shall query the video output port again upon the connection
 * for the new resolution supported by the TV.
 * The listener object is created by application and should be released by the application once the listener is removed.
 *
 * @param[in] DisplayConnectionChangeListener Pointer to Display connection change listener
 *
 * @return None
 */
    void Host::addDisplayConnectionListener (DisplayConnectionChangeListener *l)
    {
        std::list < DisplayConnectionChangeListener* > ::iterator it;
        
        it = find (dispEvntListeners.begin(), dispEvntListeners.end(), l);
        if (it == dispEvntListeners.end())
        {
            dispEvntListeners.push_back (l);
            cout << "Added Display listener...!\n";
        }
        else
        {
            cout << "Already registered to the Display listener\n";
        }
        return ;
    }


/**
 * @fn void Host::removeDisplayConnectionListener (DisplayConnectionChangeListener *l)
 * @brief This API is used to remove listeners from the Display connection change event list.
 *
 * @param[in] DisplayConnectionChangeListener The listener to remove
 *
 * @return None
 */
    void Host::removeDisplayConnectionListener (DisplayConnectionChangeListener *l)
    {
        std::list < DisplayConnectionChangeListener* > ::iterator it ;
        it = find (dispEvntListeners.begin(), dispEvntListeners.end(), l);
        if (it == dispEvntListeners.end())
        {
            cout << "Not Registered to Display Listener yet...!\n";
        }
        else
        {
            dispEvntListeners.erase (it);
            cout << "Removed from the Display listener...!\n";
        }
        return;
    }


/**
 * @fn void Host::notifyPowerChange (const  int mode)
 * @brief This function is used to get the current power state.
 *
 * @param[in] mode Power mode of the decoder.
 * @return None.
 */
    void Host::notifyPowerChange (const  int mode)
    {
    	std::list < PowerModeChangeListener* > ::iterator it;
        for ( it = powerEvntListeners.begin() ; it != powerEvntListeners.end(); it++ )
        {
            (*it)->powerModeChanged (mode);
        }
    }

    void Host::notifyDisplayConnectionChange (int portHandle, bool newConnectionStatus)
    {
        std::list < DisplayConnectionChangeListener* > ::iterator it;
        for ( it = dispEvntListeners.begin() ; it != dispEvntListeners.end(); it++ )
        {
            (*it)->displayConnectionChanged(getVideoOutputPort(portHandle), newConnectionStatus);
            getVideoOutputPort(portHandle).setDisplayConnected(newConnectionStatus);
        }
    }

/**
 * @fn bool Host::setPowerMode(int mode)
 * @brief This API is used to change the power mode of the device.
 * This function will set the power mode to active or standby and turn off all the ouput ports.
 * The accepted modes are defined as constants in host module: Host::kPowerOn, Host::kPowerOff, Host::kPowerStandby.
 * Upon the return of this API, the power mode listeners will be notified of the new power mode.
 * This function is currently not supported.
 *
 * @param[in] mode New Power Mode.
 *
 * @return None
 */
    bool Host::setPowerMode(int mode)
    {
        throw UnsupportedOperationException();
    }


/**
 * @fn int Host::getPowerMode()
 * @brief This API is used to get the current power mode of the device.
 * This function is currently not supported.
 *
 * @return  Current power mode
 */
    int Host::getPowerMode()
    {
      throw UnsupportedOperationException();
    }


/**
 * @fn List<VideoOutputPort>  Host::getVideoOutputPorts()
 * @brief This API is used to get the list of the video output ports supported on the device.
 * A complete list of ports that are physically present on the platform will be returned, regardless the state of each port.
 * If no Video Output Port is available on the device, an empty List will be returned.
 *
 * @param  None
 *
 * @return A list of videooutputport instances
 */
    List<VideoOutputPort>  Host::getVideoOutputPorts()
    {
    	List<VideoOutputPort> rPorts;

        List<VideoOutputPortType> vTypes =  VideoOutputPortConfig::getInstance().getSupportedTypes();
        for (size_t i = 0; i < vTypes.size(); i++) {
        	List<VideoOutputPort> vPorts = vTypes.at(i).getPorts();
            for (size_t j = 0; j < vPorts.size(); j++) {
            	rPorts.push_back(vPorts.at(j));
            }
        }

    	return rPorts;
    }


/**
 * @fn List<AudioOutputPort>  Host::getAudioOutputPorts()
 * @brief This API is used to get the list of the audio output ports supported on the device.
 * A complete list of ports that are physically present on the platform will be returned, regardless the state of each port.
 * If no Audio Output Port is available on the device, an empty List will be returned.
 *
 * @return A list of audiooutputport instances
 */
    List<AudioOutputPort>  Host::getAudioOutputPorts()
    {
    	List<AudioOutputPort> rPorts;

        List<AudioOutputPortType> aTypes = AudioOutputPortConfig::getInstance().getSupportedTypes();
        for (size_t i = 0; i < aTypes.size(); i++) {
        	List<AudioOutputPort> aPorts = aTypes.at(i).getPorts();
            for (size_t j = 0; j < aPorts.size(); j++) {
            	rPorts.push_back(aPorts.at(j));
            }
        }
    	return rPorts;
    }


/**
 * @fn List<VideoDevice>  Host::getVideoDevices()
 * @brief This API is used to get the list of the video devices (i.e. Decoders) supported on the device.
 * If no Video Device is available on the device, an empty List will be returned.
 *
 * @return A list of videodevice instances
 */
    List<VideoDevice>  Host::getVideoDevices()
    {
        return VideoDeviceConfig::getInstance().getDevices();
    }


/**
 * @fn VideoOutputPort &Host::getVideoOutputPort(const std::string &name)
 * @brief This API is used to get the reference to the video output port by its name.
 * The name of the port must match one of those returned by Host::getVideoOutputPorts.
 * Application shall not develop code that depends on the actual naming conventions of the video output port.
 * Such code as Host::getVideoOutputPort("HDMI0") is discouraged as the name of the left-most
 * HDMI output port may not necessarily be "HDMI0" from all implementations.
 * IllegalArgumentException will be thrown if the "name" does not match any of the ports.
 *
 * @param[in] name Name of the port
 *
 * @return Reference to its object
 */
    VideoOutputPort &Host::getVideoOutputPort(const std::string &name)
    {
        return VideoOutputPortConfig::getInstance().getPort(name);
    }


/**
 * @fn  VideoOutputPort &Host::getVideoOutputPort(int id)
 * @brief This API is used to get a reference to the video output port by its id.
 * The name of the port must match one of those returned by Host::getVideoOutputPorts.
 * Application shall not develop code that depends on the actual naming conventions of the video output port.
 * Such code as Host::getVideoOutputPort("HDMI0") is discouraged as the name of the left-most
 * HDMI output port may not necessarily be "HDMI0" from all implementations.
 * IllegalArgumentException will be thrown if the "name" does not match any of the ports.
 *
 * @param[in] id port id
 *
 * @return Reference to its object
 */
    VideoOutputPort &Host::getVideoOutputPort(int id)
    {
        return VideoOutputPortConfig::getInstance().getPort(id);
    }


/**
 * @fn AudioOutputPort &Host::getAudioOutputPort(const std::string &name)
 * @brief This API is used to get the reference to the audio output port by its name.
 * The name of the port must match one of those returned by Host::getAudioOutputPorts.
 * Application shall not develop code that depends on the actual naming conventions of the Audio output port.
 * Such code as Host::getAudioOutputPort("SPDIF0") is discouraged as the name of the left-most
 * SPDIF output port may not necessarily be "SPDIF0" from all implementations.
 * IllegalArgumentException will be thrown if the "name" does not match any of the ports.
 *
 * @param[in] name Name of the port
 *
 * @return Reference to its object
 */
    AudioOutputPort &Host::getAudioOutputPort(const std::string &name)
    {
        return AudioOutputPortConfig::getInstance().getPort(name);
    }


/**
 * @fn  AudioOutputPort &Host::getAudioOutputPort(int id)
 * @brief This API is used to get a reference to the Audio output port by its id.
 * The name of the port must match one of those returned by Host::getAudioOutputPorts.
 * Application shall not develop code that depends on the actual naming conventions of the Audio output port.
 * Such code as Host::getAudioOutputPort("SPDIF0") is discouraged as the name of the left-most
 * SPDIF output port may not necessarily be "SPDIF0" from all implementations.
 * IllegalArgumentException will be thrown if the "name" does not match any of the ports.
 *
 * @param[in] id port id
 *
 * @return Reference to its object
 */
    AudioOutputPort &Host::getAudioOutputPort(int id)
    {
        return AudioOutputPortConfig::getInstance().getPort(id);
    }


/**
 * @fn Host::getPreferredSleepMode()
 * @brief This API is used to gets the Preferred sleep Mode and implements the low power mode for RDK platform.
 * dsGetPreferredSleepMode(&mode) function returns preferred sleep mode which is persisted.
 *
 * @param None
 *
 * @return A list of sleepmode instances
 */
    SleepMode Host::getPreferredSleepMode()
    {
        dsSleepMode_t mode = dsHOST_SLEEP_MODE_LIGHT;
        dsError_t ret = dsGetPreferredSleepMode(&mode);
        if (ret == dsERR_NONE) {
          return SleepMode::getInstance(mode);
        }
        else {
          printf("Host::getPreferredSleepMode failed IPC, returning default LIGHT_SLEEP\r\n");
          return SleepMode::getInstance(dsHOST_SLEEP_MODE_LIGHT);
        }
    }


/**
 * @fn Host::setPreferredSleepMode(const SleepMode mode)
 * @brief This API is used to set the  Preferred sleep Mode and implements the low power mode for RDK platform.
 *
 * @param[in] mode  sleep mode that is expected to be persisted
 *
 * @return ID value of the sleepmode instance
 */
    int Host::setPreferredSleepMode(const SleepMode mode)
    {
        return dsSetPreferredSleepMode((dsSleepMode_t)mode.getId());
    }


/**
 * @fn  List<SleepMode> Host::getAvailableSleepModes()
 * @brief This function is used to get the available sleep modes.
 *
 * @return A list of sleepmode instance
 */
    List<SleepMode> Host::getAvailableSleepModes()
    {
        SleepMode mode = SleepMode::getInstance(1);
        return mode.getSleepModes();
    }


/**
 * @fn Host::getCPUTemperature()
 * @brief This API gets CPU temperature.
 *
 * @return CPU temperature in default float value
 * @retval temperature Indicates CPU temerature in centigrade.
 */
    float Host::getCPUTemperature()
    {
        float temperature = 45.0;
        dsGetCPUTemperature(&temperature);
        return temperature;
    }

    /**
    * @fn Host::getVersion()
    * @brief Get DS HAL API Version
    *
    * @return Returns 4 byte Versionnumber  
    * @retval In 4 byte VersionNumber, Two Most significant Bytes are Major number 
    * and Two Least Significant Bytes are minor number.
    */
    uint32_t  Host::getVersion(void) 
    {
        uint32_t versionNumber;     
        dsGetVersion(&versionNumber);
        return versionNumber;
    }
  
/**
 * @fn  void Host::getHostEDID(std::vector<uint8_t> &edid) const
 * @brief This function is used to get the EDID information of the host device.
 *
 * @param[out] edid EDID info of the device.
 * @return None.
 */
    void Host::getHostEDID(std::vector<uint8_t> &edid) const
    {
	printf("Host::getHostEDID \r\n");
        dsError_t ret = dsERR_NONE;
        int length = 0;
	unsigned char edidBytes[EDID_MAX_DATA_SIZE];

    	const char* exceptionstr = "";

        ret = dsGetHostEDID( edidBytes, &length);

        printf("Host::getHostEDID has ret %d\r\n", ret);
        if (ret == dsERR_NONE) {
            edid.clear();
            edid.insert(edid.begin(), edidBytes, edidBytes + length);
        } else {
            exceptionstr = "getHostEDID failed";
        }

        if (ret != dsERR_NONE) {
            throw Exception(ret, exceptionstr);
        }
    }

/**
 * @fn Host::getSocIDFromSDK()
 * @brief This API to get the SOC ID.
 * dsGetSocIDFromSDK() function returns the SOC ID in string format.
 *
 * @param None
 *
 * @return Device ID
 */
   std::string Host::getSocIDFromSDK()
   {
        char socID[1024];
        dsGetSocIDFromSDK(socID);
        return std::string(socID);
   }

   /**
    * Host::getCurrentAudioFormat(dsAudioFormat_t &audioFormat)
    * @brief
    *
    * @param[in/out] Audio format of currently playing content
    *
    * @return None
    */
   void Host::getCurrentAudioFormat(dsAudioFormat_t &audioFormat)
   {
       dsError_t ret = dsERR_NONE;
       dsAudioFormat_t aFormat;

       ret = dsGetAudioFormat(NULL, &aFormat);

       if (ret == dsERR_NONE)
       {
           audioFormat = aFormat;
       }
       else
       {
           throw Exception(ret);
       }
   }


   /**
    * Host::getSinkDeviceAtmosCapability(dsATMOSCapability_t & atmosCapability)
    * @brief
    *
    * @param[in/out] Sink device ATMOS capability
    *
    * @return None
    */
   void Host::getSinkDeviceAtmosCapability(dsATMOSCapability_t & atmosCapability)
   {
       dsError_t ret = dsERR_NONE;
       dsATMOSCapability_t capability;

       //TV panel sink device is panel itsel so audio port is passed as NULL
       ret = dsGetSinkDeviceAtmosCapability(NULL, &capability);

       if (ret == dsERR_NONE)
       {
           atmosCapability = capability;
       }
       else
       {
           throw Exception(ret);
       }
   }

    /**
     * @fn Host::getMS12ConfigDetails(std::string &configType)
     * @brief This API is used to retrieve the ms12 config supported by platform.
     *
     * If return is not equal to dsERR_NONE, it will throw the ret to IllegalArgumentException Handler and
     * it will pass the message as "No message for this exception" with the value of "dsERR_INVALID_PARAM" from dsError type.
     *
     * @param[in] ms12 config type.
     *
     * @return None
     */
   void Host::getMS12ConfigDetails(std::string &configType)
   {
       dsError_t ret = dsERR_NONE;
       char type[MS12_CONFIG_BUF_SIZE];
       ret = dsGetMS12ConfigType(type);
       if (ret == dsERR_NONE)
       {
           configType.assign(type);
       }
       else
       {
           throw Exception(ret);
       }
   }

   void Host::setAudioAtmosOutputMode(bool enable)
   {
       dsError_t ret = dsERR_NONE;
       //TV panel sink device is panel itsel so audio port is passed as NULL
       ret = dsSetAudioAtmosOutputMode(NULL,enable);
       if (ret != dsERR_NONE)
       {
           throw Exception(ret);
       }
   }


    /**
     * @fn Host::setAssociatedAudioMixing(const bool mixing)
     * @brief This API is used to enable/disable Associated Audio Mixing.
     *
     * If return is not equal to dsERR_NONE, it will throw the ret to IllegalArgumentException Handler and
     * it will pass the message as "No message for this exception" with the value of "dsERR_INVALID_PARAM" from dsError type.
     *
     * @param[in] mixing enable/disable Associated Audio Mixing.
     *
     * @return None
     */
    void Host::setAssociatedAudioMixing(const bool mixing)
    {
            dsError_t ret = dsERR_NONE;
            if ( (ret = dsSetAssociatedAudioMixing(NULL, mixing)) == dsERR_NONE) {
            }
            else
            {
                throw Exception(ret);
            }
    }


    /**
     * @fn  void Host::getAssociatedAudioMixing(bool *mixing)
     * @brief This API is used to get status of Associated Audio Mixing
     *
     * @return Current status of Associated Audio Mixing
     */
    void Host::getAssociatedAudioMixing(bool *mixing)
    {
            dsError_t ret = dsERR_NONE;
            bool _mixing = false;

            if(mixing == NULL) {
                ret = dsERR_INVALID_PARAM;
                throw Exception(ret);
            }

            if ( (ret = dsGetAssociatedAudioMixing(NULL, &_mixing)) == dsERR_NONE)
            {
                *mixing = _mixing;
            }
            else
            {
                throw Exception(ret);
            }
    }


    /**
     * @fn Host::setFaderControl(const int mixerBalance)
     * @brief This API is used to set the mixerbalance betweeen main and associated audio
     *
     * If return is not equal to dsERR_NONE, it will throw the ret to IllegalArgumentException Handler and
     * it will pass the message as "No message for this exception" with the value of "dsERR_INVALID_PARAM" from dsError type.
     *
     * @param[in] New mixerbalance betweeen main and associated audio.
     *
     * @return None
     */
    void Host::setFaderControl(const int mixerBalance)
    {
            dsError_t ret = dsERR_NONE;
            ret = dsSetFaderControl(NULL, mixerBalance);
            if (ret != dsERR_NONE) {
                throw Exception(ret);
            }
    }


    /**
     * @fn void Host::getFaderControl(int *mixerBalance)
     * @brief This API is used to get the mixerbalance betweeen main and associated audio
     *
     * @return Current mixerbalance betweeen main and associated audio
     */
    void Host::getFaderControl(int *mixerBalance)
    {
            dsError_t ret = dsERR_NONE;
            int _mixerBalance = 0;

            if(mixerBalance == NULL) {
                ret = dsERR_INVALID_PARAM;
                throw Exception(ret);
            }

            ret = dsGetFaderControl(NULL, &_mixerBalance);
            if (ret == dsERR_NONE)
            {
                *mixerBalance = _mixerBalance;
            }
            else
            {
                throw Exception(ret);
            }
    }


    /**
     * @fn Host::setPrimaryLanguage(const std::string pLang)
     * @brief This API is used to set Primary language
     *
     * If return is not equal to dsERR_NONE, it will throw the ret to IllegalArgumentException Handler and
     * it will pass the message as "No message for this exception" with the value of "dsERR_INVALID_PARAM" from dsError type.
     *
     * @param[string] Primary language to be set
     *
     * @return None
     */
    void Host::setPrimaryLanguage(const std::string pLang)
    {
            dsError_t ret = dsERR_NONE;
            if ( (ret = dsSetPrimaryLanguage(NULL, pLang.c_str())) == dsERR_NONE) {
            }
            else
            {
                throw Exception(ret);
            }
    }


    /**
     * @fn  void Host::getPrimaryLanguage(std::string pLang)
     * @brief This API is used to get the current Primary language
     *
     * If return is not equal to dsERR_NONE, it will throw the ret to IllegalArgumentException Handler and
     * it will pass the message as "No message for this exception" with the value of "dsERR_INVALID_PARAM" from dsError type.
     *
     * @return[string] Primary language
     */
    void Host::getPrimaryLanguage(std::string &pLang)
    {
            dsError_t ret = dsERR_NONE;
            char _pLang[MAX_LANGUAGE_LEN] = {0};
            if ( (ret = dsGetPrimaryLanguage(NULL, _pLang)) == dsERR_NONE)
            {
                pLang.assign(_pLang);
            }
            else
            {
                throw Exception(ret);
            }
    }


    /**
     * @fn Host::setSecondaryLanguage(const std::string sLang)
     * @brief This API is used to set Secondary language
     *
     * If return is not equal to dsERR_NONE, it will throw the ret to IllegalArgumentException Handler and
     * it will pass the message as "No message for this exception" with the value of "dsERR_INVALID_PARAM" from dsError type.
     *
     * @param[string] Secondary language to be set
     *
     * @return None
     */
    void Host::setSecondaryLanguage(const std::string sLang)
    {
            dsError_t ret = dsERR_NONE;
            if ( (ret = dsSetSecondaryLanguage(NULL, sLang.c_str())) == dsERR_NONE) {
            }
            else
            {
                throw Exception(ret);
            }
    }


    /**
     * @fn  void Host::getSecondaryLanguage(std::string &sLang)
     * @brief This API is used to get the current AC4 Secondary language
     *
     * If return is not equal to dsERR_NONE, it will throw the ret to IllegalArgumentException Handler and
     * it will pass the message as "No message for this exception" with the value of "dsERR_INVALID_PARAM" from dsError type.
     *
     * @return[string] AC4 Secondary language
     */
    void Host::getSecondaryLanguage(std::string &sLang)
    {
            dsError_t ret = dsERR_NONE;
            char _sLang[MAX_LANGUAGE_LEN] = {0};
            if ( (ret = dsGetSecondaryLanguage(NULL, _sLang)) == dsERR_NONE)
            {
                sLang.assign(_sLang);
            }
            else
            {
                throw Exception(ret);
            }
    }

   bool  Host::isHDMIOutPortPresent()
   {
       bool isHDMIOutPort = false;

       List<AudioOutputPortType> aTypes = AudioOutputPortConfig::getInstance().getSupportedTypes();
       for (size_t i = 0; i < aTypes.size(); i++) {
           List<AudioOutputPort> aPorts = aTypes.at(i).getPorts();
           for (size_t j = 0; j < aPorts.size(); j++) {
               string portName  = aPorts.at(j).getName();
               if (portName.find("HDMI0") != string::npos) {
                   isHDMIOutPort = true;
                   break;
               }
           }
           if (isHDMIOutPort) {
               break;
           }
       }

       return isHDMIOutPort;
   }

   std::string  Host::getDefaultVideoPortName()
   {
       std::string strDefaultVideoPortName = "HDMI0";
       bool isDefaultPortfound = false;
       List<VideoOutputPortType> vTypes = VideoOutputPortConfig::getInstance().getSupportedTypes();
       for (size_t i = 0; i < vTypes.size(); i++) {
           List<VideoOutputPort> vPorts = vTypes.at(i).getPorts();
           for (size_t j = 0; j < vPorts.size(); j++) {
               string portName  = vPorts.at(j).getName();
               /*By default assign first port as default port*/
               if (0==i && 0==j) {
                   strDefaultVideoPortName.assign (portName);
               }
               /*If HDMI0 present assign HDMI0 as default video port*/
               if (portName.find("HDMI0") != string::npos) {
                   strDefaultVideoPortName.assign (portName);
                   isDefaultPortfound = true;
                   break;
               }
               /*If INTERNAL0 present assign INTERNAL0 as default video port*/
               if (portName.find("INTERNAL0") != string::npos) {
                   strDefaultVideoPortName.assign (portName);
                   isDefaultPortfound = true;
                   break;
               }
           }
           if (isDefaultPortfound) {
               break;
           }
       }

       return strDefaultVideoPortName;
   }

   std::string  Host::getDefaultAudioPortName()
   {
       std::string strDefaultAudioPortName = "HDMI0";
       bool isDefaultPortfound = false;
       List<AudioOutputPortType> aTypes = AudioOutputPortConfig::getInstance().getSupportedTypes();
       for (size_t i = 0; i < aTypes.size(); i++) {
           List<AudioOutputPort> aPorts = aTypes.at(i).getPorts();
           for (size_t j = 0; j < aPorts.size(); j++) {
               string portName  = aPorts.at(j).getName();
               /*By default assign first port as default port*/
               if (0==i && 0==j) {
                   strDefaultAudioPortName.assign (portName);
               }
               /*If HDMI0/SPEAKER0 present assign it as default video port*/
               if (portName.find("HDMI0") != string::npos || portName.find("SPEAKER0") != string::npos) {
                   strDefaultAudioPortName.assign (portName);
                   isDefaultPortfound = true;
                   break;
               }
           }
           if (isDefaultPortfound) {
               break;
           }
       }

       return strDefaultAudioPortName;
   }

   void Host::setAudioMixerLevels (dsAudioInput_t aInput, int volume) {
    // the mixer level change is not port specific, so handle is set to NULL
    dsError_t ret = dsSetAudioMixerLevels(NULL,aInput, volume);
        if (ret != dsERR_NONE)
    {
        throw Exception(ret);
    }
    printf ("%s:%d - Set Audio Mixer levels for audio input: %d with volume = %d\n", __PRETTY_FUNCTION__, __LINE__,aInput, volume);
   }

/**
 * @fn void  Host::Register(IHDMIInEvents *listener)
 * @brief This API is used to register the Events
 *
 * @return unint32_t
 */
uint32_t Host::Register(IHDMIInEvents *listener) 
{
  return 0;
}


/**
 * @fn void  Host::UnRegister(IHDMIInEvents *listener)
 * @brief This API is used to UnRegister the Events
 *
 * @return unint32_t
 */
uint32_t Host::UnRegister(IHDMIInEvents *listener) 
{
  return 0;
}

/**
 * @fn void  Host::Register(ICompositeInEvents *Evtnotification)
 * @brief This API is used to register the Events
 *
 * @return unint32_t
 */
uint32_t Host::Register(ICompositeInEvents *listener)
{
  return 0;
}

/**
 * @fn void  Host::UnRegister(ICompositeInEvents *Evtnotification)
 * @brief This API is used to UnRegister the Events
 *
 * @return unint32_t
 */
uint32_t Host::UnRegister(ICompositeInEvents *listener)
{
  return 0;
}

/**
 * @fn void  Host::UnRegister(IDisplayHDMIHotPlugEvents *Evtnotification)
 * @brief This API is used to Register the Events
 *
 * @return unint32_t
 */
uint32_t Host::Register(IDisplayHDMIHotPlugEvents *listener)
{
  return 0;
}


/**
 * @fn void  Host::UnRegister(IDisplayHDMIHotPlugEvents *Evtnotification)
 * @brief This API is used to UnRegister the Events
 *
 * @return unint32_t
 */
uint32_t Host::UnRegister(IDisplayHDMIHotPlugEvents *listener)
{
  return 0;
}

/**
 * @fn void  Host::UnRegister(IDisplayEvents *Evtnotification)
 * @brief This API is used to Register the Events
 *
 * @return unint32_t
 */
uint32_t Host::Register(IDisplayEvents *listener)
{
  return 0;
}


/**
 * @fn void  Host::UnRegister(IDisplayEvents *Evtnotification)
 * @brief This API is used to UnRegister the Events
 *
 * @return unint32_t
 */
uint32_t Host::UnRegister(IDisplayEvents *listener)
{
  return 0;
}

/**
 * @fn void  Host::Register(IAudioOutputPortEvents *Evtnotification)
 * @brief This API is used to Register the Events
 *
 * @return unint32_t
 */
uint32_t Host::Register(IAudioOutputPortEvents *listener)
{
  return 0;
}


/**
 * @fn void  Host::UnRegister(IAudioOutputPortEvent *Evtnotification)
 * @brief This API is used to UnRegister the Events
 *
 * @return unint32_t
 */
uint32_t Host::UnRegister(IAudioOutputPortEvents *listener)
{
  return 0;
}

/**
 * @fn void  Host::Register(IVideoOutputPortEvents *Evtnotification)
 * @brief This API is used to Register the Events
 *
 * @return unint32_t
 */
uint32_t Host::Register(IVideoOutputPortEvents *listener)
{
	return 0;
}


/**
 * @fn void  Host::UnRegister(IVideoOutputPortEvents *Evtnotification)
 * @brief This API is used to UnRegister the Events
 *
 * @return unint32_t
 */
uint32_t Host::UnRegister(IVideoOutputPortEvents *listener)
{
  return 0;
}



/**
 * @fn void  Host::Register(IVideoDeviceEvents *Evtnotification)
 * @brief This API is used to Register the Events
 *
 * @return unint32_t
 */
uint32_t Host::Register(IVideoDeviceEvents *listener)
{
  return 0;
}


/**
 * @fn void  Host::UnRegister(IVideoDeviceEvents *Evtnotification)
 * @brief This API is used to UnRegister the Events
 *
 * @return unint32_t
 */
uint32_t Host::UnRegister(IVideoDeviceEvents *listener)
{
  return 0;
}



}


/** @} */

/** @} */
/** @} */
