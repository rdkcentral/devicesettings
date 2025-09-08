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

/**
 * This file provides all the devicesettings notifications for the usage of the various plugin clients.
 * The notifications are classified in to various blocks.
 *
 * The Clients have to register for the respective required blocks like IHdmiInEvents, ICompositeInEvents, 
 * IVideoOutputPortEvents, IVideoDeviceEvents etc through the Register API Call.
 * The Clients have to unregister for the respective required blocks like IHdmiInEvents, 
 * ICompositeInEvents, IVideoOutputPortEvents, IVideoDeviceEvents etc through the UnRegister API Call 
 * when the activity is completed.
 * 
 * The clients need to implement the required notification handler functions for the respective blocks 
 * which they have registered. If clients don't need to handle some notifications in a block, they 
 * need not implement those functions.
 *
 * Example: A client can choose to register for IHdmiInEvents block of events and implement only 
 * required notification function OnHdmiInEventHotPlug and can ignore other functions in the same event 
 * block like OnHdmiInEventSignalStatus.
 * 
 * The Client will receive the event notification for which it has registered and implemented the notification. 
 * This will happen when the respective action has taken place in the device.
 *
 * IHdmiInEvents 
 *     OnHdmiInEventHotPlug : Will notify the HDMI Hot Plug IN/OUT Event
 *     OnHdmiInEventSignalStatus : Will notify the HDMI Signal Status
 *     OnHdmiInEventStatus : Will notify the port specific HDMI status 
 *     OnHdmiInVideoModeUpdate : Will notify the HDMI video resolution changes
 *     OnHdmiInAllmStatus : Will notify the HDMI Auto Low Latency Mode
 *     OnHdmiInAVIContentType : Will notify the HDMI AVI info like Gaming, Cinema, Photo etc
 *     OnHdmiInVRRStatus : Will notify the HDMI VRR info(Variable Refresh Rate)
 *     OnHdmiInAVLatency : Will notify about the HDMI AV Latency info.
 *
 * ICompositeInEvents
 *     OnCompositeInHotPlug : Will notify about the Hotplug in Composite Mode.
 *     OnCompositeInSignalStatus : Will notify about the Signal Status in Composite Mode.
 *     OnCompositeInStatus : Will notify about the Composite Status in Composite Mode.
 *     OnCompositeInVideoModeUpdate : Will notify about the Update in video Mode in Composite Mode.
 *
 * IDisplayEvents
 *     OnDisplayRxSense : Will notify about the RX Sense
 *
 * IVideoDeviceEvents
 *     OnDisplayFrameratePreChange : Will notify about the Frame Rate Before Change.
 *     OnDisplayFrameratePostChange : Will notify about the Frame rate After change.
 *     OnZoomSettingsChanged : Will notify about the zoom settings changed
 *     
 * IVideoOutputPortEvents
 *     OnResolutionPreChange : Will notify about the Resolution Pre Change
 *     OnHDCPStatusChange : Will notify about the HDCP status Change
 *     OnVideoFormatUpdate : Will notify about the Video Format Change.
 * 
 * IAudioOutputPortEvents
 *     OnAssociatedAudioMixingChanged : Will notify about the Audio Mixer Change.
 *     OnAudioFaderControlChanged :  Will notify about the Fader Control Change.
 *     OnAudioPrimaryLanguageChanged : Will notify about the Primary Language Change.
 *     OnAudioSecondaryLanguageChanged : Will notify about the Secondary Language Change.
 *     OnAudioOutHotPlug : Will notify about the Audio Out Hot Plug Change.
 *     OnDolbyAtmosCapabilitiesChanged : Will notify about the ATMOS capability Change.
 *     OnAudioPortStateChanged : Will notify about the Audio Port State  Change.
 *     OnAudioModeEvent : Will notify about the Audio Mode Event Change.
 *     OnAudioLevelChangedEvent : Will notify about the Audio Level Change.
 *     OnAudioFormatUpdate : Will notify about the Audio Format Change.
 * 
 * IDisplayDeviceEvents
 *     OnDisplayHDMIHotPlug : Will notify about the HDMI Hot Plug Change
 **/

