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
* @defgroup devicesettings
* @{
* @defgroup rpc
* @{
**/


#ifndef RPDSMGRNTF_H_
#define RPDSMGRNTF_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _AudioPortState {
            AUDIO_PORT_STATE_UNKNOWN       = 0,
            AUDIO_PORT_STATE_UNINITIALIZED = 1,
            AUDIO_PORT_STATE_INITIALIZED   = 2
}AudioPortState;


typedef enum _AudioPortType  {
            AUDIO_PORT_TYPE_LR        = 0,
            AUDIO_PORT_TYPE_HDMI      = 1,
            AUDIO_PORT_TYPE_SPDIF     = 2,
            AUDIO_PORT_TYPE_SPEAKER   = 3,
            AUDIO_PORT_TYPE_HDMIARC   = 4,
            AUDIO_PORT_TYPE_HEADPHONE = 5,
            AUDIO_PORT_TYPE_MAX       = 6
 }AudioPortType;

typedef enum _DolbyAtmosCapability {
            AUDIO_DOLBYATMOS_NOT_SUPPORTED     = 0,
            AUDIO_DOLBYATMOS_DDPLUS_STREAM     = 1,
            AUDIO_DOLBYATMOS_ATMOS_METADATA    = 2,
            AUDIO_DOLBYATMOS_MAX               = 3
 }DolbyAtmosCapability;


typedef enum _StereoMode {
            AUDIO_STEREO_UNKNOWN     = 0,
            AUDIO_STEREO_MONO        = 1,
            AUDIO_STEREO_STEREO      = 2,
            AUDIO_STEREO_SURROUND    = 3,
            AUDIO_STEREO_PASSTHROUGH = 4,
            AUDIO_STEREO_DD          = 5,
            AUDIO_STEREO_DDPLUS      = 6,
            AUDIO_STEREO_MAX         = 7
 }StereoMode;


typedef enum _AudioFormat {
            AUDIO_FORMAT_NONE               = 0,
            AUDIO_FORMAT_PCM                = 1,
            AUDIO_FORMAT_DOLBY_AC3          = 2,
            AUDIO_FORMAT_DOLBY_EAC3         = 3,
            AUDIO_FORMAT_DOLBY_AC4          = 4,
            AUDIO_FORMAT_DOLBY_MAT          = 5,
            AUDIO_FORMAT_DOLBY_TRUEHD       = 6,
            AUDIO_FORMAT_DOLBY_EAC3_ATMOS   = 7,
            AUDIO_FORMAT_DOLBY_TRUEHD_ATMOS = 8,
            AUDIO_FORMAT_DOLBY_MAT_ATMOS    = 0,
            AUDIO_FORMAT_DOLBY_AC4_ATMOS    = 10,
            AUDIO_FORMAT_AAC                = 11,
            AUDIO_FORMAT_VORBIS             = 12,
            AUDIO_FORMAT_WMA                = 13,
            AUDIO_FORMAT_UNKNOWN            = 14,
            AUDIO_FORMAT_MAX                = 15
 }AudioFormat;
 

typedef enum _HDMIInPort {
            DS_HDMI_IN_PORT_NONE    = -1,
            DS_HDMI_IN_PORT_0       = 0,
            DS_HDMI_IN_PORT_1       = 1,
            DS_HDMI_IN_PORT_2       = 2,
            DS_HDMI_IN_PORT_3       = 3,
            DS_HDMI_IN_PORT_4       = 4,
            DS_HDMI_IN_PORT_MAX     = 5
        }HDMIInPort;


 typedef  enum _VideoZoom{
            DS_VIDEO_DEVICE_ZOOM_UNKNOWN           = -1,
            DS_VIDEO_DEVICE_ZOOM_NONE              = 0,
            DS_VIDEO_DEVICE_ZOOM_FULL              = 1,
            DS_VIDEO_DEVICE_ZOOM_LB_16_9           = 2,
            DS_VIDEO_DEVICE_ZOOM_LB_14_9           = 3,
            DS_VIDEO_DEVICE_ZOOM_CCO               = 4,
            DS_VIDEO_DEVICE_ZOOM_PAN_SCAN          = 5,
            DS_VIDEO_DEVICE_ZOOM_LB_2_21_1_ON_4_3  = 6,
            DS_VIDEO_DEVICE_ZOOM_LB_2_21_1_ON_16_9 = 7,
            DS_VIDEO_DEVICE_ZOOM_PLATFORM          = 8,
            DS_VIDEO_DEVICE_ZOOM_16_9_ZOOM         = 9,
            DS_VIDEO_DEVICE_ZOOM_PILLARBOX_4_3     = 10,
            DS_VIDEO_DEVICE_ZOOM_WIDE_4_3          = 11,
            DS_VIDEO_DEVICE_ZOOM_MAX               = 12
        }VideoZoom;


typedef  enum _HDMIInVRRType{
            DS_HDMIIN_VRR_NONE                  = 0,
            DS_HDMIIN_HDMI_VRR                  = 1,
            DS_HDMIIN_AMD_FREESYNC              = 2,
            DS_HDMIIN_AMD_FREESYNC_PREMIUM      = 3,
            DS_HDMIIN_AMD_FREESYNC_PREMIUM_PRO  = 4
        }HDMIInVRRType;

enum _HDMIInAviContentType {
            DS_HDMIIN_AVICONTENT_TYPE_GRAPHICS      =0,    
            DS_HDMIIN_AVICONTENT_TYPE_PHOTO         =1,       
            DS_HDMIIN_AVICONTENT_TYPE_CINEMA        =2,      
            DS_HDMIIN_AVICONTENT_TYPE_GAME          =3,        
            DS_HDMIIN_AVICONTENT_TYPE_NOT_SIGNALLED =4,
            DS_HDMIIN_AVICONTENT_TYPE_MAX           =5
        }HDMIInAviContentType;


typedef enum _HDMIInTVResolution {
            DS_HDMIIN_RESOLUTION_480I     = 0x000001,    
            DS_HDMIIN_RESOLUTION_480P     = 0x000002,    
            DS_HDMIIN_RESOLUTION_576I     = 0x000004,    
            DS_HDMIIN_RESOLUTION_576P     = 0x000008,    
            DS_HDMIIN_RESOLUTION_576P50   = 0x000010,  
            DS_HDMIIN_RESOLUTION_720P     = 0x000020,    
            DS_HDMIIN_RESOLUTION_720P50   = 0x000040,  
            DS_HDMIIN_RESOLUTION_1080I    = 0x000080,   
            DS_HDMIIN_RESOLUTION_1080P    = 0x000100,   
            DS_HDMIIN_RESOLUTION_1080P24  = 0x000200, 
            DS_HDMIIN_RESOLUTION_1080I25  = 0x000400, 
            DS_HDMIIN_RESOLUTION_1080P30  = 0x001000, 
            DS_HDMIIN_RESOLUTION_1080I50  = 0x002000, 
            DS_HDMIIN_RESOLUTION_1080P50  = 0x004000, 
            DS_HDMIIN_RESOLUTION_1080P60  = 0x008000, 
            DS_HDMIIN_RESOLUTION_2160P24  = 0x010000, 
            DS_HDMIIN_RESOLUTION_2160P25  = 0x020000, 
            DS_HDMIIN_RESOLUTION_2160P30  = 0x040000, 
            DS_HDMIIN_RESOLUTION_2160P50  = 0x080000, 
            DS_HDMIIN_RESOLUTION_2160P60  = 0x100000 
        }HDMIInTVResolution;

typedef enum _HDMIVideoAspectRatio {
            DS_HDMIIN_ASPECT_RATIO_UNKNOWN     = 0  /* @text Video Aspect Ratio UNKNOWN */,
            DS_HDMIIN_ASPECT_RATIO_4X3         = 1  /* @text Video Aspect Ratio 4X3 */,
            DS_HDMIIN_ASPECT_RATIO_16x9        = 2  /* @text Video Aspect Ratio 16x9 */
        }HDMIVideoAspectRatio;

typedef enum _HDMIInVideoStereoScopicMode{
            DS_HDMIIN_SSMODE_UNKNOWN           = 0,
            DS_HDMIIN_SSMODE_2D                = 1,
            DS_HDMIIN_SSMODE_3D_SIDE_BY_SIDE   = 2,
            DS_HDMIIN_SSMODE_3D_TOP_AND_BOTTOM = 3
 }HDMIInVideoStereoScopicMode;

 typedef enum HDMIInVideoFrameRate {
            DS_HDMIIN_FRAMERATE_UNKNOWN   = 0,
            DS_HDMIIN_FRAMERATE_24        = 1,
            DS_HDMIIN_FRAMERATE_25        = 2,      
            DS_HDMIIN_FRAMERATE_30        = 3,      
            DS_HDMIIN_FRAMERATE_60        = 4,      
            DS_HDMIIN_FRAMERATE_23_98     = 5, 
            DS_HDMIIN_FRAMERATE_29_97     = 6, 
            DS_HDMIIN_FRAMERATE_50        = 7,      
            DS_HDMIIN_FRAMERATE_59_94     = 8
   }HDMIInVideoFrameRate;

typedef struct _HDMIVideoPortResolution {
            std::string name;
            HDMIInTVResolution pixelResolution;
            HDMIVideoAspectRatio aspectRatio;
            HDMIInVideoStereoScopicMode stereoScopicMode;
            HDMIInVideoFrameRate frameRate;
            bool interlaced;
 }HDMIVideoPortResolution;


typedef enum _HDMIInSignalStatus{
            DS_HDMI_IN_SIGNAL_STATUS_NONE        = -1,   
            DS_HDMI_IN_SIGNAL_STATUS_NOSIGNAL     = 0,    
            DS_HDMI_IN_SIGNAL_STATUS_UNSTABLE     = 1,                                
            DS_HDMI_IN_SIGNAL_STATUS_NOTSUPPORTED = 2,                              
            DS_HDMI_IN_SIGNAL_STATUS_STABLE       = 3,
            DS_HDMI_IN_SIGNAL_STATUS_MAX          = 4
 }HDMIInSignalStatus;


 typedef enum _SleepMode {
            DS_HOST_SLEEPMODE_LIGHT      = 0,
            DS_HOST_SLEEPMODE_DEEP       = 1,
            DS_HOST_SLEEPMODE_MAX        = 2
 }SleepMode;


 typedef enum _CompositeInPort{
            DS_COMPOSITE_IN_PORT_NONE    = -1  /* @text UNKNOWN */,
            DS_COMPOSITE_IN_PORT_0       = 0  /* @text CompositeIn Port 0 */,
            DS_COMPOSITE_IN_PORT_1       = 1  /* @text CompositeIn Port 1 */,
            DS_COMPOSITE_IN_PORT_MAX     = 2  /* @text CompositeIn Port MAX */,
 }CompositeInPort;


typedef enum _CompositeInSignalStatus{
            DS_COMPOSITE_IN_SIGNAL_STATUS_NONE          = -1 /* @text Signal Status None */,
            DS_COMPOSITE_IN_SIGNAL_STATUS_NOSIGNAL      = 0 /* @text Signal Status No Signal */,
            DS_COMPOSITE_IN_SIGNAL_STATUS_UNSTABLE      = 1 /* @text Signal Status Unstable */,
            DS_COMPOSITE_IN_SIGNAL_STATUS_NOTSUPPORTED  = 2 /* @text Signal Status Not supported */,
            DS_COMPOSITE_IN_SIGNAL_STATUS_STABLE        = 3 /* @text Signal Status Stable */,
            DS_COMPOSITE_IN_SIGNAL_STATUS_MAX           = 4 /* @test Signal Status MAX */
        }CompositeInSignalStatus;

typedef enum _DisplayTVResolution{
            DS_DISPLAY_RESOLUTION_480I     = 0x000001,    
            DS_DISPLAY_RESOLUTION_480P     = 0x000002,    
            DS_DISPLAY_RESOLUTION_576I     = 0x000004,    
            DS_DISPLAY_RESOLUTION_576P     = 0x000008,    
            DS_DISPLAY_RESOLUTION_576P50   = 0x000010,  
            DS_DISPLAY_RESOLUTION_720P     = 0x000020,    
            DS_DISPLAY_RESOLUTION_720P50   = 0x000040,  
            DS_DISPLAY_RESOLUTION_1080I    = 0x000080,   
            DS_DISPLAY_RESOLUTION_1080P    = 0x000100,   
            DS_DISPLAY_RESOLUTION_1080P24  = 0x000200, 
            DS_DISPLAY_RESOLUTION_1080I25  = 0x000400, 
            DS_DISPLAY_RESOLUTION_1080P30  = 0x001000, 
            DS_DISPLAY_RESOLUTION_1080I50  = 0x002000, 
            DS_DISPLAY_RESOLUTION_1080P50  = 0x004000, 
            DS_DISPLAY_RESOLUTION_1080P60  = 0x008000, 
            DS_DISPLAY_RESOLUTION_2160P24  = 0x010000, 
            DS_DISPLAY_RESOLUTION_2160P25  = 0x020000, 
            DS_DISPLAY_RESOLUTION_2160P30  = 0x040000, 
            DS_DISPLAY_RESOLUTION_2160P50  = 0x080000, 
            DS_DISPLAY_RESOLUTION_2160P60  = 0x100000 
  }DisplayTVResolution;


typedef enum _DisplayVideoAspectRatio{
            DS_DISPLAY_ASPECT_RATIO_UNKNOWN     = 0  /* @text Video Aspect Ratio UNKNOWN */,
            DS_DISPLAY_ASPECT_RATIO_4X3         = 1  /* @text Video Aspect Ratio 4X3 */,
            DS_DISPLAY_ASPECT_RATIO_16x9        = 2  /* @text Video Aspect Ratio 16x9 */
        }DisplayVideoAspectRatio;

typedef enum _DisplayInVideoStereoScopicMode{
            DS_DISPLAY_SSMODE_UNKNOWN           = 0,        
            DS_DISPLAY_SSMODE_2D                = 1,                 
            DS_DISPLAY_SSMODE_3D_SIDE_BY_SIDE   = 2,    
            DS_DISPLAY_SSMODE_3D_TOP_AND_BOTTOM = 3                 
        }DisplayInVideoStereoScopicMode;

 typedef enum _DisplayInVideoFrameRate {
            DS_DISPLAY_FRAMERATE_UNKNOWN   = 0,
            DS_DISPLAY_FRAMERATE_24        = 1,
            DS_DISPLAY_FRAMERATE_25        = 2,      
            DS_DISPLAY_FRAMERATE_30        = 3,      
            DS_DISPLAY_FRAMERATE_60        = 4,      
            DS_DISPLAY_FRAMERATE_23_98     = 5, 
            DS_DISPLAY_FRAMERATE_29_97     = 6, 
            DS_DISPLAY_FRAMERATE_50        = 7,      
            DS_DISPLAY_FRAMERATE_59_94     = 8,
            DS_DISPLAY_FRAMERATE_100       = 9,
            DS_DISPLAY_FRAMERATE_119_88    = 10,
            DS_DISPLAY_FRAMERATE_120       = 11,
            DS_DISPLAY_FRAMERATE_200       = 12,
            DS_DISPLAY_FRAMERATE_239_76    = 13,
            DS_DISPLAY_FRAMERATE_240       = 14,
            DS_DISPLAY_FRAMERATE_MAX       = 15
   }DisplayInVideoFrameRate;

 typedef struct _DisplayVideoPortResolution {
	    std::string name;
            DisplayTVResolution pixelResolution;
            DisplayVideoAspectRatio aspectRatio;
            DisplayInVideoStereoScopicMode stereoScopicMode;
            DisplayInVideoFrameRate frameRate;
            bool interlaced;
  }DisplayVideoPortResolution;


  typedef enum _DisplayEvent {
            DS_DISPLAY_EVENT_CONNECTED    =0,  ///< Display connected event
            DS_DISPLAY_EVENT_DISCONNECTED =1,   ///< Display disconnected event
            DS_DISPLAY_RXSENSE_ON         =2,           ///< Rx Sense ON event
            DS_DISPLAY_RXSENSE_OFF         =3,          ///< Rx Sense OFF event
            DS_DISPLAY_HDCPPROTOCOL_CHANGE=4,  ///< HDCP Protocol Version Change event
            DS_DISPLAY_EVENT_MAX             ///< Display max event
        }DisplayEvent;


  typedef enum _FPDTimeFormat{
            DS_FPD_TIMEFORMAT_12_HOUR    = 0,
            DS_FPD_TIMEFORMAT_24_HOUR    = 1,
            DS_FPD_TIMEFORMAT_MAX        = 2
        }FPDTimeFormat;

  typedef enum _HDCPStatus{
            DS_HDCP_STATUS_UNPOWERED              = 0,           
            DS_HDCP_STATUS_UNAUTHENTICATED        = 1,         
            DS_HDCP_STATUS_AUTHENTICATED          = 2,           
            DS_HDCP_STATUS_AUTHENTICATIONFAILURE  = 3,   
            DS_HDCP_STATUS_INPROGRESS             = 4,              
            DS_HDCP_STATUS_PORTDISABLED           = 5,
            DS_HDCP_STATUS_MAX                    = 6
        }HDCPStatus;

  typedef enum _HDRStandard{
            DS_HDRSTANDARD_NONE             = 0x0,              
            DS_HDRSTANDARD_HDR10            = 0x01,            
            DS_HDRSTANDARD_HLG              = 0x02,             
            DS_HDRSTANDARD_DOLBYVISION      = 0x04,      
            DS_HDRSTANDARD_TECHNICOLORPRIME = 0x08, 
            DS_HDRSTANDARD_HDR10PLUS        = 0x10,         
            DS_HDRSTANDARD_SDR              = 0x20,
            DS_HDRSTANDARD_INVALID          = 0x80
        }HDRStandard;
 

#ifdef __cplusplus
}
#endif

#endif /* RPDSMGRNTF_H_ */


/** @} */
/** @} */
