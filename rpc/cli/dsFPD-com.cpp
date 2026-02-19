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

#include <stdio.h>
#include <string.h>
#include <chrono>
#include <thread>

#include "dsFPD.h"
#include "dsError.h"
#include "dsclientlogger.h"
#include "dsController-com.h"

using namespace WPEFramework;

// Thread safety lock for FPD interface calls
static Core::CriticalSection _fpdLock;

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

/**
 * @brief Thread-safe wrapper functions for FPD interface calls
 */
namespace FPDWrapper {

inline uint32_t SetFPDTime(Exchange::IDeviceSettingsFPD* fpdInterface, 
                           Exchange::IDeviceSettingsFPD::FPDTimeFormat timeFormat,
                           uint32_t hour, uint32_t minutes)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->SetFPDTime(timeFormat, hour, minutes);
    _fpdLock.Unlock();
    return result;
}

inline uint32_t SetFPDScroll(Exchange::IDeviceSettingsFPD* fpdInterface,
                             uint32_t scrollHoldOnDur, uint32_t horzScrollIterations,
                             uint32_t vertScrollIterations)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->SetFPDScroll(scrollHoldOnDur, horzScrollIterations, vertScrollIterations);
    _fpdLock.Unlock();
    return result;
}

inline uint32_t SetFPDBlink(Exchange::IDeviceSettingsFPD* fpdInterface,
                            Exchange::IDeviceSettingsFPD::FPDIndicator indicator,
                            uint32_t blinkDuration, uint32_t blinkIterations)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->SetFPDBlink(indicator, blinkDuration, blinkIterations);
    _fpdLock.Unlock();
    return result;
}

inline uint32_t GetFPDBrightness(Exchange::IDeviceSettingsFPD* fpdInterface,
                                 Exchange::IDeviceSettingsFPD::FPDIndicator indicator,
                                 uint32_t& brightness)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->GetFPDBrightness(indicator, brightness);
    _fpdLock.Unlock();
    return result;
}

inline uint32_t SetFPDBrightness(Exchange::IDeviceSettingsFPD* fpdInterface,
                                 Exchange::IDeviceSettingsFPD::FPDIndicator indicator,
                                 uint32_t brightness, bool persist)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->SetFPDBrightness(indicator, brightness, persist);
    _fpdLock.Unlock();
    return result;
}

inline uint32_t GetFPDTextBrightness(Exchange::IDeviceSettingsFPD* fpdInterface,
                                     Exchange::IDeviceSettingsFPD::FPDTextDisplay indicator,
                                     uint32_t& brightness)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->GetFPDTextBrightness(indicator, brightness);
    _fpdLock.Unlock();
    return result;
}

inline uint32_t SetFPDTextBrightness(Exchange::IDeviceSettingsFPD* fpdInterface,
                                     Exchange::IDeviceSettingsFPD::FPDTextDisplay indicator,
                                     uint32_t brightness)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->SetFPDTextBrightness(indicator, brightness);
    _fpdLock.Unlock();
    return result;
}

inline uint32_t GetFPDColor(Exchange::IDeviceSettingsFPD* fpdInterface,
                            Exchange::IDeviceSettingsFPD::FPDIndicator indicator,
                            uint32_t& color)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->GetFPDColor(indicator, color);
    _fpdLock.Unlock();
    return result;
}

inline uint32_t SetFPDColor(Exchange::IDeviceSettingsFPD* fpdInterface,
                            Exchange::IDeviceSettingsFPD::FPDIndicator indicator,
                            uint32_t color)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->SetFPDColor(indicator, color);
    _fpdLock.Unlock();
    return result;
}

inline uint32_t EnableFPDClockDisplay(Exchange::IDeviceSettingsFPD* fpdInterface, bool enable)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->EnableFPDClockDisplay(enable);
    _fpdLock.Unlock();
    return result;
}

inline uint32_t SetFPDState(Exchange::IDeviceSettingsFPD* fpdInterface,
                            Exchange::IDeviceSettingsFPD::FPDIndicator indicator,
                            Exchange::IDeviceSettingsFPD::FPDState state)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->SetFPDState(indicator, state);
    _fpdLock.Unlock();
    return result;
}

inline uint32_t GetFPDState(Exchange::IDeviceSettingsFPD* fpdInterface,
                            Exchange::IDeviceSettingsFPD::FPDIndicator indicator,
                            Exchange::IDeviceSettingsFPD::FPDState& state)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->GetFPDState(indicator, state);
    _fpdLock.Unlock();
    return result;
}

inline uint32_t GetFPDTimeFormat(Exchange::IDeviceSettingsFPD* fpdInterface,
                                 Exchange::IDeviceSettingsFPD::FPDTimeFormat& timeFormat)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->GetFPDTimeFormat(timeFormat);
    _fpdLock.Unlock();
    return result;
}

inline uint32_t SetFPDTimeFormat(Exchange::IDeviceSettingsFPD* fpdInterface,
                                 Exchange::IDeviceSettingsFPD::FPDTimeFormat timeFormat)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->SetFPDTimeFormat(timeFormat);
    _fpdLock.Unlock();
    return result;
}

inline uint32_t SetFPDMode(Exchange::IDeviceSettingsFPD* fpdInterface,
                           Exchange::IDeviceSettingsFPD::FPDMode mode)
{
    _fpdLock.Lock();
    uint32_t result = fpdInterface->SetFPDMode(mode);
    _fpdLock.Unlock();
    return result;
}

} // namespace FPDWrapper

// C API implementations using Thunder COM-RPC

extern "C" {

// Forward declarations
dsError_t dsSetFPDBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t eBrightness, bool toPersist);
dsError_t dsSetFPDColor(dsFPDIndicator_t eIndicator, dsFPDColor_t eColor, bool toPersist);

