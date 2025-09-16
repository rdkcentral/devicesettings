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


#include "dsAudio.h"

#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <pthread.h>
#include <atomic>
#include <dlfcn.h>
#include "dsError.h"
#include "dsUtl.h"
#include "dsTypes.h"
#include "pthread.h"
#include "libIARM.h"
#include "libIBus.h"
#include "iarmUtil.h"
#include "dsRpc.h"
#include "dsMgr.h"
#include "hostPersistence.hpp"
#include "dsserverlogger.h"
#include "dsAudioSettings.h"

#include "safec_lib.h"

#if 0
#include <dlfcn.h>
#include "libIARM.h"
#include "libIBus.h"
#include "iarmUtil.h"
#include "dsAudioSettings.h"

#define IARM_BUS_Lock(lock) pthread_mutex_lock(&dsLock)
#define IARM_BUS_Unlock(lock) pthread_mutex_unlock(&dsLock)

static pthread_mutex_t dsLock = PTHREAD_MUTEX_INITIALIZER;
#endif

extern dsAudioTypeConfig_t  kConfig_audio[dsAUDIOPORT_TYPE_MAX];
extern dsAudioPortConfig_t  kPort_audio[dsAUDIOPORT_TYPE_MAX];
extern int kConfig_size, kPort_size;
static dsAudioTypeConfig_t  *kConfigs1 = NULL;//, *kConfigs2 = kConfig_audio;
static dsAudioPortConfig_t  *kPorts1 = NULL;//, *kPorts2 = kPort_audio;
int *pKConSize, *pKPortSize;
static int kConfig_size_local = -1, kPort_size_local = -1;



static int m_isInitialized = 0;
static int m_isPlatInitialized = 0;

static bool m_MS12DAPV2Enabled = 0;
static bool m_MS12DEEnabled = 0;
static bool m_LEEnabled = 0;
static int m_volumeDuckingLevel = 0;
static float m_volumeLevel = 0;
static int m_MuteStatus = false;
static int m_isDuckingInProgress = false;
static bool m_AudioPortEnabled[dsAUDIOPORT_TYPE_MAX] = {false};

static pthread_mutex_t dsLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t audioLevelMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t      audioLevelTimerCV = PTHREAD_COND_INITIALIZER;
#ifdef IGNORE_EDID_LOGIC
int _srv_AudioHDMIAuto = 1;
int _srv_AudioSPDIFAuto = 1;
int _srv_AudioHDMIARCAuto = 1;
dsAudioStereoMode_t _srv_HDMI_Audiomode = dsAUDIO_STEREO_SURROUND;
dsAudioStereoMode_t _srv_SPDIF_Audiomode = dsAUDIO_STEREO_SURROUND;
dsAudioStereoMode_t _srv_HDMI_ARC_Audiomode = dsAUDIO_STEREO_SURROUND;
#else
int _srv_AudioHDMIAuto = 0;
int _srv_AudioSPDIFAuto = 0;
int _srv_AudioHDMIARCAuto = 0;
dsAudioStereoMode_t _srv_HDMI_Audiomode = dsAUDIO_STEREO_STEREO;
dsAudioStereoMode_t _srv_SPDIF_Audiomode = dsAUDIO_STEREO_STEREO;
dsAudioStereoMode_t _srv_HDMI_ARC_Audiomode = dsAUDIO_STEREO_STEREO;
#endif

#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
static std::atomic<bool> persist_audioLevel_timer_threadIsAlive = ATOMIC_VAR_INIT(false);
static pthread_t persist_audioLevel_timer_threadId;
static std::atomic<float> audioLevel_cache_spdif = ATOMIC_VAR_INIT(0.0);
static std::atomic<float> audioLevel_cache_speaker = ATOMIC_VAR_INIT(0.0);
static std::atomic<float> audioLevel_cache_hdmi = ATOMIC_VAR_INIT(0.0);
static std::atomic<float> audioLevel_cache_headphone = ATOMIC_VAR_INIT(0.0);
static std::atomic<bool>  audioLevel_timer_set = ATOMIC_VAR_INIT(false);
static void* persist_audioLevel_timer_threadFunc(void*arg);
#endif

#define IARM_BUS_Lock(lock) pthread_mutex_lock(&dsLock)
#define IARM_BUS_Unlock(lock) pthread_mutex_unlock(&dsLock)

IARM_Result_t _dsAudioPortInit(void *arg);
IARM_Result_t _dsGetAudioPort(void *arg);
IARM_Result_t _dsGetSupportedARCTypes(void *arg);
IARM_Result_t _dsAudioSetSAD(void *arg);
IARM_Result_t _dsAudioEnableARC(void *arg);
IARM_Result_t _dsSetStereoMode(void *arg);
IARM_Result_t _dsSetStereoAuto(void *arg);
IARM_Result_t _dsGetStereoAuto(void *arg);
IARM_Result_t _dsSetAudioMute(void *arg);
IARM_Result_t _dsIsAudioMute(void *arg);
IARM_Result_t _dsAudioPortTerm(void *arg);
IARM_Result_t _dsGetStereoMode(void *arg);
IARM_Result_t _dsGetAudioFormat(void *arg);
IARM_Result_t _dsGetEncoding(void *arg);
IARM_Result_t _dsIsAudioMSDecode(void *arg);
IARM_Result_t _dsIsAudioMS12Decode(void *arg);
IARM_Result_t _dsIsAudioPortEnabled(void *arg);


IARM_Result_t _dsGetEnablePersist(void *arg);
IARM_Result_t _dsSetEnablePersist(void *arg);

IARM_Result_t _dsEnableAudioPort(void *arg);
IARM_Result_t _dsSetAudioDucking(void *arg);
IARM_Result_t _dsGetAudioLevel(void *arg);
IARM_Result_t _dsSetAudioLevel(void *arg);
IARM_Result_t _dsGetAudioGain(void *arg);
IARM_Result_t _dsSetAudioGain(void *arg);
IARM_Result_t _dsEnableLEConfig(void *arg);
IARM_Result_t _dsGetLEConfig(void *arg);
IARM_Result_t _dsSetAudioDelay(void *arg);
IARM_Result_t _dsGetAudioDelay(void *arg);
IARM_Result_t _dsGetSinkDeviceAtmosCapability(void *arg);
IARM_Result_t _dsSetAudioAtmosOutputMode(void *arg);
IARM_Result_t _dsSetAudioMixerLevels (void *arg);
IARM_Result_t _dsSetAudioCompression(void *arg);
IARM_Result_t _dsGetAudioCompression(void *arg);
IARM_Result_t _dsSetDialogEnhancement(void *arg);
IARM_Result_t _dsGetDialogEnhancement(void *arg);
IARM_Result_t _dsSetDolbyVolumeMode(void *arg);
IARM_Result_t _dsGetDolbyVolumeMode(void *arg);
IARM_Result_t _dsSetIntelligentEqualizerMode(void *arg);
IARM_Result_t _dsGetIntelligentEqualizerMode(void *arg);
IARM_Result_t _dsSetGraphicEqualizerMode(void *arg);
IARM_Result_t _dsGetGraphicEqualizerMode(void *arg);

IARM_Result_t _dsGetVolumeLeveller(void *arg);
IARM_Result_t _dsSetVolumeLeveller(void *arg);
IARM_Result_t _dsGetBassEnhancer(void *arg);
IARM_Result_t _dsSetBassEnhancer(void *arg);
IARM_Result_t _dsIsSurroundDecoderEnabled(void *arg);
IARM_Result_t _dsEnableSurroundDecoder(void *arg);
IARM_Result_t _dsGetDRCMode(void *arg);
IARM_Result_t _dsSetDRCMode(void *arg);
IARM_Result_t _dsGetSurroundVirtualizer(void *arg);
IARM_Result_t _dsSetSurroundVirtualizer(void *arg);
IARM_Result_t _dsGetMISteering(void *arg);
IARM_Result_t _dsSetMISteering(void *arg);
IARM_Result_t _dsGetMS12AudioProfileList(void *arg);
IARM_Result_t _dsGetMS12AudioProfile(void *arg);
IARM_Result_t _dsSetMS12AudioProfile(void *arg);

IARM_Result_t _dsGetAssociatedAudioMixing(void *arg);
IARM_Result_t _dsSetAssociatedAudioMixing(void *arg);
IARM_Result_t _dsGetFaderControl(void *arg);
IARM_Result_t _dsSetFaderControl(void *arg);
IARM_Result_t _dsGetPrimaryLanguage(void *arg);
IARM_Result_t _dsSetPrimaryLanguage(void *arg);
IARM_Result_t _dsGetSecondaryLanguage(void *arg);
IARM_Result_t _dsSetSecondaryLanguage(void *arg);

IARM_Result_t _dsGetAudioCapabilities(void *arg);
IARM_Result_t _dsGetMS12Capabilities(void *arg);
IARM_Result_t _dsAudioOutIsConnected(void *arg);
IARM_Result_t _dsResetBassEnhancer(void *arg);
IARM_Result_t _dsResetSurroundVirtualizer(void *arg);
IARM_Result_t _dsResetVolumeLeveller(void *arg);
IARM_Result_t _dsResetDialogEnhancement(void *arg);
IARM_Result_t _dsSetMS12SetttingsOverride(void *arg);
IARM_Result_t _dsGetHDMIARCPortId(void *arg);

bool dsSetAudioDelayInternal(intptr_t handle,uint32_t audioDelay);
uint32_t dsGetAudioDelayInternal(dsAudioPortType_t _APortType);

static void _GetAudioModeFromPersistent(void *arg);
static dsAudioPortType_t _GetAudioPortType(intptr_t handle);
void _dsAudioOutPortConnectCB(dsAudioPortType_t portType, unsigned int uiPortNo, bool isPortConnected);
static dsError_t _dsAudioOutRegisterConnectCB (dsAudioOutPortConnectCB_t cbFun);
void _dsAudioFormatUpdateCB(dsAudioFormat_t audioFormat);
void _dsAudioAtmosCapsChangeCB(dsATMOSCapability_t atmosCap, bool status);
static dsError_t _dsAudioFormatUpdateRegisterCB (dsAudioFormatUpdateCB_t cbFun);
static dsError_t _dsAudioAtmosCapsChangeRegisterCB(dsAtmosCapsChangeCB_t CBFunc);
static std::string _dsGetCurrentProfileProperty(std::string property);
static void _dsMS12ProfileSettingOverride(intptr_t handle);
static std::string _dsGenerateProfileProperty(std::string profile,std::string property);
static bool _dsMs12ProfileSupported(intptr_t handle,std::string profile);
static IARM_Result_t _resetDialogEnhancerLevel(intptr_t handle);
static IARM_Result_t _resetBassEnhancer(intptr_t handle);
static IARM_Result_t _resetVolumeLeveller(intptr_t handle);
static IARM_Result_t _resetSurroundVirtualizer(intptr_t handle);
static IARM_Result_t _setDialogEnhancement(intptr_t handle, int enhancerLevel);
static IARM_Result_t _setBassEnhancer(intptr_t handle ,int boost);
static IARM_Result_t _setVolumeLeveller(intptr_t handle, int volLevellerMode, int volLevellerLevel);
static IARM_Result_t _setSurroundVirtualizer(intptr_t handle , int virtualizerMode , int virtualizerBoost);
static IARM_Result_t setAudioDuckingAudioLevel(intptr_t handle);

typedef dsError_t (*dsSetAudioLevel_t)(intptr_t handle, float level);
static dsSetAudioLevel_t dsSetAudioLevelFunc = 0;

typedef dsError_t (*dsGetAudioLevel_t)(intptr_t handle, float *level);
static dsGetAudioLevel_t dsGetAudioLevelfunc = 0;

