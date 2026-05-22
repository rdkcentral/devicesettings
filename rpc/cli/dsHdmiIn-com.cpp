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

#include "dsHdmiIn.h"
#include "dsError.h"
#include "dsclientlogger.h"
#include "dsController-com.h"

using namespace WPEFramework;

// Thread safety lock for HdmiIn interface calls
static Core::CriticalSection _hdmiInLock;

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
 * @brief Thread-safe wrapper functions for HdmiIn interface calls
 */
namespace HdmiInWrapper {

inline uint32_t GetHDMIInNumbefOfInputs(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface, 
                                        int32_t& count)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->GetHDMIInNumbefOfInputs(count);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t GetHDMIInStatus(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                                Exchange::IDeviceSettingsHDMIIn::HDMIInStatus& hdmiStatus,
                                Exchange::IDeviceSettingsHDMIIn::IHDMIInPortConnectionStatusIterator*& portConnectionStatus)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->GetHDMIInStatus(hdmiStatus, portConnectionStatus);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t SelectHDMIInPort(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                                 Exchange::IDeviceSettingsHDMIIn::HDMIInPort port,
                                 bool requestAudioMix,
                                 bool topMostPlane,
                                 Exchange::IDeviceSettingsHDMIIn::HDMIVideoPlaneType videoPlaneType)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->SelectHDMIInPort(port, requestAudioMix, topMostPlane, videoPlaneType);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t ScaleHDMIInVideo(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                                 const Exchange::IDeviceSettingsHDMIIn::HDMIInVideoRectangle& videoPosition)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->ScaleHDMIInVideo(videoPosition);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t SelectHDMIZoomMode(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                                   Exchange::IDeviceSettingsHDMIIn::HDMIInVideoZoom zoomMode)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->SelectHDMIZoomMode(zoomMode);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t GetHDMIVideoMode(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                                 Exchange::IDeviceSettingsHDMIIn::HDMIVideoPortResolution& videoPortResolution)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->GetHDMIVideoMode(videoPortResolution);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t GetEdidBytes(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                             Exchange::IDeviceSettingsHDMIIn::HDMIInPort port,
                             uint16_t edidBytesLength,
                             uint8_t edidBytes[])
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->GetEdidBytes(port, edidBytesLength, edidBytes);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t GetHDMISPDInformation(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                                      Exchange::IDeviceSettingsHDMIIn::HDMIInPort port,
                                      uint16_t spdBytesLength,
                                      uint8_t spdBytes[])
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->GetHDMISPDInformation(port, spdBytesLength, spdBytes);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t SetHDMIEdidVersion(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                                   Exchange::IDeviceSettingsHDMIIn::HDMIInPort port,
                                   Exchange::IDeviceSettingsHDMIIn::HDMIInEdidVersion edidVersion)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->SetHDMIEdidVersion(port, edidVersion);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t GetHDMIEdidVersion(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                                   Exchange::IDeviceSettingsHDMIIn::HDMIInPort port,
                                   Exchange::IDeviceSettingsHDMIIn::HDMIInEdidVersion& edidVersion)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->GetHDMIEdidVersion(port, edidVersion);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t GetHDMIInAllmStatus(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                                    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port,
                                    bool& allmStatus)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->GetHDMIInAllmStatus(port, allmStatus);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t GetSupportedGameFeaturesList(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                                             Exchange::IDeviceSettingsHDMIIn::IHDMIInGameFeatureListIterator*& gameFeatureList)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->GetSupportedGameFeaturesList(gameFeatureList);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t GetHDMIInAVLatency(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                                   uint32_t& videoLatency,
                                   uint32_t& audioLatency)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->GetHDMIInAVLatency(videoLatency, audioLatency);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t SetHDMIInEdid2AllmSupport(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                                          Exchange::IDeviceSettingsHDMIIn::HDMIInPort port,
                                          bool allmSupport)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->SetHDMIInEdid2AllmSupport(port, allmSupport);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t GetHDMIInEdid2AllmSupport(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                                          Exchange::IDeviceSettingsHDMIIn::HDMIInPort port,
                                          bool& allmSupport)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->GetHDMIInEdid2AllmSupport(port, allmSupport);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t SetVRRSupport(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                              Exchange::IDeviceSettingsHDMIIn::HDMIInPort port,
                              bool vrrSupport)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->SetVRRSupport(port, vrrSupport);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t GetVRRSupport(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                              Exchange::IDeviceSettingsHDMIIn::HDMIInPort port,
                              bool& vrrSupport)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->GetVRRSupport(port, vrrSupport);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t GetVRRStatus(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                             Exchange::IDeviceSettingsHDMIIn::HDMIInPort port,
                             Exchange::IDeviceSettingsHDMIIn::HDMIInVRRStatus& vrrStatus)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->GetVRRStatus(port, vrrStatus);
    _hdmiInLock.Unlock();
    return result;
}

