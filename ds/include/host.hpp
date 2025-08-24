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


#ifndef _DS_HOST_HPP_
#define _DS_HOST_HPP_

#include <iostream>
#include "powerModeChangeListener.hpp"
#include "displayConnectionChangeListener.hpp"
#include "audioOutputPort.hpp"
#include "videoOutputPort.hpp"
#include "videoDevice.hpp"
#include "sleepMode.hpp"
#include  "list.hpp"
#include "dsTypes.h"
#include "dsDisplay.h"

#include  <list>
#include <string>


/**
 * @file host.hpp
 * @brief It contains class,structures referenced by host.cpp file.
 */
using namespace std;

namespace device {


/**
 * @class Host
 * @brief Class to implement the Host interface.
 * @ingroup devicesettingsclass
 */
class Host {
public:
       
    struct IHDMIInEvent {
            
	    // @brief HDMI Event Hot Plug
            // @text onHDMIInEventHotPlug
            // @param port: port 0 or 1 et al
            // @param isConnected: is it connected (true) or not (false)
            virtual void OnHDMIInEventHotPlug(dsHdmiInPort_t port, bool isConnected) { };
 
            // @brief HDMI Event Signal status
            // @text OnHDMIInEventSignalStatus
            // @param port: port 0 or 1 et al
            // @param signalStatus: Signal Status
            virtual void OnHDMIInEventSignalStatus(dsHdmiInPort_t port, dsHdmiInSignalStatus_t signalStatus) { };
 
            // @brief HDMI Event Signal status
            // @text onHDMIInEventStatus
            // @param activePort: port 0 or 1 et al
            // @param isPresented: is it presented or not
            virtual void OnHDMIInEventStatus(dsHdmiInPort_t activePort, bool isPresented) { };
 
            // @brief HDMI Video Mode update
            // @text onHDMInVideoModeUpdate
            // @param port: port 0 or 1 et al
            // @param videoPortResolution: Video port resolution
            virtual void OnHDMInVideoModeUpdate(dsHdmiInPort_t port, const dsVideoPortResolution_t& videoPortResolution) { };
 
            // @brief HDMI ALLM (Auto Low Latency Mode) status
            // @text onHDMInAllmStatus
            // @param port: port 0 or 1 et al
            // @param allmStatus: allm status
            virtual void OnHDMInAllmStatus(dsHdmiInPort_t port, bool allmStatus) { };
 
            // @brief HDMI Event AVI content type
            // @text OnHDMInAVIContentType
            // @param port: port 0 or 1 et al
            // @param aviContentType: AVI content type
            virtual void OnHDMInAVIContentType(dsHdmiInPort_t port, dsAviContentType_t aviContentType) { };
 
            // @brief HDMI VRR status
            // @text OnHDMInVRRStatus
            // @param port: port 0 or 1 et al
            // @param vrrType: VRR type
            virtual void OnHDMInVRRStatus(dsHdmiInPort_t port, dsVRRType_t vrrType) { };

	    // @brief Zoom settings changed
            // @text OnZoomSettingsChanged
            // @param zoomSetting: Currently applied zoom setting
            virtual void OnZoomSettingsChanged(dsVideoZoom_t zoomSetting) { };
    };

    uint32_t Register(IHDMIInEvent *listener);
    uint32_t UnRegister(IHDMIInEvent *listener);


    struct ICompositeInEvent {
            // @brief Composite In Hotplug event
            // @text onCompositeInHotPlug
            // @param port: Port of the hotplug
            // @param isConnected: Is it connected (true) or not(false)
            virtual void OnCompositeInHotPlug(dsCompositeInPort_t port, bool isConnected) { };
 
            // @brief Composite In Signal status
            // @text onCompositeInSignalStatus
            // @param port: Port of the hotplug
            // @param signalStatus: Signal status
            virtual void OnCompositeInSignalStatus(dsCompositeInPort_t port, dsCompInSignalStatus_t signalStatus) { };
 
            // @brief Composite In status
            // @text onCompositeInStatus
            // @param activePort: Active port
            // @param isPresented: is it presented to user
            virtual void OnCompositeInStatus(dsCompositeInPort_t activePort, bool isPresented) { };


