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
* @defgroup devicesettings
* @{
* @defgroup rpc
* @{
**/

#ifdef USE_WPE_THUNDER_PLUGIN

#include <stdio.h>
#include <string.h>
#include <chrono>
#include <thread>

#include "dsFPD.h"
#include "dsError.h"
#include "dsclientlogger.h"

// Thunder COM-RPC includes
#ifndef MODULE_NAME
#define MODULE_NAME DeviceSettings_FPD_Client
#endif

#include <core/core.h>
#include <com/com.h>
#include <plugins/Types.h>
#include <interfaces/IDeviceSettingsManager.h>

using namespace WPEFramework;

// Thunder callsign for DeviceSettingsManager plugin
static constexpr const TCHAR callSign[] = _T("org.rdk.DeviceSettingsManager");

/**
 * @brief DeviceSettingsFPD class manages Thunder COM-RPC connection for FPD
 */
class DeviceSettingsFPD : public RPC::SmartInterfaceType<Exchange::IDeviceSettingsManagerFPD> {
private:
    using BaseClass = RPC::SmartInterfaceType<Exchange::IDeviceSettingsManagerFPD>;
    
    Exchange::IDeviceSettingsManagerFPD* _fpdInterface;
    
    static DeviceSettingsFPD* _instance;
    static Core::CriticalSection _apiLock;
    
    bool _connected;
    bool _shutdown;

    DeviceSettingsFPD()
        : BaseClass()
        , _fpdInterface(nullptr)
        , _connected(false)
        , _shutdown(false)
    {
        (void)Connect();
    }

    ~DeviceSettingsFPD()
    {
        _shutdown = true;
        BaseClass::Close(Core::infinite);
    }

    virtual void Operational(const bool upAndRunning) override
    {
        _apiLock.Lock();

        if (upAndRunning) {
            // Communicator opened && DeviceSettingsManager is Activated
            if (nullptr == _fpdInterface) {
                _fpdInterface = BaseClass::Interface();
                if (_fpdInterface != nullptr) {
                    printf("[dsFPD-com] Successfully established COM-RPC connection with DeviceSettingsManager plugin\n");
                } else {
                    fprintf(stderr, "[dsFPD-com] Failed to get interface - plugin implementation may have failed to load\n");
                }
            }
        } else {
            // DeviceSettingsManager is Deactivated || Communicator closed
            if (nullptr != _fpdInterface) {
                _fpdInterface->Release();
                _fpdInterface = nullptr;
            }
        }
        _apiLock.Unlock();
    }

    inline bool IsActivatedLocked() const
    {
        return (nullptr != _fpdInterface);
    }

    inline bool isConnected() const
    {
        return _connected;
    }

public:
    bool IsOperational() const
    {
        _apiLock.Lock();
        bool result = (isConnected() && (nullptr != _fpdInterface));
        _apiLock.Unlock();
        return result;
    }

    bool WaitForOperational(uint32_t timeoutMs = 5000) const
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

    uint32_t Connect()
    {
        uint32_t status = Core::ERROR_NONE;

        _apiLock.Lock();

        if (!isConnected()) {
            printf("[dsFPD-com] Attempting to connect to Thunder with callsign: %s\n", callSign);
            uint32_t res = BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), callSign);
            if (Core::ERROR_NONE == res) {
                _connected = true;
                printf("[dsFPD-com] Successfully opened RPC connection to Thunder\n");
            } else {
                fprintf(stderr, "[dsFPD-com] Failed to open RPC connection, error: %u. Is Thunder running?\n", res);
                status = Core::ERROR_UNAVAILABLE;
            }
        }

        if (nullptr == _fpdInterface) {
            status = Core::ERROR_NOT_EXIST;
            printf("[dsFPD-com] DeviceSettingsManager plugin not yet operational\n");
        }

        _apiLock.Unlock();

        return status;
    }

    uint32_t Disconnect()
    {
        uint32_t status = Core::ERROR_GENERAL;
        bool close = false;

        _apiLock.Lock();

        if (isConnected()) {
            close = true;
            _connected = false;
        }

        _apiLock.Unlock();

        if (close) {
            status = BaseClass::Close(Core::infinite);
        }

        return status;
    }

    static void Init()
    {
        _apiLock.Lock();
        if (nullptr == _instance) {
            _instance = new DeviceSettingsFPD();
        }
        _apiLock.Unlock();
    }

    static void Term()
    {
        _apiLock.Lock();
        if (nullptr != _instance) {
            delete _instance;
            _instance = nullptr;
        }
        _apiLock.Unlock();
    }

    static DeviceSettingsFPD* Instance()
    {
        return _instance;
    }

    // FPD API implementations
    // Note: Thunder interface doesn't have FPDInit/FPDTerm/SetFPDText methods

    Core::hresult SetFPDTime(const Exchange::IDeviceSettingsManagerFPD::FPDTimeFormat timeFormat, 
                             const uint32_t hour, const uint32_t minutes)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->SetFPDTime(timeFormat, hour, minutes);
        }
        _apiLock.Unlock();
        return result;
    }

    Core::hresult SetFPDScroll(const uint32_t scrollHoldOnDur, const uint32_t horzScrollIterations, 
                               const uint32_t vertScrollIterations)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->SetFPDScroll(scrollHoldOnDur, horzScrollIterations, vertScrollIterations);
        }
        _apiLock.Unlock();
        return result;
    }

    Core::hresult SetFPDBlink(const Exchange::IDeviceSettingsManagerFPD::FPDIndicator indicator, 
                              const uint32_t blinkDuration, const uint32_t blinkIterations)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->SetFPDBlink(indicator, blinkDuration, blinkIterations);
        }
        _apiLock.Unlock();
        return result;
    }

    Core::hresult GetFPDBrightness(const Exchange::IDeviceSettingsManagerFPD::FPDIndicator indicator, 
                                   uint32_t& brightness)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->GetFPDBrightness(indicator, brightness);
        }
        _apiLock.Unlock();
        return result;
    }

    Core::hresult SetFPDBrightness(const Exchange::IDeviceSettingsManagerFPD::FPDIndicator indicator, 
                                   const uint32_t brightness, const bool persist)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->SetFPDBrightness(indicator, brightness, persist);
        }
        _apiLock.Unlock();
        return result;
    }

    Core::hresult GetFPDTextBrightness(const Exchange::IDeviceSettingsManagerFPD::FPDTextDisplay indicator, 
                                       uint32_t& brightness)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->GetFPDTextBrightness(indicator, brightness);
        }
        _apiLock.Unlock();
        return result;
    }

    Core::hresult SetFPDTextBrightness(const Exchange::IDeviceSettingsManagerFPD::FPDTextDisplay indicator, 
                                       const uint32_t brightness)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->SetFPDTextBrightness(indicator, brightness);
        }
        _apiLock.Unlock();
        return result;
    }

    Core::hresult GetFPDColor(const Exchange::IDeviceSettingsManagerFPD::FPDIndicator indicator, 
                              uint32_t& color)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->GetFPDColor(indicator, color);
        }
        _apiLock.Unlock();
        return result;
    }

    Core::hresult SetFPDColor(const Exchange::IDeviceSettingsManagerFPD::FPDIndicator indicator, 
                              const uint32_t color)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->SetFPDColor(indicator, color);
        }
        _apiLock.Unlock();
        return result;
    }

    Core::hresult EnableFPDClockDisplay(const bool enable)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->EnableFPDClockDisplay(enable);
        }
        _apiLock.Unlock();
        return result;
    }

    Core::hresult SetFPDState(const Exchange::IDeviceSettingsManagerFPD::FPDIndicator indicator, 
                              const Exchange::IDeviceSettingsManagerFPD::FPDState state)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->SetFPDState(indicator, state);
        }
        _apiLock.Unlock();
        return result;
    }

    Core::hresult GetFPDState(const Exchange::IDeviceSettingsManagerFPD::FPDIndicator indicator, 
                              Exchange::IDeviceSettingsManagerFPD::FPDState& state)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->GetFPDState(indicator, state);
        }
        _apiLock.Unlock();
        return result;
    }

    Core::hresult GetFPDTimeFormat(Exchange::IDeviceSettingsManagerFPD::FPDTimeFormat& timeFormat)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->GetFPDTimeFormat(timeFormat);
        }
        _apiLock.Unlock();
        return result;
    }

    Core::hresult SetFPDTimeFormat(const Exchange::IDeviceSettingsManagerFPD::FPDTimeFormat timeFormat)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->SetFPDTimeFormat(timeFormat);
        }
        _apiLock.Unlock();
        return result;
    }

    Core::hresult SetFPDMode(const Exchange::IDeviceSettingsManagerFPD::FPDMode mode)
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        _apiLock.Lock();
        if (_fpdInterface) {
            result = _fpdInterface->SetFPDMode(mode);
        }
        _apiLock.Unlock();
        return result;
    }
};

