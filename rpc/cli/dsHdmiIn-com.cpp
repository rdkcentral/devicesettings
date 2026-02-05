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

/**
* @defgroup devicesettings
* @{
* @defgroup rpc
* @{
**/

#ifdef USE_WPE_THUNDER_PLUGIN

#include <stdio.h>
#include <string.h>

#include "dsHdmiIn.h"
#include "dsError.h"
#include "dsclientlogger.h"
#include "dsConnectionManager.h"

// Thunder COM-RPC includes
#ifndef MODULE_NAME
#define MODULE_NAME DeviceSettings_HDMIIn_Client
#endif

#include <interfaces/IDeviceSettingsHDMIIn.h>

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

dsError_t dsHdmiInInit(void)
{
    printf("<<<<< HDMI In is initialized in Thunder Mode (using ConnectionManager) >>>>>>>>\r\n");
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr) {
        ConnectionManager::Init();
        connMgr = ConnectionManager::Instance();
    }
    
    if (!connMgr) {
        fprintf(stderr, "[dsHdmiIn-com] Failed to create ConnectionManager instance\n");
        return dsERR_GENERAL;
    }
    
    // Wait for plugin to become operational
    if (!connMgr->WaitForOperational(5000)) {
        fprintf(stderr, "[dsHdmiIn-com] DeviceSettings plugin not operational after 5 seconds\n");
        return dsERR_GENERAL;
    }
    
    return dsERR_NONE;
}

dsError_t dsHdmiInTerm(void)
{
    // Note: Don't terminate ConnectionManager here as other components may be using it
    // ConnectionManager will be terminated by dsConnectionTerm() or at process exit
    return dsERR_NONE;
}