dsError_t dsFPInit(void)
{
    printf("<<<<< Front Panel is initialized in Thunder Mode >>>>>>>>\r\n");
    
    //DeviceSettingsController is initialized by manager.cpp
    // Just verify we have FPD interface access
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        fprintf(stderr, "[dsFPD-com] DeviceSettingsController not initialized\n");
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        fprintf(stderr, "[dsFPD-com] FPD interface not available\n");
        return dsERR_GENERAL;
    }
    
    return dsERR_NONE;
}

dsError_t dsFPTerm(void)
{
    // DeviceSettingsController cleanup handled by manager.cpp
    // No component-specific cleanup needed
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
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDTimeFormat timeFormat = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDTimeFormat>(eTime);
    
    uint32_t result = FPDWrapper::SetFPDTime(fpdInterface, timeFormat, uHour, uMinutes);
    return ConvertThunderError(result);
}

dsError_t dsSetFPScroll(unsigned int nScrollHoldOnDur, unsigned int nHorzScrollIterations, unsigned int nVertScrollIterations)
{
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    uint32_t result = FPDWrapper::SetFPDScroll(fpdInterface, nScrollHoldOnDur, nHorzScrollIterations, nVertScrollIterations);
    return ConvertThunderError(result);
}

dsError_t dsSetFPBlink(dsFPDIndicator_t eIndicator, unsigned int nBlinkDuration, unsigned int nBlinkIterations)
{
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDIndicator>(eIndicator);
    
    uint32_t result = FPDWrapper::SetFPDBlink(fpdInterface, indicator, nBlinkDuration, nBlinkIterations);
    return ConvertThunderError(result);
}

dsError_t dsGetFPBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t *pBrightness)
{
    if (pBrightness == NULL) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter: pBrightness is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDIndicator>(eIndicator);
    
    uint32_t brightness = 0;
    uint32_t result = FPDWrapper::GetFPDBrightness(fpdInterface, indicator, brightness);
    
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
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDIndicator>(eIndicator);
    
    uint32_t result = FPDWrapper::SetFPDBrightness(fpdInterface, indicator, static_cast<uint32_t>(eBrightness), toPersist);
    return ConvertThunderError(result);
}

dsError_t dsGetFPTextBrightness(dsFPDTextDisplay_t eIndicator, dsFPDBrightness_t *pBrightness)
{
    if (pBrightness == NULL) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter: pBrightness is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDTextDisplay indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDTextDisplay>(eIndicator);
    
    uint32_t brightness = 0;
    uint32_t result = FPDWrapper::GetFPDTextBrightness(fpdInterface, indicator, brightness);
    
    if (result == Core::ERROR_NONE) {
        *pBrightness = static_cast<dsFPDBrightness_t>(brightness);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsSetFPTextBrightness(dsFPDTextDisplay_t eIndicator, dsFPDBrightness_t eBrightness)
{
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDTextDisplay indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDTextDisplay>(eIndicator);
    
    uint32_t result = FPDWrapper::SetFPDTextBrightness(fpdInterface, indicator, static_cast<uint32_t>(eBrightness));
    return ConvertThunderError(result);
}

dsError_t dsGetFPColor(dsFPDIndicator_t eIndicator, dsFPDColor_t *pColor)
{
    if (pColor == NULL) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter: pColor is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDIndicator>(eIndicator);
    
    uint32_t color = 0;
    uint32_t result = FPDWrapper::GetFPDColor(fpdInterface, indicator, color);
    
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
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDIndicator>(eIndicator);
    
    // Thunder interface doesn't support persist flag - ignore it
    uint32_t result = FPDWrapper::SetFPDColor(fpdInterface, indicator, static_cast<uint32_t>(eColor));
    return ConvertThunderError(result);
}

dsError_t dsFPEnableCLockDisplay(int enable)
{
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    uint32_t result = FPDWrapper::EnableFPDClockDisplay(fpdInterface, enable != 0);
    return ConvertThunderError(result);
}

dsError_t dsSetFPState(dsFPDIndicator_t eIndicator, dsFPDState_t state)
{
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDIndicator>(eIndicator);
    Exchange::IDeviceSettingsFPD::FPDState fpdState = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDState>(state);
    
    uint32_t result = FPDWrapper::SetFPDState(fpdInterface, indicator, fpdState);
    return ConvertThunderError(result);
}

dsError_t dsGetFPState(dsFPDIndicator_t eIndicator, dsFPDState_t* state)
{
    if (state == NULL) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter: state is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDIndicator>(eIndicator);
    
    Exchange::IDeviceSettingsFPD::FPDState fpdState;
    uint32_t result = FPDWrapper::GetFPDState(fpdInterface, indicator, fpdState);
    
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
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDTimeFormat timeFormat;
    uint32_t result = FPDWrapper::GetFPDTimeFormat(fpdInterface, timeFormat);
    
    if (result == Core::ERROR_NONE) {
        *pTimeFormat = static_cast<dsFPDTimeFormat_t>(timeFormat);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsSetFPTimeFormat(dsFPDTimeFormat_t eTimeFormat)
{
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDTimeFormat timeFormat = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDTimeFormat>(eTimeFormat);
    
    uint32_t result = FPDWrapper::SetFPDTimeFormat(fpdInterface, timeFormat);
    return ConvertThunderError(result);
}

dsError_t dsSetFPDMode(dsFPDMode_t eMode)
{
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = controller->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDMode mode = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDMode>(eMode);
    
    uint32_t result = FPDWrapper::SetFPDMode(fpdInterface, mode);
    return ConvertThunderError(result);
}

} // extern "C"

/** @} */
/** @} */