            // @brief Composite In Video Mode Update
            // @text OnCompositeInVideoModeUpdate
            // @param activePort: Active port
            // @param videoResolution: See DisplayVideoPortResolution
            virtual void OnCompositeInVideoModeUpdate(dsCompositeInPort_t activePort, dsVideoPortResolution_t videoResolution) { };
    };

    uint32_t Register(ICompositeInEvent *listener);
    uint32_t UnRegister(ICompositeInEvent *listener);
    

    struct IDisplayHDMIHotPlugEvent{

            // @brief Display HDMI Hot plug event
            // @text onDisplayHDMIHotPlug
            // @param displayEvent: DS_DISPLAY_EVENT_CONNECTED or DS_DISPLAY_EVENT_DISCONNECTED
            virtual void OnDisplayHDMIHotPlug(dsDisplayEvent_t displayEvent) { };
    };
    
    uint32_t Register(IDisplayHDMIHotPlugEvent*listener);
    uint32_t UnRegister(IDisplayHDMIHotPlugEvent *listener);



    struct IDisplayEvent{

            // @brief Display RX Sense event
            // @text onDisplayRxSense
            // @param displayEvent: DS_DISPLAY_RXSENSE_ON or DS_DISPLAY_RXSENSE_OFF
            virtual void OnDisplayRxSense(dsDisplayEvent_t displayEvent) { };
 
            // @brief Display HDCP Status
            // @text OnDisplayHDCPStatus
            virtual void OnDisplayHDCPStatus() { };
    };
    
    uint32_t Register(IDisplayEvent *listener);
    uint32_t UnRegister(IDisplayEvent *listener);


    struct IAudioOutputPortEvent{

            // @brief Associated Audio mixing changed
            // @text onAssociatedAudioMixingChanged
            // @param mixing: true or false
            virtual void OnAssociatedAudioMixingChanged(bool mixing) { };
    
            // @brief Audio Fader balance changed
            // @text onAudioFaderControlChanged
            // @param mixerBalance: applied mixer balance value
            virtual void OnAudioFaderControlChanged(int mixerBalance) { };
    
            // @brief Primary language for Audio changed
            // @text onAudioPrimaryLanguageChanged
            // @param primaryLanguage: current primary language for audio
            virtual void OnAudioPrimaryLanguageChanged(const std::string& primaryLanguage) { };

            // @brief Secondary language for Audio changed
            // @text onAudioSecondaryLanguageChanged
            // @param secondaryLanguage: current secondary language for audio
            virtual void OnAudioSecondaryLanguageChanged(const std::string& secondaryLanguage) { };
    
            // @brief Audio output hot plug event
            // @text onAudioOutHotPlug
            // @param portType: Type of audio port see AudioPortType
            // @param uiPortNumber: The port number assigned by UI
            // @param isPortConnected: true (connected) or false (not connected)
            virtual void OnAudioOutHotPlug(dsAudioPortType_t  audioPortType, int uiPortNumber, bool isPortConnected) { };


            // @brief Dolby Atmos capabilities changed
            // @text onDolbyAtmosCapabilitiesChanged
            // @param atmosCapability: the dolby atmos capability
            // @param status: true (available) or false (not available)
            virtual void OnDolbyAtmosCapabilitiesChanged(dsATMOSCapability_t atmosCapability, bool status) { };

            // @brief Audio port state changed
            // @text onAudioPortStateChanged
            // @param audioPortState: audio port state
            virtual void OnAudioPortStateChanged(dsAudioPortState_t audioPortState) { };

            // @brief Audio mode for the respective audio port - raised for every type of port
            // @text onAudioModeEvent
            // @param audioPortType: audio port type see AudioPortType
            // @param audioMode: audio mode - see audioStereoMode
            virtual void OnAudioModeEvent(dsAudioPortType_t  audioPortType, dsAudioStereoMode_t audioMode) { };

            // @brief Audio Output format changed
            // @text onAudioFormatUpdate
            // @param audioFormat: Type of audio format see AudioFormat
            virtual void OnAudioFormatUpdate(dsAudioFormat_t audioFormat) { };

    };

    uint32_t Register(IAudioOutputPortEvent *listener);
    uint32_t UnRegister(IAudioOutputPortEvent *listener);


    struct IVideoDeviceEvent {
            // @brief Display Framerate Pre-change
            // @text OnDisplayFrameratePreChange
            // @param frameRate: PreChange framerate
            virtual void OnDisplayFrameratePreChange(const std::string& frameRate) { };

