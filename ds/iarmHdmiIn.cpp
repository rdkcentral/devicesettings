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
 * @file IarmHdmiIn.cpp
 * @brief Configuration of HDMI Input
 */

/**
* @defgroup devicesettings
* @{
* @defgroup ds
* @{
**/


#include <iostream>
#include <sstream>
#include <string>
#include <string.h>
#include "hdmiIn.hpp"
#include "illegalArgumentException.hpp"
#include "host.hpp"

#include "dslogger.h"
#include "dsError.h"
#include "dsTypes.h"
#include "dsHdmiIn.h"
#include "dsUtl.h"
#include "edid-parser.hpp"
#include "dsInternal.h"

#include "utils.h" // for Utils::IARM and IARM_CHECK


#include "iarmHdmiIn.hpp"
#include <algorithm>
#include <cstring>

// Static data definitions
std::mutex IarmHdmiInput::s_mutex;
std::vector<device::HdmiInput::IEvent*> IarmHdmiInput::hdmiInListener;

constexpr IarmHdmiInput::EventHandlerMapping IarmHdmiInput::eventHandlers[] = {
    { IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG,           IarmHdmiInput::OnHDMIInEventHotPlug },
    { IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS,     IarmHdmiInput::OnHDMIInEventSignalStatus },
    { IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS,            IarmHdmiInput::OnHDMIInEventStatus },
    { IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE, IarmHdmiInput::OnHDMInVideoModeUpdate },
    { IARM_BUS_DSMGR_EVENT_HDMI_IN_ALLM_STATUS,       IarmHdmiInput::OnHDMInAllmStatus },
    { IARM_BUS_DSMGR_EVENT_HDMI_IN_AVI_CONTENT_TYPE,  IarmHdmiInput::OnHDMInAVIContentType },
    { IARM_BUS_DSMGR_EVENT_HDMI_IN_AV_LATENCY,        IarmHdmiInput::OnHDMInAVLatency },
    { IARM_BUS_DSMGR_EVENT_ZOOM_SETTINGS,             IarmHdmiInput::OnHDMInVRRStatus },
    { IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG,              IarmHdmiInput::OnHDMIHotPlug }
};

const char* IarmHdmiInput::OWNER_NAME = IARM_BUS_DSMGR_NAME;

IarmHdmiInput::IarmHdmiInput() {}
IarmHdmiInput::~IarmHdmiInput() {}


uint32_t IarmHdmiInput::Register(device::HdmiInput::IEvent* listener) {
    std::lock_guard<std::mutex> lock(s_mutex);

    // First listener, register all handlers
    if (hdmiInListener.empty()) {
        for (auto& eh : eventHandlers) {
            IARM_Bus_RegisterEventHandler(OWNER_NAME, eh.eventId, eh.handler);
        }
    }

    // Add to listener list if not already present
    if (std::find(hdmiInListener.begin(), hdmiInListener.end(), listener) == hdmiInListener.end()) {
        hdmiInListener.push_back(listener);
    }

    return 0; // Success
}

// Unregister a listener and remove IARM handlers if last listener
uint32_t IarmHdmiInput::UnRegister(device::HdmiInput::IEvent* listener) {
    std::lock_guard<std::mutex> lock(s_mutex);

    auto it = std::remove(hdmiInListener.begin(), hdmiInListener.end(), listener);
    if (it != hdmiInListener.end()) {
        hdmiInListener.erase(it, hdmiInListener.end());
    }

    // If no more listeners, unregister all handlers
    if (hdmiInListener.empty()) {
        for (auto& eh : eventHandlers) {
            IARM_Bus_UnRegisterEventHandler(OWNER_NAME, eh.eventId);
        }
    }

    return 0; // Success
}



template<typename F>

void IarmHdmiInput::Dispatch(F&& fn) {
    std::lock_guard<std::mutex> lock(s_mutex);
    for (auto* listener : hdmiInListener) {
        fn(listener);
    }
}

// === Handlers Implementation ===

// Each handler receives the event and dispatches to all listeners
void IarmHdmiInput::OnHDMIInEventHotPlug(const char* owner, IARM_EventId_t eventId, void *data, size_t len) {
    // Cast data to the expected struct
    IARM_Bus_DSMgr_EventData_t* eventData = static_cast<IARM_Bus_DSMgr_EventData_t*>(data);
    dsHdmiInPort_t port = static_cast<dsHdmiInPort_t>(eventData->data.hdmi_in_connect.port);
    bool isConnected = eventData->data.hdmi_in_connect.isPortConnected;
    Dispatch([&](device::HdmiInput::IEvent* l) { l->OnHDMIInEventHotPlug(port, isConnected); });
}

