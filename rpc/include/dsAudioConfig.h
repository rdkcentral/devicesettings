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

#ifndef _DS_AUDIO_CONFIG_H_
#define _DS_AUDIO_CONFIG_H_

#include "dsTypes.h"
//#include "dsAudioSettings.h"

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

/**
 * @brief Load audio output port configuration
 * 
 * @param[in] dynamicAudioConfigs Pointer to dynamic audio configuration, or NULL for static config
 */
void dsLoadAudioOutputPortConfig(const audioConfigs_t* dynamicAudioConfigs);

/**
 * @brief Get audio type configurations
 * 
 * @param[out] outConfigSize Pointer to store the number of audio type configs, or NULL
 * @param[out] outConfigs Pointer to store the audio type configs array, or NULL
 */
void dsGetAudioTypeConfigs(int* outConfigSize, const dsAudioTypeConfig_t** outConfigs);

/**
 * @brief Get audio port configurations
 * 
 * @param[out] outPortSize Pointer to store the number of audio port configs, or NULL
 * @param[out] outPorts Pointer to store the audio port configs array, or NULL
 */
void dsGetAudioPortConfigs(int* outPortSize, const dsAudioPortConfig_t** outPorts);

#ifdef __cplusplus
}
#endif

#endif /* _DS_AUDIO_CONFIG_H_ */

/** @} */
/** @} */