#ifndef _DS_HOST_HPP_
#define _DS_HOST_HPP_
#include <memory>
#include <string>

#include "audioOutputPort.hpp"
#include "dsAVDTypes.h"
#include "dsDisplay.h"
#include "dsError.h"
#include "list.hpp"
#include "sleepMode.hpp"
#include "videoDevice.hpp"
#include "videoOutputPort.hpp"

// dsMgr.h cannot be exposed to clients as we plan to deprecate it
// this is only temporary until dsAudioPortState_t is moved to ds halif headers
#ifndef HAVE_DSAUDIOPORT_STATE_TYPE
#define HAVE_DSAUDIOPORT_STATE_TYPE
#ifdef __cplusplus
extern "C" {
#endif

typedef enum _dsAudioPortState {
    dsAUDIOPORT_STATE_UNINITIALIZED,
    dsAUDIOPORT_STATE_INITIALIZED,
    dsAUDIOPORT_STATE_MAX
} dsAudioPortState_t;

#ifdef __cplusplus
}
#endif
#endif // HAVE_DSAUDIOPORT_STATE_TYPE

/**
 * @file host.hpp
 * @brief It contains class, structures referenced by host.cpp file.
 */
using namespace std;

namespace device {

// Forward declaration of the implementation class
class IarmImpl;
// In future add a conditional to choose implementation class based on build configuration
using DefaultImpl = IarmImpl;

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

    struct IHdmiInEvents {
        virtual ~IHdmiInEvents() = default;

        // @brief HDMI Event Hot Plug
        // @param port: port 0 or 1 et al
        // @param isConnected: is it connected (true) or not (false)
        virtual void OnHdmiInEventHotPlug(dsHdmiInPort_t port, bool isConnected);

        // @brief HDMI Event Signal status
        // @param port: port 0 or 1 et al
        // @param signalStatus: Signal Status
        virtual void OnHdmiInEventSignalStatus(dsHdmiInPort_t port, dsHdmiInSignalStatus_t signalStatus);

        // @brief HDMI Event Signal status
        // @param activePort: port 0 or 1 et al
        // @param isPresented: is it presented or not
        virtual void OnHdmiInEventStatus(dsHdmiInPort_t activePort, bool isPresented);

        // @brief HDMI Video Mode update
        // @param port: port 0 or 1 et al
        // @param videoPortResolution: Video port resolution
        virtual void OnHdmiInVideoModeUpdate(dsHdmiInPort_t port, const dsVideoPortResolution_t& videoPortResolution);

        // @brief HDMI ALLM (Auto Low Latency Mode) status
        // @param port: port 0 or 1 et al
        // @param allmStatus: allm status
        virtual void OnHdmiInAllmStatus(dsHdmiInPort_t port, bool allmStatus);

        // @brief HDMI Event AVI content type
        // @param port: port 0 or 1 et al
        // @param aviContentType: AVI content type
        virtual void OnHdmiInAVIContentType(dsHdmiInPort_t port, dsAviContentType_t aviContentType);

        // @brief HDMI VRR status
        // @param port: port 0 or 1 et al
        // @param vrrType: VRR type
        virtual void OnHdmiInVRRStatus(dsHdmiInPort_t port, dsVRRType_t vrrType);

        // @brief HDMI Event AV Latency
        // @param audioDelay: audio delay (in millisecs)
        // @param videoDelay: video delay (in millisecs)
        virtual void OnHdmiInAVLatency(int audioDelay, int videoDelay);
    };

    // @brief Register a listener for HDMI device events
    // @param listener: class object implementing the listener
    // @param clientName: Optional (but highly recommended) name of the client registering for events
    //                    clientName is used for logging purposes only
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t Register(IHdmiInEvents* listener, const std::string& clientName = "");

