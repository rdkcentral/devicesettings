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
 
#ifndef __DS_INTERNAL_H__
#define __DS_INTERNAL_H__

/**
* @defgroup devicesettings
* @{
* @defgroup dsInternal
* @{
**/

#include "dsTypes.h"
#include "dsDisplay.h"
#include "dsError.h"
#include "dsRpc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RDK_PROFILE "RDK_PROFILE"
#define PROFILE_STR_TV "TV"
#define PROFILE_STR_STB "STB"


#define DS_HDMI_STATUS_FILE "/tmp/ds_hdmi_status.bin"
#define DS_HDMI_TAG_HOTPLUP "hotplug"
#define DS_HDMI_TAG_HDCPSTATUS "hdcp_status"
#define DS_HDMI_TAG_HDCPVERSION "hdcp_version"

typedef enum profile {
    PROFILE_INVALID = -1,
    PROFILE_STB = 0,
    PROFILE_TV,
    PROFILE_MAX
}profile_t;

extern profile_t profileType;

struct HdmiEnumToStrMapping
{
    int tag;
    const char *name;
};

static struct HdmiEnumToStrMapping HdmiConnectionToStrMapping [] = {
    {dsDISPLAY_EVENT_CONNECTED, "CONNECTED"},
    {dsDISPLAY_EVENT_DISCONNECTED, "DISCONNECTED"},
	{ 0,  0}
};

static struct HdmiEnumToStrMapping HdmiStatusToStrMapping [] = {
    {dsHDCP_STATUS_UNPOWERED, "UNPOWERED"},
    {dsHDCP_STATUS_UNAUTHENTICATED, "UNAUTHENTICATED"},
    {dsHDCP_STATUS_AUTHENTICATED, "AUTHENTICATED"},
    {dsHDCP_STATUS_AUTHENTICATIONFAILURE, "AUTHENTICATIONFAILURE"},
    {dsHDCP_STATUS_INPROGRESS, "INPROGRESS"},
    {dsHDCP_STATUS_PORTDISABLED, "PORTDISABLED"},
	{ 0,  0}
};

static struct HdmiEnumToStrMapping HdmiVerToStrMapping [] = {
    {dsHDCP_VERSION_1X, "VERSION_1X"},
    {dsHDCP_VERSION_2X, "VERSION_2X"},
	{ 0,  0}
};

/**
 * @brief Get DS HAL API Version.
 *
 * In 4 byte VersionNumber, Two Most significant Bytes are Major number
 * and Two Least Significant Bytes are minor number.
 *
 * @param[out] versionNumber 4 Bytes of version number of DS HAL
 *
 * @return Returns 4 byte Version Number
 * @retval dsERR_NONE Successfully got the version number from dsHAL.
 * @retval dsERR_GENERAL Failed to get the version number.
 */
dsError_t dsGetVersion(uint32_t *versionNumber);

/**
 * @brief This function returns the preferred sleep mode which is persisted.
 *
 * @param[out] pMode Data will be copied to this. This shall be preallocated before the call.
 * @return Device Settings error code
 * @retval dsERR_NONE If sucessfully dsGetPreferredSleepMode api has been called using IARM support.
 * @retval dsERR_GENERAL General failure.
 */
dsError_t dsGetPreferredSleepMode(dsSleepMode_t *pMode);

/**
 * @brief This function sets the preferred sleep mode which needs to be persisted.
 *
 * @param[in] mode Sleep mode that is expected to be persisted.
 * @return Device Settings error code
 * @retval dsERR_NONE If sucessfully dsSetPreferredSleepMode api has been called using IARM support.
 * @retval dsERR_GENERAL General failure.
 */
dsError_t dsSetPreferredSleepMode(dsSleepMode_t mode);

/**
 * @brief This function is used to get the MS12 config platform supports.
 * @param[out] ms12 config type.
 * @return Device Settings error code
 * @retval dsERR_NONE If sucessfully dsGetMS12ConfigType api has read the ms12 configType from persistance
 * @retval dsERR_GENERAL General failure.
 */
