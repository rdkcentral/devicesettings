/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2022 Sky UK
* Copyright 2022 Amlogic
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


#ifdef __cplusplus
extern "C" {
#endif


#include "dsTypes.h"
#include "dsUtl.h"

namespace  {


/*
 * Setup the supported configurations here.
 */
#ifdef HAS_SPDIF_SUPPORT
dsAudioPortType_t 			kSupportedAudioPortTypes[] 				= { dsAUDIOPORT_TYPE_SPDIF, dsAUDIOPORT_TYPE_SPEAKER, dsAUDIOPORT_TYPE_HDMI_ARC, dsAUDIOPORT_TYPE_HEADPHONE };
#else
dsAudioPortType_t                  kSupportedAudioPortTypes[]                           = { dsAUDIOPORT_TYPE_SPEAKER, dsAUDIOPORT_TYPE_HDMI_ARC };
#endif
#ifdef HAS_SPDIF_SUPPORT
dsAudioEncoding_t          kSupportedSPDIFEncodings[]                      = { dsAUDIO_ENC_PCM, dsAUDIO_ENC_AC3, };
dsAudioCompression_t       kSupportedSPDIFCompressions[]           = { dsAUDIO_CMP_NONE, dsAUDIO_CMP_LIGHT, dsAUDIO_CMP_MEDIUM, dsAUDIO_CMP_HEAVY, };
dsAudioStereoMode_t        kSupportedSPDIFStereoModes[]            = { dsAUDIO_STEREO_STEREO, dsAUDIO_STEREO_SURROUND, dsAUDIO_STEREO_PASSTHRU };
#endif
#ifdef HAS_HEADPHONE_SUPPORT
dsAudioEncoding_t          kSupportedHEADPHONEEncodings[]                      = { dsAUDIO_ENC_PCM, };
dsAudioCompression_t       kSupportedHEADPHONECompressions[]           = { dsAUDIO_CMP_NONE, dsAUDIO_CMP_LIGHT, dsAUDIO_CMP_MEDIUM, dsAUDIO_CMP_HEAVY, };
dsAudioStereoMode_t        kSupportedHEADPHONEStereoModes[]            = { dsAUDIO_STEREO_STEREO, dsAUDIO_STEREO_SURROUND, };
#endif
dsAudioEncoding_t          kSupportedSPEAKEREncodings[]                      = { dsAUDIO_ENC_PCM, dsAUDIO_ENC_AC3, };
dsAudioCompression_t       kSupportedSPEAKERCompressions[]           = { dsAUDIO_CMP_NONE, dsAUDIO_CMP_LIGHT, dsAUDIO_CMP_MEDIUM, dsAUDIO_CMP_HEAVY, };
dsAudioStereoMode_t        kSupportedSPEAKERStereoModes[]            = { dsAUDIO_STEREO_STEREO, dsAUDIO_STEREO_SURROUND, };

dsAudioEncoding_t          kSupportedARCEncodings[]                      = { dsAUDIO_ENC_PCM, dsAUDIO_ENC_AC3, };
dsAudioCompression_t       kSupportedARCCompressions[]           = { dsAUDIO_CMP_NONE, dsAUDIO_CMP_LIGHT, dsAUDIO_CMP_MEDIUM, dsAUDIO_CMP_HEAVY, };
dsAudioStereoMode_t        kSupportedARCStereoModes[]            = { dsAUDIO_STEREO_STEREO, dsAUDIO_STEREO_SURROUND, dsAUDIO_STEREO_PASSTHRU };


dsAudioTypeConfig_t 	kAudioConfigs[]= {
#ifdef HAS_SPDIF_SUPPORT
		{
		/*.typeId = */					dsAUDIOPORT_TYPE_SPDIF,
		/*.name = */					"SPDIF", //SPDIF
		/*.numSupportedCompressions = */dsUTL_DIM(kSupportedSPDIFCompressions),
		/*.compressions = */			kSupportedSPDIFCompressions,
		/*.numSupportedEncodings = */	dsUTL_DIM(kSupportedSPDIFEncodings),
		/*.encodings = */				kSupportedSPDIFEncodings,
		/*.numSupportedStereoModes = */	dsUTL_DIM(kSupportedSPDIFStereoModes),
		/*.stereoModes = */				kSupportedSPDIFStereoModes,
		},
#endif
#ifdef HAS_HEADPHONE_SUPPORT
                {
                /*.typeId = */                                  dsAUDIOPORT_TYPE_HEADPHONE,
                /*.name = */                                    "HEADPHONE", //HEADPHONE
                /*.numSupportedCompressions = */dsUTL_DIM(kSupportedHEADPHONECompressions),
                /*.compressions = */                    kSupportedHEADPHONECompressions,
                /*.numSupportedEncodings = */   dsUTL_DIM(kSupportedHEADPHONEEncodings),
                /*.encodings = */                               kSupportedHEADPHONEEncodings,
                /*.numSupportedStereoModes = */ dsUTL_DIM(kSupportedHEADPHONEStereoModes),
                /*.stereoModes = */                             kSupportedHEADPHONEStereoModes,
                },
#endif
                {
                /*.typeId = */                                  dsAUDIOPORT_TYPE_SPEAKER,
                /*.name = */                                    "SPEAKER", //SPEAKER
                /*.numSupportedCompressions = */dsUTL_DIM(kSupportedSPEAKERCompressions),
                /*.compressions = */                    kSupportedSPEAKERCompressions,
                /*.numSupportedEncodings = */   dsUTL_DIM(kSupportedSPEAKEREncodings),
                /*.encodings = */                               kSupportedSPEAKEREncodings,
                /*.numSupportedStereoModes = */ dsUTL_DIM(kSupportedSPEAKERStereoModes),
                /*.stereoModes = */                             kSupportedSPEAKERStereoModes,
                },
                {
                /*.typeId = */                                  dsAUDIOPORT_TYPE_HDMI_ARC,
                /*.name = */                                    "HDMI_ARC", //ARC/eARC
                /*.numSupportedCompressions = */dsUTL_DIM(kSupportedARCCompressions),
                /*.compressions = */                    kSupportedARCCompressions,
                /*.numSupportedEncodings = */   dsUTL_DIM(kSupportedARCEncodings),
                /*.encodings = */                               kSupportedARCEncodings,
                /*.numSupportedStereoModes = */ dsUTL_DIM(kSupportedARCStereoModes),
                /*.stereoModes = */                             kSupportedARCStereoModes,
                }
};

dsVideoPortPortId_t connectedVOPs[dsAUDIOPORT_TYPE_MAX][dsVIDEOPORT_TYPE_MAX] = {
		{/*VOPs connected to LR Audio */

		},
		{/*VOPs connected to HDMI Audio */
		},
		{/*VOPs connected to SPDIF Audio */
				{dsVIDEOPORT_TYPE_INTERNAL, 0},
		},
                {/*VOPs connected to SPEAKER Audio */
                                {dsVIDEOPORT_TYPE_INTERNAL, 0},
                },
                {/*VOPs connected to ARC Audio */
                                {dsVIDEOPORT_TYPE_INTERNAL, 0},
                },
                {/*VOPs connected to HEADPHONE Audio */
                                {dsVIDEOPORT_TYPE_INTERNAL, 0},
                }
};

dsAudioPortConfig_t kAudioPorts[] = {
#ifdef HAS_SPDIF_SUPPORT
		{
		/*.typeId = */ 					{dsAUDIOPORT_TYPE_SPDIF, 0},
		/*.connectedVOPs = */			connectedVOPs[dsAUDIOPORT_TYPE_SPDIF],
		},
#endif
#ifdef HAS_HEADPHONE_SUPPORT
                {
                /*.typeId = */                                  {dsAUDIOPORT_TYPE_HEADPHONE, 0},
                /*.connectedVOPs = */                   connectedVOPs[dsAUDIOPORT_TYPE_HEADPHONE],
                },
#endif
                {
                /*.typeId = */                                  {dsAUDIOPORT_TYPE_SPEAKER, 0},
                /*.connectedVOPs = */                   connectedVOPs[dsAUDIOPORT_TYPE_SPEAKER],
                },
                {
                /*.typeId = */                                  {dsAUDIOPORT_TYPE_HDMI_ARC, 0},
                /*.connectedVOPs = */                   connectedVOPs[dsAUDIOPORT_TYPE_HDMI_ARC],
                }
};

int kAudioConfigs_size = sizeof(kAudioConfigs)/sizeof(kAudioConfigs[0]);

int kAudioPorts_size = sizeof(kAudioPorts)/sizeof(kAudioPorts[0]);

}
#ifdef __cplusplus
}
#endif

