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
 * @file IarmCompositeIn.cpp
 * @brief Configuration of COMPOSITE Input
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
#include "compositeIn.hpp"
#include "illegalArgumentException.hpp"
#include "host.hpp"
#include "dslogger.h"
#include "dsError.h"
#include "dsTypes.h"
#include "dsCompositeIn.h"
#include "dsUtl.h"

#include "utils.h" // for Utils::IARM and IARM_CHECK


#include "IarmCompositeInput.hpp"
#include <algorithm>
#include <cstring>

// Static data definitions
std::mutex IarmCompositeInput::s_mutex;
std::vector<IarmCompositeInput::IEvent*> IarmCompositeInput::compInListener;

constexpr IarmCompositeInput::EventHandlerMapping IarmCompositeInput::eventHandlers[] = {
    { IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG,           IarmCompositeInput::OnCompositeInHotPlugHandler },
    { IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS,     IarmCompositeInput::OnCompositeInSignalStatusHandler },
    { IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS,            IarmCompositeInput::OnCompositeInStatusHandler },
    { IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE, IarmCompositeInput::OnCompositeInVideoModeUpdateHandler },
};

constexpr const char* IarmCompositeInput::OWNER_NAME = IARM_BUS_DSMGR_NAME;

IarmCompositeInput::IarmCompositeInput() {}
IarmCompositeInput::~IarmCompositeInput() {}


uint32_t IarmCompositeInput::Register(IEvent* listener) {
    std::lock_guard<std::mutex> lock(s_mutex);

    // First listener, register all handlers
    if (compInListener.empty()) {
        for (auto& eh : eventHandlers) {
            IARM_Bus_RegisterEventHandler(OWNER_NAME, eh.eventId, eh.handler);
        }
    }

    // Add to listener list if not already present
    if (std::find(compInListener.begin(), compInListener.end(), listener) == compInListener.end()) {
        compInListener.push_back(listener);
    }

    return 0; // Success
}

// Unregister a listener and remove IARM handlers if last listener
uint32_t IarmCompositeInput::UnRegister(IEvent* listener) {
    std::lock_guard<std::mutex> lock(s_mutex);

    auto it = std::remove(compInListener.begin(), compInListener.end(), listener);
    if (it != compInListener.end()) {
        compInListener.erase(it, compInListener.end());
    }

    // If no more listeners, unregister all handlers
    if (compInListener.empty()) {
        for (auto& eh : eventHandlers) {
            IARM_Bus_UnRegisterEventHandler(OWNER_NAME, eh.eventId);
        }
    }

    return 0; // Success
}



template<typename F>

void IarmCompositeInput::Dispatch(F&& fn) {
    std::lock_guard<std::mutex> lock(s_mutex);
    for (auto* listener : compInListener) {
        fn(listener);
    }
}

// ====== Handlers ======

void IarmCompositeInput::OnCompositeInHotPlugHandler(IARM_EventId_t, void* data, size_t) {
    auto* eventData = static_cast<IARM_Bus_DSMgr_EventData_t*>(data);
    dsCompositeInPort_t dsPort = static_cast<dsCompositeInPort_t>(
        eventData->data.composite_in_connect.port
    );
    CompositeInPort compPort = static_cast<CompositeInPort>(dsPort);
    bool isConnected = eventData->data.composite_in_connect.isPortConnected;

    Dispatch([&](IEvent* l) { l->OnCompositeInHotPlug(compPort, isConnected); });
}

void IarmCompositeInput::OnCompositeInSignalStatusHandler(IARM_EventId_t, void* data, size_t) {
    auto* eventData = static_cast<IARM_Bus_DSMgr_EventData_t*>(data);
    CompositeInPort compPort = static_cast<CompositeInPort>(
        static_cast<dsCompositeInPort_t>(eventData->data.composite_in_sig_status.port)
    );
    CompositeInSignalStatus compStatus = static_cast<CompositeInSignalStatus>(
        static_cast<dsCompInSignalStatus_t>(eventData->data.composite_in_sig_status.status)
    );

    Dispatch([&](IEvent* l) { l->OnCompositeInSignalStatus(compPort, compStatus); });
}

void IarmCompositeInput::OnCompositeInStatusHandler(IARM_EventId_t, void* data, size_t) {
    auto* eventData = static_cast<IARM_Bus_DSMgr_EventData_t*>(data);
    CompositeInPort compPort = static_cast<CompositeInPort>(
        static_cast<dsCompositeInPort_t>(eventData->data.composite_in_status.port)
    );
    bool isPresented = eventData->data.composite_in_status.isPresented;

    Dispatch([&](IEvent* l) { l->OnCompositeInStatus(compPort, isPresented); });
}

void IarmCompositeInput::OnCompositeInVideoModeUpdateHandler(IARM_EventId_t, void* data, size_t) {
    auto* eventData = static_cast<IARM_Bus_DSMgr_EventData_t*>(data);
    CompositeInPort compPort = static_cast<CompositeInPort>(
        static_cast<dsCompositeInPort_t>(eventData->data.composite_in_video_mode.port)
    );

    DisplayVideoPortResolution compResolution {};
    compResolution.pixelResolution = static_cast<DisplayTVResolution>(
        eventData->data.composite_in_video_mode.resolution.pixelResolution
    );
    compResolution.interlaced = eventData->data.composite_in_video_mode.resolution.interlaced;
    compResolution.frameRate = static_cast<DisplayInVideoFrameRate>(
        eventData->data.composite_in_video_mode.resolution.frameRate
    );

    Dispatch([&](IEvent* l) { l->OnCompositeInVideoModeUpdate(compPort, compResolution); });
}