void AudioConfigInit()
{
    typedef dsError_t  (*dsEnableLEConfig_t)(intptr_t handle, const bool enable);
    intptr_t handle = 0;
    void *dllib = NULL;
    static dsEnableLEConfig_t func = NULL;
    if (dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
      if (func == NULL) {
        dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsEnableLEConfig_t) dlsym(dllib, "dsEnableLEConfig");
            if (func) {
                INT_DEBUG("dsEnableLEConfig(int, bool) is defined and loaded\r\n");
                std::string _LEEnable("FALSE");
                try
                {
                    _LEEnable = device::HostPersistence::getInstance().getProperty("audio.LEEnable");
                }
                catch(...)
                {
#ifndef DS_LE_DEFAULT_DISABLED
                    _LEEnable = "TRUE";
#endif
                   INT_DEBUG("LE : Persisting default LE status: %s \r\n",_LEEnable.c_str());
                   device::HostPersistence::getInstance().persistHostProperty("audio.LEEnable",_LEEnable);
                }
                if(_LEEnable == "TRUE")
                {
                    m_LEEnabled = 1;
                    func(handle,m_LEEnabled);
                }
                else
                {
                    m_LEEnabled = 0;
                    func(handle,m_LEEnabled);
                }
            }
            else {
                INT_INFO("dsEnableLEConfig(int,  bool) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_INFO("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
      }
    }
    else {
        INT_ERROR("dsEnableLEConfig(int,  bool) is failed. since dsAUDIOPORT_TYPE_HDMI 0 port not available\r\n");
    }

#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
    typedef dsError_t (*dsSetAudioGain_t)(intptr_t handle, float gain);
    static dsSetAudioGain_t dsSetAudioGainFunc = 0;
    if (dsSetAudioGainFunc == 0) {
        dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsSetAudioGainFunc = (dsSetAudioGain_t) dlsym(dllib, "dsSetAudioGain");
            if (dsSetAudioGainFunc) {
                INT_DEBUG("dsSetAudioGain_t(int, float ) is defined and loaded\r\n");
                std::string _AudioGain("0");
                float m_audioGain = 0;
//SPEAKER init
                handle = 0;
                if (dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                    try {
                        _AudioGain = device::HostPersistence::getInstance().getProperty("SPEAKER0.audio.Gain");
                    }
                    catch(...) {
                            try {
                                INT_DEBUG("SPEAKER0.audio.Gain not found in persistence store. Try system default\n");
                                _AudioGain = device::HostPersistence::getInstance().getDefaultProperty("SPEAKER0.audio.Gain");
                            }
                            catch(...) {
                                _AudioGain = "0";
                            }
                    }
                    m_audioGain = atof(_AudioGain.c_str());
                    if (dsSetAudioGainFunc(handle, m_audioGain) == dsERR_NONE) {
                        INT_INFO("Port %s: Initialized audio gain : %f\n","SPEAKER0", m_audioGain);
                    }
                }
//HDMI init
                handle = 0;
                if (dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                    try {
                        _AudioGain = device::HostPersistence::getInstance().getProperty("HDMI0.audio.Gain");
                    }
                    catch(...) {
                            try {
                                INT_DEBUG("HDMI0.audio.Gain not found in persistence store. Try system default\n");
                                _AudioGain = device::HostPersistence::getInstance().getDefaultProperty("HDMI0.audio.Gain");
                            }
                            catch(...) {
                                _AudioGain = "0";
                            }
                    }
                    m_audioGain = atof(_AudioGain.c_str());
                    if (dsSetAudioGainFunc(handle, m_audioGain) == dsERR_NONE) {
                        INT_INFO("Port %s: Initialized audio gain : %f\n","HDMI0", m_audioGain);
                    }
                }

            }
            else {
                INT_INFO("dsSetAudioGain_t(int, float ) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    if (dsSetAudioLevelFunc == 0) {
        dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsSetAudioLevelFunc = (dsSetAudioLevel_t) dlsym(dllib, "dsSetAudioLevel");
            if (dsSetAudioLevelFunc) {
                INT_DEBUG("dsSetAudioLevel_t(int, float ) is defined and loaded\r\n");
                std::string _AudioLevel("0");
                float m_audioLevel = 0;
//SPDIF init
                handle = 0;
                if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPDIF,0,&handle) == dsERR_NONE) {
                    try {
                        _AudioLevel = device::HostPersistence::getInstance().getProperty("SPDIF0.audio.Level");
                    }
                    catch(...) {
                            try {
                                INT_DEBUG("SPDIF0.audio.Level not found in persistence store. Try system default\n");
                                _AudioLevel = device::HostPersistence::getInstance().getDefaultProperty("SPDIF0.audio.Level");
                            }
                            catch(...) {
                                _AudioLevel = "40";
                            }
                    }
                    m_audioLevel = atof(_AudioLevel.c_str());
                    audioLevel_cache_spdif = m_audioLevel;
                    if (dsSetAudioLevelFunc(handle, m_audioLevel) == dsERR_NONE) {
                        INT_INFO("Port %s: Initialized audio level : %f\n","SPDIF0", m_audioLevel);
                        m_volumeLevel = m_audioLevel;
                    }
                }
//SPEAKER init
                handle = 0;
                if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                    try {
                        _AudioLevel = device::HostPersistence::getInstance().getProperty("SPEAKER0.audio.Level");
                    }
                    catch(...) {
                            try {
                                INT_DEBUG("SPEAKER0.audio.Level not found in persistence store. Try system default\n");
                                _AudioLevel = device::HostPersistence::getInstance().getDefaultProperty("SPEAKER0.audio.Level");
                            }
                            catch(...) {
                                _AudioLevel = "40";
                            }
                    }
                    m_audioLevel = atof(_AudioLevel.c_str());
		    audioLevel_cache_speaker = m_audioLevel;
                    if (dsSetAudioLevelFunc(handle, m_audioLevel) == dsERR_NONE) {
                        m_volumeLevel = m_audioLevel;
                        INT_INFO("Port %s: Initialized audio level : %f\n","SPEAKER0", m_audioLevel);
                    }
                }
//HEADPHONE init
                handle = 0;
                if(dsGetAudioPort(dsAUDIOPORT_TYPE_HEADPHONE,0,&handle) == dsERR_NONE) {
                    try {
                        _AudioLevel = device::HostPersistence::getInstance().getProperty("HEADPHONE0.audio.Level");
                    }
                    catch(...) {
                            try {
                                INT_DEBUG("HEADPHONE0.audio.Level not found in persistence store. Try system default\n");
                                _AudioLevel = device::HostPersistence::getInstance().getDefaultProperty("HEADPHONE0.audio.Level");
                            }
                            catch(...) {
                                _AudioLevel = "40";
                            }
                    }
                    m_audioLevel = atof(_AudioLevel.c_str());
		    audioLevel_cache_headphone = m_audioLevel;
                    if (dsSetAudioLevelFunc(handle, m_audioLevel) == dsERR_NONE) {
                        INT_INFO("Port %s: Initialized audio level : %f\n","HEADPHONE0", m_audioLevel);
                    }
                }
//HDMI init
                handle = 0;
                if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                    try {
                        _AudioLevel = device::HostPersistence::getInstance().getProperty("HDMI0.audio.Level");
                    }
                    catch(...) {
                            try {
                                INT_DEBUG("HDMI0.audio.Level not found in persistence store. Try system default\n");
                                _AudioLevel = device::HostPersistence::getInstance().getDefaultProperty("HDMI0.audio.Level");
                            }
                            catch(...) {
                                _AudioLevel = "40";
                            }
                    }
                    m_audioLevel = atof(_AudioLevel.c_str());
		    audioLevel_cache_hdmi = m_audioLevel;
                    if (dsSetAudioLevelFunc(handle, m_audioLevel) == dsERR_NONE) {
                        INT_INFO("Port %s: Initialized audio level : %f\n","HDMI0", m_audioLevel);
                        m_volumeLevel = m_audioLevel;
                    }
                }
		
            }
            else {
                INT_INFO("dsSetAudioLevel_t(int, float ) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }


    typedef dsError_t (*dsSetAudioDelay_t)(intptr_t handle, uint32_t audioDelayMs);
    static dsSetAudioDelay_t dsSetAudioDelayFunc = 0;
    if (dsSetAudioDelayFunc == 0) {
        dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsSetAudioDelayFunc = (dsSetAudioDelay_t) dlsym(dllib, "dsSetAudioDelay");
            if (dsSetAudioDelayFunc) {
                INT_DEBUG("dsSetAudioDelay_t(int, uint32_t) is defined and loaded\r\n");
                std::string _AudioDelay("0");
                int m_audioDelay = 0;
//SPEAKER init
                handle = 0;
                if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                    try {
                        _AudioDelay = device::HostPersistence::getInstance().getProperty("SPEAKER0.audio.Delay");
                    }
                    catch(...) {
                            try {
                                INT_DEBUG("SPEAKER0.audio.Delay not found in persistence store. Try system default\n");
                                _AudioDelay = device::HostPersistence::getInstance().getDefaultProperty("SPEAKER0.audio.Delay");
                            }
                            catch(...) {
                                _AudioDelay = "0";
                            }
                    }
                    m_audioDelay = atoi(_AudioDelay.c_str());
                    if (dsSetAudioDelayFunc(handle, m_audioDelay) == dsERR_NONE) {
                        INT_INFO("Port %s: Initialized audio delay : %d\n","SPEAKER0", m_audioDelay);
                    }
                }
//HDMI init
                handle = 0;
                if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                    try {
                        _AudioDelay = device::HostPersistence::getInstance().getProperty("HDMI0.audio.Delay");
                    }
                    catch(...) {
                            try {
                                INT_DEBUG("HDMI0.audio.Delay not found in persistence store. Try system default\n");
                                _AudioDelay = device::HostPersistence::getInstance().getDefaultProperty("HDMI0.audio.Delay");
                            }
                            catch(...) {
                                _AudioDelay = "0";
                            }
                    }
                    m_audioDelay = atoi(_AudioDelay.c_str());
                    if (dsSetAudioDelayFunc(handle, m_audioDelay) == dsERR_NONE) {
                        INT_INFO("Port %s: Initialized audio delay : %d\n","HDMI0", m_audioDelay);
                    }
                }
//HDMI ARC init
                handle = 0;
                if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI_ARC,0,&handle) == dsERR_NONE) {
                    try {
                        _AudioDelay = device::HostPersistence::getInstance().getProperty("HDMI_ARC0.audio.Delay");
                    }
                    catch(...) {
                            try {
                                INT_DEBUG("HDMI_ARC0.audio.Delay not found in persistence store. Try system default\n");
                                _AudioDelay = device::HostPersistence::getInstance().getDefaultProperty("HDMI_ARC0.audio.Delay");
                            }
                            catch(...) {
                                _AudioDelay = "0";
                            }
                    }
                    m_audioDelay = atoi(_AudioDelay.c_str());
                    if (dsSetAudioDelayFunc(handle, m_audioDelay) == dsERR_NONE) {
                        INT_INFO("Port %s: Initialized audio delay : %d\n","HDMI_ARC0", m_audioDelay);
                    }
                }


            }
            else {
                INT_INFO("dsSetAudioDelay_t(int, uint32_t) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    typedef dsError_t (*dsSetPrimaryLanguage_t)(intptr_t handle, const char* pLang);;
    static dsSetPrimaryLanguage_t dsSetPrimaryLanguageFunc = 0;
    if (dsSetPrimaryLanguageFunc == 0) {
        dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsSetPrimaryLanguageFunc = (dsSetPrimaryLanguage_t) dlsym(dllib, "dsSetPrimaryLanguage");
            if (dsSetPrimaryLanguageFunc) {
                INT_DEBUG("dsSetPrimaryLanguage_t(int, char* ) is defined and loaded\r\n");
                std::string _PrimaryLanguage("eng");
                handle = 0;
                try {
                    _PrimaryLanguage = device::HostPersistence::getInstance().getProperty("audio.PrimaryLanguage");
                }
                catch(...) {
                        try {
                            INT_DEBUG("audio.PrimaryLanguage not found in persistence store. Try system default\n");
                            _PrimaryLanguage = device::HostPersistence::getInstance().getDefaultProperty("audio.PrimaryLanguage");
                        }
                        catch(...) {
                            _PrimaryLanguage = "eng";
                        }
                }
                if (dsSetPrimaryLanguageFunc(handle, _PrimaryLanguage.c_str()) == dsERR_NONE) {
                    INT_INFO("Initialized Primary Language : %s\n", _PrimaryLanguage.c_str());
                }
            }
            else {
                INT_INFO("dsSetPrimaryLanguage_t(int, char* ) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }


    typedef dsError_t (*dsSetSecondaryLanguage_t)(intptr_t handle, const char* sLang);;
    static dsSetSecondaryLanguage_t dsSetSecondaryLanguageFunc = 0;
    if (dsSetSecondaryLanguageFunc == 0) {
        dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsSetSecondaryLanguageFunc = (dsSetSecondaryLanguage_t) dlsym(dllib, "dsSetSecondaryLanguage");
            if (dsSetSecondaryLanguageFunc) {
                INT_DEBUG("dsSetSecondaryLanguage_t(int, char* ) is defined and loaded\r\n");
                std::string _SecondaryLanguage("eng");
                handle = 0;
                try {
                    _SecondaryLanguage = device::HostPersistence::getInstance().getProperty("audio.SecondaryLanguage");
                }
                catch(...) {
                        try {
                            INT_DEBUG("audio.SecondaryLanguage not found in persistence store. Try system default\n");
                            _SecondaryLanguage = device::HostPersistence::getInstance().getDefaultProperty("audio.SecondaryLanguage");
                        }
                        catch(...) {
                            _SecondaryLanguage = "eng";
                        }
                }
                if (dsSetSecondaryLanguageFunc(handle, _SecondaryLanguage.c_str()) == dsERR_NONE) {
                    INT_INFO("Initialized Secondary Language : %s\n", _SecondaryLanguage.c_str());
                }
            }
            else {
                INT_INFO("dsSetSecondaryLanguage_t(int, char* ) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }


    typedef dsError_t (*dsSetFaderControl_t)(intptr_t handle, int mixerbalance);
    static dsSetFaderControl_t dsSetFaderControlFunc = 0;
    if (dsSetFaderControlFunc == 0) {
        dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsSetFaderControlFunc = (dsSetFaderControl_t) dlsym(dllib, "dsSetFaderControl");
            if (dsSetFaderControlFunc) {
                INT_DEBUG("dsSetFaderControl_t(int, int) is defined and loaded\r\n");
                std::string _FaderControl("0");
                int m_faderControl = 0;

                handle = 0;
                try {
                    _FaderControl = device::HostPersistence::getInstance().getProperty("audio.FaderControl");
                }
                catch(...) {
                        try {
                            INT_DEBUG("audio.FaderControl not found in persistence store. Try system default\n");
                            _FaderControl = device::HostPersistence::getInstance().getDefaultProperty("audio.FaderControl");
                        }
                        catch(...) {
                            _FaderControl = "0";
                        }
                }
                m_faderControl = atoi(_FaderControl.c_str());
                if (dsSetFaderControlFunc(handle, m_faderControl) == dsERR_NONE) {
                    INT_INFO("Initialized Fader Control, mixing : %d\n", m_faderControl);
                }

            }
            else {
                INT_INFO("dsSetFaderControl_t(int, int) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }


    typedef dsError_t (*dsSetAssociatedAudioMixing_t)(intptr_t handle, bool mixing);
    static dsSetAssociatedAudioMixing_t dsSetAssociatedAudioMixingFunc = 0;
    if (dsSetAssociatedAudioMixingFunc == 0) {
        dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsSetAssociatedAudioMixingFunc = (dsSetAssociatedAudioMixing_t) dlsym(dllib, "dsSetAssociatedAudioMixing");
            if (dsSetAssociatedAudioMixingFunc) {
                INT_DEBUG("dsSetAssociatedAudioMixing_t (intptr_t handle, bool mixing ) is defined and loaded\r\n");
                std::string _AssociatedAudioMixing("Disabled");
                bool m_AssociatedAudioMixing = false;
                try {
                    _AssociatedAudioMixing = device::HostPersistence::getInstance().getProperty("audio.AssociatedAudioMixing");
                }
                catch(...) {
                    try {
                        INT_DEBUG("audio.AssociatedAudioMixing not found in persistence store. Try system default\n");
                        _AssociatedAudioMixing = device::HostPersistence::getInstance().getDefaultProperty("audio.AssociatedAudioMixing");
                    }
                    catch(...) {
                        _AssociatedAudioMixing = "Disabled";
                    }
                }
                if (_AssociatedAudioMixing == "Enabled") {
                    m_AssociatedAudioMixing = true;
                }
                else {
                    m_AssociatedAudioMixing = false;
                }
                handle = 0;
                if (dsSetAssociatedAudioMixingFunc(handle, m_AssociatedAudioMixing) == dsERR_NONE) {
                    INT_INFO("Initialized AssociatedAudioMixingFunc : %d\n", m_AssociatedAudioMixing);
                }
            }
            else {
                INT_INFO("dsSetAssociatedAudioMixing_t (intptr_t handle, bool enable) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    std::string _AProfileSupport("FALSE");
    std::string _AProfile("Off");
    try {
        _AProfileSupport = device::HostPersistence::getInstance().getDefaultProperty("audio.MS12Profile.supported");
    }
    catch(...) {
        _AProfileSupport = "FALSE";
        INT_INFO("audio.MS12Profile.supported setting not found in hostDataDeafult \r\n");
    }
    INT_INFO(" audio.MS12Profile.supported = %s ..... \r\n",_AProfileSupport.c_str());


    if(_AProfileSupport == "TRUE") {
        typedef dsError_t (*dsSetMS12AudioProfile_t)(intptr_t handle, const char* profile);
        static dsSetMS12AudioProfile_t dsSetMS12AudioProfileFunc = 0;
        if (dsSetMS12AudioProfileFunc == 0) {
            dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
            if (dllib) {
                dsSetMS12AudioProfileFunc = (dsSetMS12AudioProfile_t) dlsym(dllib, "dsSetMS12AudioProfile");
                if (dsSetMS12AudioProfileFunc) {
                    INT_DEBUG("dsSetMS12AudioProfile_t(int, const char*) is defined and loaded\r\n");
                    handle = 0;
                    if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) != dsERR_NONE) {
                        INT_ERROR("dsGetAudioPort failed for SPEAKER0\n");
                    }

                    try {
                        _AProfile = device::HostPersistence::getInstance().getProperty("audio.MS12Profile");
                    }
                    catch(...) {
                            try {
                                INT_DEBUG("audio.MS12Profile not found in persistence store. Try system default\n");
                                _AProfile = device::HostPersistence::getInstance().getDefaultProperty("audio.MS12Profile");
                            }
                            catch(...) {
                                _AProfile = "Off";
                            }
                    }
        //SPEAKER init
                    handle = 0;
                    if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                        if (dsSetMS12AudioProfileFunc(handle, _AProfile.c_str()) == dsERR_NONE) {
                            INT_INFO("Port %s: Initialized MS12 Audio Profile : %s\n","SPEAKER0", _AProfile.c_str());
                            device::HostPersistence::getInstance().persistHostProperty("audio.MS12Profile",_AProfile.c_str());
                        }
                       else {
                            INT_INFO("Port %s: Initialization failed !!!  MS12 Audio Profile : %s\n","SPEAKER0", _AProfile.c_str());
                       }
                    }
    #if 0 // No Audio Profile support for STB devices
        //HDMI init
                    handle = 0;
                    if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                        if (dsSetMS12AudioProfileFunc(handle, _AProfile.c_str()) == dsERR_NONE) {
                            INT_DEBUG("Port %s: Initialized MS12 Audio Profile  : %d\n","HDMI0", _AProfile.c_str());
                        }
                    }
    #endif
                }
                else {
                    INT_INFO("dsSetMS12AudioProfile_t(int, const char*) is not defined\r\n");
                }
                dlclose(dllib);
            }
            else {
                INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
            }
        }
    }
//All MS12 Settings can be initialised through MS12 audio profiles
//User setting persistence override available for any individual setting on top of profiles
//All MS12 settings can be turned off (DAP Off Mode) by configuring Audio Profile to Off
    if((_AProfileSupport == "TRUE") && (_AProfile != "Off")) {
        std::string _profileOverride = "FALSE";
        try {
            _profileOverride = device::HostPersistence::getInstance().getDefaultProperty("audio.Compression.ms12ProfileOverride");
        }
        catch(...) {
            _profileOverride = "FALSE";
            INT_INFO(" audio.Compression.ms12ProfileOverride settings not found in system default ..... \r\n");
        }
        INT_INFO(" audio.Compression.ms12ProfileOverride = %s ..... \r\n",_profileOverride.c_str());

        if(_profileOverride == "TRUE") {
            typedef dsError_t (*dsSetAudioCompression_t)(intptr_t handle, int compressionLevel);
            static dsSetAudioCompression_t dsSetAudioCompressionFunc = 0;
            if (dsSetAudioCompressionFunc == 0) {
                void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    dsSetAudioCompressionFunc = (dsSetAudioCompression_t) dlsym(dllib, "dsSetAudioCompression");
                    if (dsSetAudioCompressionFunc) {
                        INT_DEBUG("dsSetAudioCompression_t(int, int ) is defined and loaded\r\n");
                        std::string _AudioCompression("0");
                        int m_audioCompression = 0;
                        try {
                            _AudioCompression = device::HostPersistence::getInstance().getProperty("audio.Compression");
                            m_audioCompression = atoi(_AudioCompression.c_str());
            //SPEAKER init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                                if (dsSetAudioCompressionFunc(handle, m_audioCompression) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized audio compression : %d\n","SPEAKER0", m_audioCompression);
                                }
                            }
            //HDMI init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                                if (dsSetAudioCompressionFunc(handle, m_audioCompression) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized audio compression : %d\n","HDMI0", m_audioCompression);
                                }
                            }

                        }
                        catch(...) {
                            INT_INFO("audio.Compression not found in persistence store. System Default configured through profiles\n");
                        }
                    }
                    else {
                        INT_INFO("dsSetAudioCompression_t(int, int) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
        }

        _profileOverride = "FALSE";
        try {
            _profileOverride = device::HostPersistence::getInstance().getDefaultProperty("audio.DialogEnhancer.ms12ProfileOverride");
        }
        catch(...) {
            _profileOverride = "FALSE";
            INT_INFO(" audio.DialogEnhancer.ms12ProfileOverride settings not found in system default ..... \r\n");
        }
        INT_INFO(" audio.DialogEnhancer.ms12ProfileOverride = %s ..... \r\n",_profileOverride.c_str());

        if(_profileOverride == "TRUE") {
            typedef dsError_t (*dsSetDialogEnhancement_t)(intptr_t handle, int enhancerLevel);
            static dsSetDialogEnhancement_t dsSetDialogEnhancementFunc = 0;
            if (dsSetDialogEnhancementFunc == 0) {
                void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    dsSetDialogEnhancementFunc = (dsSetDialogEnhancement_t) dlsym(dllib, "dsSetDialogEnhancement");
                    if (dsSetDialogEnhancementFunc) {
                        INT_DEBUG("dsSetDialogEnhancement_t(int, int) is defined and loaded\r\n");
                        std::string _EnhancerLevel("0");
                        int m_enhancerLevel = 0;
                        std::string _Property = _dsGetCurrentProfileProperty("EnhancerLevel");
                        try {
                            _EnhancerLevel = device::HostPersistence::getInstance().getProperty(_Property);
                            m_enhancerLevel = atoi(_EnhancerLevel.c_str());
            //SPEAKER init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                                if (dsSetDialogEnhancementFunc(handle, m_enhancerLevel) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized dialog enhancement level : %d\n","SPEAKER0", m_enhancerLevel);
                                }
                            }
            //HDMI init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                                if (dsSetDialogEnhancementFunc(handle, m_enhancerLevel) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized dialog enhancement level : %d\n","HDMI0", m_enhancerLevel);
                                }
                            }
                        }
                        catch(...) {
                            INT_INFO("audio.EnhancerLevel not found in persistence store. System Default configured through profiles\n");
                        }
                    }
                    else {
                        INT_INFO("dsSetDialogEnhancement_t(int, int ) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
        }

        _profileOverride = "FALSE";
        try {
            _profileOverride = device::HostPersistence::getInstance().getDefaultProperty("audio.DolbyVolumeMode.ms12ProfileOverride");
        }
        catch(...) {
            _profileOverride = "FALSE";
            INT_INFO(" audio.DolbyVolumeMode.ms12ProfileOverride settings not found in system default ..... \r\n");
        }
        INT_INFO(" audio.DolbyVolumeMode.ms12ProfileOverride = %s ..... \r\n",_profileOverride.c_str());

        if(_profileOverride == "TRUE") {
            typedef dsError_t (*dsSetDolbyVolumeMode_t)(intptr_t handle, bool enable);
            static dsSetDolbyVolumeMode_t dsSetDolbyVolumeModeFunc = 0;
            if (dsSetDolbyVolumeModeFunc == 0) {
                dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    dsSetDolbyVolumeModeFunc = (dsSetDolbyVolumeMode_t) dlsym(dllib, "dsSetDolbyVolumeMode");
                    if (dsSetDolbyVolumeModeFunc) {
                        INT_DEBUG("dsSetDolbyVolumeMode_t(int, bool) is defined and loaded\r\n");
                        std::string _DolbyVolumeMode("FALSE");
                        bool m_dolbyVolumeMode = false;
                        try {
                            _DolbyVolumeMode = device::HostPersistence::getInstance().getProperty("audio.DolbyVolumeMode");
                            if (_DolbyVolumeMode == "TRUE") {
                                m_dolbyVolumeMode = true;
                            }
                            else {
                                m_dolbyVolumeMode = false;
                            }
            //SPEAKER init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                                if (dsSetDolbyVolumeModeFunc(handle, m_dolbyVolumeMode) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized Dolby Volume Mode : %d\n","SPEAKER0", m_dolbyVolumeMode);
                                }
                            }
            //HDMI init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                                if (dsSetDolbyVolumeModeFunc(handle, m_dolbyVolumeMode) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized Dolby Volume Mode : %d\n","HDMI0", m_dolbyVolumeMode);
                                }
                            }
                        }
                        catch(...) {
                            INT_INFO("audio.DolbyVolumeMode not found in persistence store. System Default configured through profiles\n");
                        }
                    }
                    else {
                        INT_INFO("dsSetDolbyVolumeMode_t(int, bool) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
        }

        _profileOverride = "FALSE";
        try {
            _profileOverride = device::HostPersistence::getInstance().getDefaultProperty("audio.IntelligentEQ.ms12ProfileOverride");
        }
        catch(...) {
            _profileOverride = "FALSE";
            INT_INFO(" audio.IntelligentEQ.ms12ProfileOverride settings not found in system default ..... \r\n");
        }
        INT_INFO(" audio.IntelligentEQ.ms12ProfileOverride = %s ..... \r\n",_profileOverride.c_str());

        if(_profileOverride == "TRUE") {
            typedef dsError_t (*dsSetIntelligentEqualizerMode_t)(intptr_t handle, int mode);
            static dsSetIntelligentEqualizerMode_t dsSetIntelligentEqualizerModeFunc = 0;
            if (dsSetIntelligentEqualizerModeFunc == 0) {
                dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    dsSetIntelligentEqualizerModeFunc = (dsSetIntelligentEqualizerMode_t) dlsym(dllib, "dsSetIntelligentEqualizerMode");
                    if (dsSetIntelligentEqualizerModeFunc) {
                        INT_DEBUG("dsSetIntelligentEqualizerMode_t(int, int) is defined and loaded\r\n");
                        std::string _IEQMode("0");
                        int m_IEQMode = 0;
                        handle = 0;
                        dsError_t ret = dsERR_NONE;
                        ret = dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle);
                        if (ret != dsERR_NONE) {
                            INT_ERROR("dsGetAudioPort failed for SPEAKER0\n");
                        }
                        try {
                            _IEQMode = device::HostPersistence::getInstance().getProperty("audio.IntelligentEQ");
                            m_IEQMode = atoi(_IEQMode.c_str());
            //SPEAKER init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                                if (dsSetIntelligentEqualizerModeFunc(handle, m_IEQMode) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized Intelligent Equalizer mode : %d\n","SPEAKER0", m_IEQMode);
                                }
                            }
            //HDMI init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                                if (dsSetIntelligentEqualizerModeFunc(handle, m_IEQMode) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized Intelligent Equalizer mode : %d\n","HDMI0", m_IEQMode);
                                }
                            }
                        }
                        catch(...) {
                            INT_INFO("audio.IntelligentEQ not found in persistence store. System Default configured through profiles\n");
                        }
                    }
                    else {
                        INT_INFO("dsSetIntelligentEqualizerMode_t(int, int) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
        }

        _profileOverride = "FALSE";
        try {
            _profileOverride = device::HostPersistence::getInstance().getDefaultProperty("audio.VolumeLeveller.ms12ProfileOverride");
        }
        catch(...) {
            _profileOverride = "FALSE";
            INT_INFO(" audio.VolumeLeveller.ms12ProfileOverride settings not found in system default ..... \r\n");
        }
        INT_INFO(" audio.VolumeLeveller.ms12ProfileOverride = %s ..... \r\n",_profileOverride.c_str());

        if(_profileOverride == "TRUE") {
            typedef dsError_t (*dsSetVolumeLeveller_t)(intptr_t handle, dsVolumeLeveller_t volLeveller);
            static dsSetVolumeLeveller_t dsSetVolumeLevellerFunc = 0;
            if (dsSetVolumeLevellerFunc == 0) {
                dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    dsSetVolumeLevellerFunc = (dsSetVolumeLeveller_t) dlsym(dllib, "dsSetVolumeLeveller");
                    if (dsSetVolumeLevellerFunc) {
                        INT_DEBUG("dsSetVolumeLeveller_t(int, dsVolumeLeveller_t) is defined and loaded\r\n");
                        std::string _volLevellerMode("0");
                        std::string _volLevellerLevel("0");
                        dsVolumeLeveller_t m_volumeLeveller;
                        std::string _PropertyMode = _dsGetCurrentProfileProperty("VolumeLeveller.mode");
                        std::string _Propertylevel = _dsGetCurrentProfileProperty("VolumeLeveller.level"); 
                        try {
                            _volLevellerMode = device::HostPersistence::getInstance().getProperty(_PropertyMode);
                            _volLevellerLevel = device::HostPersistence::getInstance().getProperty(_Propertylevel);
                            m_volumeLeveller.mode = atoi(_volLevellerMode.c_str());
			    m_volumeLeveller.level = atoi(_volLevellerLevel.c_str());
                //SPEAKER init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                                if (dsSetVolumeLevellerFunc(handle, m_volumeLeveller) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized Volume Leveller : Mode: %d, Level: %d\n","SPEAKER0", m_volumeLeveller.mode, m_volumeLeveller.level);
                                }
                            }
                //HDMI init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                                if (dsSetVolumeLevellerFunc(handle, m_volumeLeveller) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized Volume Leveller : Mode: %d, Level: %d\n","HDMI0", m_volumeLeveller.mode, m_volumeLeveller.level);
                                }
                            }
                        }
                        catch(...) {
                            INT_INFO("audio.VolumeLeveller not found in persistence store. System Default configured through profiles\n");
                        }
                    }
                    else {
                        INT_INFO("dsSetVolumeLeveller_t(int, dsVolumeLeveller_t) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
        }

        _profileOverride = "FALSE";
        try {
            _profileOverride = device::HostPersistence::getInstance().getDefaultProperty("audio.BassBoost.ms12ProfileOverride");
        }
        catch(...) {
            _profileOverride = "FALSE";
            INT_INFO(" audio.BassBoost.ms12ProfileOverride settings not found in system default ..... \r\n");
        }
        INT_INFO(" audio.BassBoost.ms12ProfileOverride = %s ..... \r\n",_profileOverride.c_str());

        if(_profileOverride == "TRUE") {
            typedef dsError_t (*dsSetBassEnhancer_t)(intptr_t handle, int boost);
            static dsSetBassEnhancer_t dsSetBassEnhancerFunc = 0;
            if (dsSetBassEnhancerFunc == 0) {
                dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    dsSetBassEnhancerFunc = (dsSetBassEnhancer_t) dlsym(dllib, "dsSetBassEnhancer");
                    if (dsSetBassEnhancerFunc) {
                        INT_DEBUG("dsSetBassEnhancer_t(int, int) is defined and loaded\r\n");
                        std::string _BassBoost("0");
                        int m_bassBoost = 0;
                        try {
                            _BassBoost = device::HostPersistence::getInstance().getProperty("audio.BassBoost");
                            m_bassBoost = atoi(_BassBoost.c_str());
            //SPEAKER init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                                if (dsSetBassEnhancerFunc(handle, m_bassBoost) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized Bass Boost : %d\n","SPEAKER0", m_bassBoost);
                                }
                            }
            //HDMI init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                                if (dsSetBassEnhancerFunc(handle, m_bassBoost) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized Bass Boost : %d\n","HDMI0", m_bassBoost);
                                }
                            }
                        }
                        catch(...) {
                            INT_INFO("audio.BassBoost not found in persistence store. System Default configured through profiles\n");
                        }
                    }
                    else {
                        INT_INFO("dsSetBassEnhancer_t(int, int) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
        }

        _profileOverride = "FALSE";
        try {
            _profileOverride = device::HostPersistence::getInstance().getDefaultProperty("audio.SurroundDecoder.ms12ProfileOverride");
        }
        catch(...) {
            _profileOverride = "FALSE";
            INT_INFO(" audio.SurroundDecoder.ms12ProfileOverride settings not found in system default ..... \r\n");
        }
        INT_INFO(" audio.SurroundDecoder.ms12ProfileOverride = %s ..... \r\n",_profileOverride.c_str());

        if(_profileOverride == "TRUE") {
            typedef dsError_t (*dsEnableSurroundDecoder_t)(intptr_t handle, bool enabled);
            static dsEnableSurroundDecoder_t dsEnableSurroundDecoderFunc = 0;
            if (dsEnableSurroundDecoderFunc == 0) {
                dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    dsEnableSurroundDecoderFunc = (dsEnableSurroundDecoder_t) dlsym(dllib, "dsEnableSurroundDecoder");
                    if (dsEnableSurroundDecoderFunc) {
                        INT_DEBUG("dsEnableSurroundDecoder_t(int, bool) is defined and loaded\r\n");
                        std::string _SurroundDecoder("FALSE");
                        bool m_surroundDecoder = false;
                        try {
                            _SurroundDecoder = device::HostPersistence::getInstance().getProperty("audio.SurroundDecoderEnabled");
                            if (_SurroundDecoder == "TRUE") {
                                m_surroundDecoder = true;
                            }
                            else {
                                m_surroundDecoder = false;
                            }
            //SPEAKER init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                                if (dsEnableSurroundDecoderFunc(handle, m_surroundDecoder) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized Surroudn Decoder : %d\n","SPEAKER0", m_surroundDecoder);
                                }
                            }
            //HDMI init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                                if (dsEnableSurroundDecoderFunc(handle, m_surroundDecoder) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized Surroudn Decoder : %d\n","HDMI0", m_surroundDecoder);
                                }
                            }
                        }
                        catch(...) {
                            INT_INFO("audio.SurroundDecoderEnabled not found in persistence store. System Default configured through profiles\n");
                        }
                    }
                    else {
                        INT_INFO("dsEnableSurroundDecoder_t(int, bool) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
	    }

        _profileOverride = "FALSE";
        try {
            _profileOverride = device::HostPersistence::getInstance().getDefaultProperty("audio.DRCMode.ms12ProfileOverride");
        }
        catch(...) {
            _profileOverride = "FALSE";
            INT_INFO(" audio.DRCMode.ms12ProfileOverride settings not found in system default ..... \r\n");
        }
        INT_INFO(" audio.DRCMode.ms12ProfileOverride = %s ..... \r\n",_profileOverride.c_str());

        if(_profileOverride == "TRUE") {
            typedef dsError_t (*dsSetDRCMode_t)(intptr_t handle, int mode);
            static dsSetDRCMode_t dsSetDRCModeFunc = 0;
            if (dsSetDRCModeFunc == 0) {
                dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    dsSetDRCModeFunc = (dsSetDRCMode_t) dlsym(dllib, "dsSetDRCMode");
                    if (dsSetDRCModeFunc) {
                        INT_DEBUG("dsSetDRCMode_t(int, int) is defined and loaded\r\n");
                        std::string _DRCMode("Line");
                        int m_DRCMode = 0;
                        try {
                            _DRCMode = device::HostPersistence::getInstance().getProperty("audio.DRCMode");
                            if (_DRCMode == "RF") {
                                m_DRCMode = 1;
                            }
                            else {
                                m_DRCMode = 0;
                            }
                //SPEAKER init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                                if (dsSetDRCModeFunc(handle, m_DRCMode) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized DRCMode : %d\n","SPEAKER0", m_DRCMode);
                                }
                            }
                //HDMI init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                                if (dsSetDRCModeFunc(handle, m_DRCMode) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized DRCMode : %d\n","HDMI0", m_DRCMode);
                                }
                            }
                        }
                        catch(...) {
                            INT_INFO("audio.DRCMode not found in persistence store. System Default configured through profiles\n");
                        }
                    }
                    else {
                        INT_INFO("dsSetDRCMode_t(int, int) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
        }

        _profileOverride = "FALSE";
        try {
            _profileOverride = device::HostPersistence::getInstance().getDefaultProperty("audio.SurroundVirtualizer.ms12ProfileOverride");
        }
        catch(...) {
            _profileOverride = "FALSE";
            INT_INFO(" audio.SurroundVirtualizer.ms12ProfileOverride settings not found in system default ..... \r\n");
        }
        INT_INFO(" audio.SurroundVirtualizer.ms12ProfileOverride = %s ..... \r\n",_profileOverride.c_str());

        if(_profileOverride == "TRUE") {
            typedef dsError_t (*dsSetSurroundVirtualizer_t)(intptr_t handle, dsSurroundVirtualizer_t virtualizer);
            static dsSetSurroundVirtualizer_t dsSetSurroundVirtualizerFunc = 0;
            if (dsSetSurroundVirtualizerFunc == 0) {
                dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    dsSetSurroundVirtualizerFunc = (dsSetSurroundVirtualizer_t) dlsym(dllib, "dsSetSurroundVirtualizer");
                    if (dsSetSurroundVirtualizerFunc) {
                        INT_DEBUG("dsSetSurroundVirtualizer_t(int, dsSurroundVirtualizer_t virtualizer) is defined and loaded\r\n");
                        std::string _SVMode("0");
                        std::string _SVBoost("0");
                        dsSurroundVirtualizer_t m_virtualizer;
                        std::string _PropertyMode = _dsGetCurrentProfileProperty("SurroundVirtualizer.mode");
                        std::string _PropertyBoost = _dsGetCurrentProfileProperty("SurroundVirtualizer.boost");
                        try {
                            _SVMode = device::HostPersistence::getInstance().getProperty(_PropertyMode);
                            _SVBoost = device::HostPersistence::getInstance().getProperty(_PropertyBoost);
                            m_virtualizer.mode = atoi(_SVMode.c_str());
			    m_virtualizer.boost = atoi(_SVBoost.c_str());
                //SPEAKER init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                                if (dsSetSurroundVirtualizerFunc(handle, m_virtualizer) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized Surround Virtualizer : Mode: %d, Boost : %d\n","SPEAKER0", m_virtualizer.mode, m_virtualizer.boost);
                                }
                            }
                //HDMI init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                                if (dsSetSurroundVirtualizerFunc(handle, m_virtualizer) == dsERR_NONE){
                                    INT_INFO("Port %s: Initialized Surround Virtualizer : Mode: %d, Boost : %d\\n","HDMI0", m_virtualizer.mode, m_virtualizer.boost);
				}
                            }
                        }
                        catch(...) {
                            INT_INFO("audio.SurroundVirtualizer not found in persistence store. System Default configured through profiles \n");
                        }
                    }
                    else {
                        INT_INFO("dsSetSurroundVirtualizer_t(int, dsSurroundVirtualizer_t) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
        }

        _profileOverride = "FALSE";
        try {
            _profileOverride = device::HostPersistence::getInstance().getDefaultProperty("audio.MISteering.ms12ProfileOverride");
        }
        catch(...) {
            _profileOverride = "FALSE";
            INT_INFO(" audio.MISteering.ms12ProfileOverride settings not found in system default ..... \r\n");
        }
        INT_INFO(" audio.MISteering.ms12ProfileOverride = %s ..... \r\n",_profileOverride.c_str());

        if(_profileOverride == "TRUE") {
            typedef dsError_t (*dsSetMISteering_t)(intptr_t handle, bool enabled);
            static dsSetMISteering_t dsSetMISteeringFunc = 0;
            if (dsSetMISteeringFunc == 0) {
                dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    dsSetMISteeringFunc = (dsSetMISteering_t) dlsym(dllib, "dsSetMISteering");
                    if (dsSetMISteeringFunc) {
                        INT_DEBUG("dsSetMISteering_t(int, bool) is defined and loaded\r\n");
                        std::string _MISteering("Disabled");
                        bool m_MISteering = false;
                        try {
                            _MISteering = device::HostPersistence::getInstance().getProperty("audio.MISteering");
                            if (_MISteering == "Enabled") {
                                m_MISteering = true;
                            }
                            else {
                                m_MISteering = false;
                            }
                //SPEAKER init
                            handle = 0;
                            if (dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                                if (dsSetMISteeringFunc(handle, m_MISteering) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized MI Steering : %d\n","SPEAKER0", m_MISteering);
                                }
			    }
                //HDMI init
                            handle = 0;
                            if (dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                                if (dsSetMISteeringFunc(handle, m_MISteering) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized MI Steering : %d\n","HDMI0", m_MISteering);
                                }
                            }
                            else {
                                INT_INFO("Port %s: Initialization MI Steering : %d failed. Port not available\n","HDMI0", m_MISteering);
                            }
                        }
                        catch(...) {
                            INT_INFO("audio.MISteering not found in persistence store. System Default configured through profiles\n");
                        }
                    }
                    else {
                        INT_INFO("dsSetMISteering_t(int, bool) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
        }

        _profileOverride = "FALSE";
        try {
            _profileOverride = device::HostPersistence::getInstance().getDefaultProperty("audio.GraphicEQ.ms12ProfileOverride");
        }
        catch(...) {
            _profileOverride = "FALSE";
            INT_INFO(" audio.GraphicEQ.ms12ProfileOverride settings not found in system default ..... \r\n");
        }
        INT_INFO(" audio.GraphicEQ.ms12ProfileOverride = %s ..... \r\n",_profileOverride.c_str());

        if(_profileOverride == "TRUE") {
            typedef dsError_t (*dsSetGraphicEqualizerMode_t)(intptr_t handle, int mode);
            static dsSetGraphicEqualizerMode_t dsSetGraphicEqualizerModeFunc = 0;
            if (dsSetGraphicEqualizerModeFunc == 0) {
                dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    dsSetGraphicEqualizerModeFunc = (dsSetGraphicEqualizerMode_t) dlsym(dllib, "dsSetGraphicEqualizerMode");
                    if (dsSetGraphicEqualizerModeFunc) {
                        INT_DEBUG("dsSetGraphicEqualizerMode_t(int, int) is defined and loaded\r\n");
                        std::string _GEQMode("0");
                        int m_GEQMode = 0;
                        handle = 0;
                        if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) != dsERR_NONE) {
                            INT_ERROR("dsGetAudioPort failed for SPEAKER0\n");
                        }

		        try {
                            _GEQMode = device::HostPersistence::getInstance().getProperty("audio.GraphicEQ");
                            m_GEQMode = atoi(_GEQMode.c_str());
                //SPEAKER init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                                if (dsSetGraphicEqualizerModeFunc(handle, m_GEQMode) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized Graphic Equalizer mode : %d\n","SPEAKER0", m_GEQMode);
                                }
                            }
                //HDMI init
                            handle = 0;
                            if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                                if (dsSetGraphicEqualizerModeFunc(handle, m_GEQMode) == dsERR_NONE) {
                                    INT_INFO("Port %s: Initialized Graphic Equalizer mode : %d\n","HDMI0", m_GEQMode);
                                }
                            }
                        }
                        catch(...) {
                            INT_INFO("audio.GraphicEQ not found in persistence store. System Default configured through profiles\n");
                        }
                    }
                    else {
                        INT_INFO("dsSetGraphicEqualizerMode_t(int, int) is not defined\r\n");
                    }
                    dlclose(dllib);
                }
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }
	}
    }