            // @brief Display Framerate Post-change
            // @text OnDisplayFrameratePostChange
            // @param frameRate:  framerate post change
            virtual void OnDisplayFrameratePostChange(const std::string& frameRate) { };
     };
        
     uint32_t Register(IVideoDeviceEvent *listener);
     uint32_t UnRegister(IVideoDeviceEvent *listener);
	

     struct IVideoOutputPortEvent {
            
	    // @brief On Resolution Pre changed
            // @text OnResolutionPreChange
            // @param resolution: resolution
            virtual void OnResolutionPreChange(const ResolutionChange& resolution) { };
 
            // @brief On Resolution Post change
            // @text onResolutionPostChange
            // @param resolution: resolution
            virtual void OnResolutionPostChange(const ResolutionChange& resolution) { };
 
            // @brief On HDCP Status change
            // @text OnHDCPStatusChange
            // @param hdcpStatus: HDCP Status
            virtual void OnHDCPStatusChange(dsHdcpStatus_t hdcpStatus) { };
 
            // @brief On Video Format update
            // @text OnVideoFormatUpdate
            // @param videoFormatHDR: Video format HDR standard
            virtual void OnVideoFormatUpdate(dsHDRStandard_t videoFormatHDR) { };
    };

    uint32_t Register(IVideoOutputPortEvent *listener);
    uint32_t UnRegister(IVideoOutputPortEvent *listener);


    static const int kPowerOn;
    static const int kPowerOff;
    static const int kPowerStandby;

	bool setPowerMode(int mode);
	int getPowerMode();
    SleepMode getPreferredSleepMode();
    int setPreferredSleepMode(const SleepMode);
    List <SleepMode> getAvailableSleepModes();
	void addPowerModeListener(PowerModeChangeListener *l);
    void removePowerModeChangeListener(PowerModeChangeListener *l);
    void addDisplayConnectionListener(DisplayConnectionChangeListener *l);
    void removeDisplayConnectionListener(DisplayConnectionChangeListener *l);

	static Host& getInstance(void);

    List<VideoOutputPort> getVideoOutputPorts();
    List<AudioOutputPort> getAudioOutputPorts();
    List<VideoDevice> getVideoDevices();
    VideoOutputPort &getVideoOutputPort(const std::string &name);
    VideoOutputPort &getVideoOutputPort(int id);
    AudioOutputPort &getAudioOutputPort(const std::string &name);
    AudioOutputPort &getAudioOutputPort(int id);
    void notifyPowerChange(const  int mode);
    float getCPUTemperature();
    uint32_t  getVersion(void);
    void setVersion(uint32_t versionNumber);
    void getHostEDID(std::vector<uint8_t> &edid) const;
    std::string getSocIDFromSDK();
    void getSinkDeviceAtmosCapability(dsATMOSCapability_t & atmosCapability);
    void setAudioAtmosOutputMode(bool enable);
    void setAssociatedAudioMixing(const bool mixing);
    void getAssociatedAudioMixing(bool *mixing);
    void setFaderControl(const int mixerbalance);
    void getFaderControl(int *mixerBalance);
    void setPrimaryLanguage(const std::string pLang);
    void getPrimaryLanguage(std::string &pLang);
    void setSecondaryLanguage(const std::string sLang);
    void getSecondaryLanguage(std::string &sLang);
    bool isHDMIOutPortPresent();
    std::string getDefaultVideoPortName();
    std::string getDefaultAudioPortName();
    void getCurrentAudioFormat(dsAudioFormat_t &audioFormat);
    void getMS12ConfigDetails(std::string &configType);
    void setAudioMixerLevels (dsAudioInput_t aInput, int volume);
private:
	Host();
	virtual ~Host();
    //To Make the instance as thread-safe, using = delete, the result is, automatically generated methods (constructor, for example) from the compiler will not be created and, therefore, can not be called
    Host (const Host&)= delete;
    Host& operator=(const Host&)= delete;

    std::list < PowerModeChangeListener* > powerEvntListeners;
    std::list < DisplayConnectionChangeListener* > dispEvntListeners;
    void notifyDisplayConnectionChange(int portHandle, bool newConnectionStatus);
};

}

#endif /* _DS_HOST_HPP_ */


/** @} */
/** @} */