inline uint32_t GetHDMIVersion(Exchange::IDeviceSettingsHDMIIn* hdmiInInterface,
                               Exchange::IDeviceSettingsHDMIIn::HDMIInPort port,
                               Exchange::IDeviceSettingsHDMIIn::HDMIInCapabilityVersion& capabilityVersion)
{
    _hdmiInLock.Lock();
    uint32_t result = hdmiInInterface->GetHDMIVersion(port, capabilityVersion);
    _hdmiInLock.Unlock();
    return result;
}

} // namespace HdmiInWrapper

// C API implementations using Thunder COM-RPC

extern "C" {

dsError_t dsHdmiInInit(void)
{
    printf("<<<<< HDMI Input is initialized in Thunder Mode >>>>>>>>\r\n");
    
    // DeviceSettingsController is initialized by manager.cpp
    // Just verify we have HdmiIn interface access
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        fprintf(stderr, "[dsHdmiIn-com] DeviceSettingsController not initialized\n");
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        fprintf(stderr, "[dsHdmiIn-com] HDMIIn interface not available\n");
        return dsERR_GENERAL;
    }
    
    return dsERR_NONE;
}

dsError_t dsHdmiInTerm(void)
{
    // DeviceSettingsController cleanup handled by manager.cpp
    // No component-specific cleanup needed
    return dsERR_NONE;
}

