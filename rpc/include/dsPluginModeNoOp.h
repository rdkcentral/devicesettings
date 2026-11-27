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
 * @file dsPluginModeNoOp.h
 * @brief In DeviceSettings plugin mode (DEVICESETTINGS_PLUGIN_ENABLED) the legacy dsMgr no longer serves the DS
 *        HAL, so a dsXxx client call that is not routed to the plugin (dsSmartInterfaceHandler.h) has nobody to
 *        talk to over IARM. Such calls must neither fail nor throw in libds: this header redirects the
 *        IARM_Bus_Call() of the rpc/cli files to a no-op that returns success and zeroes the call parameter,
 *        so every output is 0 / false / dsERR_NONE.
 *
 * Include it after libIBus.h, in the files that call IARM_Bus_Call(). The calls that ARE routed to the plugin
 * return before reaching IARM_Bus_Call(), so they are not affected.
 */

#ifndef DS_PLUGIN_MODE_NOOP_H_
#define DS_PLUGIN_MODE_NOOP_H_

#ifdef DEVICESETTINGS_PLUGIN_ENABLED

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Logs the method once (INFO), zeroes arg[0..len) and returns 0 (IARM_RESULT_SUCCESS). Never throws. */
int dsSmartInterface_NoOpCall(const char* method, void* arg, size_t len);

#ifdef __cplusplus
}
#endif

#define IARM_Bus_Call(owner, method, arg, len) ((IARM_Result_t)dsSmartInterface_NoOpCall((method), (arg), (len)))

#endif /* DEVICESETTINGS_PLUGIN_ENABLED */

#endif /* DS_PLUGIN_MODE_NOOP_H_ */