dsError_t dsGetMS12ConfigType(const char *configType);

/**
 * @brief Set the power mode.
 *
 * This function sets the power mode of the host to active or standby and turns on/off
 * all the ouput ports.
 *
 * @param [in] newPower  The power mode of the host (::dsPOWER_STANDBY or ::dsPOWER_ON)
 * @return Device Settings error code
 * @retval    ::dsError_t
 *
 * @note dsPOWER_OFF is not currently being used.
 */
dsError_t dsSetHostPowerMode(int newPower);

/**
 * @brief Get the current power mode.
 *
 * This function gets the current power mode of the host. 
 *
 * @param [out] *currPower  The address of a location to hold the host's current power
 *                          mode on return. It returns one of:
 *                              - ::dsPOWER_OFF
 *                              - ::dsPOWER_STANDBY
 *                              - ::dsPOWER_ON
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t dsGetHostPowerMode(int *currPower);

/**
 * @brief To Set/override a specific audio setting in 
 *  a specific profile
 *
 * This function will override a specific audio setting in a
 * specific profile
 *
 * @param [in] handle       Handle for the Output Audio port
 * @param [in] *profileState possible values ADD and REMOVE setting from the persistence
 * @param [in] *profileName  ProfileName 
 * @param [in] *profileSettingsName MS12 property name
 * @param [in] *profileSettingValue MS12 property value 
 * @return dsError_t Error code.
 */

dsError_t  dsSetMS12AudioProfileSetttingsOverride(intptr_t handle,const char* profileState,const char* profileName,
                                                   const char* profileSettingsName,const char* profileSettingValue);


/**
 * @brief This function will set the brightness of the specified discrete LED on the front
 * panel display to the specified brightness level in multi-app mode using iarmbus call.
 * The brightness level shall be persisted if the input parameter toPersist passed is TRUE.
 *
 * @param[in] eIndicator FPD Indicator index (Power LED, Record LED, and so on).
 * @param[in] eBrightness The brightness value for the specified indicator.
 * @param[in] toPersist If set to TRUE, the brightness value shall be persisted.
 *
 * @return Device Settings error code
 * @retval dsERR_NONE Indicates dsSetFPBrightness API was successfully called using iarmbus call.
 * @retval dsERR_GENERAL Indicates error due to general failure.
 */
dsError_t dsSetFPDBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t eBrightness,bool toPersist);

/**
 * @brief This function will get the brightness of the specified discrete LED on the front
 * panel display.
 * If persist flag is passed as TRUE it will return persisted or platform default brightness otherwise return actial brightness from HAL.
 *
 * @param[in] eIndicator FPD Indicator index (Power LED, Record LED, and so on).
 * @param[out] eBrightness The brightness value for the specified indicator.
 * @param[in] persist If set to TRUE, the brightness value from persistence otherwise actial value from HAL.
 *
 * @return Device Settings error code
 * @retval dsERR_NONE Indicates dsSetFPBrightness API was successfully called using iarmbus call.
 * @retval dsERR_GENERAL Indicates error due to general failure.
 */
dsError_t dsGetFPDBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t *eBrightness,bool persist);

/**
 * @brief This function sets the color of the specified LED on the front panel in
 * multi-app mode using iarmbus call. The color of the LED shall be persisted if the
 * input parameter toPersist is set to TRUE.
 *
 * @param[in] eIndicator FPD Indicator index (Power LED, Record LED and so on).
 * @param[in] eColor Indicates the RGB color to be set for the specified LED.
 * @param[in] toPersist Indicates whether to persist the specified LED color or not.
 * (If TRUE persists the LED color else doesn't persist it)
 *
 * @return Device Settings error code
 * @retval dsERR_NONE Indicates dsSetFPColor API was successfully called using iarmbus call.
 * @retval dsERR_GENERAL Indicates error due to general failure.
 */
