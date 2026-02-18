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
* @file dsController-com.cpp
* @brief Implementation of central controller for DeviceSettings COM-RPC connections
*/

#ifndef USE_IARM

#include "DeviceSettingsController.h"
#include <stdio.h>
#include <chrono>
#include <thread>

namespace WPEFramework {

// Static member initialization
DeviceSettingsController* DeviceSettingsController::_instance = nullptr;
Core::CriticalSection DeviceSettingsController::_lock;

// Define the static constexpr member
constexpr const TCHAR DeviceSettingsController::_callSign[];

pthread_t DeviceSettingsController::_connectionThreadID = 0;
std::atomic<bool> DeviceSettingsController::_isConnecting(false);
uint32_t DeviceSettingsController::_connectionRetryCount = 0;

DeviceSettingsController::DeviceSettingsController()
    : BaseClass()
    , _deviceSettings(nullptr)
    , _fpdInterface(nullptr)
    , _hdmiInInterface(nullptr)
    , _audioInterface(nullptr)
    , _displayInterface(nullptr)
    , _videoPortInterface(nullptr)
    , _videoDeviceInterface(nullptr)
    , _hostInterface(nullptr)
    , _compositeInInterface(nullptr)
    , _connected(false)
    , _shutdown(false)
{
    printf("[DeviceSettingsController] Constructor called\n");
}

DeviceSettingsController::~DeviceSettingsController()
{
    printf("[DeviceSettingsController] Destructor called\n");
    _shutdown = true;
    
    // Release all component interfaces
    _lock.Lock();
    
    if (_compositeInInterface) {
        _compositeInInterface->Release();
        _compositeInInterface = nullptr;
    }
    if (_hostInterface) {
        _hostInterface->Release();
        _hostInterface = nullptr;
    }
    if (_videoDeviceInterface) {
        _videoDeviceInterface->Release();
        _videoDeviceInterface = nullptr;
    }
    if (_videoPortInterface) {
        _videoPortInterface->Release();
        _videoPortInterface = nullptr;
    }
    if (_displayInterface) {
        _displayInterface->Release();
        _displayInterface = nullptr;
    }
    if (_audioInterface) {
        _audioInterface->Release();
        _audioInterface = nullptr;
    }
    if (_hdmiInInterface) {
        _hdmiInInterface->Release();
        _hdmiInInterface = nullptr;
    }
    if (_fpdInterface) {
        _fpdInterface->Release();
        _fpdInterface = nullptr;
    }
    if (_deviceSettings) {
        _deviceSettings->Release();
        _deviceSettings = nullptr;
    }
    
    _lock.Unlock();
    
    BaseClass::Close(Core::infinite);
}

void DeviceSettingsController::Operational(const bool upAndRunning)
{
    _lock.Lock();
    
    if (upAndRunning) {
        printf("[DeviceSettingsController] Plugin is operational\n");
        
        // Get main IDeviceSettings interface
        if (nullptr == _deviceSettings) {
            _deviceSettings = BaseClass::Interface();
            if (_deviceSettings != nullptr) {
                printf("[DeviceSettingsController] Successfully obtained IDeviceSettings interface\n");
            } else {
                fprintf(stderr, "[DeviceSettingsController] Failed to get IDeviceSettings interface\n");
            }
        }
    } else {
        printf("[DeviceSettingsController] Plugin is not operational - releasing all interfaces\n");
        
        // Release all component interfaces
        if (_compositeInInterface) {
            _compositeInInterface->Release();
            _compositeInInterface = nullptr;
        }
        if (_hostInterface) {
            _hostInterface->Release();
            _hostInterface = nullptr;
        }
        if (_videoDeviceInterface) {
            _videoDeviceInterface->Release();
            _videoDeviceInterface = nullptr;
        }
        if (_videoPortInterface) {
            _videoPortInterface->Release();
            _videoPortInterface = nullptr;
        }
        if (_displayInterface) {
            _displayInterface->Release();
            _displayInterface = nullptr;
        }
        if (_audioInterface) {
            _audioInterface->Release();
            _audioInterface = nullptr;
        }
        if (_hdmiInInterface) {
            _hdmiInInterface->Release();
            _hdmiInInterface = nullptr;
        }
        if (_fpdInterface) {
            _fpdInterface->Release();
            _fpdInterface = nullptr;
        }
        if (_deviceSettings) {
            _deviceSettings->Release();
            _deviceSettings = nullptr;
        }
    }
    
    _lock.Unlock();
}

template<typename T>
bool DeviceSettingsController::QueryComponentInterface(T*& interfacePtr, const char* componentName)
{
    if (interfacePtr != nullptr) {
        // Already cached
        return true;
    }
    
    if (_deviceSettings == nullptr) {
        fprintf(stderr, "[DeviceSettingsController] Cannot query %s - main interface not available\n", componentName);
        return false;
    }
    
    interfacePtr = _deviceSettings->QueryInterface<T>();
    if (interfacePtr != nullptr) {
        printf("[DeviceSettingsController] Successfully obtained %s interface\n", componentName);
        return true;
    } else {
        fprintf(stderr, "[DeviceSettingsController] Failed to get %s interface\n", componentName);
        return false;
    }
}

DeviceSettingsController* DeviceSettingsController::GetInstance()
{
    return _instance;
}

uint32_t DeviceSettingsController::Initialize()
{
    printf("[DeviceSettingsController] Initialize entry\n");
    _lock.Lock();
    
    if (_instance != nullptr) {
        printf("[DeviceSettingsController] Already initialized\n");
        _lock.Unlock();
        return Core::ERROR_NONE;
    }
    
    // Create the controller instance (NOT connected yet)
    _instance = new DeviceSettingsController();
    
    if (_instance == nullptr) {
        fprintf(stderr, "[DeviceSettingsController] Failed to create instance\n");
        _lock.Unlock();
        return Core::ERROR_GENERAL;
    }
    
    _lock.Unlock();
    
    // Start Thunder connection retry thread (non-blocking, like dsMgrPwrCtrlEstablishConnection)
    printf("[DeviceSettingsController] Starting Thunder COM-RPC connection thread\n");
    EstablishThunderConnection();
    
    return Core::ERROR_NONE;
}

void DeviceSettingsController::Terminate()
{
    printf("[DeviceSettingsController] Terminate entry\n");
    
    _lock.Lock();
    
    if (_instance != nullptr) {
        delete _instance;
        _instance = nullptr;
        printf("[DeviceSettingsController] Instance terminated\n");
    }
    
    _lock.Unlock();
    
    // Note: Connection retry thread is detached and will exit on its own
    // when it successfully connects or if the process is shutting down
}

void DeviceSettingsController::EstablishThunderConnection()
{
    printf("[DeviceSettingsController] Entering EstablishThunderConnection\n");
    
    if (_isConnecting.load()) {
        printf("[DeviceSettingsController] Connection already in progress\n");
        return;
    }
    
    _isConnecting = true;
    _connectionRetryCount = 0;
    
    // Create detached thread for retry logic (matches dsMgrPwrCtrlEstablishConnection pattern)
    if (pthread_create(&_connectionThreadID, NULL, RetryThunderConnectionThread, NULL) == 0) {
        if (pthread_detach(_connectionThreadID) != 0) {
            fprintf(stderr, "[DeviceSettingsController] Thread detach failed\n");
            _isConnecting = false;
        }
        else {
            printf("[DeviceSettingsController] Connection retry thread started successfully\n");
        }
    }
    else {
        fprintf(stderr, "[DeviceSettingsController] Thread creation failed\n");
        _isConnecting = false;
    }
}

void* DeviceSettingsController::RetryThunderConnectionThread(void* arg)
{
    printf("[DeviceSettingsController] RetryThunderConnectionThread entry\n");
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (controller == nullptr) {
        fprintf(stderr, "[DeviceSettingsController] Controller instance is null\n");
        _isConnecting = false;
        return arg;
    }
    
    // Infinite retry loop (matches dsMgrPwrRetryEstablishConnThread pattern)
    while (1)
    {
        _connectionRetryCount++;
        
        uint32_t result = controller->Connect();
        
        if (result == Core::ERROR_NONE) {
            printf("[DeviceSettingsController] Thunder connection SUCCESS after %u attempts\n", 
                   _connectionRetryCount);
            
            // Post-connection initialization
            FetchAndInitializeInterfaces();
            
            _isConnecting = false;
            printf("[DeviceSettingsController] RetryThunderConnectionThread completed successfully\n");
            break;
        }
        else {
            if (_connectionRetryCount % 10 == 0) {  // Log every 3 seconds
                printf("[DeviceSettingsController] Connection attempt %u failed, retrying...\n", 
                       _connectionRetryCount);
            }
            
            // 300ms wait before retry (matches DSMGR_PWR_CNTRL_CONNECT_WAIT_TIME_MS)
            usleep(DS_CONTROLLER_CONNECT_WAIT_TIME_MS);
        }
    }
    
    printf("[DeviceSettingsController] RetryThunderConnectionThread exit\n");
    return arg;
}

void DeviceSettingsController::FetchAndInitializeInterfaces()
{
    printf("[DeviceSettingsController] FetchAndInitializeInterfaces entry\n");
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (controller == nullptr || !controller->IsOperational()) {
        fprintf(stderr, "[DeviceSettingsController] Controller not operational\n");
        return;
    }
    
    // Pre-query all component interfaces to verify availability
    uint32_t loadedCount = 0;
    
    // Query each interface (lazy loading will cache them)
    if (controller->GetFPDInterface() != nullptr) {
        printf("[DeviceSettingsController] FPD interface available\n");
        loadedCount++;
    }
    
    if (controller->GetAudioInterface() != nullptr) {
        printf("[DeviceSettingsController] Audio interface available\n");
        loadedCount++;
    }
    
    if (controller->GetDisplayInterface() != nullptr) {
        printf("[DeviceSettingsController] Display interface available\n");
        loadedCount++;
    }
    
    if (controller->GetVideoDeviceInterface() != nullptr) {
        printf("[DeviceSettingsController] VideoDevice interface available\n");
        loadedCount++;
    }
    
    if (controller->GetVideoPortInterface() != nullptr) {
        printf("[DeviceSettingsController] VideoPort interface available\n");
        loadedCount++;
    }
    
    if (controller->GetHostInterface() != nullptr) {
        printf("[DeviceSettingsController] Host interface available\n");
        loadedCount++;
    }
    
    if (controller->GetHDMIInInterface() != nullptr) {
        printf("[DeviceSettingsController] HDMIIn interface available\n");
        loadedCount++;
    }
    
    if (controller->GetCompositeInInterface() != nullptr) {
        printf("[DeviceSettingsController] CompositeIn interface available\n");
        loadedCount++;
    }
    
    printf("[DeviceSettingsController] Interface initialization complete: %u interfaces loaded\n", 
           loadedCount);
}

uint32_t DeviceSettingsController::Connect()
{
    uint32_t status = Core::ERROR_NONE;
    
    _lock.Lock();
    
    if (!isConnected()) {
        printf("[DeviceSettingsController] Attempting to connect to Thunder with callsign: %s\n", _callSign);
        uint32_t res = BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), _callSign);
        if (Core::ERROR_NONE == res) {
            _connected = true;
            printf("[DeviceSettingsController] Successfully opened RPC connection to Thunder\n");
        } else {
            fprintf(stderr, "[DeviceSettingsController] Failed to open RPC connection, error: %u. Is Thunder running?\n", res);
            status = Core::ERROR_UNAVAILABLE;
        }
    }
    
    if (nullptr == _deviceSettings) {
        status = Core::ERROR_NOT_EXIST;
        printf("[DeviceSettingsController] DeviceSettings plugin not yet operational\n");
    }
    
    _lock.Unlock();
    
    return status;
}

