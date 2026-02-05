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

#include "dsFPD.h"
#include "dsError.h"
#include "dsclientlogger.h"
#include "dsConnectionManager.h"

// Thunder COM-RPC includes
#ifndef MODULE_NAME
#define MODULE_NAME DeviceSettings_FPD_Client
#endif

#include <interfaces/IDeviceSettingsFPD.h>

using namespace WPEFramework;
using namespace DeviceSettingsClient;

/**
 * @brief Convert Thunder error code to dsError_t
 */
static dsError_t ConvertThunderError(uint32_t thunderError)
{
    if (thunderError == WPEFramework::Core::ERROR_NONE) {
        return dsERR_NONE;
    } else if (thunderError == WPEFramework::Core::ERROR_UNAVAILABLE) {
        return dsERR_OPERATION_NOT_SUPPORTED;
    } else if (thunderError == WPEFramework::Core::ERROR_BAD_REQUEST) {
        return dsERR_INVALID_PARAM;
    } else {
        return dsERR_GENERAL;
    }
}

// C API implementations using Thunder COM-RPC via ConnectionManager

extern "C" {

// Forward declarations
dsError_t dsSetFPDBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t eBrightness, bool toPersist);
dsError_t dsSetFPDColor(dsFPDIndicator_t eIndicator, dsFPDColor_t eColor, bool toPersist);

dsError_t dsFPInit(void)
{
    printf("<<<<< Front Panel is initialized in Thunder Mode (using ConnectionManager) >>>>>>>>\r\n");
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr) {
        ConnectionManager::Init();
        connMgr = ConnectionManager::Instance();
    }
    
    if (!connMgr) {
        fprintf(stderr, "[dsFPD-com] Failed to create ConnectionManager instance\n");
        return dsERR_GENERAL;
    }
    
    // Wait for plugin to become operational
    if (!connMgr->WaitForOperational(5000)) {
        fprintf(stderr, "[dsFPD-com] DeviceSettings plugin not operational after 5 seconds\n");
        return dsERR_GENERAL;
    }
    
    // Thunder interface doesn't have explicit Init method - connection is sufficient
    return dsERR_NONE;
}