dsError_t dsSetFPDColor (dsFPDIndicator_t eIndicator, dsFPDColor_t eColor,bool toPersist);

/**
 * @brief Set video port's display resolution.
 *
 * This function sets the resolution for the video corresponding to the specified port handle.
 *
 * @param [in] handle         Handle of the video port.
 * @param [in] *resolution    The address of a structure containing the video port
 *                            resolution settings.
 * @param [in] persist        In false state allows disabling persist of resolution value.
 * @return Device Settings error code
 * @retval dsERR_NONE If sucessfully dsSetResolution api has been called using IARM support.
 * @retval dsERR_GENERAL General failure.
 */
dsError_t  dsVideoPortSetResolution(intptr_t handle, dsVideoPortResolution_t *resolution, bool persist);

/**
 * @brief To set the preffered color depth mode.
 *
 * This function is used to set the preffered color depth mode.
 *
 * @param [in] handle   Handle for the video port.
 * @param [in] colorDepth color depth value.
 * @param [in] persist  to persist value
 * @return dsError_t Error code.
 */
dsError_t dsVideoPortSetPreferredColorDepth(intptr_t handle,dsDisplayColorDepth_t colorDepth, bool persist );

/**
 * @brief To get the preffered color depth mode.
 *
 * This function is used to get the preffered color depth mode.
 *
 * @param [in] handle   Handle for the video port.
 * @param [out] colorDepth color depth value.
 * @return dsError_t Error code.
 */
dsError_t dsVideoPortGetPreferredColorDepth(intptr_t handle, dsDisplayColorDepth_t *colorDepth, bool persist );

/**
 * @brief Gets the encoding type of an audio port
 *
 * This function returns the current audio encoding setting for the specified audio port.
 *
 * @param[in] handle     -  Handle for the output audio port
 * @param[out] encoding  -  Pointer to hold the encoding setting of the audio port
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 *
 */
dsError_t  dsGetAudioEncoding(intptr_t handle, dsAudioEncoding_t *encoding);

