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

#ifndef __DS_CONNECTION_MANAGER_H__
#define __DS_CONNECTION_MANAGER_H__

#ifdef USE_WPE_THUNDER_PLUGIN

#ifndef MODULE_NAME
#define MODULE_NAME DeviceSettings_ConnectionManager
#endif

#include <core/core.h>
#include <com/com.h>
#include <plugins/Types.h>
#include <interfaces/IDeviceSettingsFPD.h>
#include <interfaces/IDeviceSettingsHDMIIn.h>

namespace DeviceSettingsClient {

/**
 * @brief Centralized Connection Manager for DeviceSettings plugin
 * 
 * This class manages a single Thunder COM-RPC connection to the DeviceSettings plugin
 * and provides access to multiple component interfaces (FPD, HDMIIn, etc.) through
 * QueryInterface pattern.
 */
class ConnectionManager : public WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IDeviceSettingsFPD> {
private:
    using BaseClass = WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IDeviceSettingsFPD>;
    
    // Component interfaces obtained via QueryInterface
    WPEFramework::Exchange::IDeviceSettingsFPD* _fpdInterface;
    WPEFramework::Exchange::IDeviceSettingsHDMIIn* _hdmiInInterface;
    // Add more component interfaces here as needed
    // WPEFramework::Exchange::IDeviceSettingsCompositeIn* _compositeInInterface;
    // WPEFramework::Exchange::IDeviceSettingsHost* _hostInterface;
    
    static ConnectionManager* _instance;
    static WPEFramework::Core::CriticalSection _dsConnectionManagerlock;
    
    bool _connected;
    bool _shutdown;

    // Private constructor for singleton
    ConnectionManager();
    
    // Private destructor
    ~ConnectionManager();

    // Operational callback from Thunder
    virtual void Operational(const bool upAndRunning) override;

    inline bool isConnected() const { return _connected; }
    inline bool IsActivatedLocked() const { return (nullptr != _fpdInterface); }

public:
    // Delete copy/move constructors and assignment operators
    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;
    ConnectionManager(ConnectionManager&&) = delete;
    ConnectionManager& operator=(ConnectionManager&&) = delete;

    /**
     * @brief Initialize the connection manager singleton
     */
    static void Init();

    /**
     * @brief Terminate the connection manager singleton
     */
    static void Term();

    /**
     * @brief Get the singleton instance
     */
    static ConnectionManager* Instance();

    /**
     * @brief Check if plugin is operational (connected and activated)
     */
    bool IsOperational() const;

    /**
     * @brief Wait for plugin to become operational with timeout
     */
    bool WaitForOperational(uint32_t timeoutMs = 5000) const;

    /**
     * @brief Connect to Thunder DeviceSettings plugin
     */
    uint32_t Connect();

    /**
     * @brief Disconnect from Thunder DeviceSettings plugin
     */
    uint32_t Disconnect();

    /**
     * @brief Get FPD interface pointer
     * @return FPD interface or nullptr if not available
     */
    WPEFramework::Exchange::IDeviceSettingsFPD* GetFPDInterface();

    /**
     * @brief Get HDMIIn interface pointer
     * @return HDMIIn interface or nullptr if not available
     */
    WPEFramework::Exchange::IDeviceSettingsHDMIIn* GetHDMIInInterface();

    /**
     * @brief Lock the API mutex for thread-safe operations
     */
    static void Lock() { _dsConnectionManagerlock.Lock(); }

    /**
     * @brief Unlock the API mutex
     */
    static void Unlock() { _dsConnectionManagerlock.Unlock(); }
};

} // namespace DeviceSettingsClient

#endif // USE_WPE_THUNDER_PLUGIN

#endif // __DS_CONNECTION_MANAGER_H__
