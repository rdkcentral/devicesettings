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

#ifndef _DS_VIDEO_DEVICE_CONFIG_H_
#define _DS_VIDEO_DEVICE_CONFIG_H_

#include "dsTypes.h"
//#include "dsVideoDeviceSettings.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct videoDeviceConfig
{
	dsVideoConfig_t *pKVideoDeviceConfigs;
	int *pKVideoDeviceConfigs_size;
}videoDeviceConfig_t;

/**
 * @brief Load video device configuration
 * 
 * @param[in] dynamicVideoDeviceConfigs Pointer to dynamic video device configuration, or NULL for static config
 */
void dsLoadVideoDeviceConfig(const videoDeviceConfig_t* dynamicVideoDeviceConfigs);

/**
 * @brief Get video device configurations
 * 
 * @param[out] outConfigSize Pointer to store the number of video device configs, or NULL
 * @param[out] outConfigs Pointer to store the video device configs array, or NULL
 */
void dsGetVideoDeviceConfigs(int* outConfigSize, dsVideoConfig_t** outConfigs);

#ifdef __cplusplus
}
#endif

#endif /* _DS_VIDEO_DEVICE_CONFIG_H_ */

/** @} */
/** @} */
