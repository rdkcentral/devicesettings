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
* @defgroup ds
* @{
**/


#ifndef _DS_AUDIOOUTPUTPORT_HPP_
#define _DS_AUDIOOUTPUTPORT_HPP_

#include "audioEncoding.hpp"
#include "audioCompression.hpp"
#include "audioStereoMode.hpp"
#include "list.hpp"
#include "enumerable.hpp"
#include "dsTypes.h"
#include "dsError.h"
#include "dsMgr.h"

#include <string>
#include <stdint.h>


/**
 * @file audioOutputPort.hpp
 * @brief It contain variables,stuctures,class and functions referenced by audiooutputport code.
 */
namespace device {

class AudioOutputPortType;
class AudioOutputPortConfig;





/**
 * @class  AudioOutputPort
 * @brief Class extending Enumerable to implement the audiooutputport interface.
 * @ingroup devicesettingsclass
 */
class AudioOutputPort  : public Enumerable {
	int _type; //!< Audiooutputporttype.
	int _index; //!< Index of the audio output port.
	int _id; //!< Port id.

	intptr_t _handle;
	std::string _name;

	int _encoding; //!< Audio Encoding Types.
	int _stereoMode; //!< Audio stereo mode Types.
	uint32_t _audioDelayMs; //!< Audio delays
	uint32_t _audioDelayOffsetMs; //!< Audio delay offset
	bool _stereoAuto; //!< Audio stereo mode Types.

	float _gain; //!< Audio gain value.
	float _db; //!< Audio decibel value.
	float _maxDb; //!< Maximum audio decibel value.
	float _minDb; //!< Minimum audio decibel value.
	float _optimalLevel; //!< Optimal audio level.
	float _level; //!< Audio level.

	bool  _loopThru; //!< Method variable to check whether audio is loop thru or not.
	bool  _muted; //!< Method variable to check whether audio is muted or not.





public:

	 struct IEvent{

            // @brief Associated Audio mixing changed
            // @text onAssociatedAudioMixingChanged
            // @param mixing: true or false
            virtual void OnAssociatedAudioMixingChanged(bool mixing) { };
    
            // @brief Audio Fader balance changed
            // @text onAudioFaderControlChanged
            // @param mixerBalance: applied mixer balance value
            virtual void OnAudioFaderControlChanged(int mixerBalance) { };
    
            // @brief Primary language for Audio changed
            // @text onAudioPrimaryLanguageChanged
            // @param primaryLanguage: current primary language for audio
            virtual void OnAudioPrimaryLanguageChanged(const std::string& primaryLanguage) { };

            // @brief Secondary language for Audio changed
            // @text onAudioSecondaryLanguageChanged
            // @param secondaryLanguage: current secondary language for audio
            virtual void OnAudioSecondaryLanguageChanged(const std::string& secondaryLanguage) { };
    
            // @brief Audio output hot plug event
            // @text onAudioOutHotPlug
            // @param portType: Type of audio port see AudioPortType
            // @param uiPortNumber: The port number assigned by UI
            // @param isPortConnected: true (connected) or false (not connected)
            virtual void OnAudioOutHotPlug(dsAudioPortType_t portType, int uiPortNumber, bool isPortConnected) { };


            // @brief Dolby Atmos capabilities changed
            // @text onDolbyAtmosCapabilitiesChanged
            // @param atmosCapability: the dolby atmos capability
            // @param status: true (available) or false (not available)
            virtual void OnDolbyAtmosCapabilitiesChanged(dsATMOSCapability_t atmosCapability, bool status) { };

            // @brief Audio port state changed
            // @text onAudioPortStateChanged
            // @param audioPortState: audio port state
            virtual void OnAudioPortStateChanged(dsAudioPortState_t audioPortState) { };

            // @brief Audio mode for the respective audio port - raised for every type of port
            // @text onAudioModeEvent
            // @param audioPortType: audio port type see AudioPortType
            // @param audioMode: audio mode - see StereoMode
            virtual void OnAudioModeEvent(dsAudioPortType_t  audioPortType, dsAudioStereoMode_t audioMode) { };

            // @brief Audio level changed
            // @text OnAudioLevelChangedEvent
            // @param audioiLevel: audio level value
            virtual void OnAudioLevelChangedEvent(int audioLevel) { };

            // @brief Audio Output format changed
            // @text onAudioFormatUpdate
            // @param audioFormat: Type of audio format see AudioFormat
            virtual void OnAudioFormatUpdate(dsAudioFormat_t audioFormat) { };

        };

        uint32_t Register(IEvent *listener);
        uint32_t UnRegister(IEvent *listener);

	static AudioOutputPort & getInstance(int id);
	static AudioOutputPort & getInstance(const std::string &name);

	AudioOutputPort(const int type, const int index, const int id);
	virtual ~AudioOutputPort();

	const AudioOutputPortType & getType() const;
	int getId() const {return _id;};
	int getIndex() const {return _index; };


/**
 * @fn AudioOutputPort::getName() const
 * @brief This API gets the name of the AudioOutputPort.  The AudioOutputPort names can be
 * IDLR, HDMI and SPDIF which are created and used by the implementation to uniquely identify them.
 *
 * @return _name Indicates the name of the AudioOutputPort
 */
	const std::string &getName() const {return _name;};
	const List<AudioEncoding> getSupportedEncodings() const;
	const List<AudioCompression> getSupportedCompressions() const;
	const List<AudioStereoMode> getSupportedStereoModes() const;


	const AudioEncoding &getEncoding() const;
	int getCompression() const;
	int getDialogEnhancement() const;
	bool getDolbyVolumeMode() const;
	int getIntelligentEqualizerMode() const;
        dsVolumeLeveller_t getVolumeLeveller() const;
        int getBassEnhancer() const;
        bool isSurroundDecoderEnabled() const;
        int getDRCMode() const;
        dsSurroundVirtualizer_t getSurroundVirtualizer() const;
        bool getMISteering() const;
	int getGraphicEqualizerMode() const;
	const std::string getMS12AudioProfile() const;
	std::vector<std::string> getMS12AudioProfileList() const;
	const AudioStereoMode &getStereoMode(bool usePersist = false);

    dsError_t setEnablePort(bool enabled);	
    dsError_t reInitializeAudioOutputPort();
	void enable();
	void disable();

    bool getEnablePersist () const;
    void setEnablePersist (bool isEnabled);

	bool getStereoAuto();

	float getGain() const;
	float getDB() const;
	float getLevel() const;
	float getMaxDB() const;
	float getMinDB() const;
	float getOptimalLevel() const;
	bool getAudioDelay(uint32_t& audioDelayMs) const;
	bool getAudioDelayOffset(uint32_t& audioDelayOffsetMs) const;
	bool isLoopThru() const;
	bool isMuted() const;
	bool isConnected() const;
	bool isEnabled() const;

	bool isAudioMSDecode() const;
	bool isAudioMS12Decode() const;
	
	void setEncoding(const int encoding);
	void setCompression(const int compression);
	void setDialogEnhancement(const int level);
	void setDolbyVolumeMode(const bool mode);
	void setIntelligentEqualizerMode(const int mode);
        void setVolumeLeveller(const dsVolumeLeveller_t volLeveller);
        void setBassEnhancer(const int boost);
        void enableSurroundDecoder(const bool enable);
        void setDRCMode(const int mode);
        void setSurroundVirtualizer(const dsSurroundVirtualizer_t virtualizer);
        void setMISteering(const bool enable);
	void setGraphicEqualizerMode(const int mode);
	void setMS12AudioProfile(std::string profile);

	void setStereoMode(const int mode, const bool toPersist = true);
	void setStereoAuto(const bool autoMode, const bool toPersist = true);

	void setEncoding(const std::string & encoding);
	void setStereoMode(const std::string & mode,const bool toPersist = true);
	void getSupportedARCTypes(int *types);
	void setSAD(std::vector<int> sad_list);
	void enableARC(dsAudioARCTypes_t type, bool enable);
	void enableMS12Config(const dsMS12FEATURE_t feature,const bool enable);
	dsError_t enableLEConfig(const bool enable);
        bool GetLEConfig();
        void setAudioDelay(const uint32_t audioDelayMs);
        void setAudioDelayOffset(const uint32_t audioDelayOffsetMs);
        void getSinkDeviceAtmosCapability(dsATMOSCapability_t & atmosCapability);
        void setAudioAtmosOutputMode(bool enable);

	void setAssociatedAudioMixing(const bool mixing);
        void getAssociatedAudioMixing(bool *mixing);
        void setFaderControl(const int mixerBalance);
        void getFaderControl(int* mixerBalance);
        void setPrimaryLanguage(const std::string pLang);
        void getPrimaryLanguage(std::string &pLang);
        void setSecondaryLanguage(const std::string sLang);
        void getSecondaryLanguage(std::string &sLang);

	void setDB(const float db);
        void setGain(const float newGain);
	void setLevel(const float level);
	void setLoopThru(const bool loopThru);
	void setMuted(const bool mute);
	void setAudioDucking(dsAudioDuckingAction_t action, dsAudioDuckingType_t, const unsigned char level);
        void getAudioCapabilities(int *capabilities);
        void getMS12Capabilities(int *capabilities);
        void resetDialogEnhancement();
        void resetBassEnhancer();
        void resetSurroundVirtualizer();
        void resetVolumeLeveller();
        void setMS12AudioProfileSetttingsOverride(const std::string ProfileState,const std::string ProfileName,
                                                   const std::string ProfileSettingsName, const std::string ProfileSettingValue);
        void getHdmiArcPortId(int *portId);
};

}

#endif /* _DS_AUDIOOUTPUTPORT_HPP_ */


/** @} */
/** @} */