// Static member initialization
DeviceSettingsFPD* DeviceSettingsFPD::_instance = nullptr;
Core::CriticalSection DeviceSettingsFPD::_apiLock;

/**
 * @brief Convert Thunder error code to dsError_t
 */
static dsError_t ConvertThunderError(uint32_t thunderError)
{
    if (thunderError == Core::ERROR_NONE) {
        return dsERR_NONE;
    } else if (thunderError == Core::ERROR_UNAVAILABLE) {
        return dsERR_OPERATION_NOT_SUPPORTED;
    } else if (thunderError == Core::ERROR_BAD_REQUEST) {
        return dsERR_INVALID_PARAM;
    } else {
        return dsERR_GENERAL;
    }
}

// C API implementations using Thunder COM-RPC

extern "C" {

// Forward declarations
dsError_t dsSetFPDBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t eBrightness, bool toPersist);
dsError_t dsSetFPDColor(dsFPDIndicator_t eIndicator, dsFPDColor_t eColor, bool toPersist);

dsError_t dsFPInit(void)
{
    printf("<<<<< Front Panel is initialized in Thunder Mode >>>>>>>>\r\n");
    
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance) {
        DeviceSettingsFPD::Init();
        instance = DeviceSettingsFPD::Instance();
    }
    
    if (!instance) {
        fprintf(stderr, "[dsFPD-com] Failed to create DeviceSettingsFPD instance\n");
        return dsERR_GENERAL;
    }
    
    // Wait for plugin to become operational
    if (!instance->WaitForOperational(5000)) {
        fprintf(stderr, "[dsFPD-com] DeviceSettingsManager plugin not operational after 5 seconds\n");
        return dsERR_GENERAL;
    }
    
    // Thunder interface doesn't have explicit Init method - connection is sufficient
    return dsERR_NONE;
}