void IarmHdmiInput::OnHDMIInEventSignalStatus(const char* owner, IARM_EventId_t eventId, void *data, size_t len) {
    IARM_Bus_DSMgr_EventData_t* eventData = static_cast<IARM_Bus_DSMgr_EventData_t*>(data);
    dsHdmiInPort_t port = static_cast<dsHdmiInPort_t>(eventData->data.hdmi_in_sig_status.port);
    dsHdmiInSignalStatus_t status = static_cast<dsHdmiInSignalStatus_t>(eventData->data.hdmi_in_sig_status.status);
    Dispatch([&](device::HdmiInput::IEvent* l) { l->OnHDMIInEventSignalStatus(port, status); });
}

void IarmHdmiInput::OnHDMIInEventStatus(const char* owner, IARM_EventId_t eventId, void *data, size_t len) {
    IARM_Bus_DSMgr_EventData_t* eventData = static_cast<IARM_Bus_DSMgr_EventData_t*>(data);
    dsHdmiInPort_t activePort = static_cast<dsHdmiInPort_t>(eventData->data.hdmi_in_status.activePort);
    bool isPresented = eventData->data.hdmi_in_status.isPresented;
    Dispatch([&](device::HdmiInput::IEvent* l) { l->OnHDMIInEventStatus(activePort, isPresented); });
}

void IarmHdmiInput::OnHDMInVideoModeUpdate(const char* owner, IARM_EventId_t eventId, void *data, size_t len) {
    IARM_Bus_DSMgr_EventData_t* eventData = static_cast<IARM_Bus_DSMgr_EventData_t*>(data);
    dsHdmiInPort_t port = static_cast<dsHdmiInPort_t>(eventData->data.hdmi_in_videomode.port);
    const dsVideoPortResolution_t& res = eventData->data.hdmi_in_videomode.resolution;
    Dispatch([&](device::HdmiInput::IEvent* l) { l->OnHDMInVideoModeUpdate(port, res); });
}

void IarmHdmiInput::OnHDMInAllmStatus(const char* owner, IARM_EventId_t eventId, void *data, size_t len) {
    IARM_Bus_DSMgr_EventData_t* eventData = static_cast<IARM_Bus_DSMgr_EventData_t*>(data);
    dsHdmiInPort_t port = static_cast<dsHdmiInPort_t>(eventData->data.hdmi_in_allm.port);
    bool status = eventData->data.hdmi_in_allm.status;
    Dispatch([&](device::HdmiInput::IEvent* l) { l->OnHDMInAllmStatus(port, status); });
}

void IarmHdmiInput::OnHDMInAVIContentType(const char* owner, IARM_EventId_t eventId, void *data, size_t len) {
    IARM_Bus_DSMgr_EventData_t* eventData = static_cast<IARM_Bus_DSMgr_EventData_t*>(data);
    dsHdmiInPort_t port = static_cast<dsHdmiInPort_t>(eventData->data.hdmi_in_avi_ct.port);
    dsAviContentType_t type = static_cast<dsAviContentType_t>(eventData->data.hdmi_in_avi_ct.avi_ct);
    Dispatch([&](device::HdmiInput::IEvent* l) { l->OnHDMInAVIContentType(port, type); });
}

void IarmHdmiInput::OnHDMInAVLatency(const char* owner, IARM_EventId_t eventId, void *data, size_t len) {
    IARM_Bus_DSMgr_EventData_t* eventData = static_cast<IARM_Bus_DSMgr_EventData_t*>(data);
    int audioDelay = eventData->data.hdmi_in_av_latency.audio_latency;
    int videoDelay = eventData->data.hdmi_in_av_latency.video_latency;
    Dispatch([&](device::HdmiInput::IEvent* l) { l->OnHDMInAVLatency(audioDelay, videoDelay); });
}

void IarmHdmiInput::OnHDMInVRRStatus(const char* owner, IARM_EventId_t eventId, void *data, size_t len) {
    IARM_Bus_DSMgr_EventData_t* eventData = static_cast<IARM_Bus_DSMgr_EventData_t*>(data);
    dsVideoZoom_t zoom = static_cast<dsVideoZoom_t>(eventData->data.zoomSettings.zoomSetting);
    Dispatch([&](device::HdmiInput::IEvent* l) { l->OnZoomSettingsChanged(zoom); });
}