//MS12 Audio Profile not there
//Initialize individual settings from persistence store / system default if persistence store empty
    else if(_AProfileSupport == "FALSE") {
           typedef dsError_t (*dsSetAudioCompression_t)(intptr_t handle, int compressionLevel);
           static dsSetAudioCompression_t dsSetAudioCompressionFunc = 0;
           if (dsSetAudioCompressionFunc == 0) {
               void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
               if (dllib) {
                   dsSetAudioCompressionFunc = (dsSetAudioCompression_t) dlsym(dllib, "dsSetAudioCompression");
                   if (dsSetAudioCompressionFunc) {
                       INT_DEBUG("dsSetAudioCompression_t(int, int ) is defined and loaded\r\n");
                       std::string _AudioCompression("0");
                       int m_audioCompression = 0;
                       try {
                           _AudioCompression = device::HostPersistence::getInstance().getProperty("audio.Compression");
                       }
                       catch(...) {
                           try {
                               INT_DEBUG("audio.Compression not found in persistence store. Try system default\n");
                               _AudioCompression = device::HostPersistence::getInstance().getDefaultProperty("audio.Compression");
                           }
                           catch(...) {
                               _AudioCompression = "0";
                           }
                       }
                       m_audioCompression = atoi(_AudioCompression.c_str());
           //SPEAKER init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                           if (dsSetAudioCompressionFunc(handle, m_audioCompression) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized audio compression : %d\n","SPEAKER0", m_audioCompression);
                           }
                       }
           //HDMI init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                           if (dsSetAudioCompressionFunc(handle, m_audioCompression) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized audio compression : %d\n","HDMI0", m_audioCompression);
                           }
                       }
                   }
                   else {
                       INT_INFO("dsSetAudioCompression_t(int, int) is not defined\r\n");
                   }
                   dlclose(dllib);
               }
               else {
                   INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
               }
           }


           typedef dsError_t (*dsSetDialogEnhancement_t)(intptr_t handle, int enhancerLevel);
           static dsSetDialogEnhancement_t dsSetDialogEnhancementFunc = 0;
           if (dsSetDialogEnhancementFunc == 0) {
               void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
               if (dllib) {
                   dsSetDialogEnhancementFunc = (dsSetDialogEnhancement_t) dlsym(dllib, "dsSetDialogEnhancement");
                   if (dsSetDialogEnhancementFunc) {
                       INT_DEBUG("dsSetDialogEnhancement_t(int, int) is defined and loaded\r\n");
                       std::string _EnhancerLevel("0");
                       int m_enhancerLevel = 0;
                       try {
                           _EnhancerLevel = device::HostPersistence::getInstance().getProperty("audio.EnhancerLevel");
                       }
                       catch(...) {
                           try {
                               INT_DEBUG("audio.EnhancerLevel not found in persistence store. Try system default\n");
                               _EnhancerLevel = device::HostPersistence::getInstance().getDefaultProperty("audio.EnhancerLevel");
                           }
                           catch(...) {
                               _EnhancerLevel = "0";
                           }
                       }
                       m_enhancerLevel = atoi(_EnhancerLevel.c_str());
           //SPEAKER init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                           if (dsSetDialogEnhancementFunc(handle, m_enhancerLevel) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized dialog enhancement level : %d\n","SPEAKER0", m_enhancerLevel);
                           }
                       }
           //HDMI init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                           if (dsSetDialogEnhancementFunc(handle, m_enhancerLevel) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized dialog enhancement level : %d\n","HDMI0", m_enhancerLevel);
                           }
                       }
                   }
                   else {
                       INT_INFO("dsSetDialogEnhancement_t(int, int ) is not defined\r\n");
                   }
                   dlclose(dllib);
               }
               else {
                   INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
               }
           }


           typedef dsError_t (*dsSetDolbyVolumeMode_t)(intptr_t handle, bool enable);
           static dsSetDolbyVolumeMode_t dsSetDolbyVolumeModeFunc = 0;
           bool bDolbyVolumeOverrideCheck = true; //DolbyVolume setting only overrides when DolbyVolume is present and VolumeLeveller is not present in persistence.
           if (dsSetDolbyVolumeModeFunc == 0) {
               dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
               if (dllib) {
                   dsSetDolbyVolumeModeFunc = (dsSetDolbyVolumeMode_t) dlsym(dllib, "dsSetDolbyVolumeMode");
                   if (dsSetDolbyVolumeModeFunc) {
                       INT_DEBUG("dsSetDolbyVolumeMode_t(int, bool) is defined and loaded\r\n");
                       std::string _DolbyVolumeMode("FALSE");
                       bool m_dolbyVolumeMode = false;
                       try {
                           _DolbyVolumeMode = device::HostPersistence::getInstance().getProperty("audio.DolbyVolumeMode");
                           bDolbyVolumeOverrideCheck = false;
                       }
                       catch(...) {
                           try {
                               INT_DEBUG("audio.DolbyVolumeMode not found in persistence store. Try system default\n");
                               _DolbyVolumeMode = device::HostPersistence::getInstance().getDefaultProperty("audio.DolbyVolumeMode");
                           }
                           catch(...) {
                               _DolbyVolumeMode = "FALSE";
                           }
                       }
                       if (_DolbyVolumeMode == "TRUE") {
                           m_dolbyVolumeMode = true;
                       }
                       else {
                           m_dolbyVolumeMode = false;
                       }
           //SPEAKER init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                           if (dsSetDolbyVolumeModeFunc(handle, m_dolbyVolumeMode) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized Dolby Volume Mode : %d\n","SPEAKER0", m_dolbyVolumeMode);
                           }
                       }
           //HDMI init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                           if (dsSetDolbyVolumeModeFunc(handle, m_dolbyVolumeMode) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized Dolby Volume Mode : %d\n","HDMI0", m_dolbyVolumeMode);
                           }
                       }
                   }
                   else {
                       INT_INFO("dsSetDolbyVolumeMode_t(int, bool) is not defined\r\n");
                   }
                   dlclose(dllib);
               }
               else {
                   INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
               }
           }

           typedef dsError_t (*dsSetIntelligentEqualizerMode_t)(intptr_t handle, int mode);
           static dsSetIntelligentEqualizerMode_t dsSetIntelligentEqualizerModeFunc = 0;
           if (dsSetIntelligentEqualizerModeFunc == 0) {
               dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
               if (dllib) {
                   dsSetIntelligentEqualizerModeFunc = (dsSetIntelligentEqualizerMode_t) dlsym(dllib, "dsSetIntelligentEqualizerMode");
                   if (dsSetIntelligentEqualizerModeFunc) {
                       INT_DEBUG("dsSetIntelligentEqualizerMode_t(int, int) is defined and loaded\r\n");
                       std::string _IEQMode("0");
                       int m_IEQMode = 0;
                       handle = 0;
                       if (dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) != dsERR_NONE) {
		           INT_ERROR("dsGetAudioPort failed for SPEAKER0\n");
		       }
                       try {
                           _IEQMode = device::HostPersistence::getInstance().getProperty("audio.IntelligentEQ");
                       }
                       catch(...) {
                           try {
                               INT_DEBUG("audio.IntelligentEQ not found in persistence store. Try system default\n");
                               _IEQMode = device::HostPersistence::getInstance().getDefaultProperty("audio.IntelligentEQ");
                           }
                           catch(...) {
                               _IEQMode = "0";
                           }
                       }
                       m_IEQMode = atoi(_IEQMode.c_str());

           //SPEAKER init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                           if (dsSetIntelligentEqualizerModeFunc(handle, m_IEQMode) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized Intelligent Equalizer mode : %d\n","SPEAKER0", m_IEQMode);
                           }
                       }
           //HDMI init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                           if (dsSetIntelligentEqualizerModeFunc(handle, m_IEQMode) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized Intelligent Equalizer mode : %d\n","HDMI0", m_IEQMode);
                           }
                       }
                   }
                   else {
                       INT_INFO("dsSetIntelligentEqualizerMode_t(int, int) is not defined\r\n");
                   }
                   dlclose(dllib);
               }
               else {
                   INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
               }
           }

           typedef dsError_t (*dsSetVolumeLeveller_t)(intptr_t handle, dsVolumeLeveller_t volLeveller);
           static dsSetVolumeLeveller_t dsSetVolumeLevellerFunc = 0;
           if (dsSetVolumeLevellerFunc == 0) {
               dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
               if (dllib) {
                   dsSetVolumeLevellerFunc = (dsSetVolumeLeveller_t) dlsym(dllib, "dsSetVolumeLeveller");
                   if (dsSetVolumeLevellerFunc) {
                       INT_DEBUG("dsSetVolumeLeveller_t(int, dsVolumeLeveller_t) is defined and loaded\r\n");
		       std::string _volLevellerMode("0");
		       std::string _volLevellerLevel("0");
                       dsVolumeLeveller_t m_volumeLeveller;
                       try {
                           _volLevellerMode = device::HostPersistence::getInstance().getProperty("audio.VolumeLeveller.mode");
			   _volLevellerLevel = device::HostPersistence::getInstance().getProperty("audio.VolumeLeveller.level");
                           bDolbyVolumeOverrideCheck = true;
                       }
                       catch(...) {
                           try {
                               INT_DEBUG("audio.VolumeLeveller not found in persistence store. Try system default\n");
                               _volLevellerMode = device::HostPersistence::getInstance().getDefaultProperty("audio.VolumeLeveller.mode");
                               _volLevellerLevel = device::HostPersistence::getInstance().getDefaultProperty("audio.VolumeLeveller.level");
                           }
                           catch(...) {
                               _volLevellerMode = "0";
			       _volLevellerLevel = "0";
                           }
                       }
                       m_volumeLeveller.mode = atoi(_volLevellerMode.c_str());
		       m_volumeLeveller.level = atoi(_volLevellerLevel.c_str());
           //SPEAKER init
                       handle = 0;
                       INT_DEBUG("bDolbyVolumeOverrideCheck value:  %d\n", bDolbyVolumeOverrideCheck);
                       if(bDolbyVolumeOverrideCheck && dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                           if (dsSetVolumeLevellerFunc(handle, m_volumeLeveller) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized Volume Leveller : Mode: %d, Level: %d\n","SPEAKER0", m_volumeLeveller.mode, m_volumeLeveller.level);
                           }
                       }
           //HDMI init
                       handle = 0;
                       if(bDolbyVolumeOverrideCheck && dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                           if (dsSetVolumeLevellerFunc(handle, m_volumeLeveller) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized Volume Leveller : Mode: %d, Level: %d\n","HDMI0", m_volumeLeveller.mode, m_volumeLeveller.level);
                           }
                       }
                   }
                   else {
                       INT_INFO("dsSetVolumeLeveller_t(int, dsVolumeLeveller_t) is not defined\r\n");
                   }
                   dlclose(dllib);
               }
               else {
                   INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
               }
           }


           typedef dsError_t (*dsSetBassEnhancer_t)(intptr_t handle, int boost);
           static dsSetBassEnhancer_t dsSetBassEnhancerFunc = 0;
           if (dsSetBassEnhancerFunc == 0) {
               dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
               if (dllib) {
                   dsSetBassEnhancerFunc = (dsSetBassEnhancer_t) dlsym(dllib, "dsSetBassEnhancer");
                   if (dsSetBassEnhancerFunc) {
                       INT_DEBUG("dsSetBassEnhancer_t(int, int) is defined and loaded\r\n");
                       std::string _BassBoost("0");
                       int m_bassBoost = 0;
                       try {
                           _BassBoost = device::HostPersistence::getInstance().getProperty("audio.BassBoost");
                       }
                       catch(...) {
                           try {
                               INT_DEBUG("audio.BassBoost not found in persistence store. Try system default\n");
                               std::string _Property = _dsGetCurrentProfileProperty("BassBoost");
                               _BassBoost = device::HostPersistence::getInstance().getDefaultProperty(_Property);
                           }
                           catch(...) {
                               _BassBoost = "0";
                           }
                       }
                       m_bassBoost = atoi(_BassBoost.c_str());
           //SPEAKER init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                           if (dsSetBassEnhancerFunc(handle, m_bassBoost) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized Bass Boost : %d\n","SPEAKER0", m_bassBoost);
                           }
                       }
           //HDMI init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                           if (dsSetBassEnhancerFunc(handle, m_bassBoost) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized Bass Boost : %d\n","HDMI0", m_bassBoost);
                           }
                       }
                   }
                   else {
                       INT_INFO("dsSetBassEnhancer_t(int, int) is not defined\r\n");
                   }
                   dlclose(dllib);
               }
               else {
                   INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
               }
           }


           typedef dsError_t (*dsEnableSurroundDecoder_t)(intptr_t handle, bool enabled);
           static dsEnableSurroundDecoder_t dsEnableSurroundDecoderFunc = 0;
           if (dsEnableSurroundDecoderFunc == 0) {
               dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
               if (dllib) {
                   dsEnableSurroundDecoderFunc = (dsEnableSurroundDecoder_t) dlsym(dllib, "dsEnableSurroundDecoder");
                   if (dsEnableSurroundDecoderFunc) {
                       INT_DEBUG("dsEnableSurroundDecoder_t(int, bool) is defined and loaded\r\n");
                       std::string _SurroundDecoder("FALSE");
                       bool m_surroundDecoder = false;
                       try {
                           _SurroundDecoder = device::HostPersistence::getInstance().getProperty("audio.SurroundDecoderEnabled");
                       }
                       catch(...) {
                           try {
                               INT_DEBUG("audio.SurroundDecoderEnabled not found in persistence store. Try system default\n");
                               _SurroundDecoder = device::HostPersistence::getInstance().getDefaultProperty("audio.SurroundDecoderEnabled");
                           }
                           catch(...) {
                               _SurroundDecoder = "FALSE";
                           }
                       }
                       if (_SurroundDecoder == "TRUE") {
                           m_surroundDecoder = true;
                       }
                       else {
                           m_surroundDecoder = false;
                       }
           //SPEAKER init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                           if (dsEnableSurroundDecoderFunc(handle, m_surroundDecoder) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized Surroudn Decoder : %d\n","SPEAKER0", m_surroundDecoder);
                           }
                       }
           //HDMI init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                           if (dsEnableSurroundDecoderFunc(handle, m_surroundDecoder) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized Surroudn Decoder : %d\n","HDMI0", m_surroundDecoder);
                           }
                       }
                   }
                   else {
                       INT_INFO("dsEnableSurroundDecoder_t(int, bool) is not defined\r\n");
                   }
                   dlclose(dllib);
               }
               else {
                   INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
               }
           }


           typedef dsError_t (*dsSetDRCMode_t)(intptr_t handle, int mode);
           static dsSetDRCMode_t dsSetDRCModeFunc = 0;
           if (dsSetDRCModeFunc == 0) {
               dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
               if (dllib) {
                   dsSetDRCModeFunc = (dsSetDRCMode_t) dlsym(dllib, "dsSetDRCMode");
                   if (dsSetDRCModeFunc) {
                       INT_DEBUG("dsSetDRCMode_t(int, int) is defined and loaded\r\n");
                       std::string _DRCMode("Line");
                       int m_DRCMode = 0;
                       try {
                           _DRCMode = device::HostPersistence::getInstance().getProperty("audio.DRCMode");
                       }
                       catch(...) {
                           try {
                               INT_DEBUG("audio.DRCMode not found in persistence store. Try system default\n");
                               _DRCMode = device::HostPersistence::getInstance().getDefaultProperty("audio.DRCMode");
                           }
                           catch(...) {
                               _DRCMode = "Line";
                           }
                       }
                       if (_DRCMode == "RF") {
                           m_DRCMode = 1;
                       }
                       else {
                           m_DRCMode = 0;
                       }
           //SPEAKER init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                           if (dsSetDRCModeFunc(handle, m_DRCMode) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized DRCMode : %d\n","SPEAKER0", m_DRCMode);
                           }
                       }
           //HDMI init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                           if (dsSetDRCModeFunc(handle, m_DRCMode) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized DRCMode : %d\n","HDMI0", m_DRCMode);
                           }
                       }
                   }
                   else {
                       INT_INFO("dsSetDRCMode_t(int, int) is not defined\r\n");
                   }
                   dlclose(dllib);
               }
               else {
                   INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
               }
           }


           typedef dsError_t (*dsSetSurroundVirtualizer_t)(intptr_t handle, dsSurroundVirtualizer_t virtualizer);
           static dsSetSurroundVirtualizer_t dsSetSurroundVirtualizerFunc = 0;
           if (dsSetSurroundVirtualizerFunc == 0) {
               dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
               if (dllib) {
                   dsSetSurroundVirtualizerFunc = (dsSetSurroundVirtualizer_t) dlsym(dllib, "dsSetSurroundVirtualizer");
                   if (dsSetSurroundVirtualizerFunc) {
                       INT_DEBUG("dsSetSurroundVirtualizer_t(int, dsSurroundVirtualizer_t) is defined and loaded\r\n");
                        std::string _SVMode("0");
                        std::string _SVBoost("0");
                        dsSurroundVirtualizer_t m_virtualizer;		 

                       try {
                            _SVMode = device::HostPersistence::getInstance().getProperty("audio.SurroundVirtualizer.mode");
                            _SVBoost = device::HostPersistence::getInstance().getProperty("audio.SurroundVirtualizer.boost");
                            m_virtualizer.mode = atoi(_SVMode.c_str());
                            m_virtualizer.boost = atoi(_SVBoost.c_str());			       
                       }
                       catch(...) {
                           try {
                               INT_DEBUG("audio.SurroundVirtualizer.mode/audio.SurroundVirtualizer.boost not found in persistence store. Try system default\n");
                               _SVMode = device::HostPersistence::getInstance().getProperty("audio.SurroundVirtualizer.mode");
                               _SVBoost = device::HostPersistence::getInstance().getProperty("audio.SurroundVirtualizer.boost"); 
                           }
                           catch(...) {
                               _SVMode = "0";
                               _SVBoost = "0";
                           }
                       }
                       m_virtualizer.mode = atoi(_SVMode.c_str());
		       m_virtualizer.boost = atoi(_SVBoost.c_str());
           //SPEAKER init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                           if (dsSetSurroundVirtualizerFunc(handle, m_virtualizer) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized Surround Virtualizer : Mode: %d, Boost : %d\n","SPEAKER0", m_virtualizer.mode, m_virtualizer.boost);
                           }
                       }
           //HDMI init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                           if (dsSetSurroundVirtualizerFunc(handle, m_virtualizer) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized Surround Virtualizer : Mode: %d, Boost : %d\\n","HDMI0", m_virtualizer.mode, m_virtualizer.boost);
                           }
                       }
                   }
                   else {
                       INT_INFO("dsSetSurroundVirtualizer_t(int, dsSurroundVirtualizer_t) is not defined\r\n");
                   }
                   dlclose(dllib);
               }
               else {
                   INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
               }
           }


           typedef dsError_t (*dsSetMISteering_t)(intptr_t handle, bool enabled);
           static dsSetMISteering_t dsSetMISteeringFunc = 0;
           if (dsSetMISteeringFunc == 0) {
               dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
               if (dllib) {
                   dsSetMISteeringFunc = (dsSetMISteering_t) dlsym(dllib, "dsSetMISteering");
                   if (dsSetMISteeringFunc) {
                       INT_DEBUG("dsSetMISteering_t(int, bool) is defined and loaded\r\n");
                       std::string _MISteering("Disabled");
                       bool m_MISteering = false;
                       try {
                           _MISteering = device::HostPersistence::getInstance().getProperty("audio.MISteering");
                       }
                       catch(...) {
                           try {
                               INT_DEBUG("audio.MISteering not found in persistence store. Try system default\n");
                               _MISteering = device::HostPersistence::getInstance().getDefaultProperty("audio.MISteering");
                           }
                           catch(...) {
                               _MISteering = "Disabled";
                           }
                       }
                       if (_MISteering == "Enabled") {
                           m_MISteering = true;
                       }
                       else {
                           m_MISteering = false;
                       }

           //SPEAKER init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                            if (dsSetMISteeringFunc(handle, m_MISteering) == dsERR_NONE) {
                                INT_INFO("Port %s: Initialized MI Steering : %d\n","SPEAKER0", m_MISteering);
                            }
                        }
           //HDMI init
                       handle = 0;
                       if (dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                           if (dsSetMISteeringFunc(handle, m_MISteering) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized MI Steering : %d\n","HDMI0", m_MISteering);
                           }
                       }
                       else {
                           INT_INFO("Port %s: Initialization MI Steering : %d failed. Port not available\n","HDMI0", m_MISteering);
                       }
                   }
                   else {
                       INT_INFO("dsSetMISteering_t(int, bool) is not defined\r\n");
                   }
                   dlclose(dllib);
               }
               else {
                   INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
               }
           }


           typedef dsError_t (*dsSetGraphicEqualizerMode_t)(intptr_t handle, int mode);
           static dsSetGraphicEqualizerMode_t dsSetGraphicEqualizerModeFunc = 0;
           if (dsSetGraphicEqualizerModeFunc == 0) {
               dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
               if (dllib) {
                   dsSetGraphicEqualizerModeFunc = (dsSetGraphicEqualizerMode_t) dlsym(dllib, "dsSetGraphicEqualizerMode");
                   if (dsSetGraphicEqualizerModeFunc) {
                       INT_DEBUG("dsSetGraphicEqualizerMode_t(int, int) is defined and loaded\r\n");
                       std::string _GEQMode("0");
                       int m_GEQMode = 0;
                       handle = 0;
                       if (dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) != dsERR_NONE) {
			       INT_ERROR("dsGetAudioPort failed for SPEAKER0\n");
		       }
                       try {
                           _GEQMode = device::HostPersistence::getInstance().getProperty("audio.GraphicEQ");
                       }
                       catch(...) {
                           try {
                               INT_DEBUG("audio.GraphicEQ not found in persistence store. Try system default\n");
                               _GEQMode = device::HostPersistence::getInstance().getDefaultProperty("audio.GraphicEQ");
                           }
                           catch(...) {
                               _GEQMode = "0";
                           }
                       }
                       m_GEQMode = atoi(_GEQMode.c_str());

           //SPEAKER init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_SPEAKER,0,&handle) == dsERR_NONE) {
                           if (dsSetGraphicEqualizerModeFunc(handle, m_GEQMode) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized Graphic Equalizer mode : %d\n","SPEAKER0", m_GEQMode);
                           }
                       }
           //HDMI init
                       handle = 0;
                       if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
                           if (dsSetGraphicEqualizerModeFunc(handle, m_GEQMode) == dsERR_NONE) {
                               INT_INFO("Port %s: Initialized Graphic Equalizer mode : %d\n","HDMI0", m_GEQMode);
                           }
                       }
                   }
                   else {
                       INT_INFO("dsSetGraphicEqualizerMode_t(int, int) is not defined\r\n");
                   }
                   dlclose(dllib);
               }
               else {
                   INT_INFO("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
               }
           }
    }

    /* HDMI digital audio mode settings */
    handle = 0;
    if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI,0,&handle) == dsERR_NONE) {
       std::string _AudioModeAuto("FALSE");
       try {
          _AudioModeAuto = device::HostPersistence::getInstance().getProperty("HDMI0.AudioMode.AUTO");
       }
       catch(...) {
          try {
                INT_DEBUG("HDMI0.AudioMode.AUTO not found in persistence store. Try system default\n");
                _AudioModeAuto = device::HostPersistence::getInstance().getDefaultProperty("HDMI0.AudioMode.AUTO");
          }
          catch(...) {
                   _AudioModeAuto = "FALSE";
         }
      }
      if (_AudioModeAuto.compare("TRUE") == 0)
      {
         typedef dsError_t (*dsSetStereoAuto_t)(intptr_t handle, int autoMode);
         static dsSetStereoAuto_t func = 0;
         if (func == 0) {
            void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
            if (dllib) {
                func = (dsSetStereoAuto_t) dlsym(dllib, "dsSetStereoAuto");
                if (func) {
                    INT_DEBUG("dsSetStereoAuto_t(int, int *) is defined and loaded\r\n");
                }
                else {
                    INT_INFO("dsSetStereoAuto_t(int, int *) is not defined\r\n");
                }
                dlclose(dllib);
            }
            else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
            }
        }

        if (func != 0 && handle != NULL)
        {
            if (func(handle, 1) == dsERR_NONE)
            {
                 INT_DEBUG("dsSetStereoAuto Port HDMI_0 Audio Mode is set to Auto \n");
            }
        }
    }
    if (NULL != handle) {
       if (dsSetStereoMode(handle, _srv_HDMI_Audiomode) == dsERR_NONE)
       {
          INT_INFO("dsSetStereoMode The HDMI Port Audio Settings Mode is %d \r\n",_srv_HDMI_Audiomode);
       }
    }
  }

    /* HDMI ARC digital audio mode settings */
    handle = 0;
    if(dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI_ARC,0,&handle) == dsERR_NONE) {
       std::string _ARCAudioModeAuto("FALSE");
       try {
          _ARCAudioModeAuto = device::HostPersistence::getInstance().getProperty("HDMI_ARC0.AudioMode.AUTO");
       }
       catch(...) {
          try {
                INT_DEBUG("HDMI_ARC0.AudioMode.AUTO not found in persistence store. Try system default\n");
                _ARCAudioModeAuto = device::HostPersistence::getInstance().getDefaultProperty("HDMI_ARC0.AudioMode.AUTO");
          }
          catch(...) {
                   _ARCAudioModeAuto = "FALSE";
         }
      }
      if (_ARCAudioModeAuto.compare("TRUE") == 0)
      {
         typedef dsError_t (*dsSetStereoAuto_t)(intptr_t handle, int autoMode);
         static dsSetStereoAuto_t func = 0;
         if (func == 0) {
            void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
            if (dllib) {
                func = (dsSetStereoAuto_t) dlsym(dllib, "dsSetStereoAuto");
                if (func) {
                    INT_DEBUG("dsSetStereoAuto_t(int, int *) is defined and loaded\r\n");
                }
                else {
                    INT_INFO("dsSetStereoAuto_t(int, int *) is not defined\r\n");
                }
                dlclose(dllib);
            }
            else {
                    INT_INFO("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
            }
        }

        if (func != 0 && handle != NULL)
        {
            if (func(handle, 1) == dsERR_NONE)
            {
                 INT_DEBUG("dsSetStereoAuto Port HDMI_0ARC Audio Mode is set to Auto \n");
            }
        }

    }
    else if (_ARCAudioModeAuto.compare("FALSE") == 0)
    {
       if (NULL != handle) {
           if (dsSetStereoMode(handle, _srv_HDMI_ARC_Audiomode) == dsERR_NONE)
           {
              INT_INFO("dsSetStereoMode The HDMI ARC Port Audio Settings Mode is %d \r\n",_srv_HDMI_ARC_Audiomode);
           }
      }
   }  
 }

#endif //DS_AUDIO_SETTINGS_PERSISTENCE
}

