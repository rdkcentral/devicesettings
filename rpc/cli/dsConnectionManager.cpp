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

#ifdef USE_WPE_THUNDER_PLUGIN

#include <stdio.h>
#include <chrono>
#include <thread>
#include "dsConnectionManager.h"

using namespace WPEFramework;

namespace DeviceSettingsClient {

// Thunder callsign for DeviceSettings plugin
static constexpr const TCHAR callSign[] = _T("org.rdk.DeviceSettings");

// Static member initialization
ConnectionManager* ConnectionManager::_instance = nullptr;
Core::CriticalSection ConnectionManager::_dsConnectionManagerlock;

ConnectionManager::ConnectionManager()
    : BaseClass()
    , _fpdInterface(nullptr)
    , _hdmiInInterface(nullptr)
    , _connected(false)
    , _shutdown(false)
{
    printf("[dsConnectionManager] Initializing centralized connection manager\n");
    (void)Connect();
}

ConnectionManager::~ConnectionManager()
{
    printf("[dsConnectionManager] Destroying connection manager\n");
    _shutdown = true;
    
    // Release all component interfaces before closing base connection
    if (_hdmiInInterface) {
        _hdmiInInterface->Release();
        _hdmiInInterface = nullptr;
    }
    
    if (_fpdInterface) {
        _fpdInterface->Release();
        _fpdInterface = nullptr;
    }
    
    BaseClass::Close(Core::infinite);
}

void ConnectionManager::Operational(const bool upAndRunning)
{
    _dsConnectionManagerlock.Lock();

    if (!_shutdown) {
        printf("[dsConnectionManager] Operational callback: %s\n", upAndRunning ? "UP" : "DOWN");
    }

    if (upAndRunning) {
        // Communicator opened && DeviceSettings is Activated
        if (nullptr == _fpdInterface) {
            printf("[dsConnectionManager] Plugin activated, acquiring primary FPD interface\n");
            _fpdInterface = BaseClass::Interface();
            
            if (_fpdInterface != nullptr) {
                printf("[dsConnectionManager] Successfully established COM-RPC connection with DeviceSettings plugin\n");
                
                // Acquire secondary interfaces via QueryInterface
                if (nullptr == _hdmiInInterface) {
                    _hdmiInInterface = _fpdInterface->QueryInterface<Exchange::IDeviceSettingsHDMIIn>();
                    if (_hdmiInInterface != nullptr) {
                        printf("[dsConnectionManager] Successfully acquired HDMIIn interface via QueryInterface\n");
                    } else {
                        fprintf(stderr, "[dsConnectionManager] Failed to acquire HDMIIn interface via QueryInterface\n");
                    }
                }
                
                // Add more component interfaces here as needed:
                // if (nullptr == _compositeInInterface) {
                //     _compositeInInterface = _fpdInterface->QueryInterface<Exchange::IDeviceSettingsCompositeIn>();
                // }
                
            } else {
                fprintf(stderr, "[dsConnectionManager] Failed to get FPD interface - plugin implementation may have failed to load\n");
            }
        }
    } else {
        // DeviceSettings is Deactivated || Communicator closed
        printf("[dsConnectionManager] Plugin deactivated, releasing all interfaces\n");
        
        if (_hdmiInInterface != nullptr) {
            _hdmiInInterface->Release();
            _hdmiInInterface = nullptr;
        }
        
        if (_fpdInterface != nullptr) {
            _fpdInterface->Release();
            _fpdInterface = nullptr;
        }
    }
    
    _dsConnectionManagerlock.Unlock();
}

void ConnectionManager::Init()
{
    _dsConnectionManagerlock.Lock();
    if (nullptr == _instance) {
        _instance = new ConnectionManager();
    }
    _dsConnectionManagerlock.Unlock();
}

void ConnectionManager::Term()
{
    _dsConnectionManagerlock.Lock();
    if (nullptr != _instance) {
        delete _instance;
        _instance = nullptr;
    }
    _dsConnectionManagerlock.Unlock();
}

ConnectionManager* ConnectionManager::Instance()
{
    return _instance;
}

bool ConnectionManager::IsOperational() const
{
    _dsConnectionManagerlock.Lock();
    bool result = (isConnected() && (nullptr != _fpdInterface));
    _dsConnectionManagerlock.Unlock();
    return result;
}

bool ConnectionManager::WaitForOperational(uint32_t timeoutMs) const
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

uint32_t ConnectionManager::Connect()
{
    uint32_t status = Core::ERROR_NONE;

    _dsConnectionManagerlock.Lock();

    if (!isConnected()) {
        printf("[dsConnectionManager] Attempting to connect to Thunder with callsign: %s\n", callSign);
        uint32_t res = BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), callSign);
        if (Core::ERROR_NONE == res) {
            _connected = true;
            printf("[dsConnectionManager] Successfully opened RPC connection to Thunder\n");
        } else {
            fprintf(stderr, "[dsConnectionManager] Failed to open RPC connection, error: %u. Is Thunder running?\n", res);
            status = Core::ERROR_UNAVAILABLE;
        }
    } else {
        printf("[dsConnectionManager] Already connected\n");
    }

    if (nullptr == _fpdInterface) {
        status = Core::ERROR_NOT_EXIST;
        printf("[dsConnectionManager] DeviceSettings plugin not yet operational, waiting for Operational() callback\n");
    }

    _dsConnectionManagerlock.Unlock();

    return status;
}

uint32_t ConnectionManager::Disconnect()
{
    uint32_t status = Core::ERROR_GENERAL;
    bool close = false;

    _dsConnectionManagerlock.Lock();

    if (isConnected()) {
        close = true;
        _connected = false;
    }

    _dsConnectionManagerlock.Unlock();

    if (close) {
        status = BaseClass::Close(Core::infinite);
        printf("[dsConnectionManager] Disconnected from Thunder\n");
    }

    return status;
}

Exchange::IDeviceSettingsFPD* ConnectionManager::GetFPDInterface()
{
    _dsConnectionManagerlock.Lock();
    Exchange::IDeviceSettingsFPD* interface = _fpdInterface;
    _dsConnectionManagerlock.Unlock();
    return interface;
}

Exchange::IDeviceSettingsHDMIIn* ConnectionManager::GetHDMIInInterface()
{
    _dsConnectionManagerlock.Lock();
    Exchange::IDeviceSettingsHDMIIn* interface = _hdmiInInterface;
    _dsConnectionManagerlock.Unlock();
    return interface;
}

} // namespace DeviceSettingsClient

#endif // USE_WPE_THUNDER_PLUGIN