dsError_t dsFPTerm(void)
{
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance) {
        return dsERR_GENERAL;
    }
    
    // Thunder interface doesn't have explicit Term method
    // Terminate instance
    DeviceSettingsFPD::Term();
    
    return dsERR_NONE;
}

dsError_t dsSetFPText(const char* pszChars)
{
    if (pszChars == NULL) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter: pszChars is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    // Thunder interface doesn't support text display
    fprintf(stderr, "[dsFPD-com] SetFPText not supported in Thunder mode\n");
    return dsERR_OPERATION_NOT_SUPPORTED;
}

dsError_t dsSetFPTime(dsFPDTimeFormat_t eTime, const unsigned int uHour, const unsigned int uMinutes)
{
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsManagerFPD::FPDTimeFormat timeFormat = 
        static_cast<Exchange::IDeviceSettingsManagerFPD::FPDTimeFormat>(eTime);
    
    uint32_t result = instance->SetFPDTime(timeFormat, uHour, uMinutes);
    return ConvertThunderError(result);
}

dsError_t dsSetFPScroll(unsigned int nScrollHoldOnDur, unsigned int nHorzScrollIterations, unsigned int nVertScrollIterations)
{
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    uint32_t result = instance->SetFPDScroll(nScrollHoldOnDur, nHorzScrollIterations, nVertScrollIterations);
    return ConvertThunderError(result);
}

dsError_t dsSetFPBlink(dsFPDIndicator_t eIndicator, unsigned int nBlinkDuration, unsigned int nBlinkIterations)
{
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsManagerFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsManagerFPD::FPDIndicator>(eIndicator);
    
    uint32_t result = instance->SetFPDBlink(indicator, nBlinkDuration, nBlinkIterations);
    return ConvertThunderError(result);
}

dsError_t dsGetFPBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t *pBrightness)
{
    if (pBrightness == NULL) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter: pBrightness is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsManagerFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsManagerFPD::FPDIndicator>(eIndicator);
    
    uint32_t brightness = 0;
    uint32_t result = instance->GetFPDBrightness(indicator, brightness);
    
    if (result == Core::ERROR_NONE) {
        *pBrightness = static_cast<dsFPDBrightness_t>(brightness);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsGetFPDBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t *pBrightness, bool persist)
{
    // Note: persist parameter may not be used in GET operation for Thunder
    return dsGetFPBrightness(eIndicator, pBrightness);
}

dsError_t dsSetFPBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t eBrightness)
{
    return dsSetFPDBrightness(eIndicator, eBrightness, true);
}

dsError_t dsSetFPDBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t eBrightness, bool toPersist)
{
    if (eIndicator >= dsFPD_INDICATOR_MAX || eBrightness > 100) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsManagerFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsManagerFPD::FPDIndicator>(eIndicator);
    
    uint32_t result = instance->SetFPDBrightness(indicator, static_cast<uint32_t>(eBrightness), toPersist);
    return ConvertThunderError(result);
}

