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
#pragma once

#include <list>
#include <mutex>

#include "host.hpp"

namespace device {
class IarmHostImpl {

public:
    using IVideoDeviceEvents = device::Host::IVideoDeviceEvents;
    using IVideoPortEvents = device::Host::IVideoPortEvents;

    IarmHostImpl();
    ~IarmHostImpl();

    // @brief Register a listener for video device events
    // @param listener: class object implementing the listener
    uint32_t Register(IVideoDeviceEvents* listener);

    // @brief UnRegister a listener for video device events
    // @param listener: class object implementing the listener
    uint32_t UnRegister(IVideoDeviceEvents* listener);

    // @brief Register a listener for video port events
    // @param listener: class object implementing the listener
    uint32_t Register(IVideoPortEvents* listener);

    // @brief UnRegister a listener for video port events
    // @param listener: class object implementing the listener
    uint32_t UnRegister(IVideoPortEvents* listener);

private:
    static std::mutex s_mutex;
    static std::list<IVideoDeviceEvents*> s_videoDeviceListeners;
    static std::list<IVideoPortEvents*> s_videoPortListeners;

    template <typename F>
    static void DispatchVideoDeviceEvents(F&& fn);

    template <typename F>
    static void DispatchVideoPortEvents(F&& fn);

    template <typename T>
    uint32_t Register(std::list<T*>& listeners, T* listener);

    friend class IarmHostPriv;
};
} // namespace device
