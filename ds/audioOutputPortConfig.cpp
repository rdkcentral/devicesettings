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
* @defgroup ds
* @{
**/

#include "dsHALConfig.h"
#include "audioOutputPortConfig.hpp"
#include "dsAudioSettings.h"
#include "illegalArgumentException.hpp"
#include "dsError.h"
#include "dsUtl.h"
#include "stdlib.h"
#include "dslogger.h"

//const dsAudioTypeConfig_t  kConfig_audio ;
//const dsAudioPortConfig_t  kPort_audio ;


/*
 * Setup the supported configurations here.
 */
 dsAudioPortType_t 			kSupportedPortTypes1[] 				= { dsAUDIOPORT_TYPE_SPDIF, dsAUDIOPORT_TYPE_SPEAKER, dsAUDIOPORT_TYPE_HDMI_ARC, dsAUDIOPORT_TYPE_HEADPHONE };

 dsAudioEncoding_t          kSupportedSPDIFEncodings1[]                      = { dsAUDIO_ENC_PCM, dsAUDIO_ENC_AC3, };
 dsAudioCompression_t       kSupportedSPDIFCompressions1[]           = { dsAUDIO_CMP_NONE, dsAUDIO_CMP_LIGHT, dsAUDIO_CMP_MEDIUM, dsAUDIO_CMP_HEAVY, };
 dsAudioStereoMode_t        kSupportedSPDIFStereoModes1[]            = { dsAUDIO_STEREO_STEREO, dsAUDIO_STEREO_SURROUND, dsAUDIO_STEREO_PASSTHRU };

 dsAudioEncoding_t          kSupportedHEADPHONEEncodings1[]                      = { dsAUDIO_ENC_PCM, };
 dsAudioCompression_t       kSupportedHEADPHONECompressions1[]           = { dsAUDIO_CMP_NONE, dsAUDIO_CMP_LIGHT, dsAUDIO_CMP_MEDIUM, dsAUDIO_CMP_HEAVY, };
 dsAudioStereoMode_t        kSupportedHEADPHONEStereoModes1[]            = { dsAUDIO_STEREO_STEREO, dsAUDIO_STEREO_SURROUND, };

 dsAudioEncoding_t          kSupportedSPEAKEREncodings1[]                      = { dsAUDIO_ENC_PCM, dsAUDIO_ENC_AC3, };
 dsAudioCompression_t       kSupportedSPEAKERCompressions1[]           = { dsAUDIO_CMP_NONE, dsAUDIO_CMP_LIGHT, dsAUDIO_CMP_MEDIUM, dsAUDIO_CMP_HEAVY, };
 dsAudioStereoMode_t        kSupportedSPEAKERStereoModes1[]            = { dsAUDIO_STEREO_STEREO, dsAUDIO_STEREO_SURROUND, };

 dsAudioEncoding_t          kSupportedARCEncodings1[]                      = { dsAUDIO_ENC_PCM, dsAUDIO_ENC_AC3, };
 dsAudioCompression_t       kSupportedARCCompressions1[]           = { dsAUDIO_CMP_NONE, dsAUDIO_CMP_LIGHT, dsAUDIO_CMP_MEDIUM, dsAUDIO_CMP_HEAVY, };
 dsAudioStereoMode_t        kSupportedARCStereoModes1[]            = { dsAUDIO_STEREO_STEREO, dsAUDIO_STEREO_SURROUND, dsAUDIO_STEREO_PASSTHRU };


 dsAudioTypeConfig_t 	kConfig_audio[]= {

		{
		/*.typeId = */					dsAUDIOPORT_TYPE_SPDIF,
		/*.name = */					"SPDIF", //SPDIF
		/*.numSupportedCompressions = */dsUTL_DIM(kSupportedSPDIFCompressions1),
		/*.compressions = */			kSupportedSPDIFCompressions1,
		/*.numSupportedEncodings = */	dsUTL_DIM(kSupportedSPDIFEncodings1),
		/*.encodings = */				kSupportedSPDIFEncodings1,
		/*.numSupportedStereoModes = */	dsUTL_DIM(kSupportedSPDIFStereoModes1),
		/*.stereoModes = */				kSupportedSPDIFStereoModes1,
		},

                {
                /*.typeId = */                                  dsAUDIOPORT_TYPE_HEADPHONE,
                /*.name = */                                    "HEADPHONE", //HEADPHONE
                /*.numSupportedCompressions = */dsUTL_DIM(kSupportedHEADPHONECompressions1),
                /*.compressions = */                    kSupportedHEADPHONECompressions1,
                /*.numSupportedEncodings = */   dsUTL_DIM(kSupportedHEADPHONEEncodings1),
                /*.encodings = */                               kSupportedHEADPHONEEncodings1,
                /*.numSupportedStereoModes = */ dsUTL_DIM(kSupportedHEADPHONEStereoModes1),
                /*.stereoModes = */                             kSupportedHEADPHONEStereoModes1,
                },

                {
                /*.typeId = */                                  dsAUDIOPORT_TYPE_SPEAKER,
                /*.name = */                                    "SPEAKER", //SPEAKER
                /*.numSupportedCompressions = */dsUTL_DIM(kSupportedSPEAKERCompressions1),
                /*.compressions = */                    kSupportedSPEAKERCompressions1,
                /*.numSupportedEncodings = */   dsUTL_DIM(kSupportedSPEAKEREncodings1),
                /*.encodings = */                               kSupportedSPEAKEREncodings1,
                /*.numSupportedStereoModes = */ dsUTL_DIM(kSupportedSPEAKERStereoModes1),
                /*.stereoModes = */                             kSupportedSPEAKERStereoModes1,
                },
                {
                /*.typeId = */                                  dsAUDIOPORT_TYPE_HDMI_ARC,
                /*.name = */                                    "HDMI_ARC", //ARC/eARC
                /*.numSupportedCompressions = */dsUTL_DIM(kSupportedARCCompressions1),
                /*.compressions = */                    kSupportedARCCompressions1,
                /*.numSupportedEncodings = */   dsUTL_DIM(kSupportedARCEncodings1),
                /*.encodings = */                               kSupportedARCEncodings1,
                /*.numSupportedStereoModes = */ dsUTL_DIM(kSupportedARCStereoModes1),
                /*.stereoModes = */                             kSupportedARCStereoModes1,
                }
};

 dsVideoPortPortId_t connectedVOPs1[dsAUDIOPORT_TYPE_MAX][dsVIDEOPORT_TYPE_MAX] = {
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

 dsAudioPortConfig_t kPort_audio[] = {

		{
		/*.typeId = */ 					{dsAUDIOPORT_TYPE_SPDIF, 0},
		/*.connectedVOPs = */			connectedVOPs1[dsAUDIOPORT_TYPE_SPDIF],
		},



                {
                /*.typeId = */                                  {dsAUDIOPORT_TYPE_HEADPHONE, 0},
                /*.connectedVOPs = */                   connectedVOPs1[dsAUDIOPORT_TYPE_HEADPHONE],
                },

                {
                /*.typeId = */                                  {dsAUDIOPORT_TYPE_SPEAKER, 0},
                /*.connectedVOPs = */                   connectedVOPs1[dsAUDIOPORT_TYPE_SPEAKER],
                },
                {
                /*.typeId = */                                  {dsAUDIOPORT_TYPE_HDMI_ARC, 0},
                /*.connectedVOPs = */                   connectedVOPs1[dsAUDIOPORT_TYPE_HDMI_ARC],
                }
};

int kConfig_size =  sizeof(kConfig_audio) / sizeof(kConfig_audio[0]);
int kPort_size =  sizeof(kPort_audio) / sizeof(kPort_audio[0]);


namespace device {

//To Make the instance as thread-safe, using = default, that can request special methods from the compiler. They are Special because only compiler can create them.

AudioOutputPortConfig::AudioOutputPortConfig()= default;

AudioOutputPortConfig::~AudioOutputPortConfig()= default;


AudioOutputPortConfig & AudioOutputPortConfig::getInstance()
{
    static AudioOutputPortConfig _singleton; // _singleton instance is in thread-safe now.
	return _singleton;
}

const AudioEncoding &AudioOutputPortConfig::getEncoding(int id) const
{
	return _aEncodings.at(id);
}
const AudioCompression &AudioOutputPortConfig::getCompression(int id) const
{
	return _aCompressions.at(id);

}
const AudioStereoMode &AudioOutputPortConfig::getStereoMode(int id) const
{
	return _aStereoModes.at(id);

}
AudioOutputPortType & AudioOutputPortConfig::getPortType(int id)
{
	return _aPortTypes.at(id);
}

AudioOutputPort &AudioOutputPortConfig::getPort(int id)
{
	return _aPorts.at(id);
}

AudioOutputPort &AudioOutputPortConfig::getPort(const std::string & name)
{
	for (size_t i = 0; i < _aPorts.size(); i++) {
		if (name.compare(_aPorts.at(i).getName()) == 0) {
			return _aPorts.at(i);
		}
	}

	throw IllegalArgumentException();
}

List<AudioOutputPort> AudioOutputPortConfig::getPorts()
{
	List <AudioOutputPort> rPorts;

	for (size_t i = 0; i < _aPorts.size(); i++) {
		rPorts.push_back(_aPorts.at(i));
	}

	return rPorts;
}

List<AudioOutputPortType>  AudioOutputPortConfig::getSupportedTypes()
{
	List<AudioOutputPortType> supportedTypes;
	for (std::vector<AudioOutputPortType>::const_iterator it = _aPortTypes.begin(); it != _aPortTypes.end(); it++) {
		if (it->isEnabled()) {
			supportedTypes.push_back(*it);
		}
	}

	return supportedTypes;
}

void AudioOutputPortConfig::load()
{
	try {
		/*
		 * Load Constants First.
		 */
		for (int i = 0; i < dsAUDIO_ENC_MAX; i++) {
			_aEncodings.push_back(AudioEncoding(i));
		}

		for (int i = 0; i < dsAUDIO_CMP_MAX; i++) {
			_aCompressions.push_back(AudioCompression(i));

		}

		for (int i = 0; i < dsAUDIO_STEREO_MAX; i++) {
			_aStereoModes.push_back(AudioStereoMode(i));

		}

		for (int i = 0; i < dsAUDIOPORT_TYPE_MAX; i++) {
			_aPortTypes.push_back(AudioOutputPortType(i));

		}

		/*
		 * Initialize Audio portTypes (encodings, compressions etc.)
		 * and its port instances (db, level etc)
		 */
		for (size_t i = 0; i < dsUTL_DIM(kConfigs); i++) {
			const dsAudioTypeConfig_t *typeCfg = &kConfigs[i];
			AudioOutputPortType &aPortType = AudioOutputPortType::getInstance(typeCfg->typeId);
			aPortType.enable();
			for (size_t j = 0; j < typeCfg->numSupportedEncodings; j++) {
				aPortType.addEncoding(AudioEncoding::getInstance(typeCfg->encodings[j]));
				_aEncodings.at(typeCfg->encodings[j]).enable();
			}
			for (size_t j = 0; j < typeCfg->numSupportedCompressions; j++) {
				aPortType.addCompression(typeCfg->compressions[j]);
				_aCompressions.at(typeCfg->compressions[j]).enable();

			}
			for (size_t j = 0; j < typeCfg->numSupportedStereoModes; j++) {
				aPortType.addStereoMode(typeCfg->stereoModes[j]);
				_aStereoModes.at(typeCfg->stereoModes[j]).enable();

			}
		}


		/*
		 * set up ports based on kPorts[]
		 */
		for (size_t i = 0; i < dsUTL_DIM(kPorts); i++) {
			const dsAudioPortConfig_t *port = &kPorts[i];
			_aPorts.push_back(AudioOutputPort((port->id.type), port->id.index, i));
			_aPortTypes.at(port->id.type).addPort(_aPorts.at(i));
		}
	}
	catch(const Exception &e) {
		throw e;
	}
}

void AudioOutputPortConfig::release()
  {
	try {
              _aEncodings.clear(); 
              _aCompressions.clear();
              _aStereoModes.clear();
              _aPortTypes.clear();
              _aPorts.clear();              
	}
	catch(const Exception &e) {
		throw e;
	}

  }
}



/** @} */
/** @} */