    // @brief UnRegister a listener for HDMI device events
    // @param listener: class object implementing the listener
    dsError_t UnRegister(IHdmiInEvents* listener);

    struct ICompositeInEvents {
        virtual ~ICompositeInEvents() = default;

        // @brief Composite In Hotplug event
        // @param port: Port of the hotplug
        // @param isConnected: Is it connected (true) or not(false)
        virtual void OnCompositeInHotPlug(dsCompositeInPort_t port, bool isConnected);

        // @brief Composite In Signal status
        // @param port: Port of the hotplug
        // @param signalStatus: Signal status
        virtual void OnCompositeInSignalStatus(dsCompositeInPort_t port, dsCompInSignalStatus_t signalStatus);

        // @brief Composite In status
        // @param activePort: Active port
        // @param isPresented: is it presented to user
        virtual void OnCompositeInStatus(dsCompositeInPort_t activePort, bool isPresented);

        // @brief Composite In Video Mode Update
        // @param activePort: Active port
        // @param videoResolution: See DisplayVideoPortResolution
        virtual void OnCompositeInVideoModeUpdate(dsCompositeInPort_t activePort, dsVideoPortResolution_t videoResolution);
    };

    // @brief Register a listener for composite events
    // @param listener: class object implementing the listener
    // @param clientName: Optional (but highly recommended) name of the client registering for events
    //                    clientName is used for logging purposes only
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t Register(ICompositeInEvents* listener, const std::string& clientName = "");

    // @brief UnRegister a listener for composite events
    // @param listener: class object implementing the listener
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t UnRegister(ICompositeInEvents* listener);

    struct IDisplayEvents {
        virtual ~IDisplayEvents() = default;

        // @brief Display RX Sense event
        // @param displayEvent: DS_DISPLAY_RXSENSE_ON or DS_DISPLAY_RXSENSE_OFF
        virtual void OnDisplayRxSense(dsDisplayEvent_t displayEvent);
    };

    // @brief Register a listener for display events
    // @param listener: class object implementing the listener
    // @param clientName: Optional (but highly recommended) name of the client registering for events
    //                    clientName is used for logging purposes only
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t Register(IDisplayEvents* listener, const std::string& clientName = "");

    // @brief UnRegister a listener for display events
    // @param listener: class object implementing the listener
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t UnRegister(IDisplayEvents* listener);

    struct IVideoDeviceEvents {
        virtual ~IVideoDeviceEvents() = default;

        // @brief Display Frame rate Pre-change notification
        // @param frameRate: new framerate
        virtual void OnDisplayFrameratePreChange(const std::string& frameRate);

        // @brief Display Frame rate Post-change notification
        // @param frameRate: new framerate
        virtual void OnDisplayFrameratePostChange(const std::string& frameRate);

        // @brief Zoom settings changed
        // @param zoomSetting: Currently applied zoom setting
        virtual void OnZoomSettingsChanged(dsVideoZoom_t zoomSetting);
    };

    // @brief Register a listener for video device events
    // @param listener: class object implementing the listener
    // @param clientName: Optional (but highly recommended) name of the client registering for events
    //                    clientName is used for logging purposes only
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t Register(IVideoDeviceEvents* listener, const std::string& clientName = "");

    // @brief UnRegister a listener for video device events
    // @param listener: class object implementing the listener
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t UnRegister(IVideoDeviceEvents* listener);

    struct IVideoOutputPortEvents {
        virtual ~IVideoOutputPortEvents() = default;

        // @brief On Resolution Pre changed
        // @param width: width of the resolution
        // @param height: height of the resolution
        virtual void OnResolutionPreChange(int width, int height);

        // @brief On Resolution Post change
        // @param width: width of the resolution
        // @param height: height of the resolution
        virtual void OnResolutionPostChange(int width, int height);

        // @brief On HDCP Status change
        // @param hdcpStatus: HDCP Status
        virtual void OnHDCPStatusChange(dsHdcpStatus_t hdcpStatus);

