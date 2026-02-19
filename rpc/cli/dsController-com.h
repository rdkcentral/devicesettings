/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
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
* @file DeviceSettingsController.h
* @brief Central controller for all DeviceSettings COM-RPC connections
* 
* This controller manages a single RPC connection to the Thunder DeviceSettings plugin
* and provides access to all component interfaces (FPD, HDMIIn, Audio, etc.) through
* QueryInterface on the main IDeviceSettings interface.
*/

#pragma once

#ifdef USE_THUNDER_PLUGIN

#ifndef MODULE_NAME
#define MODULE_NAME DeviceSettings_Client
#endif

#include <core/core.h>
#include <com/com.h>
#include <plugins/Types.h>
#include <interfaces/IDeviceSettings.h>
#include <interfaces/IDeviceSettingsFPD.h>
#include <interfaces/IDeviceSettingsHDMIIn.h>
#include <interfaces/IDeviceSettingsAudio.h>
#include <interfaces/IDeviceSettingsDisplay.h>
#include <interfaces/IDeviceSettingsVideoPort.h>
#include <interfaces/IDeviceSettingsVideoDevice.h>
#include <interfaces/IDeviceSettingsHost.h>
#include <interfaces/IDeviceSettingsCompositeIn.h>

namespace WPEFramework {

/**
 * @brief Component types supported by DeviceSettings
 */
enum class DeviceSettingsComponent : uint8_t {
    FPD,
    HDMIIn,
    Audio,
    Display,
    VideoPort,
    VideoDevice,
    Host,
    CompositeIn
};

/**
 * @class DeviceSettingsController
 * @brief Singleton controller managing all DeviceSettings component interfaces
 * 
 * This class provides:
 * - Single RPC connection to Thunder DeviceSettings plugin
 * - Thread-safe access to all component interfaces
 * - Automatic lifecycle management
 * - Connection status monitoring
 * - Reconnection support
 */
class DeviceSettingsController : public RPC::SmartInterfaceType<Exchange::IDeviceSettings> {
private:
    using BaseClass = RPC::SmartInterfaceType<Exchange::IDeviceSettings>;
    
    // Main interface
    Exchange::IDeviceSettings* _deviceSettings;
    
    // Component interfaces obtained via QueryInterface
    Exchange::IDeviceSettingsFPD* _fpdInterface;
    Exchange::IDeviceSettingsHDMIIn* _hdmiInInterface;
    Exchange::IDeviceSettingsAudio* _audioInterface;
    Exchange::IDeviceSettingsDisplay* _displayInterface;
    Exchange::IDeviceSettingsVideoPort* _videoPortInterface;
    Exchange::IDeviceSettingsVideoDevice* _videoDeviceInterface;
    Exchange::IDeviceSettingsHost* _hostInterface;
    Exchange::IDeviceSettingsCompositeIn* _compositeInInterface;
    
    // Singleton instance
    static DeviceSettingsController* _instance;
    static Core::CriticalSection _lock;
    
    // Connection state
    bool _connected;
    bool _shutdown;
    
    // Thunder callsign
    static constexpr const TCHAR _callSign[] = _T("org.rdk.DeviceSettings");

    // Connection retry thread management
    static pthread_t _connectionThreadID;
    static std::atomic<bool> _isConnecting;
    static uint32_t _connectionRetryCount;
    
    #define DS_CONTROLLER_CONNECT_WAIT_TIME_MS 300000  // 300ms in microseconds
    
    // Helper methods
    static void EstablishThunderConnection();
    static void* RetryThunderConnectionThread(void* arg);
    static void FetchAndInitializeInterfaces();
    
    /**
     * @brief Private constructor for singleton pattern
     */
    DeviceSettingsController();
    
    /**
     * @brief Private destructor
     */
    ~DeviceSettingsController();
    
    /**
     * @brief Operational callback from SmartInterfaceType
     * @param upAndRunning true if plugin is operational, false otherwise
     */
    virtual void Operational(const bool upAndRunning) override;
    
