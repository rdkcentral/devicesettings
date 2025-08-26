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
#include <memory>
#include <string>

#include "audioOutputPort.hpp"
#include "dsAVDTypes.h"
#include "dsError.h"
#include "list.hpp"
#include "sleepMode.hpp"
#include "videoDevice.hpp"
#include "videoOutputPort.hpp"

/**
 * @file host.hpp
 * @brief It contains class, structures referenced by host.cpp file.
 */
using namespace std;

namespace device {

// Forward declaration of the implementation class
class IarmHostImpl;
// In future add a conditional to choose implementation class based on build configuration
using DefaultImpl = IarmHostImpl;

/**
 * @class Host
 * @brief Class to implement the Host interface.
 * @ingroup devicesettingsclass
 */
class Host {
public:
    static const int kPowerOn;
    static const int kPowerOff;
    static const int kPowerStandby;


    struct ICompositeInEvents {
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

    dsError_t  Register(ICompositeInEvents *listener);
    dsError_t  UnRegister(ICompositeInEvents *listener);
    

    struct IVideoDeviceEvents {
        // @brief Display Frame rate Pre-change notification
        // @param frameRate: new framerate
        virtual void OnDisplayFrameratePreChange(const std::string& frameRate) { };

        // @brief Display Frame rate Post-change notification
        // @param frameRate: new framerate
        virtual void OnDisplayFrameratePostChange(const std::string& frameRate) { };

        // @brief Zoom settings changed
        // @text OnZoomSettingsChanged
        // @param zoomSetting: Currently applied zoom setting
        virtual void OnZoomSettingsChanged(dsVideoZoom_t zoomSetting) { };
    };

    // @brief Register a listener for video device events
    // @param listener: class object implementing the listener
    dsError_t Register(IVideoDeviceEvents* listener);

    // @brief UnRegister a listener for video device events
    // @param listener: class object implementing the listener
    dsError_t UnRegister(IVideoDeviceEvents* listener);

    struct IVideoOutputPortEvents {

        // @brief On Resolution Pre changed
        // @param width: width of the resolution
        // @param height: height of the resolution
        virtual void OnResolutionPreChange(int width, int height) { };

        // @brief On Resolution Post change
        // @param width: width of the resolution
        // @param height: height of the resolution
        virtual void OnResolutionPostChange(int width, int height) { };

        // @brief On HDCP Status change
        // @param hdcpStatus: HDCP Status
        virtual void OnHDCPStatusChange(dsHdcpStatus_t hdcpStatus) { };

        // @brief On Video Format update
        // @param videoFormatHDR: Video format HDR standard
        virtual void OnVideoFormatUpdate(dsHDRStandard_t videoFormatHDR) { };
    };

    // @brief Register a listener for video port events
    // @param listener: class object implementing the listener
    dsError_t Register(IVideoOutputPortEvents* listener);

    // @brief UnRegister a listener for video port events
    // @param listener: class object implementing the listener
    dsError_t UnRegister(IVideoOutputPortEvents* listener);

    struct IAudioOutputPortEvents {

        // @brief Associated Audio mixing changed
        // @param mixing: true or false
        virtual void OnAssociatedAudioMixingChanged(bool mixing) { };

        // @brief Audio Fader balance changed
        // @param mixerBalance: applied mixer balance value
        virtual void OnAudioFaderControlChanged(int mixerBalance) { };

        // @brief Primary language for Audio changed
        // @param primaryLanguage: current primary language for audio
        virtual void OnAudioPrimaryLanguageChanged(const std::string& primaryLanguage) { };

        // @brief Secondary language for Audio changed
        // @param secondaryLanguage: current secondary language for audio
        virtual void OnAudioSecondaryLanguageChanged(const std::string& secondaryLanguage) { };