dsError_t dsHdmiInGetNumberOfInputs(uint8_t *pNumHdmiInputs)
{
    if (pNumHdmiInputs == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: pNumHdmiInputs is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    int32_t count = 0;
    uint32_t result = HdmiInWrapper::GetHDMIInNumbefOfInputs(hdmiInInterface, count);
    
    if (result == Core::ERROR_NONE) {
        *pNumHdmiInputs = static_cast<uint8_t>(count);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsHdmiInGetStatus(dsHdmiInStatus_t *pStatus)
{
    if (pStatus == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: pStatus is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInStatus hdmiStatus;
    Exchange::IDeviceSettingsHDMIIn::IHDMIInPortConnectionStatusIterator* portConnectionStatus = nullptr;
    
    uint32_t result = HdmiInWrapper::GetHDMIInStatus(hdmiInInterface, hdmiStatus, portConnectionStatus);
    
    if (result == Core::ERROR_NONE) {
        pStatus->isPresented = hdmiStatus.isPresented;
        pStatus->activePort = static_cast<dsHdmiInPort_t>(hdmiStatus.activePort);
        
        // Populate port connection status
        if (portConnectionStatus != nullptr) {
            Exchange::IDeviceSettingsHDMIIn::HDMIPortConnectionStatus portStatus;
            uint8_t portIndex = 0;
            
            while (portConnectionStatus->Next(portStatus) && portIndex < dsHDMI_IN_PORT_MAX) {
                pStatus->isPortConnected[portIndex] = portStatus.isPortConnected;
                portIndex++;
            }
            
            portConnectionStatus->Release();
        }
    }
    
    return ConvertThunderError(result);
}

dsError_t dsHdmiInSelectPort(dsHdmiInPort_t ePort, bool audioMix, dsVideoPlaneType_t evideoPlaneType, bool topMost)
{
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(ePort);
    
    Exchange::IDeviceSettingsHDMIIn::HDMIVideoPlaneType planeType = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIVideoPlaneType>(evideoPlaneType);
    
    uint32_t result = HdmiInWrapper::SelectHDMIInPort(hdmiInInterface, port, audioMix, topMost, planeType);
    return ConvertThunderError(result);
}

dsError_t dsHdmiInScaleVideo(int32_t x, int32_t y, int32_t width, int32_t height)
{
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInVideoRectangle videoPosition;
    videoPosition.x = x;
    videoPosition.y = y;
    videoPosition.width = width;
    videoPosition.height = height;
    
    uint32_t result = HdmiInWrapper::ScaleHDMIInVideo(hdmiInInterface, videoPosition);
    return ConvertThunderError(result);
}

dsError_t dsHdmiInSelectZoomMode(dsVideoZoom_t requestedZoomMode)
{
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInVideoZoom zoomMode = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInVideoZoom>(requestedZoomMode);
    
    uint32_t result = HdmiInWrapper::SelectHDMIZoomMode(hdmiInInterface, zoomMode);
    return ConvertThunderError(result);
}

dsError_t dsHdmiInGetCurrentVideoMode(dsVideoPortResolution_t *resolution)
{
    if (resolution == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: resolution is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIVideoPortResolution videoPortResolution;
    
    uint32_t result = HdmiInWrapper::GetHDMIVideoMode(hdmiInInterface, videoPortResolution);
    
    if (result == Core::ERROR_NONE) {
        // Copy resolution name
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

dsError_t dsGetEDIDBytesInfo(dsHdmiInPort_t iHdmiPort, unsigned char *edid, int *length)
{
    if (edid == NULL || length == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: edid or length is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    uint16_t edidBytesLength = static_cast<uint16_t>(*length);
    
    uint32_t result = HdmiInWrapper::GetEdidBytes(hdmiInInterface, port, edidBytesLength, edid);
    
    if (result == Core::ERROR_NONE) {
        printf("[dsHdmiIn-com] dsGetEDIDBytesInfo: port=%d, length=%d\n", iHdmiPort, *length);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsGetHDMISPDInfo(dsHdmiInPort_t iHdmiPort, unsigned char *spdInfo)
{
    if (spdInfo == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: spdInfo is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    // Typical SPD info frame size
    uint16_t spdBytesLength = 32;
    
    uint32_t result = HdmiInWrapper::GetHDMISPDInformation(hdmiInInterface, port, spdBytesLength, spdInfo);
    
    if (result == Core::ERROR_NONE) {
        printf("[dsHdmiIn-com] dsGetHDMISPDInfo: port=%d\n", iHdmiPort);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsSetEdidVersion(dsHdmiInPort_t iHdmiPort, tv_hdmi_edid_version_t iEdidVersion)
{
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInEdidVersion edidVersion = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInEdidVersion>(iEdidVersion);
    
    uint32_t result = HdmiInWrapper::SetHDMIEdidVersion(hdmiInInterface, port, edidVersion);
    return ConvertThunderError(result);
}

dsError_t dsGetEdidVersion(dsHdmiInPort_t iHdmiPort, tv_hdmi_edid_version_t *iEdidVersion)
{
    if (iEdidVersion == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: iEdidVersion is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInEdidVersion edidVersion;
    
    uint32_t result = HdmiInWrapper::GetHDMIEdidVersion(hdmiInInterface, port, edidVersion);
    
    if (result == Core::ERROR_NONE) {
        *iEdidVersion = static_cast<tv_hdmi_edid_version_t>(edidVersion);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsGetAllmStatus(dsHdmiInPort_t iHdmiPort, bool *allmStatus)
{
    if (allmStatus == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: allmStatus is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    bool status = false;
    uint32_t result = HdmiInWrapper::GetHDMIInAllmStatus(hdmiInInterface, port, status);
    
    if (result == Core::ERROR_NONE) {
        *allmStatus = status;
    }
    
    return ConvertThunderError(result);
}

dsError_t dsGetSupportedGameFeaturesList(dsSupportedGameFeatureList_t *feature)
{
    if (feature == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: feature is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::IHDMIInGameFeatureListIterator* gameFeatureList = nullptr;
    
    uint32_t result = HdmiInWrapper::GetSupportedGameFeaturesList(hdmiInInterface, gameFeatureList);
    
    if (result == Core::ERROR_NONE && gameFeatureList != nullptr) {
        Exchange::IDeviceSettingsHDMIIn::HDMIInGameFeatureList gameFeature;
        feature->gameFeatureCount = 0;
        feature->gameFeatureList[0] = '\0';
        
        while (gameFeatureList->Next(gameFeature)) {
            if (feature->gameFeatureCount > 0) {
                strncat(feature->gameFeatureList, ",", MAX_FEATURE_LIST_BUFFER_LEN - strlen(feature->gameFeatureList) - 1);
            }
            strncat(feature->gameFeatureList, gameFeature.gameFeature.c_str(), 
                    MAX_FEATURE_LIST_BUFFER_LEN - strlen(feature->gameFeatureList) - 1);
            feature->gameFeatureCount++;
        }
        
        gameFeatureList->Release();
    }
    
    return ConvertThunderError(result);
}

dsError_t dsGetAVLatency(int *audio_latency, int *video_latency)
{
    if (audio_latency == NULL || video_latency == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: audio_latency or video_latency is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    uint32_t videoLatency = 0;
    uint32_t audioLatency = 0;
    
    uint32_t result = HdmiInWrapper::GetHDMIInAVLatency(hdmiInInterface, videoLatency, audioLatency);
    
    if (result == Core::ERROR_NONE) {
        *video_latency = static_cast<int>(videoLatency);
        *audio_latency = static_cast<int>(audioLatency);
        printf("[dsHdmiIn-com] AVLatency: video=%d, audio=%d\n", *video_latency, *audio_latency);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsSetEdid2AllmSupport(dsHdmiInPort_t iHdmiPort, bool allm_support)
{
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    uint32_t result = HdmiInWrapper::SetHDMIInEdid2AllmSupport(hdmiInInterface, port, allm_support);
    return ConvertThunderError(result);
}

dsError_t dsGetEdid2AllmSupport(dsHdmiInPort_t iHdmiPort, bool *allm_support)
{
    if (allm_support == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: allm_support is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    bool support = false;
    uint32_t result = HdmiInWrapper::GetHDMIInEdid2AllmSupport(hdmiInInterface, port, support);
    
    if (result == Core::ERROR_NONE) {
        *allm_support = support;
    }
    
    return ConvertThunderError(result);
}

dsError_t dsHdmiInSetVRRSupport(dsHdmiInPort_t iHdmiPort, bool vrr_support)
{
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    uint32_t result = HdmiInWrapper::SetVRRSupport(hdmiInInterface, port, vrr_support);
    return ConvertThunderError(result);
}

dsError_t dsHdmiInGetVRRSupport(dsHdmiInPort_t iHdmiPort, bool *vrr_support)
{
    if (vrr_support == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: vrr_support is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    bool support = false;
    uint32_t result = HdmiInWrapper::GetVRRSupport(hdmiInInterface, port, support);
    
    if (result == Core::ERROR_NONE) {
        *vrr_support = support;
    }
    
    return ConvertThunderError(result);
}

dsError_t dsHdmiInGetVRRStatus(dsHdmiInPort_t iHdmiPort, dsHdmiInVrrStatus_t *vrrStatus)
{
    if (vrrStatus == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: vrrStatus is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInVRRStatus status;
    
    uint32_t result = HdmiInWrapper::GetVRRStatus(hdmiInInterface, port, status);
    
    if (result == Core::ERROR_NONE) {
        vrrStatus->vrrType = static_cast<dsVRRType_t>(status.vrrType);
        vrrStatus->vrrAmdfreesyncFramerate_Hz = static_cast<uint32_t>(status.vrrFreeSyncFramerateHz);
    }
    
    return ConvertThunderError(result);
}

dsError_t dsGetHdmiVersion(dsHdmiInPort_t iHdmiPort, dsHdmiMaxCapabilityVersion_t *capversion)
{
    if (capversion == NULL) {
        fprintf(stderr, "[dsHdmiIn-com] Invalid parameter: capversion is NULL\n");
        return dsERR_INVALID_PARAM;
    }
    
    DeviceSettingsController* controller = DeviceSettingsController::GetInstance();
    if (!controller) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn* hdmiInInterface = controller->GetHDMIInInterface();
    if (!hdmiInInterface) {
        return dsERR_GENERAL;
    }
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInPort port = 
        static_cast<Exchange::IDeviceSettingsHDMIIn::HDMIInPort>(iHdmiPort);
    
    Exchange::IDeviceSettingsHDMIIn::HDMIInCapabilityVersion capabilityVersion;
    
    uint32_t result = HdmiInWrapper::GetHDMIVersion(hdmiInInterface, port, capabilityVersion);
    
    if (result == Core::ERROR_NONE) {
        *capversion = static_cast<dsHdmiMaxCapabilityVersion_t>(capabilityVersion);
    }
    
    return ConvertThunderError(result);
}

} // extern "C"

#endif // USE_WPE_THUNDER_PLUGIN

/** @} */
/** @} */