uint32_t DeviceSettingsController::Disconnect()
{
    uint32_t status = Core::ERROR_GENERAL;
    bool close = false;
    
    _lock.Lock();
    
    if (isConnected()) {
        close = true;
        _connected = false;
    }
    
    _lock.Unlock();
    
    if (close) {
        status = BaseClass::Close(Core::infinite);
    }
    
    return status;
}

bool DeviceSettingsController::IsOperational() const
{
    _lock.Lock();
    bool result = (isConnected() && (_deviceSettings != nullptr));
    _lock.Unlock();
    return result;
}

bool DeviceSettingsController::IsComponentAvailable(DeviceSettingsComponent component) const
{
    _lock.Lock();
    
    bool available = false;
    switch (component) {
        case DeviceSettingsComponent::FPD:
            available = (_fpdInterface != nullptr);
            break;
        case DeviceSettingsComponent::HDMIIn:
            available = (_hdmiInInterface != nullptr);
            break;
        case DeviceSettingsComponent::Audio:
            available = (_audioInterface != nullptr);
            break;
        case DeviceSettingsComponent::Display:
            available = (_displayInterface != nullptr);
            break;
        case DeviceSettingsComponent::VideoPort:
            available = (_videoPortInterface != nullptr);
            break;
        case DeviceSettingsComponent::VideoDevice:
            available = (_videoDeviceInterface != nullptr);
            break;
        case DeviceSettingsComponent::Host:
            available = (_hostInterface != nullptr);
            break;
        case DeviceSettingsComponent::CompositeIn:
            available = (_compositeInInterface != nullptr);
            break;
    }
    
    _lock.Unlock();
    return available;
}

