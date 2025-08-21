/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 2025 Management
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
#pragma once

#include <list>
#include <mutex>

#include "videoDevice.hpp"

#include "libIBus.h"


namespace iarm {

class VideoDevicePriv {
        using IEvent = device::VideoDevice::IEvent;

public:
        VideoDevicePriv();
        uint32_t Register(IEvent *listener);
        uint32_t UnRegister(IEvent *listener);

private:
    static void OnDisplayFrameratePreChangeHandler(const std::string& frameRate);
    static void OnDisplayFrameratePostChangeHandler(const std::string& frameRate);


    template<typename F>
    static void Dispatch(F&& fn);

    struct EventHandlerMapping {
        IARM_EventId_t eventId;
        IARM_EventHandler_t handler;
    };

    static EventHandlerMapping eventHandlers[];

    // util methods
    const char* to_string(IARM_EventId_t eventId);

private:
    static std::mutex s_mutex;
    static std::list<IEvent*> s_listeners;
};

} // namespace iarm
