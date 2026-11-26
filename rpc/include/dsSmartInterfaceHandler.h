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
 * @file dsSmartInterfaceHandler.h
 * @brief C bridge used by libds / libdshalcli (non-Thunder clients such as the
 *        Widevine CDM) to reach the DeviceSettings plugin over COM-RPC instead
 *        of the legacy dsMgr IARM calls.
 *
 * Built only when DEVICESETTINGS_PLUGIN_ENABLED is defined. The implementation
 * (dsSmartInterfaceHandler.cpp) is a RPC::SmartInterfaceType<Exchange::IDeviceSettings>
 * client, same model as PowerController, so plugin activation / deactivation
 * is tracked automatically.
 *
 * Logging: lines are tagged "[DSSmartInterface][<tid>] <function>:" and go through the libds client
 * logger (dsclientlogger.h). Lifecycle, connection, plugin activation and handle changes are logged
 * at INFO / WARN, failed calls at ERROR. Set the DS_SMART_INTERFACE_TRACE environment variable to
 * also log one line per COM-RPC call with its duration.
 *
 * The handles returned here are stable tokens owned by this module, not the
 * plugin's raw handles: they are transparently re-resolved against the plugin
 * after a plugin / Thunder restart, so handles cached by libds users stay valid.
 */

#ifndef DS_SMART_INTERFACE_HANDLER_H_
#define DS_SMART_INTERFACE_HANDLER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dsError.h"
#include "dsTypes.h"
#include "dsDisplay.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Lifecycle, driven by device::Manager::Initialize() / DeInitialize().
 *
 * dsSmartInterface_Start opens the COM-RPC channel to Thunder and starts the PID file
 * monitor that re-establishes the channel when Thunder restarts. It succeeds even
 * if Thunder / the plugin is not up yet (the monitor keeps retrying); use
 * dsSmartInterface_Init to find out whether the plugin is reachable.
 * dsSmartInterface_Stop stops the monitor and closes the channel. Both are idempotent. */
dsError_t dsSmartInterface_Start(void);
dsError_t dsSmartInterface_Stop(void);

/* Readiness check: dsERR_NONE once the DeviceSettings plugin is activated,
 * dsERR_INVALID_STATE while it is not (callers retry), dsERR_NOT_INITIALIZED
 * if dsSmartInterface_Start was not called. */
dsError_t dsSmartInterface_Init(void);

/* ---- Implemented: everything the Widevine CDM needs from libds -------------------------------
 * VideoOutputPort construction / state (dsGetVideoPort, enabled, display connected, active), HDCP
 * status and protocol versions, HDMI preference, and the display EDID read that
 * device::Manager::Initialize() does while loading the video port configuration. */
dsError_t dsSmartInterface_GetVideoPort(dsVideoPortType_t type, int index, intptr_t* handle);
dsError_t dsSmartInterface_IsVideoPortEnabled(intptr_t handle, bool* enabled);
dsError_t dsSmartInterface_EnableVideoPort(intptr_t handle, bool enabled);
dsError_t dsSmartInterface_IsDisplayConnected(intptr_t handle, bool* connected);
dsError_t dsSmartInterface_IsVideoPortActive(intptr_t handle, bool* active);
dsError_t dsSmartInterface_IsHDCPEnabled(intptr_t handle, bool* enabled);
dsError_t dsSmartInterface_GetHDCPStatus(intptr_t handle, dsHdcpStatus_t* status);
dsError_t dsSmartInterface_GetHDCPProtocol(intptr_t handle, dsHdcpProtocolVersion_t* version);
dsError_t dsSmartInterface_GetHDCPReceiverProtocol(intptr_t handle, dsHdcpProtocolVersion_t* version);
dsError_t dsSmartInterface_GetHDCPCurrentProtocol(intptr_t handle, dsHdcpProtocolVersion_t* version);
dsError_t dsSmartInterface_GetHdmiPreference(intptr_t handle, dsHdcpProtocolVersion_t* version);
dsError_t dsSmartInterface_SetHdmiPreference(intptr_t handle, dsHdcpProtocolVersion_t version);
dsError_t dsSmartInterface_GetDisplay(dsVideoPortType_t type, int index, intptr_t* handle);
dsError_t dsSmartInterface_GetEDID(intptr_t handle, dsDisplayEDID_t* edid);

#ifdef __cplusplus
}
#endif

#endif /* DS_SMART_INTERFACE_HANDLER_H_ */
