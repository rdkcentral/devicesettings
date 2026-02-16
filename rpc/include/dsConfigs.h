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

#ifndef _DS_CONFIGS_H_
#define _DS_CONFIGS_H_

#include "dsTypes.h"
//#include "dsAudioSettings.h"
//#include "dsVideoPortSettings.h"
//#include "dsVideoDeviceSettings.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct audioConfigs
{
	const dsAudioTypeConfig_t  *pKAudioConfigs;
	const dsAudioPortConfig_t  *pKAudioPorts;
	int *pKConfigSize;
	int *pKPortSize;
}audioConfigs_t;

typedef struct videoPortConfigs
{
	const dsVideoPortTypeConfig_t  *pKVideoPortConfigs;
	int *pKVideoPortConfigs_size;
	const dsVideoPortPortConfig_t  *pKVideoPortPorts;
	int *pKVideoPortPorts_size;
	dsVideoPortResolution_t *pKVideoPortResolutionsSettings;
	int *pKResolutionsSettings_size;
	int *pKDefaultResIndex;
}videoPortConfigs_t;

typedef struct videoDeviceConfig
{
	dsVideoConfig_t *pKVideoDeviceConfigs;
	int *pKVideoDeviceConfigs_size;
}videoDeviceConfig_t;

/**
 * @brief Load all device settings configurations
 * 
 * Loads audio, video port, and video device configurations from HAL library
 */
void dsLoadConfigs(void);

#ifdef __cplusplus
}
#endif

#endif /* _DS_CONFIGS_H_ */

/** @} */
/** @} */
