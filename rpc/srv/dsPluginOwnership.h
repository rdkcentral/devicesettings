/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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

#ifndef DS_PLUGIN_OWNERSHIP_H_
#define DS_PLUGIN_OWNERSHIP_H_

#ifndef __cplusplus
#include <stdbool.h>
#endif

/*
 * When DEVICESETTINGS_PLUGIN_ENABLED is defined the DeviceSettings plugin is the
 * only owner of the DS HAL. The legacy dsMgr must then not initialise/terminate
 * the HAL or register HAL callbacks, otherwise two processes drive the same
 * hardware (e.g. HDMI ARC breaks on MTK).
 */
#ifdef DEVICESETTINGS_PLUGIN_ENABLED
static const bool dsMgrPluginEnabled = true;
#else
static const bool dsMgrPluginEnabled = false;
#endif

#endif /* DS_PLUGIN_OWNERSHIP_H_ */
