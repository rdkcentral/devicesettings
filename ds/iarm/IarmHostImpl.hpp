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
#include <functional>
#include <list>
#include <mutex>

#include "dsError.h"
#include "dslogger.h"
#include "host.hpp"

namespace device {

// Forward declaration for IARM Implementation Groups
class IARMGroupHdmiIn;
class IARMGroupVideoDevice;
class IARMGroupVideoOutputPort;
class IARMGroupAudioOutputPort;
class IARMGroupDisplayDevice;
class IARMGroupComposite;
class IARMGroupDisplay;

using IHdmiInEvents          = Host::IHdmiInEvents;
using IVideoDeviceEvents     = Host::IVideoDeviceEvents;
using IVideoOutputPortEvents = Host::IVideoOutputPortEvents;
using IAudioOutputPortEvents = Host::IAudioOutputPortEvents;
using IDisplayDeviceEvents   = Host::IDisplayDeviceEvents;
using ICompositeInEvents     = device::Host::ICompositeInEvents;
using IDisplayEvents         = device::Host::IDisplayEvents;

class IarmHostImpl {

    // Manages a list of listeners and corresponding IARM Event Group operations.
    // Private internal class, not to be used directly by clients.
    template <typename T, typename IARMGroup>
    class CallbackList : public std::list<std::pair<T, std::string>> {
    public:
        CallbackList()
            : m_registered(false)
        {
        }

        ~CallbackList()
        {
            // As best practise caller is supposed to call Release() explicitly
            // this is just for safety (see IarmHostImpl destructor)
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
        // clientName is for logging purposes only, if not available, pass empty string
        dsError_t Register(T listener, const std::string& clientName)
        {
            if (nullptr == listener) {
                INT_ERROR("%s listener is null", typeid(T).name());
                return dsERR_INVALID_PARAM; // Error: Listener is null
            }

            std::lock_guard<std::mutex> lock(m_mutex);

            if (!m_registered) {
                m_registered = IARMGroup::RegisterIarmEvents();

                if (!m_registered) {
                    INT_ERROR("Failed to register IARMGroup %s", typeid(IARMGroup).name());
                    return dsERR_OPERATION_FAILED; // Error: Failed to register IARM group
                }
            }

            auto it = std::find_if(this->begin(), this->end(), [listener](const std::pair<T, std::string>& pair) {
                return pair.first == listener;
            });

            if (it != this->end()) {
                // Listener already registered
                INT_ERROR("%s %p is already registered", typeid(T).name(), listener);
                return dsERR_NONE; // Success: Listener already registered
            }

            this->emplace_back(listener, clientName);

            INT_INFO("%s %s %p registered", clientName.c_str(), typeid(T).name(), listener);

            return dsERR_NONE;
        }

        // @brief UnRegister a listener, also unregister IARM events if no listeners are left
        // if the listener is not registered, it will not be removed
        dsError_t UnRegister(T listener)
        {
            if (nullptr == listener) {
                INT_ERROR("%s listener is null", typeid(T).name());
                return dsERR_INVALID_PARAM; // Error: Listener is null
            }

            std::lock_guard<std::mutex> lock(m_mutex);

            // pair.first is the listener, pair.second is the clientName
            auto it = std::find_if(this->begin(), this->end(), [listener](const std::pair<T, std::string>& pair) {
                return pair.first == listener;
            });

            if (it == this->end()) {
                // Listener not found
                INT_ERROR("%s %p is not registered", typeid(T).name(), listener);
                return dsERR_RESOURCE_NOT_AVAILABLE; // Error: Listener not found
            }

            const std::string clientName = it->second;

            this->erase(it);

            INT_INFO("%s %s %p unregistered", clientName.c_str(), typeid(T).name(), listener);

            if (this->empty() && m_registered) {
                m_registered = !IARMGroup::UnRegisterIarmEvents();
            }

            return dsERR_NONE; // Success
        }

        // @brief Release all listeners and unregister IARM events
        // This will clear the list and unregister IARM events if no listeners are left
        dsError_t Release()
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_registered) {
                m_registered = !IARMGroup::UnRegisterIarmEvents();
            }

            this->clear();
            INT_INFO("CallbackList[T=%s] released, status: %d", typeid(T).name(), m_registered);
            return dsERR_NONE; // Success
        }

        std::mutex& Mutex()
        {
            return m_mutex;
        }

    private:
        std::mutex m_mutex; // To protect access to the list and callback notifications
        bool m_registered = false; // To track if IARM events are registered
    };

public:
    IarmHostImpl() = default;
    ~IarmHostImpl();

