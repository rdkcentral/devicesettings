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


#ifndef _DS_HDMIIN_HPP_
#define _DS_HDMIIN_HPP_

#include <stdint.h>
#include <vector>

#include "dsTypes.h"
//#include "dsDisplay.h"

/**
 * @file hdmiIn.hpp
 * @brief Structures and classes for HDMI Input are defined here
 * @ingroup hdmiIn
 */

static const int8_t HDMI_IN_PORT_NONE = -1;

namespace device 
{


/**
 * @class HdmiInput
 * @brief This class manages HDMI Input
 */
class HdmiInput  
{

public:


    struct IEvent {
            
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
 
            // @brief HDMI Event AV Latency
            // @text OnHDMInAVLatency
            // @param audioDelay: audio delay (in millisecs)
            // @param videoDelay: video delay (in millisecs)
            virtual void OnHDMInAVLatency(int audioDelay, int videoDelay) { };
	    
            // @brief HDMI VRR status
            // @text OnHDMInVRRStatus
            // @param port: port 0 or 1 et al
            // @param vrrType: VRR type
            virtual void OnHDMInVRRStatus(dsHdmiInPort_t port, dsVRRType_t vrrType) { };

	    // @brief Zoom settings changed
            // @text OnZoomSettingsChanged
            // @param zoomSetting: Currently applied zoom setting
            virtual void OnZoomSettingsChanged(dsVideoZoom_t zoomSetting) { };

	    // @brief HDMI Hotplug event
            // @text OnHDMIHotPlug
            // @param displayEvent: CONNECTED or DISCONNECTED
    	    virtual void OnHDMIHotPlug(dsDisplayEvent_t displayEvent) { };
    };

    uint32_t Register(IEvent *listener);
    uint32_t UnRegister(IEvent *listener);
    
    static HdmiInput & getInstance();

    uint8_t getNumberOfInputs        () const;
    bool    isPresented              () const;
    bool    isActivePort             (int8_t Port) const;
    int8_t  getActivePort            () const;
    bool    isPortConnected          (int8_t Port) const;
    void    selectPort               (int8_t Port, bool requestAudioMix = false, int videoPlaneType = dsVideoPlane_PRIMARY, bool topMost = false) const;
    void    scaleVideo               (int32_t x, int32_t y, int32_t width, int32_t height) const;
    void    selectZoomMode           (int8_t zoomMode) const;
    void    pauseAudio               () const;
    void    resumeAudio              () const;
    std::string  getCurrentVideoMode () const;
    void getCurrentVideoModeObj (dsVideoPortResolution_t& resolution);
    void getEDIDBytesInfo (int iHdmiPort, std::vector<uint8_t> &edid) const;
    void getHDMISPDInfo (int iHdmiPort, std::vector<uint8_t> &data);
    void setEdidVersion (int iHdmiPort, int iEdidVersion);
    void getEdidVersion (int iHdmiPort, int *iEdidVersion);
    void getHdmiALLMStatus (int iHdmiPort, bool *allmStatus);
    void getSupportedGameFeatures (std::vector<std::string> &featureList);
    void getAVLatency(int *audio_latency,int *video_latency);
    void setEdid2AllmSupport(int iHdmiPort,bool allm_suppport);
    void getEdid2AllmSupport(int iHdmiPort, bool *allm_support);
    void setVRRSupport (int iHdmiPort, bool vrr_suppport);
    void getVRRSupport (int iHdmiPort, bool *vrr_suppport);
    void getVRRStatus (int iHdmiPort, dsHdmiInVrrStatus_t *vrrStatus);
    void getHdmiVersion (int iHdmiPort, dsHdmiMaxCapabilityVersion_t *capversion);
private:
    HdmiInput ();           /* default constructor */
    virtual ~HdmiInput ();  /* destructor */
};


}   /* namespace device */


#endif /* _DS_HDMIIN_HPP_ */


/** @} */
/** @} */