/**
 * @brief Gets the loop-through mode of an audio port.
 *
 * This function is used to check if the audio port is configured for loop-through.
 *
 * @param[in] handle     - Handle for the output audio port
 * @param[out] loopThru  - Status of loop-through feature for the specified audio port
 *                           ( @a true when output is looped through, @a false otherwise)
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t  dsIsAudioLoopThru(intptr_t handle, bool *loopThru);

/**
 * @brief Sets loop-through mode of an audio port.
 *
 * This function enables/disables audio loop-through on the audio port corresponding to the specified port handle.
 *
 * @param[in] handle    - Handle for the output audio port
 * @param[in] loopThru  - Flag to enable/disable loop-through
 *                          ( @a true to enable, @a false to disable)
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t  dsEnableLoopThru(intptr_t handle, bool loopThru);

/**
 * @brief Gets the current audio dB level of an audio port.
 *
 * This function returns the current audio dB level for the audio port corresponding to specified port handle.
 * The Audio dB level ranges from -1450 to 180 dB
 *
 * @param[in] handle  - Handle for the output audio port
 * @param[out] db     - Pointer to hold the Audio dB level of the specified audio port
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t  dsGetAudioDB(intptr_t handle, float *db);

/**
 * @brief Sets the current audio dB level of an audio port.
 *
 * This function sets the dB level to be used on the audio port corresponding to specified port handle.
 * Max dB is 180 and Min dB is -1450
 *
 * @param[in] handle  - Handle for the output audio port
 * @param[in] db      - Audio dB level to be used on the audio port
 *
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t  dsSetAudioDB(intptr_t handle, float db);

/**
 * @brief Gets the maximum audio dB level of an audio port.
 *
 * This function returns the maximum audio dB level supported by the audio port corresponding to specified port handle
 *
 * @param[in] handle  - Handle for the output audio port
 * @param[out] maxDb  - Pointer to hold the maximum audio dB value (float value e.g:10.0) supported by the specified audio port
 *                        Maximum value can be 180 dB
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t  dsGetAudioMaxDB(intptr_t handle, float *maxDb);

/**
 * @brief Gets the minimum audio dB level of an audio port.
 *
 * This function returns the minimum audio dB level supported by the audio port corresponding to specified port handle.
 *
 * @param[in] handle  - Handle for the output audio port
 * @param[out] minDb  - Pointer to hold the minimum audio dB value (float. e.g: 0.0) supported by the specified audio port
 *                        Minimum value can be -1450 dB
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t  dsGetAudioMinDB(intptr_t handle, float *minDb);

/**
 * @brief Gets the optimal audio level of an audio port.
 *
 * This function returns the optimal audio level (dB) of the audio port corresponding to specified port handle
 *
 * @param[in] handle        - Handle for the output audio port
 * @param[out] optimalLevel - Pointer to hold the optimal level value of the specified audio port
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t  dsGetAudioOptimalLevel(intptr_t handle, float *optimalLevel);

/**
 * @brief Sets the encoding type of an audio port
 *
 * This function sets the audio encoding type to be used on the specified audio port.
 *
 * @param[in] handle    - Handle for the output audio port
 * @param[in] encoding  - The encoding type to be used on the audio port
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t  dsSetAudioEncoding(intptr_t handle, dsAudioEncoding_t encoding);

/**
 * @brief Resets the Dialog Enhancement of audio port to default value.
 *
 * This function is used to reset the dialog enhancement of audio port corresponding to the specified port handle to its platform-specific default value.
 *
 * @param[in] handle  - Handle for the output audio port
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t dsResetDialogEnhancement(intptr_t handle);

/**
 * @brief Resets the audio bass enhancer to its default value.
 *
 * This function is used to reset the audio bass enhancer of audio port corresponding to port handle to its platform-specific default bass boost value.
 *
 * @param[in] handle  - Handle for the output audio port
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t dsResetBassEnhancer(intptr_t handle);

/**
 * @brief Resets the audio surround virtualizer level to its default value.
 *
 * This function is used to reset the audio surround virtualizer level of audio port corresponding to port handle to its platform-specific default boost value.
 *
 * @param[in] handle  - Handle for the output audio port
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t dsResetSurroundVirtualizer(intptr_t handle);

/**
 * @brief Resets the Dolby volume leveller of the audio port to its default volume level.
 *
 * This function is used to reset the Dolby volume leveller of audio port corresponding to port handle to its platform-specific default volume level.
 *
 * @param[in] handle  - Handle for the output audio port
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t dsResetVolumeLeveller(intptr_t handle);

/**
 * @brief Sets the audio ducking level of an audio port.
 *
 * This function sets the audio ducking level to be used on the specified audio port based on the audio output mode.
 * If output mode is expert mode, this will mute the audio.
 *
 * @param[in] handle  - Handle for the output audio port
 * @param[in] action  - action type to start or stop ducking. Please refer ::dsAudioDuckingAction_t
 * @param[in] type    - ducking type is absolute or relative to current volume level. Please refer ::dsAudioDuckingType_t
 * @param[in] level   - The volume level value from 0 to 100 to be used on the audio port
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t  dsSetAudioDucking(intptr_t handle, dsAudioDuckingAction_t action, dsAudioDuckingType_t type, const unsigned char level);

/**
 * @brief Gets the audio HDMI ARC port ID for each platform
 *
 * This function will get audio HDMI ARC port ID of platform
 *
 * @param[in] portId  - HDMI ARC port ID
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t dsGetHDMIARCPortId(int *portId);

#ifdef __cplusplus
}
#endif

#endif /* __DS_INTERNAL_H__ */
/** @} */
/** @} */
