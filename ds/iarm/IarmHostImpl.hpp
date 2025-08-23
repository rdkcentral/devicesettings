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

#include <algorithm>
#include <list>
#include <mutex>

#include "dslogger.h"
#include "host.hpp"

namespace device {

// Forward declaration for IARM Implementation Groups
class IARMGroupVideoDevice;
class IARMGroupVideoPort;
class IARMGroupAudioPort;

class IarmHostImpl {

    // Manages a list of listeners and corresponding IARM Event Group operations.
    template <typename T, typename IARMGroup>
    class CallbackList : public std::list<T> {
    public:
        CallbackList() = default;

        ~CallbackList()
        {
            Release();
        }

        // disable copy and move semantics
        CallbackList(const CallbackList&)            = delete;
        CallbackList& operator=(const CallbackList&) = delete;
        CallbackList(CallbackList&&)                 = delete;
        CallbackList& operator=(CallbackList&&)      = delete;

        // @brief Register a listener, also register IARM events if not already registered
        // if the listener is already registered, listener will not be added again
        // if IARM event registration fails, listener will not be added
        uint32_t Register(T listener)
        {
            if (listener == nullptr) {
                INT_ERROR("%s listener is null", typeid(T).name());
                return 1; // Error: Listener is null
            }

            if (!m_registered) {
                m_registered = IARMGroup::RegisterIarmEvents();

                if (!m_registered) {
                    INT_ERROR("Failed to register IARMGroup %s", typeid(IARMGroup).name());
                    return 1; // Error: Failed to register IARM group
                }
            }

            auto it = std::find(this->begin(), this->end(), listener);
            if (it != this->end()) {
                // Listener already registered
                INT_ERROR("%s %p is already registered", typeid(T).name(), listener);
                return 0;
            }

            this->push_back(listener);

            INT_INFO("%s %p registered", typeid(T).name(), listener);

            return 0;
        }

        // @brief UnRegister a listener, also unregister IARM events if no listeners are left
        // if the listener is not registered, it will not be removed
        uint32_t UnRegister(T listener)
        {
            if (listener == nullptr) {
                INT_ERROR("%s listener is null", typeid(T).name());
                return 1; // Error: Listener is null
            }

            auto it = std::find(this->begin(), this->end(), listener);
            if (it == this->end()) {
                // Listener not found
                INT_ERROR("%s %p is not registered", typeid(T).name(), listener);
                return 1; // Error: Listener not found
            }

            this->erase(it);

            INT_INFO("%s %p unregistered", typeid(T).name(), listener);

            if (this->empty() && m_registered) {
                m_registered = !IARMGroup::UnRegisterIarmEvents();
            }

            return 0;
        }

        // @brief Release all listeners and unregister IARM events
        // This will clear the list and unregister IARM events if no listeners are left
        uint32_t Release()
        {
            if (m_registered) {
                m_registered = !IARMGroup::UnRegisterIarmEvents();
            }

            this->clear();
            INT_INFO("CallbackList[T=%s] released, status: %d", typeid(T).name(), m_registered);
            return 0; // Success
        }

    private:
        bool m_registered = false; // really required?
    };

public:
    using IVideoDeviceEvents = device::Host::IVideoDeviceEvents;
    using IVideoPortEvents   = device::Host::IVideoPortEvents;
    using IAudioPortEvents   = device::Host::IAudioPortEvents;

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

    // @brief Register a listener for audio port events
    // @param listener: class object implementing the listener
    uint32_t Register(IAudioPortEvents* listener);

    // @brief UnRegister a listener for audio port events
    // @param listener: class object implementing the listener
    uint32_t UnRegister(IAudioPortEvents* listener);

private:
    static std::mutex s_mutex;

    static CallbackList<IVideoDeviceEvents*, IARMGroupVideoDevice> s_videoDeviceListeners;
    static CallbackList<IVideoPortEvents*, IARMGroupVideoPort> s_videoPortListeners;
    static CallbackList<IAudioPortEvents*, IARMGroupAudioPort> s_audioPortListeners;

    template <typename T, typename F>
    static void Dispatch(const std::list<T*>& listeners, F&& fn);

    static void Dispatch(std::function<void(IVideoDeviceEvents* listener)>&& fn);
    static void Dispatch(std::function<void(IVideoPortEvents* listener)>&& fn);
    static void Dispatch(std::function<void(IAudioPortEvents* listener)>&& fn);

    // Dispatch is private, so all IARMGroup implementations will need to be friends
    friend class IARMGroupVideoDevice;
    friend class IARMGroupVideoPort;
    friend class IARMGroupAudioPort;
};
} // namespace device