dsError_t dsFPTerm(void)
{
    // Note: Don't terminate ConnectionManager here as other components may be using it
    // ConnectionManager will be terminated by dsConnectionTerm() or at process exit
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
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDTimeFormat timeFormat = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDTimeFormat>(eTime);
    
    ConnectionManager::Lock();
    // Note: Interface expects minutes and seconds, but API provides hour and minutes
    // Converting: treating uMinutes as seconds for interface compatibility
    uint32_t result = fpdInterface->SetFPDTime(timeFormat, uHour, uMinutes);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsSetFPScroll(unsigned int nScrollHoldOnDur, unsigned int nHorzScrollIterations, unsigned int nVertScrollIterations)
{
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    ConnectionManager::Lock();
    uint32_t result = fpdInterface->SetFPDScroll(nScrollHoldOnDur, nHorzScrollIterations, nVertScrollIterations);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsSetFPBlink(dsFPDIndicator_t eIndicator, unsigned int nBlinkDuration, unsigned int nBlinkIterations)
{
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDIndicator>(eIndicator);
    
    ConnectionManager::Lock();
    uint32_t result = fpdInterface->SetFPDBlink(indicator, nBlinkDuration, nBlinkIterations);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsGetFPBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t *pBrightness)
{
    if (pBrightness == NULL) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter: pBrightness is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDIndicator>(eIndicator);
    
    uint32_t brightness = 0;
    ConnectionManager::Lock();
    uint32_t result = fpdInterface->GetFPDBrightness(indicator, brightness);
    ConnectionManager::Unlock();
    
    if (result == WPEFramework::Core::ERROR_NONE) {
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
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDIndicator>(eIndicator);
    
    ConnectionManager::Lock();
    uint32_t result = fpdInterface->SetFPDBrightness(indicator, static_cast<uint32_t>(eBrightness), toPersist);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsGetFPTextBrightness(dsFPDTextDisplay_t eIndicator, dsFPDBrightness_t *pBrightness)
{
    if (pBrightness == NULL) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter: pBrightness is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDTextDisplay indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDTextDisplay>(eIndicator);
    
    uint32_t brightness = 0;
    ConnectionManager::Lock();
    uint32_t result = fpdInterface->GetFPDTextBrightness(indicator, brightness);
    ConnectionManager::Unlock();
    
    if (result == WPEFramework::Core::ERROR_NONE) {
        *pBrightness = static_cast<dsFPDBrightness_t>(brightness);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsSetFPTextBrightness(dsFPDTextDisplay_t eIndicator, dsFPDBrightness_t eBrightness)
{
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDTextDisplay indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDTextDisplay>(eIndicator);
    
    ConnectionManager::Lock();
    uint32_t result = fpdInterface->SetFPDTextBrightness(indicator, static_cast<uint32_t>(eBrightness));
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsGetFPColor(dsFPDIndicator_t eIndicator, dsFPDColor_t *pColor)
{
    if (pColor == NULL) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter: pColor is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDIndicator>(eIndicator);
    
    uint32_t color = 0;
    ConnectionManager::Lock();
    uint32_t result = fpdInterface->GetFPDColor(indicator, color);
    ConnectionManager::Unlock();
    
    if (result == WPEFramework::Core::ERROR_NONE) {
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
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDIndicator>(eIndicator);
    
    // Thunder interface doesn't support persist flag - ignore it
    ConnectionManager::Lock();
    uint32_t result = fpdInterface->SetFPDColor(indicator, static_cast<uint32_t>(eColor));
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsFPEnableCLockDisplay(int enable)
{
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    ConnectionManager::Lock();
    uint32_t result = fpdInterface->EnableFPDClockDisplay(enable != 0);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsSetFPState(dsFPDIndicator_t eIndicator, dsFPDState_t state)
{
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDIndicator>(eIndicator);
    Exchange::IDeviceSettingsFPD::FPDState fpdState = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDState>(state);
    
    ConnectionManager::Lock();
    uint32_t result = fpdInterface->SetFPDState(indicator, fpdState);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsGetFPState(dsFPDIndicator_t eIndicator, dsFPDState_t* state)
{
    if (state == NULL) {
        fprintf(stderr, "[dsFPD-com] Invalid parameter: state is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDIndicator indicator = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDIndicator>(eIndicator);
    
    Exchange::IDeviceSettingsFPD::FPDState fpdState;
    ConnectionManager::Lock();
    uint32_t result = fpdInterface->GetFPDState(indicator, fpdState);
    ConnectionManager::Unlock();
    
    if (result == WPEFramework::Core::ERROR_NONE) {
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
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDTimeFormat timeFormat;
    ConnectionManager::Lock();
    uint32_t result = fpdInterface->GetFPDTimeFormat(timeFormat);
    ConnectionManager::Unlock();
    
    if (result == WPEFramework::Core::ERROR_NONE) {
        *pTimeFormat = static_cast<dsFPDTimeFormat_t>(timeFormat);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsSetFPTimeFormat(dsFPDTimeFormat_t eTimeFormat)
{
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDTimeFormat timeFormat = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDTimeFormat>(eTimeFormat);
    
    ConnectionManager::Lock();
    uint32_t result = fpdInterface->SetFPDTimeFormat(timeFormat);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsSetFPDMode(dsFPDMode_t eMode)
{
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD* fpdInterface = connMgr->GetFPDInterface();
    if (!fpdInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsFPD::FPDMode mode = 
        static_cast<Exchange::IDeviceSettingsFPD::FPDMode>(eMode);
    
    ConnectionManager::Lock();
    uint32_t result = fpdInterface->SetFPDMode(mode);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

} // extern "C"

#endif // USE_WPE_THUNDER_PLUGIN

/** @} */
/** @} */