        // @brief Audio output hot plug event
        // @param portType: Type of audio port see AudioPortType
        // @param uiPortNumber: The port number assigned by UI
        // @param isPortConnected: true (connected) or false (not connected)
        virtual void OnAudioOutHotPlug(dsAudioPortType_t portType, int uiPortNumber, bool isPortConnected) { };

        // @brief Dolby Atmos capabilities changed
        // @param atmosCapability: the Dolby Atmos capability
        // @param status: true (available) or false (not available)
        virtual void OnDolbyAtmosCapabilitiesChanged(dsATMOSCapability_t atmosCapability, bool status) { };

        // @brief Audio port state changed
        // @param audioPortState: audio port state
        // TODO: requires dsMgr.h header include ??
        // virtual void OnAudioPortStateChanged(dsAudioPortState_t audioPortState) { };

        // @brief Audio mode for the respective audio port - raised for every type of port
        // @param audioPortType: audio port type see dsAudioPortType_t
        // @param audioStereoMode: audio stereo mode - see dsAudioStereoMode_t
        virtual void OnAudioModeEvent(dsAudioPortType_t audioPortType, dsAudioStereoMode_t audioStereoMode) { };

        // @brief Audio level changed
        // @param audioiLevel: audio level value
        virtual void OnAudioLevelChangedEvent(int audioLevel) { };

        // @brief Audio Output format changed
        // @param audioFormat: Type of audio format see AudioFormat
        virtual void OnAudioFormatUpdate(dsAudioFormat_t audioFormat) { };
    };

    // @brief Register a listener for audio port events
    // @param listener: class object implementing the listener
    dsError_t Register(IAudioOutputPortEvents* listener);

    // @brief UnRegister a listener for audio port events
    // @param listener: class object implementing the listener
    dsError_t UnRegister(IAudioOutputPortEvents* listener);

    bool setPowerMode(int mode);
    int getPowerMode();
    SleepMode getPreferredSleepMode();
    int setPreferredSleepMode(const SleepMode);
    List<SleepMode> getAvailableSleepModes();

    static Host& getInstance(void);

    List<VideoOutputPort> getVideoOutputPorts();
    List<AudioOutputPort> getAudioOutputPorts();
    List<VideoDevice> getVideoDevices();
    VideoOutputPort& getVideoOutputPort(const std::string& name);
    VideoOutputPort& getVideoOutputPort(int id);
    AudioOutputPort& getAudioOutputPort(const std::string& name);
    AudioOutputPort& getAudioOutputPort(int id);
    float getCPUTemperature();
    uint32_t getVersion(void);
    void setVersion(uint32_t versionNumber);
    void getHostEDID(std::vector<uint8_t>& edid) const;
    std::string getSocIDFromSDK();
    void getSinkDeviceAtmosCapability(dsATMOSCapability_t& atmosCapability);
    void setAudioAtmosOutputMode(bool enable);
    void setAssociatedAudioMixing(const bool mixing);
    void getAssociatedAudioMixing(bool* mixing);
    void setFaderControl(const int mixerbalance);
    void getFaderControl(int* mixerBalance);
    void setPrimaryLanguage(const std::string pLang);
    void getPrimaryLanguage(std::string& pLang);
    void setSecondaryLanguage(const std::string sLang);
    void getSecondaryLanguage(std::string& sLang);
    bool isHDMIOutPortPresent();
    std::string getDefaultVideoPortName();
    std::string getDefaultAudioPortName();
    void getCurrentAudioFormat(dsAudioFormat_t& audioFormat);
    void getMS12ConfigDetails(std::string& configType);
    void setAudioMixerLevels(dsAudioInput_t aInput, int volume);

private:
    std::unique_ptr<DefaultImpl> m_impl;
    Host();
    virtual ~Host();
    // Avoid copies
    Host(const Host&)            = delete;
    Host& operator=(const Host&) = delete;

    DefaultImpl& impl();
};

}

#endif /* _DS_HOST_HPP_ */

/** @} */
/** @} */
