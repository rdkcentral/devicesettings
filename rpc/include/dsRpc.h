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


#ifndef _IARM_RPDS_H_
#define _IARM_RPDS_H_


#include "dsTypes.h"
#include "dsError.h"

#ifdef __cplusplus
extern "C" {
#endif



/*
 * Declare RPC dsAudio API names 
 */
#define  IARM_BUS_DSMGR_API_dsAudioPortInit 	"dsAudioPortInit"
#define  IARM_BUS_DSMGR_API_dsGetAudioPort		"dsGetAudioPort"
#define  IARM_BUS_DSMGR_API_dsSetStereoMode		"dsSetStereoMode"
#define  IARM_BUS_DSMGR_API_dsGetStereoMode		"dsGetStereoMode"
#define  IARM_BUS_DSMGR_API_dsSetStereoAuto		"dsSetStereoAuto"
#define  IARM_BUS_DSMGR_API_dsGetStereoAuto		"dsGetStereoAuto"
#define IARM_BUS_DSMGR_API_dsGetEncoding                "dsGetEncoding"
#define  IARM_BUS_DSMGR_API_dsGetAudioFormat            "dsGetAudioFormat"
#define  IARM_BUS_DSMGR_API_dsSetAudioMute		"dsSetAudioMute"
#define  IARM_BUS_DSMGR_API_dsIsAudioMute              "dsIsAudioMute"
#define  IARM_BUS_DSMGR_API_dsIsAudioMSDecode    "dsIsAudioMSDecode"
#define  IARM_BUS_DSMGR_API_dsIsAudioMS12Decode    "dsIsAudioMS12Decode"
#define  IARM_BUS_DSMGR_API_dsGetSupportedARCTypes    "dsGetSupportedARCTypes"
#define  IARM_BUS_DSMGR_API_dsAudioSetSAD		"dsAudioSetSAD"	
#define  IARM_BUS_DSMGR_API_dsAudioEnableARC    "dsAudioEnableARC"

#define  IARM_BUS_DSMGR_API_dsIsAudioPortEnabled  "dsIsAudioPortEnabled"
#define  IARM_BUS_DSMGR_API_dsEnableAudioPort      "dsEnableAudioPort"

#define IARM_BUS_DSMGR_API_dsGetEnablePersist "getPortEnablePersistVal"
#define IARM_BUS_DSMGR_API_dsSetEnablePersist "setPortEnablePersistVal" 

#define  IARM_BUS_DSMGR_API_dsAudioPortTerm		"dsAudioPortTerm"
#define  IARM_BUS_DSMGR_API_dsEnableMS12Config	"dsEnableMS12Config"
#define  IARM_BUS_DSMGR_API_dsEnableLEConfig            "dsEnableLEConfig"
#define IARM_BUS_DSMGR_API_dsGetLEConfig        "dsGetLEConfig"
#define  IARM_BUS_DSMGR_API_dsSetAudioDelay            "dsSetAudioDelay"
#define  IARM_BUS_DSMGR_API_dsGetAudioDelay            "dsGetAudioDelay"
#define  IARM_BUS_DSMGR_API_dsGetSinkDeviceAtmosCapability "dsGetSinkDeviceAtmosCapability"
#define  IARM_BUS_DSMGR_API_dsSetAudioAtmosOutputMode "dsSetAudioAtmosOutputMode"
#define  IARM_BUS_DSMGR_API_dsSetAudioDucking    "dsSetAudioDucking"
#define  IARM_BUS_DSMGR_API_dsSetAudioLevel            "dsSetAudioLevel"
#define  IARM_BUS_DSMGR_API_dsGetAudioLevel            "dsGetAudioLevel"
#define  IARM_BUS_DSMGR_API_dsSetAudioGain            "dsSetAudioGain"
#define  IARM_BUS_DSMGR_API_dsGetAudioGain            "dsGetAudioGain"

#define  IARM_BUS_DSMGR_API_dsSetAudioCompression         "dsSetAudioCompression"
#define  IARM_BUS_DSMGR_API_dsGetAudioCompression         "dsGetAudioCompression"
#define  IARM_BUS_DSMGR_API_dsSetDialogEnhancement        "dsSetDialogEnhancement"
#define  IARM_BUS_DSMGR_API_dsGetDialogEnhancement        "dsGetDialogEnhancement"
#define  IARM_BUS_DSMGR_API_dsSetDolbyVolumeMode          "dsSetDolbyVolumeMode"
#define  IARM_BUS_DSMGR_API_dsGetDolbyVolumeMode          "dsGetDolbyVolumeMode"
#define  IARM_BUS_DSMGR_API_dsSetIntelligentEqualizerMode  "dsSetIntelligentEqualizerMode"
#define  IARM_BUS_DSMGR_API_dsGetIntelligentEqualizerMode  "dsGetIntelligentEqualizerMode"
#define  IARM_BUS_DSMGR_API_dsGetVolumeLeveller  "dsGetVolumeLeveller"
#define  IARM_BUS_DSMGR_API_dsSetVolumeLeveller  "dsSetVolumeLeveller"
#define  IARM_BUS_DSMGR_API_dsGetBassEnhancer  "dsGetBassEnhancer"
#define  IARM_BUS_DSMGR_API_dsSetBassEnhancer  "dsSetBassEnhancer"
#define  IARM_BUS_DSMGR_API_dsIsSurroundDecoderEnabled  "dsIsSurroundDecoderEnabled"
#define  IARM_BUS_DSMGR_API_dsEnableSurroundDecoder  "dsEnableSurroundDecoder"
#define  IARM_BUS_DSMGR_API_dsGetDRCMode  "dsGetDRCMode"
#define  IARM_BUS_DSMGR_API_dsSetDRCMode  "dsSetDRCMode"
#define  IARM_BUS_DSMGR_API_dsGetSurroundVirtualizer  "dsGetSurroundVirtualizer"
#define  IARM_BUS_DSMGR_API_dsSetSurroundVirtualizer  "dsSetSurroundVirtualizer"
#define  IARM_BUS_DSMGR_API_dsGetMISteering  "dsGetMISteering"
#define  IARM_BUS_DSMGR_API_dsSetMISteering  "dsSetMISteering"
#define  IARM_BUS_DSMGR_API_dsSetGraphicEqualizerMode  "dsSetGraphicEqualizerMode"
#define  IARM_BUS_DSMGR_API_dsGetGraphicEqualizerMode  "dsGetGraphicEqualizerMode"
#define  IARM_BUS_DSMGR_API_dsGetMS12AudioProfileList  "dsGetMS12AudioProfileList"	
#define  IARM_BUS_DSMGR_API_dsGetMS12AudioProfile  "dsGetMS12AudioProfile"
#define  IARM_BUS_DSMGR_API_dsSetMS12AudioProfile  "dsSetMS12AudioProfile"
#define  IARM_BUS_DSMGR_API_dsSetMS12SetttingsOverride  "dsSetMS12SetttingsOverride"

#define  IARM_BUS_DSMGR_API_dsSetAssociatedAudioMixing  "dsSetAssociatedAudioMixing"
#define  IARM_BUS_DSMGR_API_dsGetAssociatedAudioMixing  "dsGetAssociatedAudioMixing"
#define  IARM_BUS_DSMGR_API_dsSetFaderControl  "dsSetFaderControl"
#define  IARM_BUS_DSMGR_API_dsGetFaderControl  "dsGetFaderControl"
#define  IARM_BUS_DSMGR_API_dsSetPrimaryLanguage  "dsSetPrimaryLanguage"
#define  IARM_BUS_DSMGR_API_dsGetPrimaryLanguage  "dsGetPrimaryLanguage"
#define  IARM_BUS_DSMGR_API_dsSetSecondaryLanguage  "dsSetSecondaryLanguage"
#define  IARM_BUS_DSMGR_API_dsGetSecondaryLanguage  "dsGetSecondaryLanguage"

#define IARM_BUS_DSMGR_API_dsAudioOutIsConnected     "dsAudioOutIsConnected"
#define IARM_BUS_DSMGR_API_dsGetHDMIARCPortId  "dsGetHDMIARCPortId"

/*
 * Declare RPC dsAudio Device API names 
 */
#define IARM_BUS_DSMGR_API_dsGetAudioCapabilities     "dsGetAudioCapabilities"
#define IARM_BUS_DSMGR_API_dsGetMS12Capabilities     "dsGetMS12Capabilities"
#define IARM_BUS_DSMGR_API_dsSetAudioMixerLevels          "dsSetAudioMixerLevels"
/*
 * Declare RPC dsDisplay API names 
 */
#define IARM_BUS_DSMGR_API_dsDisplayInit				"dsDisplayInit"
#define IARM_BUS_DSMGR_API_dsGetDisplay					"dsGetDisplay"
#define IARM_BUS_DSMGR_API_dsGetDisplayAspectRatio		"dsGetDisplayAspectRatio"
#define IARM_BUS_DSMGR_API_dsGetEDID					"dsGetEDID"
#define IARM_BUS_DSMGR_API_dsGetEDIDBytes               "dsGetEDIDBytes"
#define IARM_BUS_DSMGR_API_dsSetAllmEnabled "dsSetAllmEnabled"
#define IARM_BUS_DSMGR_API_dsSetAVIContentType "dsSetAVIContentType"
#define IARM_BUS_DSMGR_API_dsSetAVIScanInformation "dsSetAVIScanInformation"
#define IARM_BUS_DSMGR_API_dsDisplayTerm				"dsDisplayTerm"


/*
 * Declare RPC dsVideo Device API names 
 */
#define IARM_BUS_DSMGR_API_dsVideoDeviceInit		"dsVideoDeviceInit"
#define IARM_BUS_DSMGR_API_dsGetVideoDevice			"dsGetVideoDevice"
#define IARM_BUS_DSMGR_API_dsSetDFC					"dsSetDFC"
#define IARM_BUS_DSMGR_API_dsGetDFC					"dsGetDFC"
#define IARM_BUS_DSMGR_API_dsVideoDeviceTerm		"dsVideoDeviceTerm"
#define IARM_BUS_DSMGR_API_dsSetFRFMode         "dsSetFRFMode"
#define IARM_BUS_DSMGR_API_dsGetFRFMode         "dsGetFRFMode"
#define IARM_BUS_DSMGR_API_dsGetCurrentDisframerate             "dsGetCurrentDisframerate"
#define IARM_BUS_DSMGR_API_dsSetDisplayframerate                "dsSetDisplayframerate"
/*
 * Declare RPC dsVideo Port API names 
 */

#define IARM_BUS_DSMGR_API_dsVideoPortInit			 "dsVideoPortInit"
#define IARM_BUS_DSMGR_API_dsGetVideoPort			"dsGetVideoPort"
#define IARM_BUS_DSMGR_API_dsIsVideoPortEnabled		"dsIsVideoPortEnabled"
#define IARM_BUS_DSMGR_API_dsIsDisplayConnected		 "dsIsDisplayConnected"
#define IARM_BUS_DSMGR_API_dsIsDisplaySurround 		 "dsIsDisplaySurround"
#define IARM_BUS_DSMGR_API_dsGetSurroundMode 		 "dsGetSurroundMode"  
#define IARM_BUS_DSMGR_API_dsEnableVideoPort		 "dsEnableVideoPort"
#define IARM_BUS_DSMGR_API_dsSetResolution			"dsSetResolution"
#define IARM_BUS_DSMGR_API_dsGetResolution			"dsGetResolution"
#define IARM_BUS_DSMGR_API_dsColorDepthCapabilities			"dsColorDepthCapabilities"
#define IARM_BUS_DSMGR_API_dsGetPreferredColorDepth			"dsGetPreferredColorDepth"
#define IARM_BUS_DSMGR_API_dsSetPreferredColorDepth			"dsSetPreferredColorDepth"
#define IARM_BUS_DSMGR_API_dsVideoPortTerm			"dsVideoPortTerm"
#define IARM_BUS_DSMGR_API_dsEnableHDCP    		"dsEnableHDCP"
#define IARM_BUS_DSMGR_API_dsIsHDCPEnabled    		"dsIsHDCPEnabled"
#define IARM_BUS_DSMGR_API_dsGetHDCPStatus    		"dsGetHDCPStatus"
#define IARM_BUS_DSMGR_API_dsGetHDCPProtocol	        "dsGetHDCPProtocol"
#define IARM_BUS_DSMGR_API_dsGetHDCPReceiverProtocol	"dsGetHDCPReceiverProtocol"
#define IARM_BUS_DSMGR_API_dsGetHDCPCurrentProtocol	"dsGetHDCPCurrentProtocol"
#define IARM_BUS_DSMGR_API_dsIsVideoPortActive		"dsIsVideoPortActive"
#define IARM_BUS_DSMGR_API_dsGetHDRCapabilities     "dsGetHDRCapabilities"
#define IARM_BUS_DSMGR_API_dsGetTVHDRCapabilities     "dsGetTVHDRCapabilities"
#define IARM_BUS_DSMGR_API_dsGetSupportedTVResolution     "dsGetSupportedTVResolution"
#define IARM_BUS_DSMGR_API_dsGetSupportedVideoCodingFormats "dsGetSupportedVideoCodingFormats"
#define IARM_BUS_DSMGR_API_dsGetVideoCodecInfo "dsGetVideoCodecInfo"
#define IARM_BUS_DSMGR_API_dsSetForceDisableHDR "dsForceDisableHDR"
#define IARM_BUS_DSMGR_API_dsSetForceDisable4K "dsSetForceDisable4K"
#define IARM_BUS_DSMGR_API_dsGetForceDisable4K "dsGetForceDisable4K"
#define IARM_BUS_DSMGR_API_dsIsOutputHDR "dsIsOutputHDR"
#define IARM_BUS_DSMGR_API_dsResetOutputToSDR "dsResetOutputToSDR"
#define IARM_BUS_DSMGR_API_dsSetHdmiPreference "dsSetHdmiPreference"
#define IARM_BUS_DSMGR_API_dsGetHdmiPreference "dsGetHdmiPreference"
#define IARM_BUS_DSMGR_API_dsGetVideoEOTF           "dsGetVideoEOTF"
#define IARM_BUS_DSMGR_API_dsGetMatrixCoefficients  "dsGetMatrixCoefficients"
#define IARM_BUS_DSMGR_API_dsGetColorDepth          "dsGetColorDepths"
#define IARM_BUS_DSMGR_API_dsGetColorSpace          "dsGetColorSpace"
#define IARM_BUS_DSMGR_API_dsGetQuantizationRange "dsGetQuantizationRange"
#define IARM_BUS_DSMGR_API_dsGetCurrentOutputSettings "dsGetCurrentOutputSettings"
#define IARM_BUS_DSMGR_API_dsSetBackgroundColor "dsSetBackgroundColor"
#define IARM_BUS_DSMGR_API_dsSetForceHDRMode "dsSetForceHDRMode"
#define IARM_BUS_DSMGR_API_dsSetAllmEnabled "dsSetAllmEnabled"

/*
 * Declare RPC FP  API names 
 */

#define IARM_BUS_DSMGR_API_dsFPInit				"dsFPInit"
#define IARM_BUS_DSMGR_API_dsFPTerm				"dsFPTerm"
#define IARM_BUS_DSMGR_API_dsSetFPText			"dsSetFPText"
#define IARM_BUS_DSMGR_API_dsSetFPTime			"dsSetFPTime"
#define IARM_BUS_DSMGR_API_dsSetFPScroll		"dsSetFPScroll"
#define IARM_BUS_DSMGR_API_dsSetFPBlink			"dsSetFPBlink"
#define IARM_BUS_DSMGR_API_dsGetFPBrightness	 "dsGetFPBrightness"
#define IARM_BUS_DSMGR_API_dsSetFPBrightness	 "dsSetFPBrightness"
#define IARM_BUS_DSMGR_API_dsGetFPState           "dsGetFPState"
#define IARM_BUS_DSMGR_API_dsSetFPState			 "dsSetFPState"
#define IARM_BUS_DSMGR_API_dsSetFPColor			 "dsSetFPColor"
#define IARM_BUS_DSMGR_API_dsGetFPColor			  "dsGetFPColor"
#define IARM_BUS_DSMGR_API_dsGetFPTextBrightness "dsGetFPTextBrightness" 
#define IARM_BUS_DSMGR_API_dsSetFPTextBrightness "dsSetFPTextBrightness" 
#define IARM_BUS_DSMGR_API_dsFPEnableCLockDisplay "dsFPEnableCLockDisplay"
#define IARM_BUS_DSMGR_API_dsGetTimeFormat         "dsGetTimeFormat"
#define IARM_BUS_DSMGR_API_dsSetTimeFormat          "dsSetTimeFormat"
#define IARM_BUS_DSMGR_API_dsSetFPDMode          "dsSetFPDMode"


/*
 * Declare RPC HDMI API names 
 */
#define IARM_BUS_DSMGR_API_dsHdmiInInit                 "dsHdmiInInit"
#define IARM_BUS_DSMGR_API_dsHdmiInTerm                 "dsHdmiInTerm"
#define IARM_BUS_DSMGR_API_dsHdmiInGetNumberOfInputs    "dsHdmiInGetNumberOfInputs"
#define IARM_BUS_DSMGR_API_dsHdmiInGetStatus            "dsHdmiInGetStatus"
#define IARM_BUS_DSMGR_API_dsHdmiInSelectPort           "dsHdmiInSelectPort"
#define IARM_BUS_DSMGR_API_dsHdmiInScaleVideo           "dsHdmiInScaleVideo"
#define IARM_BUS_DSMGR_API_dsHdmiInSelectZoomMode       "dsHdmiInSelectZoomMode"
#define IARM_BUS_DSMGR_API_dsHdmiInPauseAudio           "dsHdmiInPauseAudio"
#define IARM_BUS_DSMGR_API_dsHdmiInResumeAudio          "dsHdmiInResumeAudio"
#define IARM_BUS_DSMGR_API_dsHdmiInGetCurrentVideoMode  "dsHdmiInGetCurrentVideoMode"
#define IARM_BUS_DSMGR_API_dsGetEDIDBytesInfo             "dsGetEDIDBytesInfo"
#define IARM_BUS_DSMGR_API_dsGetHDMISPDInfo             "dsGetHDMISPDInfo"
#define IARM_BUS_DSMGR_API_dsSetEdidVersion             "dsSetEdidVersion"
#define IARM_BUS_DSMGR_API_dsGetEdidVersion             "dsGetEdidVersion"
#define IARM_BUS_DSMGR_API_dsSetEdid2AllmSupport            "dsSetEdid2AllmSupport"
#define IARM_BUS_DSMGR_API_dsGetEdid2AllmSupport            "dsGetEdid2AllmSupport"
#define IARM_BUS_DSMGR_API_dsGetAllmStatus              "dsGetAllmStatus"
#define IARM_BUS_DSMGR_API_dsSetVRRSupport              "dsSetVRRSupport"
#define IARM_BUS_DSMGR_API_dsGetVRRSupport              "dsGetVRRSupport"
#define IARM_BUS_DSMGR_API_dsGetVRRStatus               "dsGetVRRStatus"
#define IARM_BUS_DSMGR_API_dsGetSupportedGameFeaturesList              "dsGetSupportedGameFeaturesList"
#define IARM_BUS_DSMGR_API_dsGetAVLatency   		"dsGetAVLatency"
#define IARM_BUS_DSMGR_API_dsGetHdmiVersion            "dsGetHdmiVersion"

/*
 * Declare RPC COMPOSITE INPUT API names
 */
#define IARM_BUS_DSMGR_API_dsCompositeInInit                 "dsCompositeInInit"
#define IARM_BUS_DSMGR_API_dsCompositeInTerm                 "dsCompositeInTerm"
#define IARM_BUS_DSMGR_API_dsCompositeInGetNumberOfInputs    "dsCompositeInGetNumberOfInputs"
#define IARM_BUS_DSMGR_API_dsCompositeInGetStatus            "dsCompositeInGetStatus"
#define IARM_BUS_DSMGR_API_dsCompositeInSelectPort           "dsCompositeInSelectPort"
#define IARM_BUS_DSMGR_API_dsCompositeInScaleVideo           "dsCompositeInScaleVideo"

/*
 * Declare RPC Host Interface  API names
 */

#define IARM_BUS_DSMGR_API_dsHostInit				"dsHostInit"
#define IARM_BUS_DSMGR_API_dsHostTerm				"dsHostTerm"
#define IARM_BUS_DSMGR_API_dsSetPreferredSleepMode  "dsSetPreferredSleepMode"
#define IARM_BUS_DSMGR_API_dsGetPreferredSleepMode  "dsGetPreferredSleepMode"
#define IARM_BUS_DSMGR_API_dsGetCPUTemperature 		"dsGetCPUTemperature"
#define IARM_BUS_DSMGR_API_dsGetVersion				"dsGetVersion"
#define IARM_BUS_DSMGR_API_dsGetSocIDFromSDK               "dsGetSocIDFromSDK"
#define IARM_BUS_DSMGR_API_dsGetHostEDID               "dsGetHostEDID"
#define IARM_BUS_DSMGR_API_dsGetMS12ConfigType         "dsGetMS12ConfigType"

/*
 * Declare Reset MS12 setting  Interface  API names
 */
#define IARM_BUS_DSMGR_API_dsResetDialogEnhancement    "dsResetDialogEnhancement"
#define IARM_BUS_DSMGR_API_dsResetBassEnhancer         "dsResetBassEnhancer"
#define IARM_BUS_DSMGR_API_dsResetSurroundVirtualizer  "dsResetSurroundVirtualizer"
#define IARM_BUS_DSMGR_API_dsResetVolumeLeveller       "dsResetVolumeLeveller"

#define IARM_BUS_DSMGR_API_SetStandbyVideoState "SetStandbyVideoState"
#define IARM_BUS_DSMGR_API_GetStandbyVideoState "GetStandbyVideoState"
#define IARM_BUS_DSMGR_API_SetAvPortState       "SetAvPortState"
#define IARM_BUS_DSMGR_API_SetLEDStatus         "SetLEDStatus"
#define IARM_BUS_DSMGR_API_SetRebootConfig         "SetRebootConfig"
#define DSMGR_MAX_VIDEO_PORT_NAME_LENGTH 16
#define PWRMGR_MAX_REBOOT_REASON_LENGTH 100

#define MS12_CONFIG_BUF_SIZE 16

typedef struct _dsMgrStandbyVideoStateParam_t{
     char port[DSMGR_MAX_VIDEO_PORT_NAME_LENGTH]; 
     int isEnabled;
     int result;
} dsMgrStandbyVideoStateParam_t;

typedef struct _dsMgrRebootConfigParam_t{
     char reboot_reason_custom[PWRMGR_MAX_REBOOT_REASON_LENGTH];
     int powerState;
     int result;
} dsMgrRebootConfigParam_t;

typedef struct _dsMgrAVPortStateParam_t{
     int avPortPowerState;
     int result;
} dsMgrAVPortStateParam_t;

typedef struct _dsMgrLEDStatusParam_t{
     int ledState;
     int result;
} dsMgrLEDStatusParam_t;

typedef struct _dsAudioGetHandleParam_t {
	dsAudioPortType_t type;
	int index;
	intptr_t handle;
} dsAudioGetHandleParam_t;

typedef struct _dsGetSupportedARCTypesParam_t {
        intptr_t handle;
        int types;
} dsGetSupportedARCTypesParam_t;

typedef struct _dsAudioSetSADParam_t {
	intptr_t handle;
	dsAudioSADList_t list;
} dsAudioSetSADParam_t;

typedef struct _dsAudioEnableARCParam_t {
        intptr_t handle;
        dsAudioARCStatus_t arcStatus;
} dsAudioEnableARCParam_t;

typedef struct _dsAudioSetStereoModeParam_t {
	intptr_t handle;
	dsAudioStereoMode_t mode;
    dsError_t rpcResult;
    bool toPersist;
} dsAudioSetStereoModeParam_t;

typedef struct _dsAudioSetStereoAutoParam_t {
	intptr_t handle;
    int autoMode;
    bool toPersist;
} dsAudioSetStereoAutoParam_t;

typedef struct _dsAudioSetMutedParam_t {
	intptr_t handle;
	bool mute;
} dsAudioSetMutedParam_t;

typedef struct _dsEdidIgnoreParam_t {
	intptr_t handle;
	bool ignoreEDID;
} dsEdidIgnoreParam_t;

typedef struct _dsAudioSetLevelParam_t {
        intptr_t handle;
        float level;
} dsAudioSetLevelParam_t;

typedef struct _dsAudioSetDuckingParam_t {
        intptr_t handle;
        dsAudioDuckingAction_t action;
        dsAudioDuckingType_t type;
        unsigned char level;
} dsAudioSetDuckingParam_t;

typedef struct _dsAudioGainParam_t {
        intptr_t handle;
        float gain;
} dsAudioGainParam_t;

typedef struct _dsAudioFormatParam_t {
        intptr_t handle;
	dsAudioFormat_t audioFormat;
} dsAudioFormatParam_t;

typedef struct _dsAudioGetEncodingModeParam_t {
	intptr_t handle;
	dsAudioEncoding_t encoding;
} dsAudioGetEncodingModeParam_t;

typedef struct _dsAudioGetMS11Param_t {
    intptr_t handle;
    bool ms11Enabled;
} dsAudioGetMS11Param_t;

typedef struct _dsAudioGetMS12Param_t {
    intptr_t handle;
    bool ms12Enabled;
} dsAudioGetMS12Param_t;

typedef struct _dsGetAudioDelayParam_t {
       intptr_t handle;
       uint32_t audioDelayMs;
} dsGetAudioDelayParam_t;

typedef struct _dsAudioSetAtmosOutputModeParam_t {
    intptr_t handle;
    bool enable;
} dsAudioSetAtmosOutputModeParam_t;
  
typedef struct _dsGetAudioAtmosCapabilityParam_t {
       intptr_t handle;
       dsATMOSCapability_t capability;
} dsGetAudioAtmosCapabilityParam_t;

typedef struct _dsSetAudioDelayParam_t {
       intptr_t handle;
       uint32_t audioDelayMs;
} dsSetAudioDelayParam_t;

typedef struct _dsAudioDelayOffsetParam_t {
       intptr_t handle;
       uint32_t audioDelayOffsetMs;
} dsAudioDelayOffsetParam_t;

typedef struct _dsAudioCompressionParam_t {
       intptr_t handle;
       int compression;
} dsAudioCompressionParam_t;

typedef struct _dsDialogEnhancementParam_t {
       intptr_t handle;
       int enhancerLevel;
} dsDialogEnhancementParam_t;

typedef struct _dsSetDolbyVolumeParam_t {
       intptr_t handle;
       bool enable;
} dsSetDolbyVolumeParam_t;

typedef struct _dsIntelligentEqualizerModeParam_t {
       intptr_t handle;
       int mode;
} dsIntelligentEqualizerModeParam_t;

typedef struct _dsVolumeLevellerParam_t {
       intptr_t handle;
       dsVolumeLeveller_t volLeveller;
} dsVolumeLevellerParam_t;

typedef struct _dsBassEnhancerParam_t {
       intptr_t handle;
       int boost;
} dsBassEnhancerParam_t;

typedef struct _dsSurroundDecoderParam_t {
       intptr_t handle;
       bool enable;
} dsSurroundDecoderParam_t;

typedef struct _dsDRCModeParam_t {
       intptr_t handle;
       int mode;
} dsDRCModeParam_t;

typedef struct _dsSurroundVirtualizerParam_t {
       intptr_t handle;
       dsSurroundVirtualizer_t virtualizer;
} dsSurroundVirtualizerParam_t;

typedef struct _dsMISteeringParam_t {
       intptr_t handle;
       bool enable;
} dsMISteeringParam_t;

typedef struct _dsGrpahicEqualizerModeParam_t {
       intptr_t handle;
       int mode;
} dsGraphicEqualizerModeParam_t;

typedef struct _dsAssociatedAudioMixingParam_t {
       intptr_t handle;
       bool mixing;
} dsAssociatedAudioMixingParam_t;

typedef struct _dsFaderControlParam_t {
       intptr_t handle;
       int mixerbalance;
} dsFaderControlParam_t;

typedef struct _dsPrimaryLanguageParam_t {
        intptr_t handle;
        char primaryLanguage[MAX_LANGUAGE_LEN];
} dsPrimaryLanguageParam_t;

typedef struct _dsSecondaryLanguageParam_t {
        intptr_t handle;
        char secondaryLanguage[MAX_LANGUAGE_LEN];
} dsSecondaryLanguageParam_t;


#define MAX_PROFILE_STRING_LEN 32
typedef struct _dsMS12AudioProfileParam_t {
	intptr_t handle;
	char profile [MAX_PROFILE_STRING_LEN];
} dsMS12AudioProfileParam_t;

typedef struct _dsMS12SetttingsOverrideParam_t {
        intptr_t handle;
        char profileState[MAX_PROFILE_STRING_LEN];
        char profileName[MAX_PROFILE_STRING_LEN];
        char profileSettingsName[MAX_PROFILE_STRING_LEN];
        char profileSettingValue[MAX_PROFILE_STRING_LEN];
} dsMS12SetttingsOverrideParam_t;

typedef struct _dsMS12AudioProfileListParam_t {
	intptr_t handle;
	dsMS12AudioProfileList_t profileList;
} dsMS12AudioProfileListParam_t;

typedef struct _dsVideoPortGetHandleParam_t {
	dsVideoPortType_t type;
	int index;
	intptr_t handle;
} dsVideoPortGetHandleParam_t;

typedef struct _dsVideoPortEnabledParam_t {
	intptr_t handle;
	bool enabled;
	char portName [32];
} dsVideoPortIsEnabledParam_t, dsVideoPortSetEnabledParam_t, dsVideoPortIsHDCPEnabledParam_t,dsAudioPortEnabledParam_t;


typedef struct _dsVideoPortIsActiveParam_t {
	intptr_t handle;
	bool active;
    dsError_t   result;
} dsVideoPortIsActiveParam_t;

typedef struct _dsVideoPortGetHDCPStatus_t {
	intptr_t handle;
	dsHdcpStatus_t hdcpStatus;
} dsVideoPortGetHDCPStatus_t;

typedef struct _dsVideoPortGetHDCPProtocolVersion_t {
	intptr_t handle;
	dsHdcpProtocolVersion_t protocolVersion;
} dsVideoPortGetHDCPProtocolVersion_t;

typedef struct _dsSetBackgroundColorParam_t{
        intptr_t handle;
        dsVideoBackgroundColor_t color;
} dsSetBackgroundColorParam_t;

typedef struct _dsVideoPortIsDisplayConnectedParam_t {
	intptr_t handle;
	bool connected;
} dsVideoPortIsDisplayConnectedParam_t;

typedef struct _dsVideoPortIsDisplaySurroundParam_t {
	intptr_t handle;
	bool surround;
} dsVideoPortIsDisplaySurroundParam_t;

typedef struct _dsVideoPortGetSurroundModeParam_t {
	intptr_t handle;
	int surround;
} dsVideoPortGetSurroundModeParam_t;


typedef struct _dsVideoPortGetResolutionParam_t {
	intptr_t handle;
    bool toPersist;
	dsVideoPortResolution_t resolution;
} dsVideoPortGetResolutionParam_t;


typedef struct _dsVideoPortSetResolutionParam_t {
	dsError_t result;
	intptr_t handle;
	bool toPersist;
    bool forceCompatible;
	dsVideoPortResolution_t resolution;
} dsVideoPortSetResolutionParam_t;

typedef struct {
	dsError_t result;
	intptr_t handle;
	unsigned int colorDepthCapability;
} dsColorDepthCapabilitiesParam_t;

typedef struct {
	dsError_t result;
	intptr_t handle;
	dsDisplayColorDepth_t colorDepth;
	bool toPersist;
} dsPreferredColorDepthParam_t;

typedef struct _dsVideoDeviceGetHandleParam_t {
	int index;
	intptr_t handle;
} dsVideoDeviceGetHandleParam_t;

typedef struct _dsVideoDeviceSetDFCParam_t {
	intptr_t handle;
	dsVideoZoom_t dfc;
} dsVideoDeviceSetDFCParam_t;

typedef struct _dsDisplayGetHandleParam_t {
	dsVideoPortType_t type;
	int index;
	intptr_t handle;
} dsDisplayGetHandleParam_t;

typedef struct _dsDisplayGetAspectRatioParam_t {
	intptr_t handle;
    dsVideoAspectRatio_t aspectRatio;
} dsDisplayGetAspectRatioParam_t;

typedef struct _dsDisplayGetEDIDParam_t {
	intptr_t handle;
    dsDisplayEDID_t edid;
} dsDisplayGetEDIDParam_t;

typedef struct _dsDisplaySetAVIContentTypeParam_t
{
    dsError_t result;
    intptr_t  handle;
    dsAviContentType_t contentType;
}dsDisplaySetAVIContentTypeParam_t;

typedef struct _dsDisplaySetAVIScanInfoParam_t
{
    dsError_t result;
    intptr_t  handle;
    dsAVIScanInformation_t scanInfo;
}dsDisplaySetAVIScanInfoParam_t;

typedef struct _dsDisplaySetAllmEnabledParam_t
{
    dsError_t result;
    intptr_t  handle;
    bool     enabled;
}dsDisplaySetAllmEnabledParam_t;

typedef struct _dsSupportedResolutionParam_t {
    dsError_t result;
    intptr_t  handle;
    int       resolutions;
}dsSupportedResolutionParam_t;

typedef struct _dsDisplayGetEDIDBytesParam_t {
    int result;
	intptr_t handle;
    int length;
    unsigned char bytes[1024];
} dsDisplayGetEDIDBytesParam_t;

typedef struct _dsMS12ConfigTypeParam_t {
   dsError_t result;
   char configType[MS12_CONFIG_BUF_SIZE]; 
}dsMS12ConfigTypeParam_t;
typedef struct _dsFPDTimeParam
{
    dsFPDTimeFormat_t eTime;
    unsigned int nHours;
    unsigned int nMinutes;
}dsFPDTimeParam_t;


typedef struct _dsFPDTimeFormatParam
{
    dsFPDTimeFormat_t eTime;
}dsFPDTimeFormatParam_t;

typedef struct _dsFPDScrollParam
{
    unsigned int nScrollHoldOnDur;
    unsigned int nHorzScrollIterations;
    unsigned int nVertScrollIterations;
}dsFPDScrollParam_t;

typedef struct _dsFPDBlinkParam
{
    dsFPDIndicator_t eIndicator;
    unsigned int nBlinkDuration;
    unsigned int nBlinkIterations;
}dsFPDBlinkParam_t;

typedef struct _dsFPDBrightParam
{
    dsFPDIndicator_t eIndicator;
    dsFPDBrightness_t eBrightness;
    bool toPersist;
}dsFPDBrightParam_t;


typedef struct _dsFPDTextBrightParam
{
    dsFPDTextDisplay_t eIndicator;
    dsFPDBrightness_t eBrightness;
}dsFPDTextBrightParam_t;


typedef struct _dsFPDStateParam
{
    dsFPDIndicator_t eIndicator;
    dsFPDState_t state;
}dsFPDStateParam_t;

typedef struct _dsFPDColorParam
{
    dsFPDIndicator_t eIndicator;
    dsFPDColor_t eColor;
    bool toPersist;
}dsFPDColorParam_t;

typedef struct _dsFPDModeParam
{
    dsFPDMode_t eMode;
}dsFPDModeParam_t;

typedef struct _dsEnableHDCPParam 
{
    intptr_t handle;
    bool contentProtect;
    char hdcpKey[HDCP_KEY_MAX_SIZE];
    size_t keySize;
    dsError_t rpcResult;
} dsEnableHDCPParam_t;

typedef enum _dsSleepMode_t{
    dsHOST_SLEEP_MODE_LIGHT,       /**< Light sleep mode.                                */
    dsHOST_SLEEP_MODE_DEEP,        /**< Deep sleep mode.                                 */
    dsHOST_SLEEP_MODE_MAX,         /**< Maximum index for sleep modes                    */
} dsSleepMode_t;

/**
 * Sleep mode validation check.
 */
#define dsSleepMode_isValid(t)  (((t)  >= dsHOST_SLEEP_MODE_LIGHT) && ((t) < dsHOST_SLEEP_MODE_MAX))


typedef struct _dsPreferredSleepMode
{
    dsSleepMode_t mode;
} dsPreferredSleepMode;

typedef struct _dsCPUThermalParam
{
	float  temperature;
} dsCPUThermalParam;

typedef struct _dsVesrionParam
{
	uint32_t  versionNumber;
} dsVesrionParam;

typedef struct _dsVideoRect
{
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} dsVideoRect_t;

typedef struct _dsHdmiInGetNumberOfInputsParam_t
{
    dsError_t   result;
    uint8_t     numHdmiInputs;
} dsHdmiInGetNumberOfInputsParam_t;

typedef struct _dsHdmiInGetStatusParam_t
{
    dsError_t           result;
    dsHdmiInStatus_t    status;
} dsHdmiInGetStatusParam_t;

typedef struct _dsHdmiInSelectPortParam_t
{
    dsError_t       result;
    dsHdmiInPort_t  port;
    bool requestAudioMix;
    bool topMostPlane;
    dsVideoPlaneType_t videoPlaneType;
} dsHdmiInSelectPortParam_t;

typedef struct _dsHdmiInScaleVideoParam_t
{
    dsError_t       result;
    dsVideoRect_t   videoRect;
} dsHdmiInScaleVideoParam_t;

typedef struct _dsHdmiInSelectZoomModeParam_t
{
    dsError_t      result;
    dsVideoZoom_t  zoomMode;
} dsHdmiInSelectZoomModeParam_t;

typedef struct _dsHdmiInGetResolutionParam_t
{
	dsError_t               result;
	dsVideoPortResolution_t resolution;
} dsHdmiInGetResolutionParam_t;


typedef struct _dsCompositeInGetNumberOfInputsParam_t
{
    dsError_t   result;
    uint8_t     numCompositeInputs;
} dsCompositeInGetNumberOfInputsParam_t;

typedef struct _dsCompositeInGetStatusParam_t
{
    dsError_t           result;
    dsCompositeInStatus_t    status;
} dsCompositeInGetStatusParam_t;

typedef struct _dsCompositeInSelectPortParam_t
{
    dsError_t       result;
    dsCompositeInPort_t  port;
} dsCompositeInSelectPortParam_t;

typedef struct _dsCompositeInScaleVideoParam_t
{
    dsError_t       result;
    dsVideoRect_t   videoRect;
} dsCompositeInScaleVideoParam_t;


typedef struct _dsGetHDRCapabilitiesParam_t
{
    dsError_t               result;
    intptr_t                handle;
    int                     capabilities;
} dsGetHDRCapabilitiesParam_t;

typedef struct _dsGetVideoFormatsParam_t
{
    dsError_t               result;
    intptr_t                handle;
    int                     videoFormats;
} dsGetVideoFormatsParam_t;

typedef struct _dsGetAudioCapabilitiesParam_t
{
    dsError_t               result;
    intptr_t                handle;
    int                     capabilities;
} dsGetAudioCapabilitiesParam_t;

typedef struct _dsGetMS12CapabilitiesParam_t
{
    dsError_t               result;
    intptr_t                handle;
    int                     capabilities;
} dsGetMS12CapabilitiesParam_t;

typedef struct _dsMS12ConfigParam_t
{
    intptr_t           handle;
    dsMS12FEATURE_t    feature;
    bool               enable;
} dsMS12ConfigParam_t;

typedef struct _dsLEConfigParam_t
{
    intptr_t           handle;
    bool               enable;
} dsLEConfigParam_t;

typedef struct
{
    dsError_t result;
    intptr_t handle;
    unsigned int supported_formats;
} dsGetSupportedVideoCodingFormatsParam_t;

typedef struct
{
    dsError_t result;
    intptr_t handle;
    dsVideoCodingFormat_t format;
    dsVideoCodecInfo_t info;
} dsGetVideoCodecInfoParam_t;

typedef struct
{
	dsError_t result;
	intptr_t handle;
	bool disable;
} dsForceDisableHDRParam_t;

typedef struct
{
        dsError_t result;
        intptr_t handle;
        dsHDRStandard_t  hdrMode;
} dsForceHDRModeParam_t;

typedef struct
{
	dsError_t result;
	intptr_t handle;
	bool disable;
} dsForceDisable4KParam_t;
  
typedef struct _dsIsOutputHDRParam_t {
    dsError_t result;
    intptr_t handle;
    bool hdr;
} dsIsOutputHDRParam_t;

typedef struct _dsSetHdmiPreferenceParam_t {
    dsError_t result;
    intptr_t handle;
    dsHdcpProtocolVersion_t hdcpCurrentProtocol;
}dsSetHdmiPreferenceParam_t;

typedef struct _dsGetHdmiPreferenceParam_t {
    dsError_t result;
    intptr_t handle;
    dsHdcpProtocolVersion_t hdcpCurrentProtocol;
}dsGetHdmiPreferenceParam_t;

typedef struct _dsGetLEConfigParam_t {
    dsError_t result;
    intptr_t handle;
    bool enable;
}dsGetLEConfigParam_t;

#define DS_DEVICEID_LEN_MAX 1024

typedef struct _dsGetSocIDFromSDKParam_t {
    dsError_t result;
    char socID[DS_DEVICEID_LEN_MAX];
} dsGetSocIDFromSDKParam_t;

#define DSSCART_PARAM_LEN_MAX 1024
#define DSSCART_VALUE_LEN_MAX 1024

typedef struct _dsScartParamParam_t {
    int result;
    intptr_t handle;
    char param_bytes[DSSCART_PARAM_LEN_MAX];
    char value_bytes[DSSCART_VALUE_LEN_MAX];
} dsScartParamParam_t;

typedef struct _dsEot_t {
    dsError_t result;
    intptr_t handle;
    dsHDRStandard_t video_eotf;
} dsEot_t;

typedef struct _dsMatrixCoefficients_t {
    dsError_t result;
    intptr_t handle;
    dsDisplayMatrixCoefficients_t matrix_coefficients;
} dsMatrixCoefficients_t;

typedef struct _dsColorDepth_t {
    dsError_t result;
    intptr_t handle;
    uint32_t color_depth;
} dsColorDepth_t;

typedef struct _dsColorSpace_t {
    dsError_t result;
    intptr_t handle;
    dsDisplayColorSpace_t color_space;
} dsColorSpace_t;

typedef struct _dsQuantizationRange_t {
    dsError_t result;
    intptr_t handle;
    dsDisplayQuantizationRange_t quantization_range;
} dsQuantizationRange_t;

typedef struct _dsCurrentOutputSettings_t {
    dsError_t result;
    intptr_t handle;
    dsHDRStandard_t video_eotf;
    dsDisplayMatrixCoefficients_t matrix_coefficients;
    uint32_t color_depth;
    dsDisplayColorSpace_t color_space;
    dsDisplayQuantizationRange_t quantization_range;
} dsCurrentOutputSettings_t;

typedef struct _dsAudioOutIsConnectedParam_t
{
    dsError_t               result;
    intptr_t                handle;
    bool                    isCon;
} dsAudioOutIsConnectedParam_t;

typedef struct _dsGetEDIDBytesInfoParam_t
{
    dsError_t               result;
    dsHdmiInPort_t          iHdmiPort;
    unsigned char           edid [MAX_EDID_BYTES_LEN];
    int                     length;
}dsGetEDIDBytesInfoParam_t;

typedef struct _dsGetHDMISPDInfoParam_t
{
    dsError_t               result;
    dsHdmiInPort_t          iHdmiPort;
    unsigned char           spdInfo [1024];
}dsGetHDMISPDInfoParam_t;

typedef struct
{
    intptr_t handle;
    int frfmode;
} dsFRFParam_t;

typedef struct
{
    intptr_t handle;
    char framerate[20];
}dsFramerateParam_t;

typedef struct _dsEdidVersionParam_t
{
    dsError_t               result;
    dsHdmiInPort_t          iHdmiPort;
    tv_hdmi_edid_version_t  iEdidVersion;
}dsEdidVersionParam_t;

typedef struct _dsEdidAllmSupportParam_t
{
    dsError_t               result;
    dsHdmiInPort_t          iHdmiPort;
    bool                    allmSupport;
}dsEdidAllmSupportParam_t;

typedef struct _dsVRRSupportParam_t
{
    dsError_t               result;
    dsHdmiInPort_t          iHdmiPort;
    bool                    vrrSupport;
}dsVRRSupportParam_t;

typedef struct _dsVRRStatusParam_t
{
    dsError_t               result;
    dsHdmiInPort_t          iHdmiPort;
    dsHdmiInVrrStatus_t             vrrStatus;
}dsVRRStatusParam_t;

typedef struct _dsGetHDMIARCPortIdParam_t {
        dsError_t result;
        int portId;
} dsGetHDMIARCPortIdParam_t;

typedef struct _dsAllmStatusParam_t
{
    dsError_t               result;
    dsHdmiInPort_t          iHdmiPort;
    bool                    allmStatus;
}dsAllmStatusParam_t;

typedef struct _dsSupportedGameFeatureListParam_t
{
    dsError_t                       result;
    dsSupportedGameFeatureList_t    featureList;
}dsSupportedGameFeatureListParam_t;

typedef struct _dsTVAudioVideoLatencyParam_t
{
      dsError_t  result;
      int32_t audio_output_delay;
      int32_t video_latency;
}dsTVAudioVideoLatencyParam_t;

typedef struct _dsSetAudioMixerLevelsParam_t
{
    intptr_t handle;
    dsAudioInput_t aInput;
    int            volume;
}dsSetAudioMixerLevelsParam_t;


typedef struct _dsHdmiVersionParam_t
{
    dsError_t               result;
    dsHdmiInPort_t          iHdmiPort;
    dsHdmiMaxCapabilityVersion_t iCapVersion;
}dsHdmiVersionParam_t;

#ifdef __cplusplus
}
#endif

#endif /* RPDS_H_ */


/** @} */
/** @} */