bool DeviceSettingsController::WaitForOperational(uint32_t timeoutMs) const
{
    const uint32_t pollIntervalMs = 100;
    uint32_t elapsedMs = 0;
    
    while (elapsedMs < timeoutMs) {
        if (IsOperational()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMs));
        elapsedMs += pollIntervalMs;
    }
    
    return false;
}

bool DeviceSettingsController::WaitForComponent(DeviceSettingsComponent component, uint32_t timeoutMs) const
{
    const uint32_t pollIntervalMs = 100;
    uint32_t elapsedMs = 0;
    
    while (elapsedMs < timeoutMs) {
        if (IsComponentAvailable(component)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMs));
        elapsedMs += pollIntervalMs;
    }
    
    return false;
}

Exchange::IDeviceSettingsFPD* DeviceSettingsController::GetFPDInterface()
{
    _lock.Lock();
    QueryComponentInterface<Exchange::IDeviceSettingsFPD>(_fpdInterface, "IDeviceSettingsFPD");
    Exchange::IDeviceSettingsFPD* result = _fpdInterface;
    _lock.Unlock();
    return result;
}

Exchange::IDeviceSettingsHDMIIn* DeviceSettingsController::GetHDMIInInterface()
{
    _lock.Lock();
    QueryComponentInterface<Exchange::IDeviceSettingsHDMIIn>(_hdmiInInterface, "IDeviceSettingsHDMIIn");
    Exchange::IDeviceSettingsHDMIIn* result = _hdmiInInterface;
    _lock.Unlock();
    return result;
}