    // @brief Register a listener for HDMI device events
    // @param listener: class object implementing the listener
    // @param clientName: name of the client registering the listener, for logging purposes only
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t Register(IHdmiInEvents* listener, const std::string& clientName);

    // @brief UnRegister a listener for HDMI device events
    // @param listener: class object implementing the listener
    dsError_t UnRegister(IHdmiInEvents* listener);

    // @brief Register a listener for video device events
    // @param listener: class object implementing the listener
    // @param clientName: name of the client registering the listener, for logging purposes only
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t Register(IVideoDeviceEvents* listener, const std::string& clientName);

    // @brief UnRegister a listener for video device events
    // @param listener: class object implementing the listener
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t UnRegister(IVideoDeviceEvents* listener);

    // @brief Register a listener for video port events
    // @param listener: class object implementing the listener
    // @param clientName: name of the client registering the listener, for logging purposes only
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t Register(IVideoOutputPortEvents* listener, const std::string& clientName);

    // @brief UnRegister a listener for video port events
    // @param listener: class object implementing the listener
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t UnRegister(IVideoOutputPortEvents* listener);

    // @brief Register a listener for audio port events
    // @param listener: class object implementing the listener
    // @param clientName: name of the client registering the listener, for logging purposes only
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t Register(IAudioOutputPortEvents* listener, const std::string& clientName);

    // @brief UnRegister a listener for audio port events
    // @param listener: class object implementing the listener
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t UnRegister(IAudioOutputPortEvents* listener);

    // @brief Register a listener for Composite events
    // @param listener: class object implementing the listener
    // @param clientName: name of the client registering the listener, for logging purposes only
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t Register(ICompositeInEvents* listener, const std::string& clientName);

    // @brief UnRegister a listener for Composite events
    // @param listener: class object implementing the listener
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t UnRegister(ICompositeInEvents* listener);

    // @brief Register a listener for Composite events
    // @param listener: class object implementing the listener
    // @param clientName: name of the client registering the listener, for logging purposes only
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t Register(IDisplayEvents* listener, const std::string& clientName);

    // @brief UnRegister a listener for Composite events
    // @param listener: class object implementing the listener
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t UnRegister(IDisplayEvents* listener);

    // @brief Register a listener for display device events
    // @param listener: class object implementing the listener
    // @param clientName: name of the client registering the listener, for logging purposes only
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t Register(IDisplayDeviceEvents* listener, const std::string& clientName);

    // @brief UnRegister a listener for display device events
    // @param listener: class object implementing the listener
    // @return dsERR_NONE on success, appropriate dsError_t on failure
    dsError_t UnRegister(IDisplayDeviceEvents* listener);

private:
    static CallbackList<IHDMIInEvents*, IARMGroupHdmiIn> s_hdmiInListeners;
    static CallbackList<IVideoDeviceEvents*, IARMGroupVideoDevice> s_videoDeviceListeners;
    static CallbackList<IVideoOutputPortEvents*, IARMGroupVideoOutputPort> s_videoOutputPortListeners;
    static CallbackList<IAudioOutputPortEvents*, IARMGroupAudioOutputPort> s_audioOutputPortListeners;
    static CallbackList<ICompositeInEvents*, IARMGroupComposite> s_compositeListeners;
    static CallbackList<IDisplayEvents*, IARMGroupDisplay> s_displayListeners;
    static CallbackList<IDisplayDeviceEvents*, IARMGroupDisplayDevice> s_displayDeviceListeners;

    template <typename T, typename F>
    static void Dispatch(const std::list<std::pair<T*, std::string>>& listeners, F&& fn);

    static void Dispatch(std::function<void(IHdmiInEvents* listener)>&& fn);
    static void Dispatch(std::function<void(IVideoDeviceEvents* listener)>&& fn);
    static void Dispatch(std::function<void(IVideoOutputPortEvents* listener)>&& fn);
    static void Dispatch(std::function<void(IAudioOutputPortEvents* listener)>&& fn);
    static void Dispatch(std::function<void(ICompositeInEvents* listener)>&& fn);
    static void Dispatch(std::function<void(IDisplayEvents* listener)>&& fn);
    static void Dispatch(std::function<void(IDisplayDeviceEvents* listener)>&& fn);

    // Dispatch is private, so all IARMGroup implementations will need to be friends
    friend class IARMGroupHdmiIn;
    friend class IARMGroupVideoDevice;
    friend class IARMGroupVideoOutputPort;
    friend class IARMGroupAudioOutputPort;
    friend class IARMGroupComposite;
    friend class IARMGroupDisplay;
    friend class IARMGroupDisplayDevice;
};
} // namespace device
