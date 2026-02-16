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

#ifndef _DS_VIDEO_PORT_CONFIG_H_
#define _DS_VIDEO_PORT_CONFIG_H_

#include "dsTypes.h"
//#include "dsVideoPortSettings.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/**
 * @brief Load video output port configuration
 * 
 * @param[in] dynamicVideoPortConfigs Pointer to dynamic video port configuration, or NULL for static config
 */
void dsLoadVideoOutputPortConfig(const videoPortConfigs_t* dynamicVideoPortConfigs);

/**
 * @brief Get video port type configurations
 * 
 * @param[out] outConfigSize Pointer to store the number of video port type configs, or NULL
 * @param[out] outConfigs Pointer to store the video port type configs array, or NULL
 */
void dsGetVideoPortTypeConfigs(int* outConfigSize, const dsVideoPortTypeConfig_t** outConfigs);

/**
 * @brief Get video port port configurations
 * 
 * @param[out] outPortSize Pointer to store the number of video port port configs, or NULL
 * @param[out] outPorts Pointer to store the video port port configs array, or NULL
 */
void dsGetVideoPortPortConfigs(int* outPortSize, const dsVideoPortPortConfig_t** outPorts);

/**
 * @brief Get video port resolutions
 * 
 * @param[out] outResolutionSize Pointer to store the number of video port resolutions, or NULL
 * @param[out] outResolutions Pointer to store the video port resolutions array, or NULL
 */
void dsGetVideoPortResolutions(int* outResolutionSize, dsVideoPortResolution_t** outResolutions);

/**
 * @brief Get default resolution index
 * 
 * @param[out] outDefaultIndex Pointer to store the default resolution index, or NULL
 */
void dsGetDefaultResolutionIndex(int* outDefaultIndex);

#ifdef __cplusplus
}
#endif

#endif /* _DS_VIDEO_PORT_CONFIG_H_ */

/** @} */
/** @} */