dsError_t dsGetFPTextBrightness(dsFPDTextDisplay_t eIndicator, dsFPDBrightness_t *pBrightness)
{
    if (pBrightness == NULL) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter: pBrightness is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsManagerFPD::FPDTextDisplay indicator = 
        static_cast<Exchange::IDeviceSettingsManagerFPD::FPDTextDisplay>(eIndicator);
    
    uint32_t brightness = 0;
    uint32_t result = instance->GetFPDTextBrightness(indicator, brightness);
    
    if (result == Core::ERROR_NONE) {
        *pBrightness = static_cast<dsFPDBrightness_t>(brightness);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsSetFPTextBrightness(dsFPDTextDisplay_t eIndicator, dsFPDBrightness_t eBrightness)
{
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsManagerFPD::FPDTextDisplay indicator = 
        static_cast<Exchange::IDeviceSettingsManagerFPD::FPDTextDisplay>(eIndicator);
    
    uint32_t result = instance->SetFPDTextBrightness(indicator, static_cast<uint32_t>(eBrightness));
    return ConvertThunderError(result);
}

dsError_t dsGetFPColor(dsFPDIndicator_t eIndicator, dsFPDColor_t *pColor)
{
    if (pColor == NULL) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter: pColor is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsManagerFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsManagerFPD::FPDIndicator>(eIndicator);
    
    uint32_t color = 0;
    uint32_t result = instance->GetFPDColor(indicator, color);
    
    if (result == Core::ERROR_NONE) {
        *pColor = static_cast<dsFPDColor_t>(color);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsSetFPColor(dsFPDIndicator_t eIndicator, dsFPDColor_t eColor)
{
    return dsSetFPDColor(eIndicator, eColor, true);
}

dsError_t dsSetFPDColor(dsFPDIndicator_t eIndicator, dsFPDColor_t eColor, bool toPersist)
{
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsManagerFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsManagerFPD::FPDIndicator>(eIndicator);
    
    // Thunder interface doesn't support persist flag - ignore it
    uint32_t result = instance->SetFPDColor(indicator, static_cast<uint32_t>(eColor));
    return ConvertThunderError(result);
}

dsError_t dsFPEnableCLockDisplay(int enable)
{
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    uint32_t result = instance->EnableFPDClockDisplay(enable != 0);
    return ConvertThunderError(result);
}

dsError_t dsSetFPState(dsFPDIndicator_t eIndicator, dsFPDState_t state)
{
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsManagerFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsManagerFPD::FPDIndicator>(eIndicator);
    Exchange::IDeviceSettingsManagerFPD::FPDState fpdState = 
        static_cast<Exchange::IDeviceSettingsManagerFPD::FPDState>(state);
    
    uint32_t result = instance->SetFPDState(indicator, fpdState);
    return ConvertThunderError(result);
}

dsError_t dsGetFPState(dsFPDIndicator_t eIndicator, dsFPDState_t* state)
{
    if (state == NULL) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter: state is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsManagerFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsManagerFPD::FPDIndicator>(eIndicator);
    
    Exchange::IDeviceSettingsManagerFPD::FPDState fpdState;
    uint32_t result = instance->GetFPDState(indicator, fpdState);
    
    if (result == Core::ERROR_NONE) {
        *state = static_cast<dsFPDState_t>(fpdState);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsGetFPTimeFormat(dsFPDTimeFormat_t *pTimeFormat)
{
    if (pTimeFormat == NULL) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter: pTimeFormat is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsManagerFPD::FPDTimeFormat timeFormat;
    uint32_t result = instance->GetFPDTimeFormat(timeFormat);
    
    if (result == Core::ERROR_NONE) {
        *pTimeFormat = static_cast<dsFPDTimeFormat_t>(timeFormat);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsSetFPTimeFormat(dsFPDTimeFormat_t eTimeFormat)
{
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsManagerFPD::FPDTimeFormat timeFormat = 
        static_cast<Exchange::IDeviceSettingsManagerFPD::FPDTimeFormat>(eTimeFormat);
    
    uint32_t result = instance->SetFPDTimeFormat(timeFormat);
    return ConvertThunderError(result);
}

dsError_t dsSetFPDMode(dsFPDMode_t eMode)
{
    DeviceSettingsFPD* instance = DeviceSettingsFPD::Instance();
    if (!instance || !instance->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsManagerFPD::FPDMode mode = 
        static_cast<Exchange::IDeviceSettingsManagerFPD::FPDMode>(eMode);
    
    uint32_t result = instance->SetFPDMode(mode);
    return ConvertThunderError(result);
}

} // extern "C"

#endif // USE_WPE_THUNDER_PLUGIN

/** @} */
/** @} */