bool searchConfigs()
{
	//int kConfig_size_local = -1, kPort_size_local = -1;

	//IARM_BUS_Lock(lock);

	//if ( kConfigs1 == NULL) 
	{
        void *dllib = dlopen("libds.so", RTLD_LAZY);
        if (dllib) {
            kConfigs1 = (dsAudioTypeConfig_t *) dlsym(dllib, "kConfig_audio");
            if (kConfigs1) {
                INT_DEBUG("kConfig_audio is defined and loaded kConfigs1 = %p\r\n", kConfigs1);
				printf("%d:%s: kConfig_audio is defined and loaded kConfigs1 = %p\n", __LINE__, __func__, kConfigs1);
            }
            else {
                INT_INFO("kConfig_audio is not defined\r\n");
				printf("%d:%s: kConfig_audio is not defined\n", __LINE__, __func__);
                //IARM_BUS_Unlock(lock);
                //dlclose(dllib);
                //return IARM_RESULT_INVALID_STATE;
            }
		
			pKConSize = (int *) dlsym(dllib, "kConfig_size");
			pKPortSize = (int *) dlsym(dllib, "kPort_size");
			if(pKConSize)
			{
				kConfig_size_local = *pKConSize;
				printf("%d:%s: pKConSize is defined and loaded kConfig_size_local = %d\n", __LINE__, __func__, kConfig_size_local);
			}
			else
			{
				printf("%d:%s: pKConSize is not defined\n", __LINE__, __func__);
				kConfig_size_local = -1;
			}
			if(pKPortSize)
			{
				kPort_size_local = *pKPortSize;
				printf("%d:%s: pKPortSize is defined and loaded kPort_size_local = %d\n", __LINE__, __func__, kPort_size_local);
			}
			else
			{
				printf("%d:%s: pKPortSize is not defined\n", __LINE__, __func__);
				kPort_size_local = -1;
			}
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening libdshalsrv.so failed\r\n");
			printf("%d:%s: Opening libdshalsrv.so failed\n", __LINE__, __func__);
        }
    }

	//if ( kPorts1 == NULL) 
	{
        void *dllib = dlopen("libds.so", RTLD_LAZY);
        if (dllib) {
            kPorts1 = (dsAudioPortConfig_t *) dlsym(dllib, "kPort_audio");
            if (kPorts1) {
                INT_DEBUG("kPort_audio is defined and loaded kPorts1 = %p\r\n", kPorts1);
				printf("%d:%s: kPort_audio is defined and loaded kPorts1 = %p\n", __LINE__, __func__, kPorts1);
            }
            else {
                INT_INFO("kPort_audio is not defined\r\n");
				printf("%d:%s: kPort_audio is not defined\n", __LINE__, __func__);
                //IARM_BUS_Unlock(lock);
                //dlclose(dllib);
                //return IARM_RESULT_INVALID_STATE;
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening libdshalsrv.so failed\r\n");
			printf("%d:%s: Opening libdshalsrv.so failed\n", __LINE__, __func__);
        }
    }
	printf("\n\n=========================================================================================================================\n\n");
	printf("\n%d:%s print configs using extern\n", __LINE__, __func__);
	//printf("%d:%s: size of(kConfig_audio) = %d size of(kPort_audio) = %d\n", __LINE__, __func__, kConfig_size, kPort_size);
	//kConfig_size_local = kConfig_size;
	//kPort_size_local = kPort_size;

	if(kConfigs1 != NULL)
	{
	for (size_t i = 0; i < kConfig_size_local; i++) {
			//const dsAudioTypeConfig_t *typeCfg = &kConfig_audio[i];
			const dsAudioTypeConfig_t *typeCfg = &kConfigs1[i];
			//AudioOutputPortType &aPortType = AudioOutputPortType::getInstance(typeCfg->typeId);
			//aPortType.enable();
			printf("%d:%s: typeCfg->typeId = %d\n", __LINE__, __func__, typeCfg->typeId);
			printf("%d:%s: typeCfg->name = %s\n", __LINE__, __func__, typeCfg->name);
			printf("%d:%s: typeCfg->numSupportedEncodings = %zu\n", __LINE__, __func__, typeCfg->numSupportedEncodings);
			printf("%d:%s: typeCfg->numSupportedCompressions = %zu\n", __LINE__, __func__, typeCfg->numSupportedCompressions);
			printf("%d:%s: typeCfg->numSupportedStereoModes = %zu\n", __LINE__, __func__, typeCfg->numSupportedStereoModes);
		}
	}
	else
	{
		printf("%d:%s: kConfig_audio is NULL\n", __LINE__, __func__);
	}
	if(kPorts1 != NULL)
	{
		/*
		 * set up ports based on kPorts[]
		 */
		for (size_t i = 0; i < kPort_size_local; i++) {
			//const dsAudioPortConfig_t *port = &kPort_audio[i];
			const dsAudioPortConfig_t *port = &kPorts1[i];
			printf("%d:%s: port->id.type = %d\n", __LINE__, __func__, port->id.type);
			printf("%d:%s: port->id.index = %d\n", __LINE__, __func__, port->id.index);
		}
	}
	else
	{
		printf("%d:%s: kPort_audio is NULL\n", __LINE__, __func__);
	}
	printf("\n\n=========================================================================================================================\n\n");

	return (kConfigs1 || kPorts1);
}

IARM_Result_t dsAudioMgr_init()
{
   IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsAudioPortInit, _dsAudioPortInit);

    INT_INFO("[%s]: Calling searchConfigs()\r\n", __FUNCTION__);
    searchConfigs();
    INT_INFO("[%s]: end searchConfigs()\r\n", __FUNCTION__);

   IARM_BUS_Lock(lock);
   try
	{
		/* Get the AudioModesettings FOR HDMI from Persistence */
#ifdef IGNORE_EDID_LOGIC
		std::string _AudioModeSettings("SURROUND");
#else
		std::string _AudioModeSettings("STEREO");
#endif     
		INT_INFO("[%s] Checking Host persistance", __FUNCTION__);
        try{
		_AudioModeSettings = device::HostPersistence::getInstance().getProperty("HDMI0.AudioMode");
        }
        catch(...)
        {
              INT_INFO("[%s] HDMI0.AudioMode not in host persistance. Checking default.", __FUNCTION__);
               try{
                  _AudioModeSettings = device::HostPersistence::getInstance().getDefaultProperty("HDMI0.AudioMode");
               }
               catch(...){
                  INT_INFO("[%s] HDMI0.AudioMode not in default host persistance.", __FUNCTION__);
               }
        }
		INT_DEBUG("The HDMI Audio Mode Setting on startup  is %s \r\n",_AudioModeSettings.c_str());
		if (_AudioModeSettings.compare("SURROUND") == 0)
		{
			_srv_HDMI_Audiomode = dsAUDIO_STEREO_SURROUND;
		}
		else if (_AudioModeSettings.compare("PASSTHRU") == 0)
		{
			_srv_HDMI_Audiomode = dsAUDIO_STEREO_PASSTHRU;
		}
                else if (_AudioModeSettings.compare("DOLBYDIGITAL") == 0)
                {
                        _srv_HDMI_Audiomode = dsAUDIO_STEREO_DD;
                }
                else if (_AudioModeSettings.compare("DOLBYDIGITALPLUS") == 0)
                {
                        _srv_HDMI_Audiomode = dsAUDIO_STEREO_DDPLUS;
                }
                else if (_AudioModeSettings.compare("STEREO") == 0)
                {
                        _srv_HDMI_Audiomode = dsAUDIO_STEREO_STEREO;
                }
                else
                {
#ifdef IGNORE_EDID_LOGIC
                  _srv_HDMI_Audiomode = dsAUDIO_STEREO_SURROUND;
#else
                  _srv_HDMI_Audiomode = dsAUDIO_STEREO_STEREO;
#endif
                }
		/* Get the AutoModesettings FOR HDMI from Persistence */
                /* If HDMI persistence is surround, Auto defaults to true */
            std::string _AudioModeAuto("FALSE");

            #if 0
               /* 
                Commenting this to fix the persistent settings
                Audio mode should not be forced to Auto 
                To enabale this we need to change the DS Mgr implementation 
                which reads the _dsGetStereoMode to  know the persistent value...*/
                if (_srv_HDMI_Audiomode == dsAUDIO_STEREO_SURROUND) 
                {
                    _AudioModeAuto = "TRUE";
                }
            #endif

            try {
                _AudioModeAuto = device::HostPersistence::getInstance().getProperty("HDMI0.AudioMode.AUTO",_AudioModeAuto);
            }
	        catch(...) {
                INT_INFO("HDMI0.AudioMode.AUTO not found in persistence store. Try system default\n");
                try {
                    _AudioModeAuto = device::HostPersistence::getInstance().getDefaultProperty("HDMI0.AudioMode.AUTO");
                }
                catch(...) {
#ifdef IGNORE_EDID_LOGIC
	                _AudioModeAuto = true;
#else
	                _AudioModeAuto = false;
#endif
                }
	    }
           std::string _ARCAudioModeAuto("FALSE");
	   std::string _SPDIFAudioModeAuto("FALSE");

	   try {
		    _ARCAudioModeAuto = device::HostPersistence::getInstance().getProperty("HDMI_ARC0.AudioMode.AUTO");
	   }
	   catch(...) {
               try {
                   INT_DEBUG("HDMI_ARC0.AudioMode.AUTO not found in persistence store. Try system default\n");
                   _ARCAudioModeAuto = device::HostPersistence::getInstance().getDefaultProperty("HDMI_ARC0.AudioMode.AUTO");
               }
               catch(...) {
                   _ARCAudioModeAuto = "FALSE";
               }

	   }

           try {
                _SPDIFAudioModeAuto = device::HostPersistence::getInstance().getProperty("SPDIF0.AudioMode.AUTO");
           }
           catch(...) {
               try {
                   INT_DEBUG("SPDIF0.AudioMode.AUTO not found in persistence store. Try system default\n");
                   _SPDIFAudioModeAuto = device::HostPersistence::getInstance().getDefaultProperty("SPDIF0.AudioMode.AUTO");
               }
               catch(...) {
                   _SPDIFAudioModeAuto = "FALSE";
               }
           }

           if(_AudioModeAuto.compare("TRUE") == 0)
           {
                _srv_AudioHDMIAuto = 1;
           }
           else if(_AudioModeAuto.compare("FALSE") == 0)
           {
                _srv_AudioHDMIAuto = 0;
           }
           if(_ARCAudioModeAuto.compare("TRUE") == 0)
           {
                _srv_AudioHDMIARCAuto = 1;
           }
           else if(_ARCAudioModeAuto.compare("FALSE") == 0)
           {
              _srv_AudioHDMIARCAuto = 0;
           }
           if(_SPDIFAudioModeAuto.compare("TRUE") == 0)
           {
                _srv_AudioSPDIFAuto = 1;
           }
           else if(_SPDIFAudioModeAuto.compare("FALSE") == 0)
           {
                _srv_AudioSPDIFAuto = 0;
           }

        INT_DEBUG("The HDMI Audio Auto Setting on startup  is %s \r\n",_AudioModeAuto.c_str());
        INT_DEBUG("The HDMI ARC Audio Auto Setting on startup  is %s \r\n",_ARCAudioModeAuto.c_str());
        INT_DEBUG("The SPDIF Audio Auto Setting on startup  is %s \r\n",_SPDIFAudioModeAuto.c_str());
		/* Get the AudioModesettings for SPDIF from Persistence */
		std::string _SPDIFModeSettings("STEREO");
		_SPDIFModeSettings = device::HostPersistence::getInstance().getProperty("SPDIF0.AudioMode",_SPDIFModeSettings);
		INT_DEBUG("The SPDIF Audio Mode Setting on startup  is %s \r\n",_SPDIFModeSettings.c_str());
		if (_SPDIFModeSettings.compare("SURROUND") == 0)
		{
			_srv_SPDIF_Audiomode = dsAUDIO_STEREO_SURROUND;
		}
		else if (_SPDIFModeSettings.compare("PASSTHRU") == 0)
		{
			_srv_SPDIF_Audiomode = dsAUDIO_STEREO_PASSTHRU;
		}
        else 
        {
			_srv_SPDIF_Audiomode = dsAUDIO_STEREO_STEREO;
        }
                /* Get the AudioModesettings for HDMI_ARC from Persistence */
                std::string _ARCModeSettings("STEREO");
                _ARCModeSettings = device::HostPersistence::getInstance().getProperty("HDMI_ARC0.AudioMode",_ARCModeSettings);
                INT_DEBUG("The HDMI ARC Audio Mode Setting on startup  is %s \r\n",_ARCModeSettings.c_str());
                if (_ARCModeSettings.compare("SURROUND") == 0)
                {
                        _srv_HDMI_ARC_Audiomode = dsAUDIO_STEREO_SURROUND;
                }
                else if (_ARCModeSettings.compare("PASSTHRU") == 0)
                {
                        _srv_HDMI_ARC_Audiomode = dsAUDIO_STEREO_PASSTHRU;
                }
                else
                {
                        _srv_HDMI_ARC_Audiomode = dsAUDIO_STEREO_STEREO;
                }
	}
	catch(...) 
	{
		INT_INFO("Exception in Getting the Audio  settings on Startup..... \r\n");
	}
    	if (!m_isPlatInitialized) {
    		dsAudioPortInit();
                AudioConfigInit();
	   	}
        /*coverity[missing_lock]  CID-19380 using Coverity Annotation to ignore error*/
        m_isPlatInitialized ++;
        {
           IARM_Bus_DSMgr_EventData_t audio_portstate_event_data;
           audio_portstate_event_data.data.AudioPortStateInfo.audioPortState = dsAUDIOPORT_STATE_INITIALIZED;
           INT_INFO("%s: AudioOutPort PortInitState:%d \r\n", __FUNCTION__, audio_portstate_event_data.data.AudioPortStateInfo.audioPortState);
           IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                           (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_AUDIO_PORT_STATE,
                           (void *)&audio_portstate_event_data,
                           sizeof(audio_portstate_event_data));

       }

       #ifdef DS_AUDIO_SETTINGS_PERSISTENCE
       persist_audioLevel_timer_threadIsAlive=true;
       int err = pthread_create(&persist_audioLevel_timer_threadId, NULL,persist_audioLevel_timer_threadFunc,NULL);
       if(err) {
	   INT_ERROR("%s Failed to create audio level persistence update timer thread\n",__func__);
           persist_audioLevel_timer_threadIsAlive=false;
       }
       #endif

        IARM_BUS_Unlock(lock);  //CID:136568 - Data race condition
    return IARM_RESULT_SUCCESS;
}



IARM_Result_t dsAudioMgr_term()
{
    #ifdef DS_AUDIO_SETTINGS_PERSISTENCE
	if(persist_audioLevel_timer_threadIsAlive){
		persist_audioLevel_timer_threadIsAlive=false;
          	pthread_cond_signal(&audioLevelTimerCV);
		pthread_join(persist_audioLevel_timer_threadId,NULL);
	}
    #endif
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsAudioPortInit(void *arg)
{
    IARM_BUS_Lock(lock);

    if (!m_isInitialized) {

        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetAudioPort,_dsGetAudioPort);
	IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetSupportedARCTypes,_dsGetSupportedARCTypes);
	IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsAudioSetSAD,_dsAudioSetSAD);
	IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsAudioEnableARC,_dsAudioEnableARC);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetStereoMode,_dsSetStereoMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetStereoMode,_dsGetStereoMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetStereoAuto,_dsSetStereoAuto);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetStereoAuto,_dsGetStereoAuto);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetAudioMute,_dsSetAudioMute);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsIsAudioMute,_dsIsAudioMute);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetAudioDucking,_dsSetAudioDucking);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetAudioLevel,_dsSetAudioLevel);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetAudioLevel,_dsGetAudioLevel);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetAudioGain,_dsSetAudioGain);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetAudioGain,_dsGetAudioGain);
	IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetAudioFormat,_dsGetAudioFormat);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetEncoding,_dsGetEncoding);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsIsAudioMSDecode,_dsIsAudioMSDecode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsIsAudioMS12Decode,_dsIsAudioMS12Decode);

        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsIsAudioPortEnabled,_dsIsAudioPortEnabled);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsEnableAudioPort,_dsEnableAudioPort);

        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetEnablePersist, _dsGetEnablePersist);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetEnablePersist, _dsSetEnablePersist);

        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsAudioPortTerm,_dsAudioPortTerm);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsEnableLEConfig,_dsEnableLEConfig);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetLEConfig,_dsGetLEConfig);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetAudioDelay, _dsSetAudioDelay);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetAudioDelay, _dsGetAudioDelay);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetSinkDeviceAtmosCapability, _dsGetSinkDeviceAtmosCapability);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetAudioAtmosOutputMode, _dsSetAudioAtmosOutputMode);      
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetAudioAtmosOutputMode, _dsSetAudioAtmosOutputMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetAudioCompression, _dsSetAudioCompression);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetAudioCompression, _dsGetAudioCompression);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetDialogEnhancement, _dsSetDialogEnhancement);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetDialogEnhancement, _dsGetDialogEnhancement);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetDolbyVolumeMode, _dsSetDolbyVolumeMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetDolbyVolumeMode	, _dsGetDolbyVolumeMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetIntelligentEqualizerMode, _dsSetIntelligentEqualizerMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetIntelligentEqualizerMode, _dsGetIntelligentEqualizerMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetVolumeLeveller, _dsGetVolumeLeveller);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetVolumeLeveller, _dsSetVolumeLeveller);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetBassEnhancer, _dsGetBassEnhancer);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetBassEnhancer, _dsSetBassEnhancer);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsIsSurroundDecoderEnabled, _dsIsSurroundDecoderEnabled);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsEnableSurroundDecoder, _dsEnableSurroundDecoder);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetDRCMode, _dsGetDRCMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetDRCMode, _dsSetDRCMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetSurroundVirtualizer, _dsGetSurroundVirtualizer);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetSurroundVirtualizer, _dsSetSurroundVirtualizer);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetMISteering, _dsGetMISteering);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetMISteering, _dsSetMISteering);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetGraphicEqualizerMode, _dsGetGraphicEqualizerMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetGraphicEqualizerMode, _dsSetGraphicEqualizerMode);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetMS12AudioProfileList, _dsGetMS12AudioProfileList);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetMS12AudioProfile, _dsGetMS12AudioProfile);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetMS12AudioProfile, _dsSetMS12AudioProfile);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetAudioMixerLevels,  _dsSetAudioMixerLevels);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetAssociatedAudioMixing, _dsSetAssociatedAudioMixing);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetAssociatedAudioMixing, _dsGetAssociatedAudioMixing);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetFaderControl, _dsSetFaderControl);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetFaderControl, _dsGetFaderControl);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetPrimaryLanguage, _dsSetPrimaryLanguage);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetPrimaryLanguage, _dsGetPrimaryLanguage);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetSecondaryLanguage, _dsSetSecondaryLanguage);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetSecondaryLanguage, _dsGetSecondaryLanguage);

        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetAudioCapabilities,_dsGetAudioCapabilities); 
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetMS12Capabilities,_dsGetMS12Capabilities); 
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsAudioOutIsConnected, _dsAudioOutIsConnected); 
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsSetMS12SetttingsOverride, _dsSetMS12SetttingsOverride);

        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsResetDialogEnhancement,_dsResetDialogEnhancement);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsResetBassEnhancer,_dsResetBassEnhancer);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsResetSurroundVirtualizer,_dsResetSurroundVirtualizer);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsResetVolumeLeveller,_dsResetVolumeLeveller);
        IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_dsGetHDMIARCPortId, _dsGetHDMIARCPortId);

        dsError_t eRet = _dsAudioOutRegisterConnectCB (_dsAudioOutPortConnectCB);
        if (dsERR_NONE != eRet) {
            INT_DEBUG("%s: _dsAudioOutRegisterConnectCB eRet:%04x", __FUNCTION__, eRet);
        }

        eRet = _dsAudioFormatUpdateRegisterCB (_dsAudioFormatUpdateCB) ;
        if (dsERR_NONE != eRet) {
            INT_DEBUG("%s: _dsAudioFormatUpdateRegisterCB eRet:%04x", __FUNCTION__, eRet);
        }

		eRet = _dsAudioAtmosCapsChangeRegisterCB (_dsAudioAtmosCapsChangeCB) ;
        if (dsERR_NONE != eRet) {
            INT_DEBUG("%s: _dsAudioAtmosCapsChangeRegisterCB eRet:%04x", __FUNCTION__, eRet);
        }


        m_isInitialized = 1;
    }
    
    if (!m_isPlatInitialized) {
        /* Nexus init, if any here */
        dsAudioPortInit();
        AudioConfigInit();
    }
    m_isPlatInitialized++;
 

 IARM_BUS_Unlock(lock);

 return IARM_RESULT_SUCCESS;

}

IARM_Result_t _dsGetAudioPort(void *arg)
{
    _DEBUG_ENTER();

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    dsError_t ret = dsERR_NONE;

    IARM_BUS_Lock(lock);

    dsAudioGetHandleParam_t *param = (dsAudioGetHandleParam_t *)arg;

    if (param != NULL)
    {
        INT_INFO("%s..%d-%d \r\n",__func__,param->type,param->index);
        ret = dsGetAudioPort(param->type, param->index, &param->handle);
        if(ret == dsERR_NONE) {
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);

    return result;
}



IARM_Result_t _dsGetStereoMode(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    dsError_t ret = dsERR_NONE;
    dsAudioSetStereoModeParam_t *param = (dsAudioSetStereoModeParam_t *)arg;

    if (param != NULL && param->toPersist && NULL != param->handle) {
        _GetAudioModeFromPersistent(arg);
        result = IARM_RESULT_SUCCESS;
    }
    else if (param != NULL && NULL != param->handle)
    {
        dsAudioPortType_t _APortType = _GetAudioPortType(param->handle);
        /* In Auto Mode, get the effective mode */
        if (_APortType == dsAUDIOPORT_TYPE_SPDIF && _srv_AudioSPDIFAuto) {
            dsAudioStereoMode_t stereoMode = dsAUDIO_STEREO_UNKNOWN;
            ret = dsGetStereoMode(param->handle, &stereoMode);
            if(ret == dsERR_NONE) {
                result = IARM_RESULT_SUCCESS;
            }
            param->mode = stereoMode;
        }
        else if (_APortType == dsAUDIOPORT_TYPE_HDMI && _srv_AudioHDMIAuto) {
            dsAudioStereoMode_t stereoMode = dsAUDIO_STEREO_UNKNOWN;
            ret = dsGetStereoMode(param->handle, &stereoMode);
            if(ret == dsERR_NONE) {
                result = IARM_RESULT_SUCCESS;
            }
            param->mode = stereoMode;
        }
        else if (_APortType == dsAUDIOPORT_TYPE_HDMI_ARC && _srv_AudioHDMIARCAuto) {
            dsAudioStereoMode_t stereoMode = dsAUDIO_STEREO_UNKNOWN;
            ret = dsGetStereoMode(param->handle, &stereoMode);
            if(ret == dsERR_NONE) {
                result = IARM_RESULT_SUCCESS;
            }
            param->mode = stereoMode;
        }
        else {
            if (_APortType == dsAUDIOPORT_TYPE_SPDIF)
            {
                param->mode = _srv_SPDIF_Audiomode;
                INT_INFO("The SPDIF Port Audio Settings Mode is %d \r\n",param->mode);
            }
            else if (_APortType == dsAUDIOPORT_TYPE_HDMI) {
                param->mode = _srv_HDMI_Audiomode;
                INT_INFO("The HDMI Port Audio Settings Mode is %d \r\n",param->mode);
            }
            else if (_APortType == dsAUDIOPORT_TYPE_HDMI_ARC) {
                param->mode = _srv_HDMI_ARC_Audiomode;
                INT_INFO("The HDMI ARC Port Audio Settings Mode is %d \r\n",param->mode);
            }

            result = IARM_RESULT_SUCCESS;
        } 
   }

    IARM_BUS_Unlock(lock);    

    return result;
}


IARM_Result_t _dsSetStereoMode(void *arg)
{
    _DEBUG_ENTER();
    IARM_Bus_DSMgr_EventData_t eventData;

    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    dsError_t ret = dsERR_NONE;
    dsAudioSetStereoModeParam_t *param = (dsAudioSetStereoModeParam_t *)arg;
    if (NULL != param->handle) {
        ret = dsSetStereoMode(param->handle, param->mode);
        param->rpcResult = ret;
    }
    else {
        ret = dsERR_INVALID_PARAM;
        param->rpcResult = dsERR_INVALID_PARAM;
    }

    if (ret == dsERR_NONE)
    {
        dsAudioPortType_t _APortType = _GetAudioPortType(param->handle);
        try
        {
            if(param->mode == dsAUDIO_STEREO_STEREO)
            {
                INT_INFO("Setting Audio Mode STEREO with persistent value : %d \r\n",param->toPersist);

                if (_APortType == dsAUDIOPORT_TYPE_HDMI)
                {
                    if (param->toPersist)
                    device::HostPersistence::getInstance().persistHostProperty("HDMI0.AudioMode","STEREO");
                
                    _srv_HDMI_Audiomode = dsAUDIO_STEREO_STEREO;
                }
                else if (_APortType == dsAUDIOPORT_TYPE_SPDIF)
                {
                    if (param->toPersist)
                    device::HostPersistence::getInstance().persistHostProperty("SPDIF0.AudioMode","STEREO");
                
                    _srv_SPDIF_Audiomode = dsAUDIO_STEREO_STEREO;
                }
                else if (_APortType == dsAUDIOPORT_TYPE_HDMI_ARC)
                {
                    if (param->toPersist)
                    device::HostPersistence::getInstance().persistHostProperty("HDMI_ARC0.AudioMode","STEREO");

                    _srv_HDMI_ARC_Audiomode = dsAUDIO_STEREO_STEREO;
                }
                eventData.data.Audioport.mode = dsAUDIO_STEREO_STEREO;
                eventData.data.Audioport.type = _APortType;
                IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)IARM_BUS_DSMGR_EVENT_AUDIO_MODE,(void *)&eventData, sizeof(eventData));

            }
            else if(param->mode == dsAUDIO_STEREO_SURROUND)
            {
                INT_INFO("Setting Audio Mode SURROUND with persistent value %d \r\n",param->toPersist);

                if (_APortType == dsAUDIOPORT_TYPE_HDMI)
                {
                    if (param->toPersist)
                    device::HostPersistence::getInstance().persistHostProperty("HDMI0.AudioMode","SURROUND");
                
                    _srv_HDMI_Audiomode = dsAUDIO_STEREO_SURROUND;
                }
                else if (_APortType == dsAUDIOPORT_TYPE_SPDIF)
                {
                    if (param->toPersist)
                    device::HostPersistence::getInstance().persistHostProperty("SPDIF0.AudioMode","SURROUND");

                    _srv_SPDIF_Audiomode = dsAUDIO_STEREO_SURROUND;
                }
                else if (_APortType == dsAUDIOPORT_TYPE_HDMI_ARC)
                {
                    if (param->toPersist)
                    device::HostPersistence::getInstance().persistHostProperty("HDMI_ARC0.AudioMode","SURROUND");

                    _srv_HDMI_ARC_Audiomode = dsAUDIO_STEREO_SURROUND;
                }

                eventData.data.Audioport.mode = dsAUDIO_STEREO_SURROUND;
                eventData.data.Audioport.type = _APortType;
                IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)IARM_BUS_DSMGR_EVENT_AUDIO_MODE,(void *)&eventData, sizeof(eventData));
            }
            else if(param->mode == dsAUDIO_STEREO_DD)
            {
                INT_INFO("Setting Audio Mode Dolby Digital with persistent value %d \r\n",param->toPersist);

                if (_APortType == dsAUDIOPORT_TYPE_HDMI)
                {
                    if (param->toPersist)
                    device::HostPersistence::getInstance().persistHostProperty("HDMI0.AudioMode","DOLBYDIGITAL");

                    _srv_HDMI_Audiomode = dsAUDIO_STEREO_DD;
                }
                eventData.data.Audioport.mode = dsAUDIO_STEREO_DD;
                eventData.data.Audioport.type = _APortType;
                IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)IARM_BUS_DSMGR_EVENT_AUDIO_MODE,(void *)&eventData, sizeof(eventData));
            }
            else if(param->mode == dsAUDIO_STEREO_DDPLUS)
            {
                INT_INFO("Setting Audio Mode Dolby Digital Plus with persistent value %d \r\n",param->toPersist);

                if (_APortType == dsAUDIOPORT_TYPE_HDMI)
                {
                    if (param->toPersist)
                    device::HostPersistence::getInstance().persistHostProperty("HDMI0.AudioMode","DOLBYDIGITALPLUS");

                    _srv_HDMI_Audiomode = dsAUDIO_STEREO_DDPLUS;
                }
                eventData.data.Audioport.mode = dsAUDIO_STEREO_DDPLUS;
                eventData.data.Audioport.type = _APortType;
                IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)IARM_BUS_DSMGR_EVENT_AUDIO_MODE,(void *)&eventData, sizeof(eventData));
            }
            else if(param->mode == dsAUDIO_STEREO_PASSTHRU)
            {
                INT_INFO("Setting Audio Mode PASSTHRU with persistent value %d \r\n",param->toPersist);

                if (_APortType == dsAUDIOPORT_TYPE_HDMI)
                {
                    if (param->toPersist)
                    device::HostPersistence::getInstance().persistHostProperty("HDMI0.AudioMode","PASSTHRU");
                    _srv_HDMI_Audiomode = dsAUDIO_STEREO_PASSTHRU;
                }
                else if (_APortType == dsAUDIOPORT_TYPE_SPDIF)
                {
                    if (param->toPersist)
                    device::HostPersistence::getInstance().persistHostProperty("SPDIF0.AudioMode","PASSTHRU");
                    _srv_SPDIF_Audiomode = dsAUDIO_STEREO_PASSTHRU;
                }
                else if (_APortType == dsAUDIOPORT_TYPE_HDMI_ARC)
                {
                    if (param->toPersist)
                    device::HostPersistence::getInstance().persistHostProperty("HDMI_ARC0.AudioMode","PASSTHRU");

                    _srv_HDMI_ARC_Audiomode = dsAUDIO_STEREO_PASSTHRU;
                }

                eventData.data.Audioport.mode = dsAUDIO_STEREO_PASSTHRU;
                eventData.data.Audioport.type = _APortType;
                IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)IARM_BUS_DSMGR_EVENT_AUDIO_MODE,(void *)&eventData, sizeof(eventData));

            }

            result = IARM_RESULT_SUCCESS;
        }
        catch(...)
        {
            INT_INFO("Error in Setting audio mode... \r\n");
        }

    }

    IARM_BUS_Unlock(lock);

    return result;
}