    /**
     * @brief Check if connected (without lock)
     */
    inline bool isConnected() const { return _connected; }
    
    /**
     * @brief Query and cache a component interface
     * @tparam T Component interface type
     * @param interfacePtr Reference to cached interface pointer
     * @param componentName Name for logging
     * @return true if interface obtained successfully
     */
    template<typename T>
    bool QueryComponentInterface(T*& interfacePtr, const char* componentName);

public:
    // Delete copy constructor and assignment operator
    DeviceSettingsController(const DeviceSettingsController&) = delete;
    DeviceSettingsController& operator=(const DeviceSettingsController&) = delete;
    
    /**
     * @brief Get singleton instance
     * @return Pointer to singleton instance
     */
    static DeviceSettingsController* GetInstance();
    
    /**
     * @brief Initialize the controller
     * @return Core::ERROR_NONE on success, error code otherwise
     */
    static uint32_t Initialize();
    
    /**
     * @brief Terminate the controller and release all resources
     */
    static void Terminate();
    
    /**
     * @brief Connect to Thunder DeviceSettings plugin
     * @return Core::ERROR_NONE on success, error code otherwise
     */
    uint32_t Connect();
    
    /**
     * @brief Disconnect from Thunder DeviceSettings plugin
     * @return Core::ERROR_NONE on success, error code otherwise
     */
    uint32_t Disconnect();
    
    /**
     * @brief Check if the main interface is operational
     * @return true if operational, false otherwise
     */
    bool IsOperational() const;
    
    /**
     * @brief Check if a specific component interface is available
     * @param component Component type to check
     * @return true if available, false otherwise
     */
    bool IsComponentAvailable(DeviceSettingsComponent component) const;
    
    /**
     * @brief Wait for the main interface to become operational
     * @param timeoutMs Timeout in milliseconds
     * @return true if operational within timeout, false otherwise
     */
    bool WaitForOperational(uint32_t timeoutMs = 5000) const;
    
    /**
     * @brief Wait for a specific component to become available
     * @param component Component type to wait for
     * @param timeoutMs Timeout in milliseconds
     * @return true if available within timeout, false otherwise
     */
    bool WaitForComponent(DeviceSettingsComponent component, uint32_t timeoutMs = 5000) const;
    
    /**
     * @brief Get FPD component interface
     * @return Pointer to IDeviceSettingsFPD interface or nullptr
     */
    Exchange::IDeviceSettingsFPD* GetFPDInterface();
    
    /**
     * @brief Get HDMIIn component interface
     * @return Pointer to IDeviceSettingsHDMIIn interface or nullptr
     */
    Exchange::IDeviceSettingsHDMIIn* GetHDMIInInterface();
    
    /**
     * @brief Get Audio component interface
     * @return Pointer to IDeviceSettingsAudio interface or nullptr
     */
    Exchange::IDeviceSettingsAudio* GetAudioInterface();
    
    /**
     * @brief Get Display component interface
     * @return Pointer to IDeviceSettingsDisplay interface or nullptr
     */
    Exchange::IDeviceSettingsDisplay* GetDisplayInterface();
    
    /**
     * @brief Get VideoPort component interface
     * @return Pointer to IDeviceSettingsVideoPort interface or nullptr
     */
    Exchange::IDeviceSettingsVideoPort* GetVideoPortInterface();
    
    /**
     * @brief Get VideoDevice component interface
     * @return Pointer to IDeviceSettingsVideoDevice interface or nullptr
     */
    Exchange::IDeviceSettingsVideoDevice* GetVideoDeviceInterface();
    
    /**
     * @brief Get Host component interface
     * @return Pointer to IDeviceSettingsHost interface or nullptr
     */
    Exchange::IDeviceSettingsHost* GetHostInterface();
    
    /**
     * @brief Get CompositeIn component interface
     * @return Pointer to IDeviceSettingsCompositeIn interface or nullptr
     */
    Exchange::IDeviceSettingsCompositeIn* GetCompositeInInterface();
};

} // namespace WPEFramework

#endif // USE_THUNDER_PLUGIN