        // @brief On Video Format update
        // @param videoFormatHDR: Video format HDR standard
        virtual void OnVideoFormatUpdate(dsHDRStandard_t videoFormatHDR);
    };

    // @brief Register a listener for video port events
    // @param listener: class object implementing the listener
    // @param clientName: Optional (but highly recommended) name of the client registering for events
    //                    clientName is used for logging purposes only
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t Register(IVideoOutputPortEvents* listener, const std::string& clientName = "");

    // @brief UnRegister a listener for video port events
    // @param listener: class object implementing the listener
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t UnRegister(IVideoOutputPortEvents* listener);

    struct IAudioOutputPortEvents {
        virtual ~IAudioOutputPortEvents() = default;

        // @brief Associated Audio mixing changed
        // @param mixing: true or false
        virtual void OnAssociatedAudioMixingChanged(bool mixing);

        // @brief Audio Fader balance changed
        // @param mixerBalance: applied mixer balance value
        virtual void OnAudioFaderControlChanged(int mixerBalance);

        // @brief Primary language for Audio changed
        // @param primaryLanguage: current primary language for audio
        virtual void OnAudioPrimaryLanguageChanged(const std::string& primaryLanguage);

        // @brief Secondary language for Audio changed
        // @param secondaryLanguage: current secondary language for audio
        virtual void OnAudioSecondaryLanguageChanged(const std::string& secondaryLanguage);

        // @brief Audio output hot plug event
        // @param portType: Type of audio port see AudioPortType
        // @param uiPortNumber: The port number assigned by UI
        // @param isPortConnected: true (connected) or false (not connected)
        virtual void OnAudioOutHotPlug(dsAudioPortType_t portType, uint32_t uiPortNumber, bool isPortConnected);

        // @brief Dolby Atmos capabilities changed
        // @param atmosCapability: the Dolby Atmos capability
        // @param status: true (available) or false (not available)
        virtual void OnDolbyAtmosCapabilitiesChanged(dsATMOSCapability_t atmosCapability, bool status);

        // @brief Audio port state changed
        // @param audioPortState: audio port state
        virtual void OnAudioPortStateChanged(dsAudioPortState_t audioPortState);

        // @brief Audio mode for the respective audio port - raised for every type of port
        // @param audioPortType: audio port type see dsAudioPortType_t
        // @param audioStereoMode: audio stereo mode - see dsAudioStereoMode_t
        virtual void OnAudioModeEvent(dsAudioPortType_t audioPortType, dsAudioStereoMode_t audioStereoMode);

        // @brief Audio level changed
        // @param audioiLevel: audio level value
        virtual void OnAudioLevelChangedEvent(int audioLevel);

        // @brief Audio Output format changed
        // @param audioFormat: Type of audio format see AudioFormat
        virtual void OnAudioFormatUpdate(dsAudioFormat_t audioFormat);
    };

    // @brief Register a listener for audio port events
    // @param listener: class object implementing the listener
    // @param clientName: Optional (but highly recommended) name of the client registering for events
    //                    clientName is used for logging purposes only
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t Register(IAudioOutputPortEvents* listener, const std::string& clientName = "");

    // @brief UnRegister a listener for audio port events
    // @param listener: class object implementing the listener
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t UnRegister(IAudioOutputPortEvents* listener);

    struct IDisplayDeviceEvents {
        virtual ~IDisplayDeviceEvents() = default;

        // @brief Display HDMI (out) Hot plug event
        // @param displayEvent: display event type see dsDisplayEvent_t
        virtual void OnDisplayHDMIHotPlug(dsDisplayEvent_t displayEvent);
    };

    // @brief Register a listener for display device events
    // @param listener: class object implementing the listener
    // @param clientName: Optional (but highly recommended) name of the client registering for events
    //                    clientName is used for logging purposes only
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t Register(IDisplayDeviceEvents* listener, const std::string& clientName = "");

    // @brief UnRegister a listener for display device events
    // @param listener: class object implementing the listener
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t UnRegister(IDisplayDeviceEvents* listener);

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