IARM_Result_t _dsGetStereoAuto(void *arg)
{
    _DEBUG_ENTER();
    if ( !arg ) {
        return IARM_RESULT_INVALID_PARAM;
    }
    IARM_BUS_Lock(lock);

    dsAudioSetStereoAutoParam_t *param = (dsAudioSetStereoAutoParam_t *)arg;

    dsAudioPortType_t _APortType = _GetAudioPortType(param->handle);

    if (param != NULL)
    {
        if (_APortType == dsAUDIOPORT_TYPE_HDMI) {
            param->autoMode = _srv_AudioHDMIAuto ? 1 : 0;
        }
        else if (_APortType == dsAUDIOPORT_TYPE_HDMI_ARC) {
            param->autoMode = _srv_AudioHDMIARCAuto ? 1 : 0;
        }
        else if (_APortType == dsAUDIOPORT_TYPE_SPDIF) {
            param->autoMode = _srv_AudioSPDIFAuto ? 1 : 0;
        }
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsSetStereoAuto(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    dsAudioSetStereoAutoParam_t *param = (dsAudioSetStereoAutoParam_t *)arg;

    dsAudioPortType_t _APortType = _GetAudioPortType(param->handle);

    if (param->toPersist) {
	switch(_APortType) {
	    case dsAUDIOPORT_TYPE_HDMI:
	        device::HostPersistence::getInstance().persistHostProperty("HDMI0.AudioMode.AUTO", param->autoMode ? "TRUE" : "FALSE");
            _srv_AudioHDMIAuto = param->autoMode;
		break;

	    case dsAUDIOPORT_TYPE_HDMI_ARC:
	        device::HostPersistence::getInstance().persistHostProperty("HDMI_ARC0.AudioMode.AUTO", param->autoMode ? "TRUE" : "FALSE");
            _srv_AudioHDMIARCAuto = param->autoMode;
		break;

	    case dsAUDIOPORT_TYPE_SPDIF:
		device::HostPersistence::getInstance().persistHostProperty("SPDIF0.AudioMode.AUTO", param->autoMode ? "TRUE" : "FALSE");
            _srv_AudioSPDIFAuto = param->autoMode;
		break;

	    default:
		break;
	}
    }

    if ((_APortType == dsAUDIOPORT_TYPE_HDMI_ARC) || (_APortType == dsAUDIOPORT_TYPE_SPDIF)) {
        typedef dsError_t (*dsSetStereoAuto_t)(intptr_t handle, int autoMode);
        static dsSetStereoAuto_t func = 0;
        if (func == 0) {
            void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
            if (dllib) {
                func = (dsSetStereoAuto_t) dlsym(dllib, "dsSetStereoAuto");
                if (func) {
                    INT_DEBUG("dsSetStereoAuto_t(int, int *) is defined and loaded\r\n");
                }
                else {
                    INT_INFO("dsSetStereoAuto_t(int, int *) is not defined\r\n");
                }
                dlclose(dllib);
            }
            else {
                INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
            }
        }

        if (func != 0)
        {
            if (func(param->handle, param->autoMode) == dsERR_NONE)
            {
               result = IARM_RESULT_SUCCESS;
            }
        }
    }

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsSetAudioDucking(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);
    int volume = 0;
    bool portEnabled = false;
    dsAudioSetDuckingParam_t *param = (dsAudioSetDuckingParam_t *)arg;
    IARM_Bus_DSMgr_EventData_t eventData;
    INT_DEBUG("%s action : %d type :%d val :%d m_volumeLevel:%f \n",__FUNCTION__,param->action,param->type,param->level,m_volumeLevel );

    dsError_t ret = dsIsAudioPortEnabled(param->handle, &portEnabled);
    if (ret != dsERR_NONE) {
        INT_INFO("%s failed dsIsAudioPortEnabled\n",__FUNCTION__);
    }

    if(param->action == dsAUDIO_DUCKINGACTION_START)
    {
        m_isDuckingInProgress = true;
	if(param->type == dsAUDIO_DUCKINGTYPE_RELATIVE )
	{
             volume = (m_volumeLevel * param->level) / 100;
	}
	else
	{
           if(param->level > m_volumeLevel)
           {
		 volume =  m_volumeLevel;
	   }
           else
	   {
        	 volume = param->level;
           }
	}
    }
    else
    {
	m_isDuckingInProgress = false;
	volume = m_volumeLevel;
    }

    if(m_MuteStatus || !portEnabled)
    {
        INT_INFO("%s mute on/port disabled so ignore the duckig request\n",__FUNCTION__);
        m_volumeDuckingLevel = volume;
        IARM_BUS_Unlock(lock);
        return IARM_RESULT_SUCCESS;
    }

    INT_DEBUG(":%s adjusted volume volume :%d m_volumeDuckingLevel :%d\n",__FUNCTION__,volume,m_volumeDuckingLevel );

    // apply volume to hal layer
    dsAudioPortType_t _APortType = _GetAudioPortType(param->handle);
    if (dsSetAudioLevelFunc == 0)
    {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsSetAudioLevelFunc = (dsSetAudioLevel_t) dlsym(dllib, "dsSetAudioLevel");
            if (dsSetAudioLevelFunc) {
                INT_DEBUG("dsSetAudioLevel_t(int, float ) is defined and loaded \r\n");
	    }
            else {
                INT_INFO("dsSetAudioLevel_t(int, float ) is not defined \r\n");
	    }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed \r\n", RDK_DSHAL_NAME);
	}
    }
    if (dsSetAudioLevelFunc != 0 )
    {
        dsSetAudioLevelFunc(param->handle, volume);
    }

    if(volume != m_volumeDuckingLevel)
    {
        m_volumeDuckingLevel = volume;

        dsAudioStereoMode_t mode = dsAUDIO_STEREO_STEREO;
        if (_APortType == dsAUDIOPORT_TYPE_SPDIF)
        {
                mode = _srv_SPDIF_Audiomode;
        }
        else if (_APortType == dsAUDIOPORT_TYPE_HDMI) {
                mode = _srv_HDMI_Audiomode;
        }
        INT_DEBUG("The Port type is :%d  Audio Settings Mode is %d \r\n",_APortType, mode);

        if(mode == dsAUDIO_STEREO_PASSTHRU && volume != 100)
        {
            eventData.data.AudioLevelInfo.level = 0;
            INT_DEBUG(" IARM_BUS_DSMGR_EVENT_AUDIO_LEVEL_CHANGED PASSTHRU mode volume:%d \n",eventData.data.AudioLevelInfo.level);
        }
        else
        {
            eventData.data.AudioLevelInfo.level = volume;
            INT_DEBUG(" IARM_BUS_DSMGR_EVENT_AUDIO_LEVEL_CHANGED  volume:%d \n ",eventData.data.AudioLevelInfo.level);
        }
        IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,(IARM_EventId_t)IARM_BUS_DSMGR_EVENT_AUDIO_LEVEL_CHANGED,(void *)&eventData, sizeof(eventData));
    }
    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsGetAudioGain(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetAudioGain_t)(intptr_t handle, float *gain);
    static dsGetAudioGain_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetAudioGain_t) dlsym(dllib, "dsGetAudioGain");
            if (func) {
                INT_DEBUG("dsGetAudioGain_t(int, float *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetAudioGain_t(int, float *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsAudioGainParam_t *param = (dsAudioGainParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        float gain = 0;
        param->gain = 0;
        if (func(param->handle, &gain) == dsERR_NONE)
        {
           param->gain = gain;
           result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsGetAudioLevel(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    if (dsGetAudioLevelfunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsGetAudioLevelfunc = (dsGetAudioLevel_t) dlsym(dllib, "dsGetAudioLevel");
            if (dsGetAudioLevelfunc) {
                INT_DEBUG("dsGetAudioLevel_t(int, float *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetAudioLevel_t(int, float *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsAudioSetLevelParam_t *param = (dsAudioSetLevelParam_t *)arg;

    if (dsGetAudioLevelfunc != 0 && param != NULL)
    {
        float level = 0;
        param->level = 0;
        if (dsGetAudioLevelfunc(param->handle, &level) == dsERR_NONE)
        {
           param->level = level;
           result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsSetAudioGain(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSetAudioGain_t)(intptr_t handle, float gain);
    static dsSetAudioGain_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetAudioGain_t) dlsym(dllib, "dsSetAudioGain");
            if (func) {
                INT_DEBUG("dsSetAudioGain_t(int, float ) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetAudioGain_t(int, float ) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsAudioGainParam_t *param = (dsAudioGainParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->gain) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            std::string _AudioGain = std::to_string(param->gain);
            dsAudioPortType_t _APortType = _GetAudioPortType(param->handle);
            switch(_APortType) {
                case dsAUDIOPORT_TYPE_SPDIF:
                    INT_INFO("%s: port: %s , persist audio gain: %f\n",__func__,"SPDIF0", param->gain);
                    device::HostPersistence::getInstance().persistHostProperty("SPDIF0.audio.Gain",_AudioGain);
                    break;
                case dsAUDIOPORT_TYPE_HDMI:
                    INT_INFO("%s: port: %s , persist audio gain: %f\n",__func__,"HDMI0", param->gain);
                    device::HostPersistence::getInstance().persistHostProperty("HDMI0.audio.Gain",_AudioGain);
                    break;
                case dsAUDIOPORT_TYPE_SPEAKER:
                    INT_INFO("%s: port: %s , persist audio gain: %f\n",__func__,"SPEAKER0", param->gain);
                    device::HostPersistence::getInstance().persistHostProperty("SPEAKER0.audio.Gain",_AudioGain);
                    break;
                default:
                    break; 
            }
#endif
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);

    return result;

}

IARM_Result_t _dsSetAudioLevel(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    if (dsSetAudioLevelFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsSetAudioLevelFunc = (dsSetAudioLevel_t) dlsym(dllib, "dsSetAudioLevel");
            if (dsSetAudioLevelFunc) {
                INT_DEBUG("dsSetAudioLevel_t(int, float ) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetAudioLevel_t(int, float ) is not defined\r\n");
                IARM_BUS_Unlock(lock);
                dlclose(dllib);
                return IARM_RESULT_INVALID_STATE;
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsAudioSetLevelParam_t *param = (dsAudioSetLevelParam_t *)arg;
    if (dsSetAudioLevelFunc != 0 && param != NULL)
    {
        dsAudioPortType_t _APortType = _GetAudioPortType(param->handle);
        if(_APortType == dsAUDIOPORT_TYPE_SPEAKER)
        {
            INT_DEBUG("_dsSetAudioLevel param->level :%f m_isDuckingInProgress :%d  \n",param->level,m_isDuckingInProgress);
            float currlevel = 0;
            if (dsGetAudioLevelfunc == 0) {
                void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
                if (dllib) {
                    dsGetAudioLevelfunc = (dsGetAudioLevel_t) dlsym(dllib, "dsGetAudioLevel");
                    if (dsGetAudioLevelfunc) {
                        INT_DEBUG("dsGetAudioLevel_t(int, float *) is defined and loaded\r\n");
                    }
                    else {
                        INT_INFO("dsGetAudioLevel_t(int, float *) is not defined\r\n");
                        dlclose(dllib);
                        IARM_BUS_Unlock(lock);
                        return IARM_RESULT_INVALID_STATE;
                    }
                    dlclose(dllib);
                } 
                else {
                    INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
                }
            }

            if (dsGetAudioLevelfunc != 0 && param != NULL)
            {
               if( dsGetAudioLevelfunc(param->handle, &currlevel) == dsERR_NONE)
               {
                 result = IARM_RESULT_SUCCESS;
               }
            }
            if(m_isDuckingInProgress && currlevel != m_volumeDuckingLevel )
            {
               dsSetAudioLevelFunc(param->handle, m_volumeDuckingLevel);
               m_volumeLevel = (int) param->level;
            }
            else if (m_isDuckingInProgress || dsSetAudioLevelFunc(param->handle, param->level) == dsERR_NONE)
            {
              m_volumeLevel = (int) param->level;
              result = IARM_RESULT_SUCCESS;
            }
        } else if( dsSetAudioLevelFunc(param->handle, param->level) == dsERR_NONE) {
            result = IARM_RESULT_SUCCESS;
        }
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
        std::string _AudioLevel = std::to_string(param->level);
        switch(_APortType) {
                case dsAUDIOPORT_TYPE_SPDIF:
                    if(persist_audioLevel_timer_threadIsAlive) {
                      audioLevel_cache_spdif = {param->level};
                    }
                    else {
                      device::HostPersistence::getInstance().persistHostProperty("SPDIF0.audio.Level",_AudioLevel);
                    }
                    break;
                case dsAUDIOPORT_TYPE_HDMI:
                    if(persist_audioLevel_timer_threadIsAlive) {
                      audioLevel_cache_hdmi = {param->level};
                    }
                    else {
                      device::HostPersistence::getInstance().persistHostProperty("HDMI0.audio.Level",_AudioLevel);
                    }
                    break;
                case dsAUDIOPORT_TYPE_SPEAKER:
		    if(persist_audioLevel_timer_threadIsAlive) {
                      audioLevel_cache_speaker = {param->level};
                    }
                    else {
                      device::HostPersistence::getInstance().persistHostProperty("SPEAKER0.audio.Level",_AudioLevel);
                    }
                    break;
                case dsAUDIOPORT_TYPE_HEADPHONE:
                    if(persist_audioLevel_timer_threadIsAlive) {
                      audioLevel_cache_headphone = {param->level};
                    }
                    else {
                      device::HostPersistence::getInstance().persistHostProperty("HEADPHONE0.audio.Level",_AudioLevel);
                    }
                    break;
                default:
                    break;
        }
	if(persist_audioLevel_timer_threadIsAlive){
		if(!audioLevel_timer_set){
                  	pthread_mutex_lock(&audioLevelMutex);
			audioLevel_timer_set = true;
                  	if(pthread_cond_signal(&audioLevelTimerCV) != 0){
                        	INT_INFO("Error in signalling pthread CV\n");
                        }
                  	pthread_mutex_unlock(&audioLevelMutex);
                }
	}

#endif
    }

    IARM_BUS_Unlock(lock);

    return result;

}

static IARM_Result_t setAudioDuckingAudioLevel(intptr_t handle)
{
    float volume = 0;
    if(m_isDuckingInProgress)
    {
         volume = m_volumeDuckingLevel;
    }
    else
    {
         volume = m_volumeLevel;
    }
    if (dsSetAudioLevelFunc == 0)
    {
         void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
         if (dllib) {
             dsSetAudioLevelFunc = (dsSetAudioLevel_t) dlsym(dllib, "dsSetAudioLevel");
             if (dsSetAudioLevelFunc) {
                INT_DEBUG("dsSetAudioLevel_t(int, float ) is defined and loaded \r\n");
             }
             else {
                INT_INFO("dsSetAudioLevel_t(int, float ) is not defined \r\n");
                dlclose(dllib);
                return IARM_RESULT_INVALID_STATE;
             }
             dlclose(dllib);
         }
         else {
             INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed \r\n", RDK_DSHAL_NAME);
         }
    } else {
         dsSetAudioLevelFunc(handle, volume);
    }
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t _dsSetAudioMute(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    dsError_t ret = dsERR_NONE;

    dsAudioSetMutedParam_t *param = (dsAudioSetMutedParam_t *)arg;
    dsAudioPortType_t _APortType = _GetAudioPortType(param->handle);
    if(param->mute == false && _APortType == dsAUDIOPORT_TYPE_SPEAKER )
    {
       if(IARM_RESULT_SUCCESS != setAudioDuckingAudioLevel(param->handle)){
           IARM_BUS_Unlock(lock);
           return IARM_RESULT_INVALID_STATE;
       }
    }
   
    ret = dsSetAudioMute(param->handle, param->mute);
    if (ret == dsERR_NONE) {
	    m_MuteStatus = param->mute;
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            std::string _mute = param->mute ? "TRUE" : "FALSE";
            switch(_APortType) {
                case dsAUDIOPORT_TYPE_SPDIF:
                    INT_DEBUG("%s: port: %s , persist audio mute: %s\n",__func__,"SPDIF0", param->mute ? "TRUE" : "FALSE");
                    device::HostPersistence::getInstance().persistHostProperty("SPDIF0.audio.mute", _mute);
                    break;
                case dsAUDIOPORT_TYPE_HDMI:
                    INT_DEBUG("%s: port: %s , persist audio mute: %s\n",__func__,"HDMI0", param->mute ? "TRUE" : "FALSE");
                    device::HostPersistence::getInstance().persistHostProperty("HDMI0.audio.mute", _mute);
                    break;
                case dsAUDIOPORT_TYPE_SPEAKER:
                    INT_DEBUG("%s: port: %s , persist audio mute: %s\n",__func__,"SPEAKER0", param->mute ? "TRUE" : "FALSE");
                    device::HostPersistence::getInstance().persistHostProperty("SPEAKER0.audio.mute", _mute);
                    break;
                case dsAUDIOPORT_TYPE_HEADPHONE:
                    INT_DEBUG("%s: port: %s , persist audio mute: %s\n",__func__,"HEADPHONE0", param->mute ? "TRUE" : "FALSE");
                    device::HostPersistence::getInstance().persistHostProperty("HEADPHONE0.audio.mute", _mute);
                    break;
                default:
                    break;
    }
#endif
        result = IARM_RESULT_SUCCESS;
    }
    IARM_BUS_Unlock(lock);

    return result;
}


IARM_Result_t _dsIsAudioMute(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;

    dsAudioSetMutedParam_t *param = (dsAudioSetMutedParam_t *)arg;
    bool muted = false;
    
    dsError_t ret = dsIsAudioMute(param->handle, &muted);
    if (ret == dsERR_NONE) {
        param->mute = muted;

#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
        std::string isMuteKey("");
        std::string _mute("FALSE");
        dsAudioPortType_t _APortType = _GetAudioPortType(param->handle);
            switch(_APortType) {
                case dsAUDIOPORT_TYPE_SPDIF:
                    isMuteKey.append("SPDIF0.audio.mute");
                    break;
                case dsAUDIOPORT_TYPE_HDMI:
                    isMuteKey.append("HDMI0.audio.mute");
                    break;
                case dsAUDIOPORT_TYPE_SPEAKER:
                    isMuteKey.append("SPEAKER0.audio.mute");
                    break;
                case dsAUDIOPORT_TYPE_HEADPHONE:
                    isMuteKey.append("HEADPHONE0.audio.mute");
                    break;
                default:
                    break;
            }
        try {
            _mute = device::HostPersistence::getInstance().getProperty(isMuteKey);
        }
        catch(...) {
            INT_INFO("%s : Exception in getting the %s from persistence storage\n", __FUNCTION__, isMuteKey.c_str());
            _mute = "FALSE";
        }
        if ("TRUE" == _mute) {
            param->mute = true;
        }
        INT_DEBUG("%s: persist value:%s for :%s\n", __FUNCTION__, _mute.c_str(), isMuteKey.c_str());
#endif //DS_AUDIO_SETTINGS_PERSISTENCE end

        result = IARM_RESULT_SUCCESS;
    }

    IARM_BUS_Unlock(lock);

    return result;
}


IARM_Result_t _dsIsAudioPortEnabled(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;

    dsAudioPortEnabledParam_t *param = (dsAudioPortEnabledParam_t *)arg;
    bool enabled = false;
    
    dsError_t ret = dsIsAudioPortEnabled(param->handle, &enabled);
    if (ret == dsERR_NONE) {
        param->enabled = enabled;
        result = IARM_RESULT_SUCCESS;
    }
    INT_DEBUG("%s : returned ret: %04x enabled: %s\n", __FUNCTION__, ret, param->enabled? "TRUE":"FALSE");

    IARM_BUS_Unlock(lock);

    return result;
}

bool dsSetAudioDelayInternal(intptr_t handle, uint32_t audioDelay)
{
    bool result = false;
    typedef dsError_t (*dsSetAudioDelay_t)(intptr_t handle, uint32_t audioDelayMs);
    static dsSetAudioDelay_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetAudioDelay_t) dlsym(dllib, "dsSetAudioDelay");
            if (func) {
                INT_DEBUG("dsSetAudioDelay_t(int, uint32_t) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetAudioDelay_t(int, uint32_t) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    if (func != 0 )
    {
        if (func(handle, audioDelay) != dsERR_NONE)
        {
            INT_INFO("%s: (SERVER) Unable to set audiodelay\n", __FUNCTION__);
        }
        else
        {
            INT_INFO("%s: (SERVER) success \n", __FUNCTION__);
	    result = true;
        }
    }
    return result;
}



IARM_Result_t _dsEnableAudioPort(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    dsError_t ret = dsERR_NONE;

    dsAudioPortEnabledParam_t *param = (dsAudioPortEnabledParam_t *)arg;

    dsAudioPortType_t _APortType = _GetAudioPortType(param->handle);
    if( _APortType == dsAUDIOPORT_TYPE_SPEAKER )
    {
        bool muted = false;
        dsError_t ret = dsIsAudioMute(param->handle, &muted);
        if(ret != dsERR_NONE)
        {
            INT_INFO("%s : Failed to get the Mute status of Speaker", __FUNCTION__);
        }
        if(param->enabled == 1 && muted != true ) {
            if(IARM_RESULT_SUCCESS != setAudioDuckingAudioLevel(param->handle)) {
                IARM_BUS_Unlock(lock);
                return IARM_RESULT_INVALID_STATE;
            }
        }
        else {
            INT_INFO("%s : Not setting the Audio Ducking level as Mute status of Speaker is %d",__FUNCTION__, muted);
        }
    }
    ret = dsEnableAudioPort(param->handle, param->enabled);
    if(ret == dsERR_NONE) {
          result = IARM_RESULT_SUCCESS;
    }

    std::string isEnabledAudioPortKey("audio.");
    isEnabledAudioPortKey.append (param->portName);
    isEnabledAudioPortKey.append (".isEnabled");


    /*Ensure settings is enabled properly in HAL*/
    bool bAudioPortEnableVerify = false;
    ret = dsIsAudioPortEnabled (param->handle, &bAudioPortEnableVerify);
    if(dsERR_NONE == ret) {
        if (bAudioPortEnableVerify != param->enabled) {
            INT_DEBUG("%s : %s Audio port status verification failed. param->enabled: %d bAudioPortEnableVerify:%d\n", 
                    __FUNCTION__, isEnabledAudioPortKey.c_str(), param->enabled, bAudioPortEnableVerify);
        }
        else {
            INT_DEBUG("%s : %s Audio port status verification passed. status %d\n", __FUNCTION__, isEnabledAudioPortKey.c_str(), param->enabled);
            if (dsAUDIOPORT_TYPE_MAX>_APortType) {
                m_AudioPortEnabled[_APortType] = param->enabled;
                INT_INFO(" %s : port id:%d enabled status :%d\n", __FUNCTION__, _APortType , param->enabled);
                if(m_AudioPortEnabled[_APortType])
                {
                    uint32_t audioDelay = dsGetAudioDelayInternal(_APortType);
                    bool isAudioDelaySet = dsSetAudioDelayInternal(param->handle,audioDelay);
                    INT_INFO(" %s :updated audio delay for port enable  port id:%d audioDelay:%d\n", __FUNCTION__, _APortType , audioDelay);
                }
            }
        }
    }
    else {
        INT_INFO("%s : %s Audio port status:%s verification step: dsIsAudioPortEnabled call failed\n", 
               __FUNCTION__, isEnabledAudioPortKey.c_str(), param->enabled? "TRUE":"FALSE");
    }
 
    IARM_BUS_Unlock(lock);
    
    return result;
}

IARM_Result_t _dsGetEnablePersist(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;

    dsAudioPortEnabledParam_t *param = (dsAudioPortEnabledParam_t *)arg;
    //By default all the ports are enabled.
    bool enabled = true;

    std::string isEnabledAudioPortKey("audio.");
    isEnabledAudioPortKey.append (param->portName);
    isEnabledAudioPortKey.append (".isEnabled");
    std::string _AudioPortEnable("TRUE");
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
    try {
        _AudioPortEnable = device::HostPersistence::getInstance().getProperty(isEnabledAudioPortKey);
    }
    catch(...) {
        try {
            INT_DEBUG("Init: %s : %s port enable settings not found in persistence store. Try system default\n",__FUNCTION__, isEnabledAudioPortKey.c_str());
            _AudioPortEnable = device::HostPersistence::getInstance().getDefaultProperty(isEnabledAudioPortKey);
        }
        catch(...) {
            /*By default enable all the ports*/
            _AudioPortEnable = "TRUE";
        }
    }
    if ("FALSE" == _AudioPortEnable) { 
       INT_DEBUG("%s: persist dsEnableAudioPort value: _AudioPortEnable:%s:\n", __FUNCTION__, _AudioPortEnable.c_str());  
        enabled = false;
    }
    else {
        INT_DEBUG("%s: persist dsEnableAudioPort value: _AudioPortEnable:%s:\n", __FUNCTION__, _AudioPortEnable.c_str());  
        enabled = true;
    }

#endif //DS_AUDIO_SETTINGS_PERSISTENCE end
    
    param->enabled = enabled;
    result = IARM_RESULT_SUCCESS;
    INT_INFO("%s: persist dsEnableAudioPort value: %s for the port %s AudioPortEnable: %s result:%d \n", 
           __FUNCTION__, param->enabled? "TRUE":"FALSE", isEnabledAudioPortKey.c_str(), _AudioPortEnable.c_str(), result);

    IARM_BUS_Unlock(lock);

    return result;
}


IARM_Result_t _dsSetEnablePersist(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    dsError_t ret = dsERR_NONE;

    dsAudioPortEnabledParam_t *param = (dsAudioPortEnabledParam_t *)arg;
    result = IARM_RESULT_SUCCESS;

    std::string isEnabledAudioPortKey("audio.");
    isEnabledAudioPortKey.append (param->portName);
    isEnabledAudioPortKey.append (".isEnabled");
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
    INT_DEBUG("%s: persist dsEnableAudioPort value: %s for the port %s\n", __FUNCTION__, param->enabled? "TRUE":"FALSE", isEnabledAudioPortKey.c_str());
    device::HostPersistence::getInstance().persistHostProperty(isEnabledAudioPortKey.c_str(), param->enabled? ("TRUE"):("FALSE"));
#endif //DS_AUDIO_SETTINGS_PERSISTENCE end
 
    IARM_BUS_Unlock(lock);
    
    return result;
}


IARM_Result_t _dsAudioPortTerm(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    m_isPlatInitialized--;

    if (0 == m_isPlatInitialized)
    {
        dsError_t ret = dsAudioPortTerm();
        if(ret != dsERR_NONE) {
            INT_INFO("_dsAudioPortTerm is not success\r\n");
        }
    }

    IARM_BUS_Unlock(lock);

    return IARM_RESULT_SUCCESS;
}


IARM_Result_t _dsGetAudioFormat(void *arg)
{
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetAudioFormat_t)(intptr_t handle, dsAudioFormat_t *audioFormat);
    static dsGetAudioFormat_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetAudioFormat_t) dlsym(dllib, "dsGetAudioFormat");
            if (func) {
                INT_DEBUG("dsGetAudioFormat_t(int, dsAudioFormat_t *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetAudioFormat_t(int, dsAudioFormat_t *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsAudioFormatParam_t *param = (dsAudioFormatParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        dsAudioFormat_t aFormat = dsAUDIO_FORMAT_NONE;
        param->audioFormat = dsAUDIO_FORMAT_NONE;

        if (func(param->handle, &aFormat) == dsERR_NONE)
        {
           param->audioFormat = aFormat;
           result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsGetEncoding(void *arg)
{
  IARM_BUS_Unlock(lock);

#ifndef RDK_DSHAL_NAME
    #warning   "RDK_DSHAL_NAME is not defined"
    #define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif

    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;

    IARM_BUS_Lock(lock);
    dsError_t ret = dsERR_NONE;
    dsAudioSetStereoModeParam_t *s_param = (dsAudioSetStereoModeParam_t *)arg;
    if (s_param != NULL && NULL != s_param->handle)
    {
        dsAudioStereoMode_t stereoMode = dsAUDIO_STEREO_UNKNOWN;
        ret = dsGetStereoMode(s_param->handle, &stereoMode);
        if(ret == dsERR_NONE) {
            result = IARM_RESULT_SUCCESS;
        }
        s_param->mode = stereoMode;
   }

   dsAudioEncoding_t _encoding = dsAUDIO_ENC_NONE;
   if(result == IARM_RESULT_SUCCESS){
       switch(s_param->mode){
           case dsAUDIO_STEREO_STEREO:
               _encoding = dsAUDIO_ENC_PCM;
	       break;
           case dsAUDIO_STEREO_DD:
	       _encoding = dsAUDIO_ENC_AC3;
	       break;
           case dsAUDIO_STEREO_DDPLUS:
	       _encoding = dsAUDIO_ENC_EAC3;
	       break;
           case dsAUDIO_STEREO_SURROUND:
	   case dsAUDIO_STEREO_PASSTHRU:
	       _encoding = dsAUDIO_ENC_DISPLAY;
	       break;
           case dsAUDIO_STEREO_UNKNOWN:
	   default:
	       _encoding = dsAUDIO_ENC_NONE;
	       break;
       }
   }
   dsAudioGetEncodingModeParam_t *param = (dsAudioGetEncodingModeParam_t *)arg;
   param->encoding = _encoding;
   INT_DEBUG("param->encoding = %d\r\n",_encoding);
   IARM_BUS_Unlock(lock);
   return result;

}

static dsAudioPortType_t _GetAudioPortType(intptr_t handle)
{
    int numPorts,i;
    intptr_t halhandle = 0;

    numPorts = dsUTL_DIM(kSupportedPortTypes);
    
    for(i=0; i< numPorts; i++)
    {
        if(dsGetAudioPort (kPorts[i].id.type, kPorts[i].id.index, &halhandle) == dsERR_NONE) {
            if (handle == halhandle)
            {
                return kPorts[i].id.type;
            }
        }
    }
    INT_INFO("Error: The Requested Audio Port is not part of Platform Port Configuration \r\n");
    return dsAUDIOPORT_TYPE_MAX;
}


IARM_Result_t _dsIsAudioMSDecode(void *arg)
{

    IARM_BUS_Unlock(lock);

#ifndef RDK_DSHAL_NAME
    #warning   "RDK_DSHAL_NAME is not defined"
    #define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    typedef dsError_t  (*dsIsAudioMSDecode_t)(intptr_t handle, bool *HasMS11Decode);
    static dsIsAudioMSDecode_t func = NULL;
    if (func == NULL) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsIsAudioMSDecode_t) dlsym(dllib, "dsIsAudioMSDecode");
            if (func) {
                INT_DEBUG("dsIsAudioMSDecode(int, bool*) is defined and loaded\r\n");
            }   
            else {
                INT_INFO("dsIsAudioMSDecode(int, bool*) is not defined\r\n");
            }   
            dlclose(dllib);
        }   
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }   
    }   

    dsAudioGetMS11Param_t *param = (dsAudioGetMS11Param_t *)arg;
    if (func != NULL) {
        bool HasMS11Decode = false;
        dsError_t ret = func(param->handle, &HasMS11Decode);
        if (ret == dsERR_NONE) {
            param->ms11Enabled = HasMS11Decode;
            result = IARM_RESULT_SUCCESS;
        }
    }else {
        param->ms11Enabled = false;
    }

    IARM_BUS_Unlock(lock);

    return result;
}

IARM_Result_t _dsIsAudioMS12Decode(void *arg)
{

    IARM_BUS_Unlock(lock);

#ifndef RDK_DSHAL_NAME
    #warning   "RDK_DSHAL_NAME is not defined"
    #define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);
    
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    typedef dsError_t  (*dsIsAudioMS12Decode_t)(intptr_t handle, bool *HasMS12Decode);
    static dsIsAudioMS12Decode_t func = NULL;
    if (func == NULL) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsIsAudioMS12Decode_t) dlsym(dllib, "dsIsAudioMS12Decode");
            if (func) {
                INT_DEBUG("dsIsAudioMS12Decode(int, bool*) is defined and loaded\r\n");
            }   
            else {
                INT_INFO("dsIsAudioMS12Decode(int, bool*) is not defined\r\n");
            }   
            dlclose(dllib);
        }   
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }   
    }   

    dsAudioGetMS12Param_t *param = (dsAudioGetMS12Param_t *)arg;
    if (func != NULL) {
        bool HasMS12Decode = false;
        dsError_t ret = func(param->handle, &HasMS12Decode);
        if (ret == dsERR_NONE) {
            param->ms12Enabled = HasMS12Decode;
            result = IARM_RESULT_SUCCESS;
        }
    }else {
        param->ms12Enabled = false;
    }

    IARM_BUS_Unlock(lock);

    return result;
}

IARM_Result_t _dsSetAudioDelay(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_SUCCESS;
    typedef dsError_t (*dsSetAudioDelay_t)(intptr_t handle, uint32_t audioDelayMs);
    static dsSetAudioDelay_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetAudioDelay_t) dlsym(dllib, "dsSetAudioDelay");
            if (func) {
                INT_DEBUG("dsSetAudioDelay_t(int, uint32_t) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetAudioDelay_t(int, uint32_t) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsSetAudioDelayParam_t *param = (dsSetAudioDelayParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        dsAudioPortType_t _APortType = _GetAudioPortType(param->handle);
        if (dsAUDIOPORT_TYPE_MAX>_APortType ) {
            if (m_AudioPortEnabled[_APortType])
            {
                if(func(param->handle, param->audioDelayMs) != dsERR_NONE)
                {
                    INT_INFO("%s: (SERVER) Unable to set audiodelay\n", __FUNCTION__);
                    result = IARM_RESULT_INVALID_STATE;
                }
            }
            else
            {
                INT_INFO("%s: (SERVER) Not setting  audiodelay as port is not enabled: %d \n", __FUNCTION__,m_AudioPortEnabled[_APortType]);
            }
        }
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
        std::string _AudioDelay = std::to_string(param->audioDelayMs);
        switch(_APortType) {
            case dsAUDIOPORT_TYPE_SPDIF:
                INT_DEBUG("%s: port: %s , persist audio delay: %d\n",__func__,"SPDIF0", param->audioDelayMs);
                device::HostPersistence::getInstance().persistHostProperty("SPDIF0.audio.Delay",_AudioDelay);
                break;
            case dsAUDIOPORT_TYPE_HDMI:
                INT_DEBUG("%s: port: %s , persist audio delay: %d\n",__func__,"HDMI0", param->audioDelayMs);
                device::HostPersistence::getInstance().persistHostProperty("HDMI0.audio.Delay",_AudioDelay);
                break;
            case dsAUDIOPORT_TYPE_SPEAKER:
                INT_DEBUG("%s: port: %s , persist audio delay: %d\n",__func__,"SPEAKER0", param->audioDelayMs);
                device::HostPersistence::getInstance().persistHostProperty("SPEAKER0.audio.Delay",_AudioDelay);
                break;
            case dsAUDIOPORT_TYPE_HDMI_ARC:
                INT_DEBUG("%s: port: %s , persist audio delay: %d\n",__func__,"HDMI_ARC0", param->audioDelayMs);
                device::HostPersistence::getInstance().persistHostProperty("HDMI_ARC0.audio.Delay",_AudioDelay);
                break;
            default:
                INT_DEBUG("%s: port: UNKNOWN , persist audio delay: %d : NOT SET\n",__func__, param->audioDelayMs);
                break;
        }
#endif
    }
    else {
        result = IARM_RESULT_INVALID_STATE;
    }

    IARM_BUS_Unlock(lock);
    return result;

}

uint32_t dsGetAudioDelayInternal(dsAudioPortType_t _APortType)
{
       std::string audioDelayMs = "0";
       uint32_t returnAudioDelayMs = 0;
       switch(_APortType) {
            case dsAUDIOPORT_TYPE_SPDIF:
                {
                   try {
                        audioDelayMs = device::HostPersistence::getInstance().getProperty("SPDIF0.audio.Delay");
                    }
                    catch(...) {
                            try {
                                INT_DEBUG("SPDIF0.audio.Delay not found in persistence store. Try system default\n");
                                audioDelayMs = device::HostPersistence::getInstance().getDefaultProperty("SPDIF0.audio.Delay");
                            }
                            catch(...) {
                                audioDelayMs = "0";
                            }
                    }
                }
                break;
            case dsAUDIOPORT_TYPE_HDMI:
                {
                   try {
                        audioDelayMs = device::HostPersistence::getInstance().getProperty("HDMI0.audio.Delay");
                    }
                    catch(...) {
                            try {
                                INT_DEBUG("HDMI0.audio.Delay not found in persistence store. Try system default\n");
                                audioDelayMs = device::HostPersistence::getInstance().getDefaultProperty("HDMI0.audio.Delay");
                            }
                            catch(...) {
                                audioDelayMs = "0";
                            }
                    }
                }
                break;
            case dsAUDIOPORT_TYPE_SPEAKER:
                {
                   try {
                        audioDelayMs = device::HostPersistence::getInstance().getProperty("SPEAKER0.audio.Delay");
                    }
                    catch(...) {
                            try {
                                INT_DEBUG("SPEAKER0.audio.Delay not found in persistence store. Try system default\n");
                                audioDelayMs = device::HostPersistence::getInstance().getDefaultProperty("SPEAKER0.audio.Delay");
                            }
                            catch(...) {
                                audioDelayMs = "0";
                            }
                    }
                }
                break;
            case dsAUDIOPORT_TYPE_HDMI_ARC:
                {
                   try {
                        audioDelayMs = device::HostPersistence::getInstance().getProperty("HDMI_ARC0.audio.Delay");
                    }
                    catch(...) {
                            try {
                                INT_DEBUG("HDMI_ARC0.audio.Delay not found in persistence store. Try system default\n");
                                audioDelayMs = device::HostPersistence::getInstance().getDefaultProperty("HDMI_ARC0.audio.Delay");
                            }
                            catch(...) {
                                audioDelayMs = "0";
                            }
                    }
                }                break;
            default:
                INT_DEBUG("%s: port: UNKNOWN , persist audio delay: %s : NOT SET\n",__func__, audioDelayMs.c_str());
                break;
        }
    try {
        returnAudioDelayMs = std::stoul(audioDelayMs);
        INT_DEBUG("Audio delay value returnAudioDelayMs :%d \n",returnAudioDelayMs);
    }
    catch(...) {
        INT_INFO("%s: Exception in getting the audio delay from persistence storage, returning default value 0\n", __FUNCTION__);
        returnAudioDelayMs = 0;
    }
    return returnAudioDelayMs;
}


IARM_Result_t _dsGetAudioDelay(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    dsGetAudioDelayParam_t *param = (dsGetAudioDelayParam_t *)arg;
    if (param != NULL)
    {
       uint32_t audioDelayMs = 0;
       dsAudioPortType_t _APortType = _GetAudioPortType(param->handle);
       audioDelayMs = dsGetAudioDelayInternal(_APortType);
       param->audioDelayMs = audioDelayMs;
       INT_INFO("%s: (SERVER) getAudioDelay audioDelayMs: %d \n", __FUNCTION__,audioDelayMs);
       result = IARM_RESULT_SUCCESS;
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsGetAudioDelayOffset(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    typedef dsError_t (*dsGetAudioDelayOffset_t)(intptr_t handle, uint32_t *audioDelayOffsetMs);
    static dsGetAudioDelayOffset_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetAudioDelayOffset_t) dlsym(dllib, "dsGetAudioDelayOffset");
            if (func) {
                INT_DEBUG("dsGetAudioDelayOffset_t(int, uint32_t*) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetAudioDelayOffset_t(int, uint32_t*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsAudioDelayOffsetParam_t *param = (dsAudioDelayOffsetParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        uint32_t audioDelayOffsetMs = 0;
        param->audioDelayOffsetMs = 0;
        if (func(param->handle, &audioDelayOffsetMs) == dsERR_NONE)
        {
            param->audioDelayOffsetMs = audioDelayOffsetMs;
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsSetAudioAtmosOutputMode(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    typedef dsError_t (*dsSetAudioAtmosOutputMode_t)(intptr_t handle, bool enable);
    static dsSetAudioAtmosOutputMode_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetAudioAtmosOutputMode_t) dlsym(dllib, "dsSetAudioAtmosOutputMode");
            if (func) {
                INT_DEBUG("dsSetAudioAtmosOutputMode_t (intptr_t handle, bool enable ) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetAudioAtmosOutputMode_t (intptr_t handle, bool enable) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsAudioSetAtmosOutputModeParam_t *param = (dsAudioSetAtmosOutputModeParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        if(func(param->handle, param->enable) == dsERR_NONE) {
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsGetSinkDeviceAtmosCapability(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    typedef dsError_t (*dsGetSinkDeviceAtmosCapability_t)(intptr_t handle, dsATMOSCapability_t *capability);
    static dsGetSinkDeviceAtmosCapability_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetSinkDeviceAtmosCapability_t) dlsym(dllib, "dsGetSinkDeviceAtmosCapability");
            if (func) {
                INT_DEBUG("dsGetSinkDeviceAtmosCapability_t (intptr_t handle, dsATMOSCapability_t *capability ) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetSinkDeviceAtmosCapability_t (intptr_t handle, dsATMOSCapability_t *capability ) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsGetAudioAtmosCapabilityParam_t *param = (dsGetAudioAtmosCapabilityParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        dsATMOSCapability_t capability = dsAUDIO_ATMOS_NOTSUPPORTED;

        param->capability= dsAUDIO_ATMOS_NOTSUPPORTED;
        if (func(param->handle, &capability) == dsERR_NONE)
        {
            param->capability = capability;
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsSetAudioCompression(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSetAudioCompression_t)(intptr_t handle, int compressionLevel);
    static dsSetAudioCompression_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetAudioCompression_t) dlsym(dllib, "dsSetAudioCompression");
            if (func) {
                INT_DEBUG("dsSetAudioCompression_t(int, int ) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetAudioCompression_t(int, int) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsAudioCompressionParam_t *param = (dsAudioCompressionParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->compression) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            std::string _Compression = std::to_string(param->compression);
            INT_DEBUG("%s: persist audio compression: %d\n",__func__, param->compression);
            device::HostPersistence::getInstance().persistHostProperty("audio.Compression",_Compression);
#endif
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsGetAudioCompression(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetAudioCompression_t)(intptr_t handle, int *compressionLevel);
    static dsGetAudioCompression_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetAudioCompression_t) dlsym(dllib, "dsGetAudioCompression");
            if (func) {
                INT_DEBUG("dsGetAudioCompression_t(int, int *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetAudioCompression_t(int, int *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsAudioCompressionParam_t *param = (dsAudioCompressionParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        int compression = 0;
        param->compression= 0;
        if (func(param->handle, &compression) == dsERR_NONE)
        {
           param->compression = compression;
           result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsSetDialogEnhancement(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);
    dsDialogEnhancementParam_t *param = (dsDialogEnhancementParam_t *)arg;
    if(param != NULL)
        result = _setDialogEnhancement(param->handle, param->enhancerLevel);
    IARM_BUS_Unlock(lock);
    return result;
}

static IARM_Result_t _setDialogEnhancement(intptr_t handle, int enhancerLevel)
{
    typedef dsError_t (*dsSetDialogEnhancement_t)(intptr_t handle, int enhancerLevel);
    static dsSetDialogEnhancement_t func = 0;
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;

    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetDialogEnhancement_t) dlsym(dllib, "dsSetDialogEnhancement");
            if (func) {
                INT_DEBUG("dsSetDialogEnhancement_t(int, int) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetDialogEnhancement_t(int, int ) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    std::string _Property = _dsGetCurrentProfileProperty("EnhancerLevel");
    if (func != 0)
    {
        if (func(handle, enhancerLevel) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            std::string _EnhancerLevel = std::to_string(enhancerLevel);
            INT_DEBUG("%s: persist enhancer level: %d\n",__func__, enhancerLevel);
            device::HostPersistence::getInstance().persistHostProperty(_Property ,_EnhancerLevel);
#endif
            result = IARM_RESULT_SUCCESS;
        }
    }
    return result;
}


IARM_Result_t _dsGetDialogEnhancement(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetDialogEnhancement_t)(intptr_t handle, int *enhancerLevel);
    static dsGetDialogEnhancement_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetDialogEnhancement_t) dlsym(dllib, "dsGetDialogEnhancement");
            if (func) {
                INT_DEBUG("dsGetDialogEnhancement_t(int, int *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetDialogEnhancement_t(int, int *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsDialogEnhancementParam_t *param = (dsDialogEnhancementParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        int enhancerLevel = 0;
        param->enhancerLevel = 0;
        if (func(param->handle, &enhancerLevel) == dsERR_NONE)
        {
           param->enhancerLevel = enhancerLevel;
           result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsSetDolbyVolumeMode(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSetDolbyVolumeMode_t)(intptr_t handle, bool enable);
    static dsSetDolbyVolumeMode_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetDolbyVolumeMode_t) dlsym(dllib, "dsSetDolbyVolumeMode");
            if (func) {
                INT_DEBUG("dsSetDolbyVolumeMode_t(int, bool) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetDolbyVolumeMode_t(int, bool) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsSetDolbyVolumeParam_t *param = (dsSetDolbyVolumeParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->enable) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            INT_DEBUG("%s: persist dolby volume mode: %s\n",__func__, param->enable ? "TRUE":"FALSE");
            device::HostPersistence::getInstance().persistHostProperty("audio.DolbyVolumeMode",param->enable ? "TRUE":"FALSE");
#endif
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsGetDolbyVolumeMode(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetDolbyVolumeMode_t)(intptr_t handle, bool *enable);
    static dsGetDolbyVolumeMode_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetDolbyVolumeMode_t) dlsym(dllib, "dsGetDolbyVolumeMode");
            if (func) {
                INT_DEBUG("dsGetDolbyVolumeMode_t(int, bool *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetDolbyVolumeMode_t(int, bool *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsSetDolbyVolumeParam_t *param = (dsSetDolbyVolumeParam_t *)arg;
    bool enable = false;

    if (func != 0 && param != NULL)
    {
	param->enable = false;
        if (func(param->handle, &enable) == dsERR_NONE)
        {
            param->enable = enable;
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsSetIntelligentEqualizerMode(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSetIntelligentEqualizerMode_t)(intptr_t handle, int mode);
    static dsSetIntelligentEqualizerMode_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetIntelligentEqualizerMode_t) dlsym(dllib, "dsSetIntelligentEqualizerMode");
            if (func) {
                INT_DEBUG("dsSetIntelligentEqualizerMode_t(int, int) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetIntelligentEqualizerMode_t(int, int) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsIntelligentEqualizerModeParam_t *param = (dsIntelligentEqualizerModeParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->mode) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            std::string _IntelligentEQ = std::to_string(param->mode);
            INT_INFO("%s: persist intelligent equalizer value: %d\n",__func__, param->mode);
            device::HostPersistence::getInstance().persistHostProperty("audio.IntelligentEQ",_IntelligentEQ);
#endif
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsGetIntelligentEqualizerMode(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetIntelligentEqualizerMode_t)(intptr_t handle, int *mode);
    static dsGetIntelligentEqualizerMode_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetIntelligentEqualizerMode_t) dlsym(dllib, "dsGetIntelligentEqualizerMode");
            if (func) {
                INT_DEBUG("dsGetIntelligentEqualizerMode_t(int, int *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetIntelligentEqualizerMode_t(int, int *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsIntelligentEqualizerModeParam_t *param = (dsIntelligentEqualizerModeParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        int  mode = 0;
        param->mode = 0;
        if (func(param->handle, &mode) == dsERR_NONE)
        {
            param->mode = mode;
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsGetVolumeLeveller(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetVolumeLeveller_t)(intptr_t handle, dsVolumeLeveller_t *volLeveller);
    static dsGetVolumeLeveller_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetVolumeLeveller_t) dlsym(dllib, "dsGetVolumeLeveller");
            if (func) {
                INT_DEBUG("dsGetVolumeLeveller_t(int, dsVolumeLeveller_t *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetVolumeLeveller_t(int, dsVolumeLeveller_t *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsVolumeLevellerParam_t *param = (dsVolumeLevellerParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        dsVolumeLeveller_t volLeveller;
	volLeveller.mode = 0;
	volLeveller.level = 0;
	param->volLeveller.mode = 0;
        param->volLeveller.level = 0;
        if (func(param->handle, &volLeveller) == dsERR_NONE)
        {
	    param->volLeveller.mode = volLeveller.mode;
            param->volLeveller.level = volLeveller.level;
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsSetVolumeLeveller(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    dsVolumeLevellerParam_t *param = (dsVolumeLevellerParam_t *)arg;
    if( param != NULL )
       result = _setVolumeLeveller(param->handle, param->volLeveller.mode, param->volLeveller.level);
    IARM_BUS_Unlock(lock);
    return result;
}

static IARM_Result_t _setVolumeLeveller(intptr_t handle, int volLevellerMode, int volLevellerLevel)
{
    typedef dsError_t (*dsSetVolumeLeveller_t)(intptr_t handle, dsVolumeLeveller_t volLeveller);
    static dsSetVolumeLeveller_t func = 0;
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;

    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetVolumeLeveller_t) dlsym(dllib, "dsSetVolumeLeveller");
            if (func) {
                INT_DEBUG("dsSetVolumeLeveller_t(int, dsVolumeLeveller_t) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetVolumeLeveller_t(int, dsVolumeLeveller_t) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsVolumeLevellerParam_t param;

    param.handle = handle;
    param.volLeveller.mode = volLevellerMode;
    param.volLeveller.level = volLevellerLevel;

    if (func != 0 )
    {
        if (func(param.handle, param.volLeveller) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            std::string _PropertyMode = _dsGetCurrentProfileProperty("VolumeLeveller.mode");
            std::string _Propertylevel = _dsGetCurrentProfileProperty("VolumeLeveller.level");
            std::string _mode = std::to_string(param.volLeveller.mode);
            INT_DEBUG("%s: persist volume leveller mode: %d\n",__func__, param.volLeveller.mode);
            device::HostPersistence::getInstance().persistHostProperty(_PropertyMode,_mode);

	    if((param.volLeveller.mode == 0) || (param.volLeveller.mode == 1)) {
                std::string _level = std::to_string(param.volLeveller.level);
                INT_DEBUG("%s: persist volume leveller value: %d\n",__func__, param.volLeveller.level);
                device::HostPersistence::getInstance().persistHostProperty(_Propertylevel,_level);
	    }
#endif
            result = IARM_RESULT_SUCCESS;
        }
    }

    return result;
}


IARM_Result_t _dsGetBassEnhancer(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetBassEnhancer_t)(intptr_t handle, int *boost);
    static dsGetBassEnhancer_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetBassEnhancer_t) dlsym(dllib, "dsGetBassEnhancer");
            if (func) {
                INT_DEBUG("dsGetBassEnhancer_t(int, int *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetBassEnhancer_t(int, int *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsBassEnhancerParam_t *param = (dsBassEnhancerParam_t *)arg;
    int boost = 0;
    if (func != 0 && param != NULL)
    {
        param->boost = 0;  //CID:155155 - Rverse_inull
        if (func(param->handle, &boost) == dsERR_NONE)
        {
            param->boost = boost;
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsSetBassEnhancer(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);
    dsBassEnhancerParam_t *param = (dsBassEnhancerParam_t *)arg;
    if( param != NULL )
       result = _setBassEnhancer(param->handle, param->boost);
    IARM_BUS_Unlock(lock);
    return result;
}

static IARM_Result_t _setBassEnhancer(intptr_t handle ,int boost)
{
    typedef dsError_t (*dsSetBassEnhancer_t)(intptr_t handle, int boost);
    static dsSetBassEnhancer_t func = 0;
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;

    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetBassEnhancer_t) dlsym(dllib, "dsSetBassEnhancer");
            if (func) {
                INT_DEBUG("dsSetBassEnhancer_t(int, int) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetBassEnhancer_t(int, int) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    if (func != 0)
    {
        if (func(handle, boost) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            std::string _BassBoost = std::to_string(boost);
            INT_DEBUG("%s: persist boost value: %d\n",__func__, boost);
            device::HostPersistence::getInstance().persistHostProperty("audio.BassBoost", _BassBoost);
#endif
            result = IARM_RESULT_SUCCESS;
        }
    }

    return result;
}


IARM_Result_t _dsIsSurroundDecoderEnabled(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsIsSurroundDecoderEnabled_t)(intptr_t handle, bool *enabled);
    static dsIsSurroundDecoderEnabled_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsIsSurroundDecoderEnabled_t) dlsym(dllib, "dsIsSurroundDecoderEnabled");
            if (func) {
                INT_DEBUG("dsIsSurroundDecoderEnabled_t(int, bool *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsIsSurroundDecoderEnabled_t(int, bool *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsSurroundDecoderParam_t *param = (dsSurroundDecoderParam_t *)arg;
    bool enable = false;

    if (func != 0 && param != NULL)
    {
        param->enable = false;   //CID:155170 - Reverse_inull
        if (func(param->handle, &enable) == dsERR_NONE)
        {
            param->enable = enable;
            result = IARM_RESULT_SUCCESS;

        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsEnableSurroundDecoder(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsEnableSurroundDecoder_t)(intptr_t handle, bool enabled);
    static dsEnableSurroundDecoder_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsEnableSurroundDecoder_t) dlsym(dllib, "dsEnableSurroundDecoder");
            if (func) {
                INT_DEBUG("dsEnableSurroundDecoder_t(int, bool) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsEnableSurroundDecoder_t(int, bool) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsSurroundDecoderParam_t *param = (dsSurroundDecoderParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->enable) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            INT_INFO("%s: persist surround decoder value: %s\n",__func__, param->enable ? "TRUE":"FALSE");
            device::HostPersistence::getInstance().persistHostProperty("audio.SurroundDecoderEnabled",param->enable ? "TRUE":"FALSE");
#endif
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsGetDRCMode(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetDRCMode_t)(intptr_t handle, int *mode);
    static dsGetDRCMode_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetDRCMode_t) dlsym(dllib, "dsGetDRCMode");
            if (func) {
                INT_DEBUG("dsGetDRCMode_t(int, int *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetDRCMode_t(int, int *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsDRCModeParam_t *param = (dsDRCModeParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        int mode = 0;
        param->mode = 0;
        if (func(param->handle, &mode) == dsERR_NONE)
        {
            param->mode = mode;
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsSetDRCMode(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSetDRCMode_t)(intptr_t handle, int mode);
    static dsSetDRCMode_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetDRCMode_t) dlsym(dllib, "dsSetDRCMode");
            if (func) {
                INT_DEBUG("dsSetDRCMode_t(int, int) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetDRCMode_t(int, int) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsDRCModeParam_t *param = (dsDRCModeParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->mode) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            INT_INFO("%s: persist DRC Mode value: %s\n",__func__, param->mode ? "RF":"Line");
            device::HostPersistence::getInstance().persistHostProperty("audio.DRCMode",param->mode ? "RF":"Line");
#endif
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsGetSurroundVirtualizer(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetSurroundVirtualizer_t)(intptr_t handle, dsSurroundVirtualizer_t *virtualizer);
    static dsGetSurroundVirtualizer_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetSurroundVirtualizer_t) dlsym(dllib, "dsGetSurroundVirtualizer");
            if (func) {
                INT_DEBUG("dsGetSurroundVirtualizer_t(int, dsSurroundVirtualizer_t *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetSurroundVirtualizer_t(int, dsSurroundVirtualizer_t *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsSurroundVirtualizerParam_t *param = (dsSurroundVirtualizerParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        dsSurroundVirtualizer_t virtualizer;
	virtualizer.mode = 0;
	virtualizer.boost = 0;	
        param->virtualizer.mode = 0;
	param->virtualizer.boost = 0;
        if (func(param->handle, &virtualizer) == dsERR_NONE)
        {
            param->virtualizer.mode = virtualizer.mode;
	    param->virtualizer.boost = virtualizer.boost;
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsSetSurroundVirtualizer(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    dsSurroundVirtualizerParam_t *param = (dsSurroundVirtualizerParam_t *)arg;
    if( param != NULL )
       result = _setSurroundVirtualizer(param->handle, param->virtualizer.mode, param->virtualizer.boost);
    IARM_BUS_Unlock(lock);
    return result;
}

static IARM_Result_t _setSurroundVirtualizer(intptr_t handle , int virtualizerMode , int virtualizerBoost)
{
    typedef dsError_t (*dsSetSurroundVirtualizer_t)(intptr_t handle, dsSurroundVirtualizer_t virtualizer);
    static dsSetSurroundVirtualizer_t func = 0;
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;

    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetSurroundVirtualizer_t) dlsym(dllib, "dsSetSurroundVirtualizer");
            if (func) {
                INT_DEBUG("dsSetSurroundVirtualizer_t(int, dsSurroundVirtualizer_t) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetSurroundVirtualizer_t(int, dsSurroundVirtualizer_t) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsSurroundVirtualizerParam_t param;
    param.handle = handle;
    param.virtualizer.mode = virtualizerMode;
    param.virtualizer.boost = virtualizerBoost;

    if (func != 0)
    {
        if (func(param.handle, param.virtualizer) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            std::string _mode = std::to_string(param.virtualizer.mode);
            INT_INFO("%s: persist surround virtualizer mode: %d\n",__func__, param.virtualizer.mode);
            std::string _PropertyMode = _dsGetCurrentProfileProperty("SurroundVirtualizer.mode");
            std::string _Propertylevel = _dsGetCurrentProfileProperty("SurroundVirtualizer.boost"); 
            device::HostPersistence::getInstance().persistHostProperty(_PropertyMode,_mode);

            if(((param.virtualizer.mode) >= 0) && ((param.virtualizer.mode) <= 2)) {
                std::string _boost = std::to_string(param.virtualizer.boost);
                INT_INFO("%s: persist surround virtualizer boost value: %d\n",__func__, param.virtualizer.boost);
                device::HostPersistence::getInstance().persistHostProperty(_Propertylevel,_boost);
            }
#endif
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsGetMISteering(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetMISteering_t)(intptr_t handle, bool *enabled);
    static dsGetMISteering_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetMISteering_t) dlsym(dllib, "dsGetMISteering");
            if (func) {
                INT_DEBUG("dsGetMISteering_t(int, bool *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetMISteering_t(int, bool *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsMISteeringParam_t *param = (dsMISteeringParam_t *)arg;
    bool enable = false;
    if (func != 0 && param != NULL)
    {
        param->enable = false;  //CID:155153 - Reverse_inull
        if (func(param->handle, &enable) == dsERR_NONE)
        {
            param->enable = enable;
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsSetMISteering(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSetMISteering_t)(intptr_t handle, bool enabled);
    static dsSetMISteering_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetMISteering_t) dlsym(dllib, "dsSetMISteering");
            if (func) {
                INT_DEBUG("dsSetMISteering_t(int, bool) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetMISteering_t(int, bool) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsMISteeringParam_t *param = (dsMISteeringParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->enable) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            INT_INFO("%s: persist MISteering value: %s\n", __func__, param->enable ? "Enabled":"Disabled");
            device::HostPersistence::getInstance().persistHostProperty("audio.MISteering",param->enable ? "Enabled":"Disabled");
#endif
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsSetGraphicEqualizerMode(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSetGraphicEqualizerMode_t)(intptr_t handle, int mode);
    static dsSetGraphicEqualizerMode_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetGraphicEqualizerMode_t) dlsym(dllib, "dsSetGraphicEqualizerMode");
            if (func) {
                INT_DEBUG("dsSetGraphicEqualizerMode_t(int, int) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetGraphicEqualizerMode_t(int, int) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsGraphicEqualizerModeParam_t *param = (dsGraphicEqualizerModeParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->mode) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            std::string _GraphicEQ = std::to_string(param->mode);
            INT_INFO("%s: persist graphic equalizer value: %d\n",__func__, param->mode);
            device::HostPersistence::getInstance().persistHostProperty("audio.GraphicEQ",_GraphicEQ);
#endif
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsGetGraphicEqualizerMode(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetGraphicEqualizerMode_t)(intptr_t handle, int *mode);
    static dsGetGraphicEqualizerMode_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetGraphicEqualizerMode_t) dlsym(dllib, "dsGetGraphicEqualizerMode");
            if (func) {
                INT_DEBUG("dsGetGraphicEqualizerMode_t(int, int *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetGraphicEqualizerMode_t(int, int *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsGraphicEqualizerModeParam_t *param = (dsGraphicEqualizerModeParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        int  mode = 0;
        param->mode = 0;
        if (func(param->handle, &mode) == dsERR_NONE)
        {
            param->mode = mode;
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsGetMS12AudioProfileList(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetMS12AudioProfileList_t)(intptr_t handle, dsMS12AudioProfileList_t* profiles);
    static dsGetMS12AudioProfileList_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetMS12AudioProfileList_t) dlsym(dllib, "dsGetMS12AudioProfileList");
            if (func) {
                INT_DEBUG("dsGetMS12AudioProfileList_t(int, dsMS12AudioProfileList_t*) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetMS12AudioProfileList_t(int, dsMS12AudioProfileList_t*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsMS12AudioProfileListParam_t *param = (dsMS12AudioProfileListParam_t *)arg;
    dsMS12AudioProfileList_t pList;
    dsError_t ret = dsERR_NONE;
    if (func != 0 && param != NULL)
    {
	ret = func(param->handle, &pList);
        if (ret == dsERR_NONE)
        {
	    INT_INFO("%s: Total number of supported profiles: %d\n",__FUNCTION__, pList.audioProfileCount);
	    INT_INFO("%s: Profile List: %s\n",__FUNCTION__, pList.audioProfileList);
	    param->profileList.audioProfileCount = pList.audioProfileCount;
	    strncpy(param->profileList.audioProfileList,pList.audioProfileList,MAX_PROFILE_LIST_BUFFER_LEN);
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsGetMS12AudioProfile(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetMS12AudioProfile_t)(intptr_t handle, char* profile);
    static dsGetMS12AudioProfile_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetMS12AudioProfile_t) dlsym(dllib, "dsGetMS12AudioProfile");
            if (func) {
                INT_DEBUG("dsGetMS12AudioProfile_t(int, char* ) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetMS12AudioProfile_t(int, char*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsMS12AudioProfileParam_t *param = (dsMS12AudioProfileParam_t *)arg;
    char m_profile[MAX_PROFILE_STRING_LEN] = {0};
    if (func != 0 && param != NULL)
    {
        if (func(param->handle, m_profile) == dsERR_NONE)
        {
            strncpy(param->profile, m_profile, MAX_PROFILE_STRING_LEN);
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsSetMS12AudioProfile(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSetMS12AudioProfile_t)(intptr_t handle, const char* profile);
    static dsSetMS12AudioProfile_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetMS12AudioProfile_t) dlsym(dllib, "dsSetMS12AudioProfile");
            if (func) {
                INT_DEBUG("dsSetMS12AudioProfile_t(int, const char*) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetMS12AudioProfile_t(int, const char*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsMS12AudioProfileParam_t *param = (dsMS12AudioProfileParam_t*)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->profile) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            INT_INFO("%s: persist MS12 Audio Profile selection : %s\n", __func__, param->profile);
            device::HostPersistence::getInstance().persistHostProperty("audio.MS12Profile",param->profile);
#endif
            result = IARM_RESULT_SUCCESS;
        }

        if(strcmp(param->profile,"Off")) {
        /* override the user changed MS12 Audio Profile settings if its not OFF profile */
        INT_INFO("%s: override user changed MS12 Audio Profile settings : %s\n", __func__, param->profile);
        _dsMS12ProfileSettingOverride(param->handle);
        }
    }
    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsSetAssociatedAudioMixing(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSetAssociatedAudioMixing_t)(intptr_t handle, bool mixing);
    static dsSetAssociatedAudioMixing_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetAssociatedAudioMixing_t) dlsym(dllib, "dsSetAssociatedAudioMixing");
            if (func) {
                INT_DEBUG("dsSetAssociatedAudioMixing_t(int, bool) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetMS12AudioProfile_t(int, bool) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsAssociatedAudioMixingParam_t *param = (dsAssociatedAudioMixingParam_t*)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->mixing) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            INT_INFO("%s: persist Associated Audio Mixing status : %s\n", __func__, param->mixing ? "Enabled":"Disabled");
            device::HostPersistence::getInstance().persistHostProperty("audio.AssociatedAudioMixing",param->mixing ? "Enabled":"Disabled");
#endif
            IARM_Bus_DSMgr_EventData_t associated_audio_mixing_event_data;
            INT_INFO("%s: Associated Audio Mixing status changed :%d \r\n", __FUNCTION__, param->mixing);
            associated_audio_mixing_event_data.data.AssociatedAudioMixingInfo.mixing = param->mixing;

            IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                                   (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_AUDIO_ASSOCIATED_AUDIO_MIXING_CHANGED,
                                   (void *)&associated_audio_mixing_event_data,
                                   sizeof(associated_audio_mixing_event_data));

            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsGetAssociatedAudioMixing(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetAssociatedAudioMixing_t)(intptr_t handle, bool *mixing);
    static dsGetAssociatedAudioMixing_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetAssociatedAudioMixing_t) dlsym(dllib, "dsGetAssociatedAudioMixing");
            if (func) {
                INT_DEBUG("dsGetAssociatedAudioMixing_t(int, bool *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetAssociatedAudioMixing_t(int, bool *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsAssociatedAudioMixingParam_t *param = (dsAssociatedAudioMixingParam_t *)arg;
    bool mixing = false;
    if (func != 0 && param != NULL)
    {
        param->mixing = false;
        if (func(param->handle, &mixing) == dsERR_NONE)
        {
            param->mixing = mixing;
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsSetFaderControl(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSetFaderControl_t)(intptr_t handle, int mixerbalance);
    static dsSetFaderControl_t func = 0;

    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetFaderControl_t) dlsym(dllib, "dsSetFaderControl");
            if (func) {
                INT_DEBUG("dsSetFaderControl_t(int, int) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetFaderControl_t(int, int) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsFaderControlParam_t *param = (dsFaderControlParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->mixerbalance) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            std::string _mixerbalance = std::to_string(param->mixerbalance);
            INT_INFO("%s: persist fader control level: %d\n",__func__, param->mixerbalance);
            device::HostPersistence::getInstance().persistHostProperty("audio.FaderControl",_mixerbalance);
#endif
            IARM_Bus_DSMgr_EventData_t fader_control_event_data;
            INT_INFO("%s: Fader Control changed :%d \r\n", __FUNCTION__, param->mixerbalance);
            fader_control_event_data.data.FaderControlInfo.mixerbalance = param->mixerbalance;

            IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                                   (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_AUDIO_FADER_CONTROL_CHANGED,
                                   (void *)&fader_control_event_data,
                                   sizeof(fader_control_event_data));

            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}




IARM_Result_t _dsGetFaderControl(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetFaderControl_t)(intptr_t handle, int *mixerbalance);
    static dsGetFaderControl_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetFaderControl_t) dlsym(dllib, "dsGetFaderControl");
            if (func) {
                INT_DEBUG("dsGetFaderControl_t(int, int *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetFaderControl_t(int, int *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsFaderControlParam_t *param = (dsFaderControlParam_t *)arg;
    int mixerbalance = 0;
    if (func != 0 && param != NULL)
    {
        param->mixerbalance = 0;
        if (func(param->handle, &mixerbalance) == dsERR_NONE)
        {
            param->mixerbalance = mixerbalance;
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsSetPrimaryLanguage(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSetPrimaryLanguage_t)(intptr_t handle, const char* pLang);
    static dsSetPrimaryLanguage_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetPrimaryLanguage_t) dlsym(dllib, "dsSetPrimaryLanguage");
            if (func) {
                INT_DEBUG("dsSetPrimaryLanguage_t(int, const char*) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetPrimaryLanguage_t(int, const char*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsPrimaryLanguageParam_t *param = (dsPrimaryLanguageParam_t*)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->primaryLanguage) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            INT_INFO("%s: persist Primary Language : %s\n", __func__, param->primaryLanguage);
            device::HostPersistence::getInstance().persistHostProperty("audio.PrimaryLanguage",param->primaryLanguage);
#endif
            IARM_Bus_DSMgr_EventData_t primary_language_event_data;
            INT_INFO("%s: Primary Language changed :%s \r\n", __FUNCTION__, param->primaryLanguage);
	    memset(primary_language_event_data.data.AudioLanguageInfo.audioLanguage,'\0',MAX_LANGUAGE_LEN);
            strncpy(primary_language_event_data.data.AudioLanguageInfo.audioLanguage, param->primaryLanguage, MAX_LANGUAGE_LEN-1);
 
            IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                                   (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_AUDIO_PRIMARY_LANGUAGE_CHANGED,
                                   (void *)&primary_language_event_data,
                                   sizeof(primary_language_event_data));

            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsGetPrimaryLanguage(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetPrimaryLanguage_t)(intptr_t handle, char* pLang);
    static dsGetPrimaryLanguage_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetPrimaryLanguage_t) dlsym(dllib, "dsGetPrimaryLanguage");
            if (func) {
                INT_DEBUG("dsGetPrimaryLanguage_t(int, char* ) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetPrimaryLanguage_t(int, char*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsPrimaryLanguageParam_t *param = (dsPrimaryLanguageParam_t *)arg;
    char primaryLanguage[MAX_LANGUAGE_LEN] = {0};
    if (func != 0 && param != NULL)
    {
        if (func(param->handle, primaryLanguage) == dsERR_NONE)
        {
            strncpy(param->primaryLanguage, primaryLanguage, MAX_LANGUAGE_LEN);
            
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsSetSecondaryLanguage(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsSetSecondaryLanguage_t)(intptr_t handle, const char* sLang);
    static dsSetSecondaryLanguage_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetSecondaryLanguage_t) dlsym(dllib, "dsSetSecondaryLanguage");
            if (func) {
                INT_DEBUG("dsSetSecondaryLanguage_t(int, const char*) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetSecondaryLanguage_t(int, const char*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsSecondaryLanguageParam_t *param = (dsSecondaryLanguageParam_t*)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->secondaryLanguage) == dsERR_NONE)
        {
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
            INT_INFO("%s: persist Secondary Language : %s\n", __func__, param->secondaryLanguage);
            device::HostPersistence::getInstance().persistHostProperty("audio.SecondaryLanguage",param->secondaryLanguage);
#endif
            IARM_Bus_DSMgr_EventData_t secondary_language_event_data;
            INT_INFO("%s: Secondary Language changed :%s \r\n", __FUNCTION__, param->secondaryLanguage);
	    memset(secondary_language_event_data.data.AudioLanguageInfo.audioLanguage,'\0',MAX_LANGUAGE_LEN);
            strncpy(secondary_language_event_data.data.AudioLanguageInfo.audioLanguage, param->secondaryLanguage, MAX_LANGUAGE_LEN-1);

            IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                                   (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_AUDIO_SECONDARY_LANGUAGE_CHANGED,
                                   (void *)&secondary_language_event_data,
                                   sizeof(secondary_language_event_data));

            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsGetSecondaryLanguage(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetSecondaryLanguage_t)(intptr_t handle, char* sLang);
    static dsGetSecondaryLanguage_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetSecondaryLanguage_t) dlsym(dllib, "dsGetSecondaryLanguage");
            if (func) {
                INT_DEBUG("dsGetSecondaryLanguage_t(int, char* ) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetSecondaryLanguage_t(int, char*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsSecondaryLanguageParam_t *param = (dsSecondaryLanguageParam_t *)arg;
    char secondaryLanguage[MAX_LANGUAGE_LEN] = {0};
    if (func != 0 && param != NULL)
    {
        if (func(param->handle, secondaryLanguage) == dsERR_NONE)
        {
            strncpy(param->secondaryLanguage, secondaryLanguage, MAX_LANGUAGE_LEN);
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsSetMS12SetttingsOverride(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);
    dsMS12SetttingsOverrideParam_t *param = (dsMS12SetttingsOverrideParam_t*)arg;
    std::string _hostProperty;
    std::string _value;
    std::string _AProfile("Off");
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
    if(!(_dsMs12ProfileSupported(param->handle,param->profileName))) {
         INT_INFO("%s: Unknow MS12 profile %s \n",__func__, param->profileName);
         IARM_BUS_Unlock(lock);
         return result;
    }
    try {
          _AProfile = device::HostPersistence::getInstance().getProperty("audio.MS12Profile");
    }
    catch(...) {
         try {
             INT_DEBUG("audio.MS12Profile not found in persistence store. Try system default\n");
            _AProfile = device::HostPersistence::getInstance().getDefaultProperty("audio.MS12Profile");
        }
        catch(...) {
            _AProfile = "Off";
        }
    }

    if(strcmp(param->profileName,_AProfile.c_str()) == 0) {
         if(strcmp(param->profileSettingsName, "DialogEnhance") == 0) {
            if(strcmp(param->profileState, "ADD") == 0) {
               result = _setDialogEnhancement(param->handle,atoi(param->profileSettingValue));
            }
            else if(strcmp(param->profileState, "REMOVE") == 0) {
               result =  _resetDialogEnhancerLevel(param->handle);
            }
         }
         else if(strcmp(param->profileSettingsName, "VolumeLevellerMode") == 0) {
            std::string _PropertyMode = _dsGetCurrentProfileProperty("VolumeLeveller.mode");
            if((atoi(param->profileSettingValue) == 0) || (atoi(param->profileSettingValue) == 1)) {
                device::HostPersistence::getInstance().persistHostProperty(_PropertyMode,param->profileSettingValue);
                result = IARM_RESULT_SUCCESS;
            }else {
                INT_INFO("%s: Unknow MS12 property value %s %s \n",__func__, param->profileSettingsName,param->profileSettingValue);
                result = IARM_RESULT_INVALID_STATE;
            }
         }
         else if(strcmp(param->profileSettingsName, "VolumeLevellerLevel") == 0) {
            if(strcmp(param->profileState, "ADD") == 0) {
               std::string _volLevellerMode("0");
               std::string _PropertyMode = _dsGetCurrentProfileProperty("VolumeLeveller.mode");
               _volLevellerMode = device::HostPersistence::getInstance().getProperty(_PropertyMode);
               result = _setVolumeLeveller(param->handle,atoi(_volLevellerMode.c_str()),atoi(param->profileSettingValue));
            }
            else if(strcmp(param->profileState, "REMOVE") == 0) {
               result = _resetVolumeLeveller(param->handle);
            }
         }
         else if(strcmp(param->profileSettingsName, "BassEnhancer") == 0) {
            if(strcmp(param->profileState, "ADD") == 0) {
               result = _setBassEnhancer(param->handle,atoi(param->profileSettingValue));
            }
            else if(strcmp(param->profileState, "REMOVE") == 0) {
               result = _resetBassEnhancer(param->handle);
            }
         }
         else if(strcmp(param->profileSettingsName, "SurroundVirtualizerMode") == 0) {
             std::string _PropertyMode = _dsGetCurrentProfileProperty("SurroundVirtualizer.mode");
             if((atoi(param->profileSettingValue) >= 0) && (atoi(param->profileSettingValue) <= 2)) {
                device::HostPersistence::getInstance().persistHostProperty(_PropertyMode,param->profileSettingValue);
                result = IARM_RESULT_SUCCESS;
             }
             else {
                INT_INFO("%s: Unknow MS12 property value %s %s \n",__func__, param->profileSettingsName,param->profileSettingValue);
                result = IARM_RESULT_INVALID_STATE;
             }
         }
         else if(strcmp(param->profileSettingsName, "SurroundVirtualizerLevel") == 0) {
            if(strcmp(param->profileState, "ADD") == 0) {
               std::string _SVMode("0");
               std::string _PropertyMode = _dsGetCurrentProfileProperty("SurroundVirtualizer.mode");
               _SVMode = device::HostPersistence::getInstance().getProperty(_PropertyMode);
               result = _setSurroundVirtualizer(param->handle,atoi(_SVMode.c_str()),atoi(param->profileSettingValue));
            }
            else if(strcmp(param->profileState, "REMOVE") == 0) {
               result = _resetSurroundVirtualizer(param->handle);
            }
         }
         else {
           INT_INFO("%s: Unknow MS12 property %s \n",__func__, param->profileSettingsName);
           result = IARM_RESULT_INVALID_STATE;
         }
         if(result != IARM_RESULT_SUCCESS)
         {
            IARM_BUS_Unlock(lock);
            return result;
         }
    }
    else {
       if(strcmp(param->profileSettingsName, "DialogEnhance") == 0) {
          _hostProperty = _dsGenerateProfileProperty(param->profileName,"EnhancerLevel");
       }
       else if(strcmp(param->profileSettingsName, "VolumeLevellerMode") == 0) {
          _hostProperty = _dsGenerateProfileProperty(param->profileName,"VolumeLeveller.mode");
       }
       else if(strcmp(param->profileSettingsName, "VolumeLevellerLevel") == 0) {
          _hostProperty = _dsGenerateProfileProperty(param->profileName,"VolumeLeveller.level");
       }
       else if(strcmp(param->profileSettingsName, "BassEnhancer") == 0) {
          _hostProperty = "audio.BassBoost";
       }
       else if(strcmp(param->profileSettingsName, "SurroundVirtualizerMode") == 0) {
          _hostProperty = _dsGenerateProfileProperty(param->profileName,"SurroundVirtualizer.mode");
       }
       else if(strcmp(param->profileSettingsName, "SurroundVirtualizerLevel") == 0) {
          _hostProperty = _dsGenerateProfileProperty(param->profileName,"SurroundVirtualizer.boost");
       }
       else {
          INT_INFO("%s: Unknow MS12 property %s \n",__func__, param->profileSettingsName);
          IARM_BUS_Unlock(lock);
          return result;
       }
       if(strcmp(param->profileState, "ADD") == 0) {
          INT_INFO("%s: Profile %s property %s persist value: %s\n",__func__,param->profileName, param->profileSettingsName , param->profileSettingValue);
          device::HostPersistence::getInstance().persistHostProperty(_hostProperty ,param->profileSettingValue);
       }
       else if(strcmp(param->profileState, "REMOVE") == 0) {
          try {
               _value = device::HostPersistence::getInstance().getDefaultProperty(_hostProperty);
          }
          catch(...) {
              _value = "0";
          }
          INT_INFO("%s: Profile %s property %s persist value: %s\n",__func__,param->profileName, param->profileSettingsName , _value.c_str());
          device::HostPersistence::getInstance().persistHostProperty(_hostProperty,_value);
       }
       else {
          INT_INFO("%s: Unknow State %s \n",__func__, param->profileState);
          IARM_BUS_Unlock(lock);
          return result;
       }
    }
    result = IARM_RESULT_SUCCESS;
#endif
    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsGetSupportedARCTypes(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsGetSupportedARCTypes_t)(intptr_t handle, int *types);
    static dsGetSupportedARCTypes_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetSupportedARCTypes_t) dlsym(dllib, "dsGetSupportedARCTypes");
            if (func) {
                INT_DEBUG("dsGetSupportedARCTypes_t(int, int*) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetSupportedARCTypes_t(int, int*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsGetSupportedARCTypesParam_t *param = (dsGetSupportedARCTypesParam_t *)arg;
    int types = dsAUDIOARCSUPPORT_NONE;

    if (func != 0 && param != NULL)
    {
        param->types = dsAUDIOARCSUPPORT_NONE;   //CID:163840 - Reverse_inull
        if (func(param->handle, &types) == dsERR_NONE)
        {
            param->types = types;
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}


IARM_Result_t _dsAudioSetSAD(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    typedef dsError_t (*dsAudioSetSAD_t)(intptr_t handle, dsAudioSADList_t sad_list);
    static dsAudioSetSAD_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsAudioSetSAD_t) dlsym(dllib, "dsAudioSetSAD");
            if (func) {
                INT_DEBUG("dsAudioSetSAD_t(int, dsAudioSADList_t) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsAudioSetSAD_t(int, dsAudioSADList_t) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsAudioSetSADParam_t *param = (dsAudioSetSADParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->list) == dsERR_NONE)
        {
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsAudioEnableARC(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);

    dsError_t ret = dsERR_GENERAL;
    std::string _isEnabledAudoARCPortKey("audio.hdmiArc0.isEnabled");
    std::string _audoARCPortTypeKey("audio.hdmiArc0.type");
    //Default is eARC
    std::string _audoARCPortiCapVal("eARC");


    typedef dsError_t (*dsAudioEnableARC_t)(intptr_t handle, dsAudioARCStatus_t arcStatus);
    static dsAudioEnableARC_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsAudioEnableARC_t) dlsym(dllib, "dsAudioEnableARC");
            if (func) {
                INT_DEBUG("dsAudioEnableARC_t(int, dsAudioARCStatus_t) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsAudioEnableARC_t(int, dsAudioARCStatus_t) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsAudioEnableARCParam_t *param = (dsAudioEnableARCParam_t *)arg;

    if (func != 0 && param != NULL)
    {
        if (func(param->handle, param->arcStatus) == dsERR_NONE)
        {
            result = IARM_RESULT_SUCCESS;
        }



        /*Ensure settings is enabled properly in HAL*/
        ret = dsERR_NONE;
        bool bAudioPortEnableVerify = false;
        ret = (dsError_t) dsIsAudioPortEnabled (param->handle, &bAudioPortEnableVerify);
        if(dsERR_NONE == ret) {
            if (bAudioPortEnableVerify != param->arcStatus.status) {
                INT_INFO("Init: %s : %s Audio port status:%s verification failed. bAudioPortEnable: %d bAudioPortEnableVerify:%d\n",
                        __FUNCTION__, _audoARCPortTypeKey.c_str(), _isEnabledAudoARCPortKey.c_str(), param->arcStatus.status, bAudioPortEnableVerify);
            }
            else {
                INT_INFO("%s : %s Audio port status verification passed. status %d\n", 
                       __FUNCTION__, _isEnabledAudoARCPortKey.c_str(), param->arcStatus.status); 
            }
        }
        else {
            INT_INFO("Init: %s : %s Audio port status:%s verification step: dsIsAudioPortEnabled call failed\n", 
                    __FUNCTION__, _audoARCPortTypeKey.c_str(), _isEnabledAudoARCPortKey.c_str());
        }

    }

    IARM_BUS_Unlock(lock);
    return result;
}

IARM_Result_t _dsEnableLEConfig(void *arg)
{

#ifndef RDK_DSHAL_NAME
    #warning   "RDK_DSHAL_NAME is not defined"
    #define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;

    IARM_BUS_Lock(lock);

    typedef dsError_t  (*dsEnableLEConfig_t)(intptr_t handle,const bool enable);
    static dsEnableLEConfig_t func = NULL;
    if (func == NULL) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsEnableLEConfig_t) dlsym(dllib, "dsEnableLEConfig");
            if (func) {
                INT_DEBUG("dsEnableLEConfig(int, bool) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsEnableLEConfig(int, bool) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    _dsLEConfigParam_t *param = (_dsLEConfigParam_t *)arg;
    if (func != NULL) {
        INT_DEBUG("LE: %s  enable status:%d \r\n",__FUNCTION__,param->enable);

        if(param->enable != m_LEEnabled)
        {
            m_LEEnabled = param->enable;
            //Persist DAPV2 setting
            if(m_LEEnabled)
                device::HostPersistence::getInstance().persistHostProperty("audio.LEEnable","TRUE");
            else
                device::HostPersistence::getInstance().persistHostProperty("audio.LEEnable","FALSE");

            dsError_t ret = func(param->handle, param->enable);
            if (ret == dsERR_NONE) {
                result = IARM_RESULT_SUCCESS;
            }
        }
        else
        {
            INT_INFO("LE: %s Current enable status is same as requested:%d \r\n",__FUNCTION__,param->enable);
            result = IARM_RESULT_SUCCESS;
        }
    }

    IARM_BUS_Unlock(lock);

    return result;
}

IARM_Result_t _dsGetLEConfig(void *arg)
{

#ifndef RDK_DSHAL_NAME
    #warning   "RDK_DSHAL_NAME is not defined"
    #define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);

    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    typedef dsError_t  (*dsGetLEConfig_t)(intptr_t handle, bool *enable);
    static dsGetLEConfig_t func = NULL;
    if (func == NULL) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetLEConfig_t) dlsym(dllib, "dsGetLEConfig");
            if (func) {
                INT_DEBUG("dsGetLEConfig(int , bool *) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetLEConfig(int , bool *) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsGetLEConfigParam_t *param = (dsGetLEConfigParam_t*) arg;
    if (param != NULL) {
        param->result = dsERR_GENERAL;

        if (func != NULL) {
            param->result = func(param->handle, &param->enable);
            if(param->result == dsERR_NONE) {
                result = IARM_RESULT_SUCCESS;
            }
        }
    }


    IARM_BUS_Unlock(lock);

    return result;

}

static void _GetAudioModeFromPersistent(void *arg)
{
    _DEBUG_ENTER();

    dsAudioSetStereoModeParam_t *param = (dsAudioSetStereoModeParam_t *)arg;

    if (param != NULL)
    {
        dsAudioPortType_t _APortType = _GetAudioPortType(param->handle);
        std::string _AudioModeSettings("STEREO");

        if (_APortType == dsAUDIOPORT_TYPE_SPDIF)
        {   
           _AudioModeSettings = device::HostPersistence::getInstance().getProperty("SPDIF0.AudioMode",_AudioModeSettings);
           INT_INFO("The SPDIF Audio Mode Setting From Persistent is %s \r\n",_AudioModeSettings.c_str());
        }
        else if (_APortType == dsAUDIOPORT_TYPE_HDMI) {
            try{
                _AudioModeSettings = device::HostPersistence::getInstance().getProperty("HDMI0.AudioMode");
            }
            catch(...){
                  try{
                      _AudioModeSettings = device::HostPersistence::getInstance().getDefaultProperty("HDMI0.AudioMode");
                  }
                  catch(...){
                      INT_INFO("HDMI0.AudioMode Not in persistent");
                  }
            }
            INT_INFO("The HDMI Audio Mode Setting From Persistent is %s \r\n",_AudioModeSettings.c_str());
        }
	else if (_APortType == dsAUDIOPORT_TYPE_HDMI_ARC){
	    _AudioModeSettings = device::HostPersistence::getInstance().getProperty("HDMI_ARC0.AudioMode",_AudioModeSettings);
	    INT_INFO("The HDMI_ARC Audio Mode Setting From Persistent is %s \r\n",_AudioModeSettings.c_str());
	}

        if (_AudioModeSettings.compare("SURROUND") == 0)
        {
            param->mode = dsAUDIO_STEREO_SURROUND;
        }
        else if (_AudioModeSettings.compare("PASSTHRU") == 0)
        {
            param->mode = dsAUDIO_STEREO_PASSTHRU;
        }
        else if (_AudioModeSettings.compare("DOLBYDIGITAL") == 0)
        {
            param->mode = dsAUDIO_STEREO_DD;
        }
        else if (_AudioModeSettings.compare("DOLBYDIGITALPLUS") == 0)
        {
            param->mode = dsAUDIO_STEREO_DDPLUS;
        }
        else
        {
            param->mode = dsAUDIO_STEREO_STEREO;
        } 
    }
}

IARM_Result_t _dsGetAudioCapabilities(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);
    
    typedef dsError_t (*dsGetAudioCapabilitiesFunc_t)(intptr_t handle, int *capabilities);
    static dsGetAudioCapabilitiesFunc_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetAudioCapabilitiesFunc_t)dlsym(dllib, "dsGetAudioCapabilities");
            if (func) {
                INT_DEBUG("dsGetAudioCapabilities() is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetAudioCapabilities() is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }
    dsGetAudioCapabilitiesParam_t *param = (dsGetAudioCapabilitiesParam_t *)arg;
    if(0 != func) {
        param->result = func(param->handle, &param->capabilities);
    }
    else {
        param->capabilities = dsAUDIOSUPPORT_NONE;
    }

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}


IARM_Result_t _dsGetMS12Capabilities(void *arg)
{
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);
    
    typedef dsError_t (*dsGetMS12CapabilitiesFunc_t)(intptr_t handle, int *capabilities);
    static dsGetMS12CapabilitiesFunc_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetMS12CapabilitiesFunc_t)dlsym(dllib, "dsGetMS12Capabilities");
            if (func) {
                INT_DEBUG("dsGetMS12Capabilities() is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetMS12Capabilities() is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }
    dsGetMS12CapabilitiesParam_t *param = (dsGetMS12CapabilitiesParam_t *)arg;
    if(0 != func) {
        param->result = func(param->handle, &param->capabilities);
    }
    else {
        param->capabilities = dsMS12SUPPORT_NONE;
    }

    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

void _dsAudioOutPortConnectCB(dsAudioPortType_t portType, unsigned int uiPortNo, bool isPortConnected)
{
    IARM_Bus_DSMgr_EventData_t audio_out_hpd_eventData;
    INT_INFO("%s: AudioOutPort type:%d portNo:%d Hotplug happened\r\n", 
            __FUNCTION__, portType, uiPortNo);
    audio_out_hpd_eventData.data.audio_out_connect.portType = portType;
    audio_out_hpd_eventData.data.audio_out_connect.uiPortNo = uiPortNo;
    audio_out_hpd_eventData.data.audio_out_connect.isPortConnected = isPortConnected;
        
    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                           (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_AUDIO_OUT_HOTPLUG,
                           (void *)&audio_out_hpd_eventData, 
                           sizeof(audio_out_hpd_eventData));
    INT_INFO("%s portType%d uiPortNo:%d isPortConnected:%d", 
            __FUNCTION__, portType, uiPortNo, isPortConnected);           
}

static dsError_t _dsAudioOutRegisterConnectCB (dsAudioOutPortConnectCB_t cbFun) {
    dsError_t eRet = dsERR_GENERAL; 
    INT_DEBUG("%s: %d - Inside \n", __FUNCTION__, __LINE__);

    typedef dsError_t (*dsAudioOutRegisterConnectCB_t)(dsAudioOutPortConnectCB_t cbFunArg);
    static dsAudioOutRegisterConnectCB_t dsAudioOutRegisterConnectCBFun = 0;
    if (dsAudioOutRegisterConnectCBFun == 0) {
        INT_DEBUG("%s: %d - dlerror: %s\n", __FUNCTION__, __LINE__, dlerror());
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsAudioOutRegisterConnectCBFun = (dsAudioOutRegisterConnectCB_t) dlsym(dllib, "dsAudioOutRegisterConnectCB");
            if(dsAudioOutRegisterConnectCBFun == 0) {
                INT_INFO("%s: dsAudioOutRegisterConnectCB (int) is not defined %s\r\n", __FUNCTION__, dlerror());
                eRet = dsERR_GENERAL;
            }
            else {
                INT_DEBUG("%s: dsAudioOutRegisterConnectCB is loaded\r\n", __FUNCTION__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s: Opening RDK_DSHAL_NAME [%s] failed %s\r\n", 
                   __FUNCTION__, RDK_DSHAL_NAME, dlerror());
            eRet = dsERR_GENERAL;
        }
    }
    if (0 != dsAudioOutRegisterConnectCBFun) { 
        eRet = dsAudioOutRegisterConnectCBFun (cbFun);
        INT_INFO("%s: dsAudioOutRegisterConnectCBFun registered\r\n", __FUNCTION__);
    }
    else {
        INT_INFO("%s: dsAudioOutRegisterConnectCBFun NULL\r\n", __FUNCTION__);
    }
    return eRet;
}

IARM_Result_t _dsAudioOutIsConnected (void *arg) {
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);

    IARM_Result_t eIarmRet = IARM_RESULT_INVALID_PARAM;
    dsAudioOutIsConnectedParam_t* param = (dsAudioOutIsConnectedParam_t*) arg; 
    dsError_t eRet = dsERR_GENERAL; 
    //By default all audio ports are connected
    bool isConnected = true;
    param->isCon = true;

    INT_DEBUG("%s: %d - Inside \n", __FUNCTION__, __LINE__);

    typedef dsError_t (*dsAudioOutIsConnected_t)(intptr_t handleArg, bool* pisConArg);
    static dsAudioOutIsConnected_t dsAudioOutIsConFunc = 0;
    if (dsAudioOutIsConFunc == 0) {
        INT_DEBUG("%s: %d -  dlerror:%s\n", __FUNCTION__, __LINE__, dlerror());
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsAudioOutIsConFunc = (dsAudioOutIsConnected_t) dlsym(dllib, "dsAudioOutIsConnected");
            if(dsAudioOutIsConFunc == 0) {
                INT_INFO("%s: dsAudioOutIsConnected is not defined %s\r\n", __FUNCTION__, dlerror());
                eRet = dsERR_GENERAL;
            }
            else {
                INT_DEBUG("%s: dsAudioOutIsConnected is loaded\r\n", __FUNCTION__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s: Opening RDK_DSHAL_NAME [%s] failed %s\r\n", 
                   __FUNCTION__, RDK_DSHAL_NAME, dlerror());
            eRet = dsERR_GENERAL;
        }
    }
    if (0 != dsAudioOutIsConFunc) { 
        eRet = dsAudioOutIsConFunc (param->handle, &isConnected);
        INT_INFO("%s: pisCon:%d eRet:%04x\r\n", 
               __FUNCTION__, isConnected, eRet);
    }
    else {
        INT_INFO("%s: dsAudioOutIsConFunc NULL\n", __FUNCTION__);
    }
    
    param->result = eRet;
    if (dsERR_NONE == eRet) {
        param->isCon = isConnected;
        eIarmRet = IARM_RESULT_SUCCESS;
    }


    IARM_BUS_Unlock(lock);
    return eIarmRet;
}

void _dsAudioFormatUpdateCB(dsAudioFormat_t audioFormat)
{
    IARM_Bus_DSMgr_EventData_t audio_format_event_data;
    INT_INFO("%s: AudioOutPort format:%d \r\n", __FUNCTION__, audioFormat);
    audio_format_event_data.data.AudioFormatInfo.audioFormat = audioFormat;

    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                           (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_AUDIO_FORMAT_UPDATE,
                           (void *)&audio_format_event_data,
                           sizeof(audio_format_event_data));
}

static dsError_t _dsAudioFormatUpdateRegisterCB (dsAudioFormatUpdateCB_t cbFun) {
    dsError_t eRet = dsERR_GENERAL;
    INT_DEBUG("%s: %d - Inside \n", __FUNCTION__, __LINE__);

    typedef dsError_t (*dsAudioFormatUpdateRegisterCB_t)(dsAudioFormatUpdateCB_t cbFunArg);
    static dsAudioFormatUpdateRegisterCB_t dsAudioFormatUpdateRegisterCBFun = 0;
    if (dsAudioFormatUpdateRegisterCBFun == 0) {
        INT_DEBUG("%s: %d - dlerror: %s\n", __FUNCTION__, __LINE__, dlerror());
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsAudioFormatUpdateRegisterCBFun = (dsAudioFormatUpdateRegisterCB_t) dlsym(dllib, "dsAudioFormatUpdateRegisterCB");
            if(dsAudioFormatUpdateRegisterCBFun == 0) {
                INT_INFO("%s: dsAudioFormatUpdateRegisterCB is not defined %s\r\n", __FUNCTION__, dlerror());
                eRet = dsERR_GENERAL;
            }
            else {
                INT_DEBUG("%s: dsAudioFormatUpdateRegisterCB is loaded\r\n", __FUNCTION__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s: Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__, RDK_DSHAL_NAME, dlerror());
            eRet = dsERR_GENERAL;
        }
    }
    if (0 != dsAudioFormatUpdateRegisterCBFun) {
        eRet = dsAudioFormatUpdateRegisterCBFun (cbFun);
        INT_INFO("%s: dsAudioFormatUpdateRegisterCBFun registered\r\n", __FUNCTION__);
    }
    else {
        INT_INFO("%s: dsAudioFormatUpdateRegisterCBFun NULL\r\n", __FUNCTION__);
    }

    return eRet;
}

void _dsAudioAtmosCapsChangeCB(dsATMOSCapability_t atmosCaps, bool status)
{
    IARM_Bus_DSMgr_EventData_t atmos_caps_change_event_data;
    INT_INFO("%s: Atmos caps changed :%d \r\n", __FUNCTION__, atmosCaps);
    atmos_caps_change_event_data.data.AtmosCapsChange.caps = atmosCaps;
    atmos_caps_change_event_data.data.AtmosCapsChange.status = status;

    IARM_Bus_BroadcastEvent(IARM_BUS_DSMGR_NAME,
                           (IARM_EventId_t)IARM_BUS_DSMGR_EVENT_ATMOS_CAPS_CHANGED,
                           (void *)&atmos_caps_change_event_data,
                           sizeof(atmos_caps_change_event_data));
}

static dsError_t _dsAudioAtmosCapsChangeRegisterCB (dsAtmosCapsChangeCB_t cbFun) {
    dsError_t eRet = dsERR_GENERAL;
    INT_DEBUG("%s: %d - Inside \n", __FUNCTION__, __LINE__);

	typedef dsError_t (*dsAudioAtmosCapsChangeRegisterCB_t) (dsAtmosCapsChangeCB_t cbFunc);
    static dsAudioAtmosCapsChangeRegisterCB_t dsAudioAtmosCapsChangeRegisterCBFunc = 0;
    if (dsAudioAtmosCapsChangeRegisterCBFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsAudioAtmosCapsChangeRegisterCBFunc = (dsAudioAtmosCapsChangeRegisterCB_t) dlsym(dllib, "dsAudioAtmosCapsChangeRegisterCB");
            if(dsAudioAtmosCapsChangeRegisterCBFunc == 0) {
                INT_INFO("%s: dsAudioAtmosCapsChangeRegisterCB is not defined %s\r\n", __FUNCTION__, dlerror());
                eRet = dsERR_GENERAL;
            }
            else {
                INT_DEBUG("%s: dsAudioAtmosCapsChangeRegisterCB is loaded\r\n", __FUNCTION__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s: Opening RDK_DSHAL_NAME [%s] failed %s\r\n",
                   __FUNCTION__, RDK_DSHAL_NAME, dlerror());
            eRet = dsERR_GENERAL;
        }
    }
    if (0 != dsAudioAtmosCapsChangeRegisterCBFunc) {
        eRet = dsAudioAtmosCapsChangeRegisterCBFunc (cbFun);
        INT_INFO("%s: dsAudioAtmosCapsChangeRegisterCBFunc registered\r\n", __FUNCTION__);
    }
    else {
        INT_INFO("%s: dsAudioAtmosCapsChangeRegisterCBFunc NULL\r\n", __FUNCTION__);
    }

    return eRet;
}

IARM_Result_t _dsResetBassEnhancer(void *arg)
{
   _DEBUG_ENTER();
   IARM_BUS_Lock(lock);

   IARM_Result_t result = IARM_RESULT_INVALID_STATE;
   intptr_t *handle = (intptr_t*)arg;
   result = _resetBassEnhancer(*handle);
   IARM_BUS_Unlock(lock);
   return result;

}

static IARM_Result_t _resetBassEnhancer(intptr_t handle)
{
    typedef dsError_t (*dsSetBassEnhancer_t)(intptr_t handle, int boost);
    static dsSetBassEnhancer_t func = 0;
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetBassEnhancer_t) dlsym(dllib, "dsSetBassEnhancer");
            if (func) {
                INT_DEBUG("dsSetBassEnhancer_t(int, int) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetBassEnhancer_t(int, int) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
    if (func != 0) {
       std::string _Property = _dsGetCurrentProfileProperty("BassBoost");
       std::string _BassBoost("0");
       int m_bassBoost = 0;
       try {
            _BassBoost = device::HostPersistence::getInstance().getDefaultProperty(_Property);
       }
       catch(...) {
            _BassBoost = "0";
       }
       m_bassBoost = atoi(_BassBoost.c_str());

       if (func(handle,m_bassBoost) == dsERR_NONE) {
           INT_INFO("%s:%s Initialized Bass Boost : %d\n",__func__, _Property.c_str(), m_bassBoost);
           device::HostPersistence::getInstance().persistHostProperty("audio.BassBoost" ,_BassBoost);
           result = IARM_RESULT_SUCCESS;
       }
   }
#endif
   return result; 
}

IARM_Result_t _dsResetVolumeLeveller(void *arg)
{
    _DEBUG_ENTER();
    IARM_BUS_Lock(lock);
    intptr_t *handle = (intptr_t*)arg;
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;

    result = _resetVolumeLeveller(*handle);
    IARM_BUS_Unlock(lock);
    return result;
}

static IARM_Result_t _resetVolumeLeveller(intptr_t handle)
{
    typedef dsError_t (*dsSetVolumeLeveller_t)(intptr_t handle, dsVolumeLeveller_t volLeveller);
    static dsSetVolumeLeveller_t func = 0;
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;

    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetVolumeLeveller_t) dlsym(dllib, "dsSetVolumeLeveller");
            if (func) {
                INT_DEBUG("dsSetVolumeLeveller_t(int, dsVolumeLeveller_t) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetVolumeLeveller_t(int, dsVolumeLeveller_t) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
    if (func != 0) {
       std::string _PropertyMode = _dsGetCurrentProfileProperty("VolumeLeveller.mode");
       std::string _Propertylevel = _dsGetCurrentProfileProperty("VolumeLeveller.level");
       std::string _volLevellerMode("0");
       std::string _volLevellerLevel("0");
       dsVolumeLeveller_t m_volumeLeveller;
       try {
           _volLevellerMode = device::HostPersistence::getInstance().getDefaultProperty(_PropertyMode);
           _volLevellerLevel = device::HostPersistence::getInstance().getDefaultProperty(_Propertylevel);
       }
       catch(...) {
           _volLevellerMode = "0";
           _volLevellerLevel = "0";
       }
       m_volumeLeveller.mode = atoi(_volLevellerMode.c_str());
       m_volumeLeveller.level = atoi(_volLevellerLevel.c_str());
       if (func(handle, m_volumeLeveller) == dsERR_NONE) {
          INT_INFO("%s %s %s Default Volume Leveller : Mode: %d, Level: %d\n",__func__,_PropertyMode.c_str(),_Propertylevel.c_str(), m_volumeLeveller.mode, m_volumeLeveller.level);
          device::HostPersistence::getInstance().persistHostProperty(_PropertyMode ,_volLevellerMode);
          device::HostPersistence::getInstance().persistHostProperty(_Propertylevel ,_volLevellerLevel);
          result = IARM_RESULT_SUCCESS;
       }
   }
#endif
   IARM_BUS_Unlock(lock);
   return result;
}

IARM_Result_t _dsResetSurroundVirtualizer(void *arg)
{
   _DEBUG_ENTER();
   IARM_BUS_Lock(lock);
   intptr_t *handle = (intptr_t*)arg;
   IARM_Result_t result = IARM_RESULT_INVALID_STATE;

   result = _resetSurroundVirtualizer(*handle);
   IARM_BUS_Unlock(lock);
   return result;
}

static IARM_Result_t _resetSurroundVirtualizer(intptr_t handle)
{
   typedef dsError_t (*dsSetSurroundVirtualizer_t)(intptr_t handle, dsSurroundVirtualizer_t virtualizer);
   static dsSetSurroundVirtualizer_t func = 0;
   IARM_Result_t result = IARM_RESULT_INVALID_STATE;
   if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsSetSurroundVirtualizer_t) dlsym(dllib, "dsSetSurroundVirtualizer");
            if (func) {
                INT_DEBUG("dsSetSurroundVirtualizer_t(int, dsSurroundVirtualizer_t) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetSurroundVirtualizer_t(int, dsSurroundVirtualizer_t) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
  }
#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
  if (func != 0) {
       std::string _PropertyMode = _dsGetCurrentProfileProperty("SurroundVirtualizer.mode");
       std::string _Propertylevel = _dsGetCurrentProfileProperty("SurroundVirtualizer.boost");
       std::string _SVMode("0");
       std::string _SVBoost("0");
       dsSurroundVirtualizer_t m_virtualizer;
       try {
          _SVMode = device::HostPersistence::getInstance().getDefaultProperty(_PropertyMode);
          _SVBoost = device::HostPersistence::getInstance().getDefaultProperty(_Propertylevel);
       }
       catch(...) {
          _SVMode = "0";
          _SVBoost = "0";
       }
       m_virtualizer.mode = atoi(_SVMode.c_str());
       m_virtualizer.boost = atoi(_SVBoost.c_str());
       if (func(handle, m_virtualizer) == dsERR_NONE) {
           INT_INFO("%s %s %s Default Surround Virtualizer : Mode: %d, Boost : %d\n",__func__,_PropertyMode.c_str(),_Propertylevel.c_str(), m_virtualizer.mode, m_virtualizer.boost);
           device::HostPersistence::getInstance().persistHostProperty(_PropertyMode ,_SVMode);
           device::HostPersistence::getInstance().persistHostProperty(_Propertylevel ,_SVBoost);
           result = IARM_RESULT_SUCCESS;
       }
  }
#endif
  return result;
}

IARM_Result_t _dsResetDialogEnhancement(void *arg)
{
  _DEBUG_ENTER();
  IARM_BUS_Lock(lock);
  IARM_Result_t result = IARM_RESULT_INVALID_STATE;
  intptr_t *handle = (intptr_t*)arg;

  result = _resetDialogEnhancerLevel(*handle);
  IARM_BUS_Unlock(lock);
  return result;
}

static IARM_Result_t  _resetDialogEnhancerLevel(intptr_t handle)
{
  typedef dsError_t (*dsSetDialogEnhancement_t)(intptr_t handle, int enhancerLevel);
  static dsSetDialogEnhancement_t func = 0;
  IARM_Result_t result = IARM_RESULT_INVALID_STATE;
  if (func == 0) {
     void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
     if (dllib) {
        func = (dsSetDialogEnhancement_t) dlsym(dllib, "dsSetDialogEnhancement");
        if (func) {
           INT_DEBUG("dsSetDialogEnhancement_t(int, int) is defined and loaded\r\n");
        }
        else {
           INT_INFO("dsSetDialogEnhancement_t(int, int ) is not defined\r\n");
        }
        dlclose(dllib);
     }
     else {
        INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
     }
  }

#ifdef DS_AUDIO_SETTINGS_PERSISTENCE
  std::string _Property = _dsGetCurrentProfileProperty("EnhancerLevel");
  if (func) {
     std::string _EnhancerLevel("0");
     int m_enhancerLevel = 0;
     try {
         _EnhancerLevel = device::HostPersistence::getInstance().getDefaultProperty(_Property);
     }
     catch(...) {
         _EnhancerLevel = "0";
     }
     m_enhancerLevel = atoi(_EnhancerLevel.c_str());
     if (func(handle, m_enhancerLevel) == dsERR_NONE) {
         INT_INFO("%s %s Default dialog enhancement level : %d\n",__func__,_Property.c_str(), m_enhancerLevel);
         device::HostPersistence::getInstance().persistHostProperty(_Property ,_EnhancerLevel);
         result = IARM_RESULT_SUCCESS;
     }
  }
#endif
 return result;
}

std::string _dsGetCurrentProfileProperty(std::string property)
{
   std::string _AProfile("Off");
   std::string _AProfileSupport("FALSE");
   try {
        _AProfileSupport = device::HostPersistence::getInstance().getDefaultProperty("audio.MS12Profile.supported");
   }
   catch(...) {
        _AProfileSupport = "FALSE";
        INT_INFO("audio.MS12Profile.supported setting not found in hostDataDeafult \r\n");
   }
   INT_INFO(" audio.MS12Profile.supported = %s ..... \r\n",_AProfileSupport.c_str());

   if(_AProfileSupport == "TRUE") {
      try {
          _AProfile = device::HostPersistence::getInstance().getProperty("audio.MS12Profile");
      }
      catch(...) {
         try {
             INT_DEBUG("audio.MS12Profile not found in persistence store. Try system default\n");
            _AProfile = device::HostPersistence::getInstance().getDefaultProperty("audio.MS12Profile");
        }
        catch(...) {
            _AProfile = "Off";
        }
      }
   }
   std::string profileProperty("audio.");
   if(_AProfileSupport == "TRUE") {
      profileProperty.append (_AProfile);
      profileProperty.append (".");
   }
   profileProperty.append (property);
   return profileProperty;
}

static std::string _dsGenerateProfileProperty(std::string profile,std::string property)
{
   std::string profileProperty("audio.");
   profileProperty.append (profile);
   profileProperty.append (".");
   profileProperty.append (property);
   return profileProperty;
}

void _dsMS12ProfileSettingOverride(intptr_t handle)
{
    typedef dsError_t (*dsSetDialogEnhancement_t)(intptr_t handle, int enhancerLevel);
    static dsSetDialogEnhancement_t dsSetDialogEnhancementfunc = 0;
    if (dsSetDialogEnhancementfunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
          dsSetDialogEnhancementfunc = (dsSetDialogEnhancement_t) dlsym(dllib, "dsSetDialogEnhancement");
          if (dsSetDialogEnhancementfunc) {
             INT_DEBUG("dsSetDialogEnhancement_t(int, int) is defined and loaded\r\n");
          }
          else {
             INT_INFO("dsSetDialogEnhancement_t(int, int ) is not defined\r\n");
          }
          dlclose(dllib);
       }
       else {
          INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
       }
    }

    if (dsSetDialogEnhancementfunc) {
       std::string _EnhancerLevel("0");
       int m_enhancerLevel = 0;
       std::string _Property = _dsGetCurrentProfileProperty("EnhancerLevel");
       try {
          _EnhancerLevel = device::HostPersistence::getInstance().getProperty(_Property);
       }
       catch(...) {
            try {
                INT_DEBUG("audio.EnhancerLevel not found in persistence store. Try system default\n");
                _EnhancerLevel = device::HostPersistence::getInstance().getDefaultProperty(_Property);
            }
            catch(...) {
                _EnhancerLevel = "0";
            }
       }
       m_enhancerLevel = atoi(_EnhancerLevel.c_str());
       if (dsSetDialogEnhancementfunc(handle, m_enhancerLevel) == dsERR_NONE) {
           device::HostPersistence::getInstance().persistHostProperty(_Property ,_EnhancerLevel);
           INT_INFO("%s: persist enhancer level: %d\n",__func__,m_enhancerLevel );
       }
    }
 
    typedef dsError_t (*dsSetBassEnhancer_t)(intptr_t handle, int boost);
    static dsSetBassEnhancer_t dsSetBassEnhancerFunc = 0;
    if (dsSetBassEnhancerFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsSetBassEnhancerFunc = (dsSetBassEnhancer_t) dlsym(dllib, "dsSetBassEnhancer");
            if (dsSetBassEnhancerFunc) {
                INT_DEBUG("dsSetBassEnhancer_t(int, int) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetBassEnhancer_t(int, int) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
           INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }
    if (dsSetBassEnhancerFunc != 0)
    {
        std::string _BassBoost("0");
        int m_bassBoost = 0;
        std::string _Property = _dsGetCurrentProfileProperty("BassBoost");
        try {
            _BassBoost = device::HostPersistence::getInstance().getProperty("audio.BassBoost");
        }
        catch(...) {
            try {
                INT_DEBUG("audio.EnhancerLevel not found in persistence store. Try system default\n");
                _BassBoost = device::HostPersistence::getInstance().getDefaultProperty(_Property);
            }
            catch(...) {
                _BassBoost = "0";
            }
        }
        m_bassBoost = atoi(_BassBoost.c_str());
        if (dsSetBassEnhancerFunc(handle, m_bassBoost) == dsERR_NONE)
        {
            INT_INFO("%s: persist boost value: %d\n",__func__, m_bassBoost);
            device::HostPersistence::getInstance().persistHostProperty("audio.BassBoost" ,_BassBoost);
        }
    }

    typedef dsError_t (*dsSetVolumeLeveller_t)(intptr_t handle, dsVolumeLeveller_t volLeveller);
    static dsSetVolumeLeveller_t dsSetVolumeLevellerfunc = 0;
    if (dsSetVolumeLevellerfunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            dsSetVolumeLevellerfunc = (dsSetVolumeLeveller_t) dlsym(dllib, "dsSetVolumeLeveller");
            if (dsSetVolumeLevellerfunc) {
                INT_DEBUG("dsSetVolumeLeveller_t(int, dsVolumeLeveller_t) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetVolumeLeveller_t(int, dsVolumeLeveller_t) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
              INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    if (dsSetVolumeLevellerfunc != 0 )
    {
        std::string _volLevellerMode("0");
        std::string _volLevellerLevel("0");
        dsVolumeLeveller_t m_volumeLeveller;
        std::string _PropertyMode = _dsGetCurrentProfileProperty("VolumeLeveller.mode");
        std::string _PropertyLevel = _dsGetCurrentProfileProperty("VolumeLeveller.level");
        try {
            _volLevellerMode = device::HostPersistence::getInstance().getProperty(_PropertyMode);
            _volLevellerLevel = device::HostPersistence::getInstance().getProperty(_PropertyLevel);
        }
        catch(...) {
            try {
                _volLevellerMode = device::HostPersistence::getInstance().getDefaultProperty(_PropertyMode);
                _volLevellerLevel = device::HostPersistence::getInstance().getDefaultProperty(_PropertyLevel);
            }
            catch(...) {
                _volLevellerMode = "0";
                _volLevellerLevel = "0";
            }
        }
        m_volumeLeveller.mode = atoi(_volLevellerMode.c_str());
        m_volumeLeveller.level = atoi(_volLevellerLevel.c_str());
        if (dsSetVolumeLevellerfunc(handle, m_volumeLeveller) == dsERR_NONE)
        {
            INT_INFO("%s: persist volume leveller mode: %d\n",__func__, m_volumeLeveller.mode);
            device::HostPersistence::getInstance().persistHostProperty(_PropertyMode,_volLevellerMode);
            INT_INFO("%s: persist volume leveller value: %d\n",__func__, m_volumeLeveller.level);
            device::HostPersistence::getInstance().persistHostProperty(_PropertyLevel,_volLevellerLevel);
        }
    }   
    
    typedef dsError_t (*dsSetSurroundVirtualizer_t)(intptr_t handle, dsSurroundVirtualizer_t virtualizer);
    static dsSetSurroundVirtualizer_t dsSetSurroundVirtualizerfunc = 0;
    if (dsSetSurroundVirtualizerfunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
        dsSetSurroundVirtualizerfunc = (dsSetSurroundVirtualizer_t) dlsym(dllib, "dsSetSurroundVirtualizer");
            if (dsSetSurroundVirtualizerfunc) {
                INT_DEBUG("dsSetSurroundVirtualizer_t(int, dsSurroundVirtualizer_t) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsSetSurroundVirtualizer_t(int, dsSurroundVirtualizer_t) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
              INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    if (dsSetSurroundVirtualizerfunc != 0 )
    {
        std::string _SVMode("0");
        std::string _SVBoost("0");
        dsSurroundVirtualizer_t m_virtualizer;
        std::string _PropertyMode = _dsGetCurrentProfileProperty("SurroundVirtualizer.mode");
        std::string _PropertyBoost = _dsGetCurrentProfileProperty("SurroundVirtualizer.boost");
        try {
            _SVMode = device::HostPersistence::getInstance().getProperty(_PropertyMode);
            _SVBoost = device::HostPersistence::getInstance().getProperty(_PropertyBoost);
        }
        catch(...) {
            try {
                _SVMode = device::HostPersistence::getInstance().getDefaultProperty(_PropertyMode);
                _SVBoost = device::HostPersistence::getInstance().getDefaultProperty(_PropertyBoost);
            }
            catch(...) {
                _SVMode = "0";
                _SVBoost = "0";
            }
        }
        m_virtualizer.mode = atoi(_SVMode.c_str());
        m_virtualizer.boost = atoi(_SVBoost.c_str());

        if (dsSetSurroundVirtualizerfunc(handle,m_virtualizer) == dsERR_NONE)
        {
            INT_INFO("%s: persist surround virtualizer mode: %d\n",__func__, m_virtualizer.mode);
            device::HostPersistence::getInstance().persistHostProperty(_PropertyMode,_SVMode);
            INT_INFO("%s: persist surround virtualizer boost value: %d\n",__func__, m_virtualizer.boost);
            device::HostPersistence::getInstance().persistHostProperty(_PropertyBoost,_SVBoost);
 
        }
    }
}

bool _dsMs12ProfileSupported(intptr_t handle,std::string profile)
{
    typedef dsError_t (*dsGetMS12AudioProfileList_t)(intptr_t handle, dsMS12AudioProfileList_t* profiles);
    static dsGetMS12AudioProfileList_t func = 0;
    if (func == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
            func = (dsGetMS12AudioProfileList_t) dlsym(dllib, "dsGetMS12AudioProfileList");
            if (func) {
                INT_DEBUG("dsGetMS12AudioProfileList_t(int, dsMS12AudioProfileList_t*) is defined and loaded\r\n");
            }
            else {
                INT_INFO("dsGetMS12AudioProfileList_t(int, dsMS12AudioProfileList_t*) is not defined\r\n");
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("Opening RDK_DSHAL_NAME [%s] failed\r\n", RDK_DSHAL_NAME);
        }
    }

    dsMS12AudioProfileList_t pProfilesStr;
    bool result = 0;
    dsError_t ret = dsERR_NONE;
    if (func != 0 )
    {
        ret = func(handle, &pProfilesStr);
        if (ret == dsERR_NONE)
        {
            if(strstr(pProfilesStr.audioProfileList,profile.c_str()))
               result = 1;
            else
               result = 0;
        }
    }
    return result;
}

IARM_Result_t _dsGetHDMIARCPortId(void *arg)
{
#ifndef RDK_DSHAL_NAME
#warning   "RDK_DSHAL_NAME is not defined"
#define RDK_DSHAL_NAME "RDK_DSHAL_NAME is not defined"
#endif
    _DEBUG_ENTER();

    IARM_BUS_Lock(lock);
    std::string _HDMIARCPortId("0");
    dsGetHDMIARCPortIdParam_t *param = ( dsGetHDMIARCPortIdParam_t *)arg;
    try {
            _HDMIARCPortId = device::HostPersistence::getInstance().getDefaultProperty("HDMIARC.port.Id");
    }
    catch(...) {
            _HDMIARCPortId = "-1";
    }
    param->portId = atoi(_HDMIARCPortId.c_str());
    INT_DEBUG("The HDMI ARC Port Id is %d \r\n",param->portId);
    IARM_BUS_Unlock(lock);
    return IARM_RESULT_SUCCESS;
}

#ifdef DS_AUDIO_SETTINGS_PERSISTENCE

static void* persist_audioLevel_timer_threadFunc(void* arg) {
	float prev_audioLevel_spdif = audioLevel_cache_spdif, prev_audioLevel_speaker = audioLevel_cache_speaker, prev_audioLevel_hdmi = audioLevel_cache_hdmi, prev_audioLevel_headphone = audioLevel_cache_headphone;
	INT_DEBUG("%s Audio level persistence update timer thread running...\n",__func__);
	struct timespec ts;
	    while(1){
	      clock_gettime(CLOCK_REALTIME, &ts);
              ts.tv_sec += 5;

              pthread_mutex_lock(&audioLevelMutex);
              while(!audioLevel_timer_set){
                int rc = pthread_cond_timedwait(&audioLevelTimerCV, &audioLevelMutex, &ts);
                if (rc == ETIMEDOUT)
                        continue;
              }
              if(!persist_audioLevel_timer_threadIsAlive){
		pthread_mutex_unlock(&audioLevelMutex);
                break;
              }
	      // wait for 3 sec, then update the latest audio level from cache variable
	      if(audioLevel_timer_set){
                sleep(3);

                if(float(audioLevel_cache_spdif) != prev_audioLevel_spdif){
                  INT_INFO("%s: port: %s ,persist audio level: %f\n",__func__,"SPDIF0.audio.Level",float(audioLevel_cache_spdif));
                  device::HostPersistence::getInstance().persistHostProperty("SPDIF0.audio.Level",std::to_string(audioLevel_cache_spdif));
                  prev_audioLevel_spdif = float(audioLevel_cache_spdif);
                }
                if(float(audioLevel_cache_hdmi) != prev_audioLevel_hdmi){
                  INT_INFO("%s: port: %s ,persist audio level: %f\n",__func__,"HDMI0.audio.Level",float(audioLevel_cache_hdmi));
                  device::HostPersistence::getInstance().persistHostProperty("HDMI0.audio.Level",std::to_string(audioLevel_cache_hdmi));
                  prev_audioLevel_hdmi = float(audioLevel_cache_hdmi);
                }
                if(float(audioLevel_cache_speaker) != prev_audioLevel_speaker){
                  INT_INFO("%s: port: %s ,persist audio level: %f\n",__func__,"SPEAKER0.audio.Level",float(audioLevel_cache_speaker));
                  device::HostPersistence::getInstance().persistHostProperty("SPEAKER0.audio.Level",std::to_string(audioLevel_cache_speaker));
                  prev_audioLevel_speaker = float(audioLevel_cache_speaker);
                }
                if(float(audioLevel_cache_headphone) != prev_audioLevel_headphone){
                  INT_INFO("%s: port: %s ,persist audio level: %f\n",__func__,"HEADPHONE0.audio.Level",float(audioLevel_cache_headphone));
                  device::HostPersistence::getInstance().persistHostProperty("HEADPHONE0.audio.Level",std::to_string(audioLevel_cache_headphone));
                  prev_audioLevel_headphone = float(audioLevel_cache_headphone);
                }
                else{
                  INT_INFO("%s Audio level is same as last set value.Not updating persistence\n",__func__);
                }
                audioLevel_timer_set = false;
	      }
              pthread_mutex_unlock(&audioLevelMutex);
            }

        pthread_exit(NULL);
        return NULL;
}
static dsError_t setAudioMixerLevels (intptr_t handle, dsAudioInput_t aInput, int volume) {
    dsError_t eRet = dsERR_GENERAL;
	
    typedef dsError_t (*dsSetAudioMixerLevels_t)(intptr_t handle,dsAudioInput_t aInput, int volume);
    static dsSetAudioMixerLevels_t dsSetAudioMixerLevelsFunc = 0;

    if (dsSetAudioMixerLevelsFunc == 0) {
       void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
       if (dllib) {
            dsSetAudioMixerLevelsFunc = (dsSetAudioMixerLevels_t) dlsym(dllib, "dsSetAudioMixerLevels");
            if(dsSetAudioMixerLevelsFunc) {
                INT_INFO("%s:%d dsSetAudioMixerLevels (intptr_t,dsAudioInput_t,int) is not defined %s\r\n", __FUNCTION__, __LINE__, dlerror());
            }
            else {
                INT_DEBUG("%s:%d dsSetAudioMixerLevels loaded\r\n", __FUNCTION__, __LINE__);
            }
            dlclose(dllib);
        }
        else {
            INT_ERROR("%s:%d dsSetAudioMixerLevels API Opening failed %s\r\n",
                   __FUNCTION__, __LINE__, dlerror());
        }
    }
    handle = 0;
    if (0 != dsSetAudioMixerLevelsFunc) {
        eRet = dsSetAudioMixerLevelsFunc (handle,aInput, volume);
        INT_INFO("[srv] %s: dsSetAudioMixerLevelsFunc eRet: %d \r\n", __FUNCTION__, eRet);
    }
    else {
        INT_INFO("%s:  dsSetAudioMixerLevelsFunc = %p\n", __FUNCTION__, dsSetAudioMixerLevelsFunc);
    }
    return eRet;
}


IARM_Result_t _dsSetAudioMixerLevels (void *arg)
{
    _DEBUG_ENTER();
    dsSetAudioMixerLevelsParam_t *param = (dsSetAudioMixerLevelsParam_t *) arg;
    IARM_Result_t result = IARM_RESULT_INVALID_STATE;
    IARM_BUS_Lock(lock);
    if(setAudioMixerLevels(param->handle,param->aInput, param->volume) == dsERR_NONE){
          INT_INFO("[srv] %s: _dsSetAudioMixerLevels with aInput: %d and volume %d\r\n", __FUNCTION__, param->aInput,param->volume);
          result = IARM_RESULT_SUCCESS;
    }
    IARM_BUS_Unlock(lock);
    return result;
}

#endif

/** @} */
/** @} */