Exchange::IDeviceSettingsAudio* DeviceSettingsController::GetAudioInterface()
{
    _lock.Lock();
    QueryComponentInterface<Exchange::IDeviceSettingsAudio>(_audioInterface, "IDeviceSettingsAudio");
    Exchange::IDeviceSettingsAudio* result = _audioInterface;
    _lock.Unlock();
    return result;
}

Exchange::IDeviceSettingsDisplay* DeviceSettingsController::GetDisplayInterface()
{
    _lock.Lock();
    QueryComponentInterface<Exchange::IDeviceSettingsDisplay>(_displayInterface, "IDeviceSettingsDisplay");
    Exchange::IDeviceSettingsDisplay* result = _displayInterface;
    _lock.Unlock();
    return result;
}

Exchange::IDeviceSettingsVideoPort* DeviceSettingsController::GetVideoPortInterface()
{
    _lock.Lock();
    QueryComponentInterface<Exchange::IDeviceSettingsVideoPort>(_videoPortInterface, "IDeviceSettingsVideoPort");
    Exchange::IDeviceSettingsVideoPort* result = _videoPortInterface;
    _lock.Unlock();
    return result;
}

Exchange::IDeviceSettingsVideoDevice* DeviceSettingsController::GetVideoDeviceInterface()
{
    _lock.Lock();
    QueryComponentInterface<Exchange::IDeviceSettingsVideoDevice>(_videoDeviceInterface, "IDeviceSettingsVideoDevice");
    Exchange::IDeviceSettingsVideoDevice* result = _videoDeviceInterface;
    _lock.Unlock();
    return result;
}

Exchange::IDeviceSettingsHost* DeviceSettingsController::GetHostInterface()
{
    _lock.Lock();
    QueryComponentInterface<Exchange::IDeviceSettingsHost>(_hostInterface, "IDeviceSettingsHost");
    Exchange::IDeviceSettingsHost* result = _hostInterface;
    _lock.Unlock();
    return result;
}

Exchange::IDeviceSettingsCompositeIn* DeviceSettingsController::GetCompositeInInterface()
{
    _lock.Lock();
    QueryComponentInterface<Exchange::IDeviceSettingsCompositeIn>(_compositeInInterface, "IDeviceSettingsCompositeIn");
    Exchange::IDeviceSettingsCompositeIn* result = _compositeInInterface;
    _lock.Unlock();
    return result;
}

} // namespace WPEFramework

#endif // USE_IARM