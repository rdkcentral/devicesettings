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
* @defgroup sample
* @{
**/


#include <iostream>
#include "videoOutputPort.hpp"
#include "host.hpp"
#include "videoResolution.hpp"
#include "audioOutputPort.hpp"
#include "audioEncoding.hpp"
#include "audioCompression.hpp"
#include "audioStereoMode.hpp"
#include "audioOutputPortType.hpp"
#include "manager.hpp"

#include "dsUtl.h"
#include "dsError.h"
#include "list.hpp"
#include "libIBus.h"


int main(int argc, char *argv[]) 
{
   
	IARM_Bus_Init("SampleDSClient");
	IARM_Bus_Connect();


    try{
	    device::Manager::Initialize();
        device::List<device::VideoOutputPort> vPorts = device::Host::getInstance().getVideoOutputPorts();
        for (size_t i = 0; i < vPorts.size(); i++) {
        
                uint32_t audioDelayMs = 0;
                device::AudioOutputPort &aPort = vPorts.at(i).getAudioOutputPort();

                aPort.getAudioDelay(audioDelayMs);
                printf("AudioPort Name- [%s] =======================\r\n",    aPort.getName().c_str());

                printf("\t Compression- [%d]\r\n",    aPort.getCompression());
                printf("\t MS12 MultiStreamDecode Status - [%s]\r\n",aPort.isAudioMS12Decode()? "On" : "Off");
                printf("\t DialogEnhancement- [%d]\r\n",    aPort.getDialogEnhancement());
                printf("\t DolbyVolumeMode- [%d]\r\n",    aPort.getDolbyVolumeMode());
                printf("\t IntelligentEqualizerMode- [%d]\r\n",    aPort.getIntelligentEqualizerMode());
                printf("\t Volume Leveller - [%d]\r\n",    aPort.getVolumeLeveller().level);
                printf("\t Bass Enhancer Boost value - [%d]\r\n",aPort.getBassEnhancer());
                printf("\t Surround Decoder Status - [%s]\r\n",aPort.isSurroundDecoderEnabled()? "Enabled" : "Disabled");
                printf("\t Dynamic Range Control Mode - [%s]\r\n",aPort.getDRCMode()? "RF" : "Line");
                printf("\t Surround Virtualizer boost - [%d]\r\n",    aPort.getSurroundVirtualizer().boost);
                printf("\t Media Intelligent Steering - [%s]\r\n",aPort.getMISteering()? "On" : "Off");
                printf("\t GraphicEqualizerMode- [%d]\r\n",    aPort.getGraphicEqualizerMode());
                printf("\t Encoding- [%s]\r\n",       aPort.getEncoding().getName().c_str());
                printf("\t Stereo Mode- [%s]\r\n",    aPort.getStereoMode().getName().c_str());
                printf("\t Gain - [%f]\r\n",          aPort.getGain());
                printf("\t DB- [%f]\r\n",             aPort.getDB());
                printf("\t LoopThru- [%s]\r\n",       aPort.isLoopThru() ? "True" : "False");
                printf("\t Mute Status - [%s]\r\n",   aPort.isMuted()? "On" : "Off");
                printf("\t MS11 MultiStreamDecode Status - [%s]\r\n",aPort.isAudioMSDecode()? "On" : "Off");
                printf("\t Loudness Equivalence - [%s]\r\n",aPort.GetLEConfig()? "Enabled" : "Disabled");
                printf("\t MaxDB - [%f]\r\n",         aPort.getMaxDB());
                printf("\t MinDB- [%f]\r\n",          aPort.getMinDB());
                printf("\t Optimal Level- [%f]\r\n",  aPort.getOptimalLevel());
                printf("\t Audio Level- [%f]\r\n",    aPort.getLevel());
                printf("\t Audio delay - [%lu] ms\r\n", audioDelayMs);
                printf("\t Supported Compressions - [%d]\r\n", aPort.getSupportedCompressions().size());

            for (size_t i = 0; i < aPort.getSupportedCompressions().size(); i++) {
                printf("\t\t - [%s]\r\n", aPort.getSupportedCompressions().at(i).getName().c_str());
            }
            printf("\t Supported Encodings - [%d]\r\n", aPort.getSupportedEncodings().size());
            for (size_t i = 0; i < aPort.getSupportedEncodings().size(); i++) {
                printf("\t\t - [%s]\r\n", aPort.getSupportedEncodings().at(i).getName().c_str());
            }
            printf("\t Supported Stereo MOdes - [%d]\r\n", aPort.getSupportedStereoModes().size());
            for (size_t i = 0; i < aPort.getSupportedStereoModes().size(); i++) {
                printf("\t\t - [%s]\r\n", aPort.getSupportedStereoModes().at(i).getName().c_str());
            }

            printf("==========================================\r\n\r\n");
        }

        device::Manager::DeInitialize();

    }
    catch (...)
    {
    	printf("Exception Caught during [%s]\r\n", __FUNCTION__);
    }

	IARM_Bus_Disconnect();
	IARM_Bus_Term();

	
    return 0;
}


/** @} */
/** @} */