dsError_t dsHdmiInGetNumberOfInputs(uint8_t* pNumberOfInputs)
{
    if (pNumberOfInputs == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: pNumberOfInputs is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    int32_t count = 0;
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->GetHDMIInNumbefOfInputs(count);
    ConnectionManager::Unlock();
    
    if (result == WPEFramework::Core::ERROR_NONE) {
        *pNumberOfInputs = static_cast<uint8_t>(count);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsHdmiInGetStatus(dsHdmiInStatus_t* pStatus)
{
    if (pStatus == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: pStatus is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInStatus hdmiStatus;
    Exchange::IDeviceSettingsHDMIIn::IHDMIInPortConnectionStatusIterator* portConnectionStatus = nullptr;
    
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->GetHDMIInStatus(hdmiStatus, portConnectionStatus);
    ConnectionManager::Unlock();
    
    if (result == WPEFramework::Core::ERROR_NONE) {
        pStatus->isPresented = hdmiStatus.isPresented;
        pStatus->activePort = static_cast<dsHdmiInPort_t>(hdmiStatus.activePort);
        
        // TODO: Handle portConnectionStatus iterator if needed
        if (portConnectionStatus) {
            portConnectionStatus->Release();
        }
    }
    
    return ConvertThunderError(result);
}

dsError_t dsHdmiInSelectPort(dsHdmiInPort_t ePort, bool audioMix, dsVideoPlaneType_t videoPlaneType, bool topMost)
{
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(ePort);
    Exchange::IDeviceSettingsHDMIIn::HDMIVideoPlaneType planeType = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIVideoPlaneType>(videoPlaneType);
    
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->SelectHDMIInPort(port, audioMix, topMost, planeType);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsHdmiInScaleVideo(int32_t x, int32_t y, int32_t width, int32_t height)
{
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInVideoRectangle videoPosition;
    videoPosition.x = x;
    videoPosition.y = y;
    videoPosition.width = width;
    videoPosition.height = height;
    
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->ScaleHDMIInVideo(videoPosition);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsHdmiInSelectZoomMode(dsVideoZoom_t zoomMode)
{
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInVideoZoom zoom = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInVideoZoom>(zoomMode);
    
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->SelectHDMIZoomMode(zoom);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsHdmiInGetCurrentVideoMode(dsVideoPortResolution_t* resolution)
{
    if (resolution == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: resolution is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIVideoPortResolution videoPortResolution;
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->GetHDMIVideoMode(videoPortResolution);
    ConnectionManager::Unlock();
    
    if (result == WPEFramework::Core::ERROR_NONE) {
        strncpy(resolution->name, videoPortResolution.name.c_str(), sizeof(resolution->name) - 1);
        resolution->name[sizeof(resolution->name) - 1] = '\0';
        resolution->pixelResolution = static_cast<dsVideoResolution_t>(videoPortResolution.pixelResolution);
        resolution->aspectRatio = static_cast<dsVideoAspectRatio_t>(videoPortResolution.aspectRatio);
        resolution->stereoScopicMode = static_cast<dsVideoStereoScopicMode_t>(videoPortResolution.stereoScopicMode);
        resolution->frameRate = static_cast<dsVideoFrameRate_t>(videoPortResolution.frameRate);
        resolution->interlaced = videoPortResolution.interlaced;
    }
    
    return ConvertThunderError(result);
}

dsError_t dsGetEDIDBytesInfo(dsHdmiInPort_t iHdmiPort, unsigned char* edid, int* length)
{
    if (edid == NULL || length == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->GetEdidBytes(port, static_cast<uint16_t>(*length), edid);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsGetHDMISPDInfo(dsHdmiInPort_t iHdmiPort, unsigned char* data)
{
    if (data == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: data is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    // Assuming SPD info frame size (adjust as needed)
    const uint16_t spdBytesLength = 32;
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->GetHDMISPDInformation(port, spdBytesLength, data);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsGetEdidVersion(dsHdmiInPort_t iHdmiPort, tv_hdmi_edid_version_t* edidVersion)
{
    if (edidVersion == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: edidVersion is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    Exchange::IDeviceSettingsHDMIIn::HDMIInEdidVersion version;
    
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->GetHDMIEdidVersion(port, version);
    ConnectionManager::Unlock();
    
    if (result == WPEFramework::Core::ERROR_NONE) {
        *edidVersion = static_cast<tv_hdmi_edid_version_t>(version);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsSetEdidVersion(dsHdmiInPort_t iHdmiPort, tv_hdmi_edid_version_t edidVersion)
{
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    Exchange::IDeviceSettingsHDMIIn::HDMIInEdidVersion version = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInEdidVersion>(edidVersion);
    
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->SetHDMIEdidVersion(port, version);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsGetAllmStatus(dsHdmiInPort_t iHdmiPort, bool* allmStatus)
{
    if (allmStatus == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: allmStatus is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->GetHDMIInAllmStatus(port, *allmStatus);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsGetEdid2AllmSupport(dsHdmiInPort_t iHdmiPort, bool* allmSupport)
{
    if (allmSupport == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: allmSupport is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->GetHDMIInEdid2AllmSupport(port, *allmSupport);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsSetEdid2AllmSupport(dsHdmiInPort_t iHdmiPort, bool allmSupport)
{
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->SetHDMIInEdid2AllmSupport(port, allmSupport);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsGetAVLatency(int* audio_latency, int* video_latency)
{
    if (audio_latency == NULL || video_latency == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    uint32_t videoLatency = 0;
    uint32_t audioLatency = 0;
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->GetHDMIInAVLatency(videoLatency, audioLatency);
    ConnectionManager::Unlock();
    
    if (result == WPEFramework::Core::ERROR_NONE) {
        *audio_latency = static_cast<int>(audioLatency);
        *video_latency = static_cast<int>(videoLatency);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsGetHdmiVersion(dsHdmiInPort_t iHdmiPort, dsHdmiMaxCapabilityVersion_t* capversion)
{
    if (capversion == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: capversion is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    Exchange::IDeviceSettingsHDMIIn::HDMIInCapabilityVersion capabilityVersion;
    
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->GetHDMIVersion(port, capabilityVersion);
    ConnectionManager::Unlock();
    
    if (result == WPEFramework::Core::ERROR_NONE) {
        *capversion = static_cast<dsHdmiMaxCapabilityVersion_t>(capabilityVersion);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsHdmiInSetVRRSupport(dsHdmiInPort_t iHdmiPort, bool vrrSupport)
{
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->SetVRRSupport(port, vrrSupport);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsHdmiInGetVRRSupport(dsHdmiInPort_t iHdmiPort, bool* vrrSupport)
{
    if (vrrSupport == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: vrrSupport is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->GetVRRSupport(port, *vrrSupport);
    ConnectionManager::Unlock();
    
    return ConvertThunderError(result);
}

dsError_t dsHdmiInGetVRRStatus(dsHdmiInPort_t iHdmiPort, dsHdmiInVrrStatus_t* vrrStatus)
{
    if (vrrStatus == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: vrrStatus is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    ConnectionManager* connMgr = ConnectionManager::Instance();
    if (!connMgr || !connMgr->IsOperational()) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = connMgr->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    Exchange::IDeviceSettingsHDMIIn::HDMIInVRRStatus status;
    
    ConnectionManager::Lock();
    uint32_t result = hdmiInInterface->GetVRRStatus(port, status);
    ConnectionManager::Unlock();
    
    if (result == WPEFramework::Core::ERROR_NONE) {
        vrrStatus->vrrType = static_cast<dsVRRType_t>(status.vrrType);
        vrrStatus->vrrAmdfreesyncFramerate_Hz = status.vrrFreeSyncFramerateHz;
    }
    
    return ConvertThunderError(result);
}

dsError_t dsGetSupportedGameFeaturesList(dsSupportedGameFeatureList_t* feature)
{
    if (feature == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: feature is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    // This function is not yet implemented in Thunder COM-RPC mode
    // For now, return empty list
    feature->gameFeatureCount = 0;
    feature->gameFeatureList[0] = '\0';
    fprintf(stderr, "[dsHdmiIn-com] dsGetSupportedGameFeaturesList not fully implemented in Thunder mode\n");
    return dsERR_OPERATION_NOT_SUPPORTED;
}

// Placeholder for unsupported APIs in Thunder mode
dsError_t dsHdmiInRegisterConnectCB(dsHdmiInConnectCB_t CBFunc)
{
    fprintf(stderr, "[dsHdmiIn-com] dsHdmiInRegisterConnectCB not supported in Thunder mode\n");
    return dsERR_OPERATION_NOT_SUPPORTED;
}

dsError_t dsHdmiInRegisterSignalChangeCB(dsHdmiInSignalChangeCB_t CBFunc)
{
    fprintf(stderr, "[dsHdmiIn-com] dsHdmiInRegisterSignalChangeCB not supported in Thunder mode\n");
    return dsERR_OPERATION_NOT_SUPPORTED;
}

dsError_t dsHdmiInRegisterStatusChangeCB(dsHdmiInStatusChangeCB_t CBFunc)
{
    fprintf(stderr, "[dsHdmiIn-com] dsHdmiInRegisterStatusChangeCB not supported in Thunder mode\n");
    return dsERR_OPERATION_NOT_SUPPORTED;
}

dsError_t dsHdmiInRegisterVideoModeUpdateCB(dsHdmiInVideoModeUpdateCB_t CBFunc)
{
    fprintf(stderr, "[dsHdmiIn-com] dsHdmiInRegisterVideoModeUpdateCB not supported in Thunder mode\n");
    return dsERR_OPERATION_NOT_SUPPORTED;
}

dsError_t dsHdmiInRegisterAllmChangeCB(dsHdmiInAllmChangeCB_t CBFunc)
{
    fprintf(stderr, "[dsHdmiIn-com] dsHdmiInRegisterAllmChangeCB not supported in Thunder mode\n");
    return dsERR_OPERATION_NOT_SUPPORTED;
}

dsError_t dsHdmiInRegisterAVLatencyChangeCB(dsAVLatencyChangeCB_t CBFunc)
{
    fprintf(stderr, "[dsHdmiIn-com] dsHdmiInRegisterAVLatencyChangeCB not supported in Thunder mode\n");
    return dsERR_OPERATION_NOT_SUPPORTED;
}

dsError_t dsHdmiInRegisterAviContentTypeChangeCB(dsHdmiInAviContentTypeChangeCB_t CBFunc)
{
    fprintf(stderr, "[dsHdmiIn-com] dsHdmiInRegisterAviContentTypeChangeCB not supported in Thunder mode\n");
    return dsERR_OPERATION_NOT_SUPPORTED;
}

} // extern "C"

#endif // USE_WPE_THUNDER_PLUGIN

/** @} */
/** @} */
