#include <chrono>
#include <mutex>
#include <sstream>

#include "IarmHostImpl.hpp"

#include "dsMgr.h"
/*#include "dsTypes.h"*/
#include "dslogger.h"

#include "libIBus.h"

using IVideoDeviceEvents = device::Host::IVideoDeviceEvents;
using IVideoOutputPortEvents   = device::Host::IVideoOutputPortEvents;
using IAudioOutputPortEvents   = device::Host::IAudioOutputPortEvents;
using ICompositeInEvents   = device::Host::ICompositeInEvents;

namespace device {

struct EventHandlerMapping {
    IARM_EventId_t eventId;
    IARM_EventHandler_t handler;
};

// unregisterIarmEvents can be called by registerIarmEvents in case of failure.
// Hence defined before registerIarmEvents
template <size_t N>
static bool unregisterIarmEvents(const EventHandlerMapping (&handlers)[N])
{
    bool unregistered = true;

    for (const auto& eh : handlers) {
        if (IARM_RESULT_SUCCESS != IARM_Bus_UnRegisterEventHandler(IARM_BUS_DSMGR_NAME, eh.eventId)) {
            INT_ERROR("Failed to unregister IARM event handler for %d", eh.eventId);
            unregistered = false;
            // don't break here, try to unregister all handlers
        }
    }
    return unregistered;
}

template <size_t N>
static bool registerIarmEvents(const EventHandlerMapping (&handlers)[N])
{
    bool registered = true;

    for (const auto& eh : handlers) {
        if (IARM_RESULT_SUCCESS != IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, eh.eventId, eh.handler)) {
            INT_ERROR("Failed to register IARM event handler for %d", eh.eventId);
            registered = false;
            // no point in continuing as we will attempt to unregister anyway
            break;
        }
    }

    if (!registered) {
        // in case of failure / partial failure
        // we should unregister any handlers that were registered
        unregisterIarmEvents(handlers);
    }

    return registered;
}

// IARMGroupXYZ are c to c++ shim (all methods are static)
// Required to perform group event register and unregister with IARM
// Thread safety to be ensured by the caller
class IARMGroupVideoDevice {
public:
    static bool RegisterIarmEvents()
    {
        return registerIarmEvents(handlers);
    }

    static bool UnRegisterIarmEvents()
    {
        return unregisterIarmEvents(handlers);
    }

private:
    static void iarmDisplayFrameratePreChangeHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE received %s %d", owner, eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            std::string framerate(eventData->data.DisplayFrameRateChange.framerate);

            IarmHostImpl::Dispatch([&framerate](IVideoDeviceEvents* listener) {
                listener->OnDisplayFrameratePreChange(framerate);
            });
        } else {
            INT_ERROR("Invalid data received for display framerate pre-change");
        }
    }

    static void iarmDisplayFrameratePostChangeHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE received %s %d", owner, eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            std::string framerate(eventData->data.DisplayFrameRateChange.framerate);

            IarmHostImpl::Dispatch([&framerate](IVideoDeviceEvents* listener) {
                listener->OnDisplayFrameratePostChange(framerate);
            });
        } else {
            INT_ERROR("Invalid data received for display framerate post-change");
        }
    }

    static constexpr EventHandlerMapping handlers[] = {
        { IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE,  &IARMGroupVideoDevice::iarmDisplayFrameratePreChangeHandler  },
        { IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, &IARMGroupVideoDevice::iarmDisplayFrameratePostChangeHandler },
    };
};

class IARMGroupVideoPort {
public:
    static bool RegisterIarmEvents()
    {
        return registerIarmEvents(handlers);
    }

    static bool UnRegisterIarmEvents()
    {
        return unregisterIarmEvents(handlers);
    }

private:
    static void iarmResolutionPreChangeHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_RES_PRECHANGE received, eventId = %d", eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            int width  = eventData->data.resn.width;
            int height = eventData->data.resn.height;

            IarmHostImpl::Dispatch([width, height](IVideoOutputPortEvents* listener) {
                listener->OnResolutionPreChange(width, height);
            });
        } else {
            INT_ERROR("Invalid data received for resolution pre-change");
        }
    }

    static void iarmResolutionPostChangeHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_RES_POSTCHANGE received, eventId = %d", eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            int width  = eventData->data.resn.width;
            int height = eventData->data.resn.height;

            IarmHostImpl::Dispatch([width, height](IVideoOutputPortEvents* listener) {
                listener->OnResolutionPostChange(width, height);
            });
        } else {
            INT_ERROR("Invalid data received for resolution post-change");
        }
    }

    static void iarmHDCPStatusChangeHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_HDCP_STATUS received, eventId=%d", eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            // TODO
        } else {
            INT_ERROR("Invalid data received for HDCP status change");
        }
    }

    static void iarmVideoFormatUpdateHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_VIDEO_FORMAT_UPDATE received, eventId=%d", eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            dsHDRStandard_t videoFormat = eventData->data.VideoFormatInfo.videoFormat;

            IarmHostImpl::Dispatch([videoFormat](IVideoOutputPortEvents* listener) {
                listener->OnVideoFormatUpdate(videoFormat);
            });
        } else {
            INT_ERROR("Invalid data received for video format update");
        }
    }

    static constexpr EventHandlerMapping handlers[] = {
        { IARM_BUS_DSMGR_EVENT_RES_PRECHANGE,       &IARMGroupVideoPort::iarmResolutionPreChangeHandler  },
        { IARM_BUS_DSMGR_EVENT_RES_POSTCHANGE,      &IARMGroupVideoPort::iarmResolutionPostChangeHandler },
        { IARM_BUS_DSMGR_EVENT_HDCP_STATUS,         &IARMGroupVideoPort::iarmHDCPStatusChangeHandler     },
        { IARM_BUS_DSMGR_EVENT_VIDEO_FORMAT_UPDATE, &IARMGroupVideoPort::iarmVideoFormatUpdateHandler    },
    };
};

class IARMGroupAudioPort {
public:
    static bool RegisterIarmEvents()
    {
        return registerIarmEvents(handlers);
    }

    static bool UnRegisterIarmEvents()
    {
        return unregisterIarmEvents(handlers);
    }

private:
    static void iarmAssociatedAudioMixingChangedHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_ASSOCIATED_AUDIO_MIXING_CHANGED received, eventId=%d", eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            bool mixing = eventData->data.AssociatedAudioMixingInfo.mixing;

            IarmHostImpl::Dispatch([mixing](IAudioOutputPortEvents* listener) {
                listener->OnAssociatedAudioMixingChanged(mixing);
            });
        } else {
            INT_ERROR("Invalid data received for associated audio mixing change");
        }
    };

    static void iarmAudioFaderControlChangedHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_FADER_CONTROL_CHANGED received, eventId=%d", eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            int mixerBalance = eventData->data.FaderControlInfo.mixerbalance;

            IarmHostImpl::Dispatch([mixerBalance](IAudioOutputPortEvents* listener) {
                listener->OnAudioFaderControlChanged(mixerBalance);
            });
        } else {
            INT_ERROR("Invalid data received for audio fader control change");
        }
    };

    static void iarmAudioPrimaryLanguageChangedHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_PRIMARY_LANGUAGE_CHANGED received, eventId=%d", eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            std::string primaryLanguage(eventData->data.AudioLanguageInfo.audioLanguage);

            IarmHostImpl::Dispatch([&primaryLanguage](IAudioOutputPortEvents* listener) {
                listener->OnAudioPrimaryLanguageChanged(primaryLanguage);
            });
        } else {
            INT_ERROR("Invalid data received for primary language change");
        }
    };

    static void iarmAudioSecondaryLanguageChangedHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_SECONDARY_LANGUAGE_CHANGED received, eventId=%d", eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            std::string secondaryLanguage(eventData->data.AudioLanguageInfo.audioLanguage);

            IarmHostImpl::Dispatch([&secondaryLanguage](IAudioOutputPortEvents* listener) {
                listener->OnAudioSecondaryLanguageChanged(secondaryLanguage);
            });
        } else {
            INT_ERROR("Invalid data received for secondary language change");
        }
    };

    static void iarmAudioOutHotPlugHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_OUT_HOTPLUG received, eventId=%d", eventId);
        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (eventData) {
            dsAudioPortType_t portType = eventData->data.audio_out_connect.portType;
            int uiPortNumber           = eventData->data.audio_out_connect.uiPortNo;
            bool isPortConnected       = eventData->data.audio_out_connect.isPortConnected;

            IarmHostImpl::Dispatch([portType, uiPortNumber, isPortConnected](IAudioOutputPortEvents* listener) {
                listener->OnAudioOutHotPlug(portType, uiPortNumber, isPortConnected);
            });
        } else {
            INT_ERROR("Invalid data received for audio out hot plug change");
        }
    };

    static void iarmDolbyAtmosCapabilitiesChangedHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_ATMOS_CAPS_CHANGED received, eventId=%d", eventId);
        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (eventData) {
            dsATMOSCapability_t atmosCapability = eventData->data.AtmosCapsChange.caps;
            bool status                         = eventData->data.AtmosCapsChange.status;

            IarmHostImpl::Dispatch([atmosCapability, status](IAudioOutputPortEvents* listener) {
                listener->OnDolbyAtmosCapabilitiesChanged(atmosCapability, status);
            });

        } else {
            INT_ERROR("Invalid data received for Dolby Atmos capabilities change");
        }
    };

    // TODO: requires dsMgr.h header for dsAudioPortState_t ?
    static void iarmAudioPortStateChangedHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_PORT_STATE received, eventId=%d", eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            dsAudioPortState_t audioPortState = eventData->data.AudioPortStateInfo.audioPortState;

            IarmHostImpl::Dispatch([audioPortState](IAudioOutputPortEvents* listener) {
                // TODO:
                // listener->OnAudioPortStateChanged(audioPortState);
            });
        } else {
            INT_ERROR("Invalid data received for audio port state change");
        }
    };

    static void iarmAudioModeEventHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_MODE received, eventId=%d", eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            dsAudioPortType_t audioPortType     = static_cast<dsAudioPortType_t>(eventData->data.Audioport.type);
            dsAudioStereoMode_t audioStereoMode = static_cast<dsAudioStereoMode_t>(eventData->data.Audioport.mode);

            IarmHostImpl::Dispatch([audioPortType, audioStereoMode](IAudioOutputPortEvents* listener) {
                listener->OnAudioModeEvent(audioPortType, audioStereoMode);
            });
        } else {
            INT_ERROR("Invalid data received for audio mode change");
        }
    };

    static void iarmAudioLevelChangedEventHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_LEVEL_CHANGED received, eventId=%d", eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            int audioLevel = eventData->data.AudioLevelInfo.level;

            IarmHostImpl::Dispatch([audioLevel](IAudioOutputPortEvents* listener) {
                //listener->OnAudioLevelChangedEvent(audioLevel);
            });
        } else {
            INT_ERROR("Invalid data received for audio level change");
        }
    };

    static void iarmAudioFormatUpdateHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_FORMAT_UPDATE received, eventId=%d", eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            dsAudioFormat_t audioFormat = eventData->data.AudioFormatInfo.audioFormat;

            IarmHostImpl::Dispatch([audioFormat](IAudioOutputPortEvents* listener) {
                listener->OnAudioFormatUpdate(audioFormat);
            });
        } else {
            INT_ERROR("Invalid data received for audio format update");
        }
    };

private:
    static constexpr EventHandlerMapping handlers[] = {
        { IARM_BUS_DSMGR_EVENT_AUDIO_ASSOCIATED_AUDIO_MIXING_CHANGED, &IARMGroupAudioPort::iarmAssociatedAudioMixingChangedHandler  },
        { IARM_BUS_DSMGR_EVENT_AUDIO_FADER_CONTROL_CHANGED,           &IARMGroupAudioPort::iarmAudioFaderControlChangedHandler      },
        { IARM_BUS_DSMGR_EVENT_AUDIO_PRIMARY_LANGUAGE_CHANGED,        &IARMGroupAudioPort::iarmAudioPrimaryLanguageChangedHandler   },
        { IARM_BUS_DSMGR_EVENT_AUDIO_SECONDARY_LANGUAGE_CHANGED,      &IARMGroupAudioPort::iarmAudioSecondaryLanguageChangedHandler },
        { IARM_BUS_DSMGR_EVENT_AUDIO_OUT_HOTPLUG,                     &IARMGroupAudioPort::iarmAudioOutHotPlugHandler               },
        { IARM_BUS_DSMGR_EVENT_ATMOS_CAPS_CHANGED,                    &IARMGroupAudioPort::iarmDolbyAtmosCapabilitiesChangedHandler },
        { IARM_BUS_DSMGR_EVENT_AUDIO_PORT_STATE,                      &IARMGroupAudioPort::iarmAudioPortStateChangedHandler         },
        { IARM_BUS_DSMGR_EVENT_AUDIO_MODE,                            &IARMGroupAudioPort::iarmAudioModeEventHandler                },
        { IARM_BUS_DSMGR_EVENT_AUDIO_LEVEL_CHANGED,                   &IARMGroupAudioPort::iarmAudioLevelChangedEventHandler        },
        { IARM_BUS_DSMGR_EVENT_AUDIO_FORMAT_UPDATE,                   &IARMGroupAudioPort::iarmAudioFormatUpdateHandler             },
    };
};



class IARMGroupComposite {
public:
    static bool RegisterIarmEvents()
    {
        return registerIarmEvents(handlers);
    }

    static bool UnRegisterIarmEvents()
    {
        return unregisterIarmEvents(handlers);
    }

private:
	static void iarmCompositeInHotPlugHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
	{
		INT_INFO("IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG received, eventId=%d", eventId);		
		
		IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (eventData) {
            dsCompositeInPort_t compositePort = eventData->data.composite_in_connect.port;
			bool isConnected = eventData->data.composite_in_connect.isPortConnected;
            
			IarmHostImpl::Dispatch([compositePort](ICompositeInEvents* listener) {
                listener->OnCompositeInHotPlug(compositePort,isConnected);
            });
        } else {
            INT_ERROR("Invalid data received for Composite Status Handler in iarmCompositeInHotPlugHandler");
        }		
	}


	static void iarmCompositeInSignalStatusHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
	{
		INT_INFO("IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS received, eventId=%d", eventId);		
		
		IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (eventData) {
            dsCompositeInPort_t compositePort = eventData->data.composite_in_sig_status.port;
			dsCompInSignalStatus_t compositeSigStatus = eventData->data.composite_in_sig_status.status;
            
			IarmHostImpl::Dispatch([compositePort](ICompositeInEvents* listener) {
                listener->OnCompositeInSignalStatus(compositePort,compositeSigStatus);
            });
        } else {
            INT_ERROR("Invalid data received for Composite Status Handler in iarmCompositeInSignalStatusHandler");
        }		
	}

	static void iarmCompositeInStatusHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
	{
		INT_INFO("IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS received, eventId=%d", eventId);		
		
		IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (eventData) {
            dsCompositeInPort_t compositePort = eventData->data.composite_in_status.port;
			bool isPresented = eventData->data.composite_in_status.isPresented;
            
			IarmHostImpl::Dispatch([compositePort](ICompositeInEvents* listener) {
                listener->OnCompositeInStatus(compositePort,isPresented);
            });
        } else {
            INT_ERROR("Invalid data received for Composite Status Handler in iarmCompositeInStatusHandler");
        }		
	}


	static void iarmCompositeInVideoModeUpdateHandler(const char*, IARM_EventId_t eventId, void* data, size_t len) 
	{
        INT_INFO("IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE received, eventId=%d", eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            dsCompositeInPort_t compositePort = eventData->data.composite_in_video_mode.port;
			dsVideoPortResolution_t videoResolution;
			videoResolution.pixelResolution = eventData->data.composite_in_video_mode.resolution.pixelResolution;
			videoResolution.interlaced = eventData->data.composite_in_video_mode.resolution.interlaced;
			videoResolution.frameRate = eventData->data.composite_in_video_mode.resolution.frameRate;		
            IarmHostImpl::Dispatch([compositePort](ICompositeInEvents* listener) {
                listener->OnCompositeInVideoModeUpdate(compositePort,videoResolution);
            });
        } else {
            INT_ERROR("Invalid data received for Composite Video Mode Update in iarmCompositeInVideoModeUpdateHandler");
        }
    );
}

private:
    static constexpr EventHandlerMapping handlers[] = {
        { IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG,                  &IARMGroupComposite::iarmCompositeInHotPlugHandler          },
        { IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS,            &IARMGroupComposite::iarmCompositeInSignalStatusHandler     },
        { IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS,                   &IARMGroupComposite::iarmCompositeInStatusHandler           },
        { IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE,        &IARMGroupComposite::iarmCompositeInVideoModeUpdateHandler  },
    };
};


// static data
std::mutex IarmHostImpl::s_mutex;
IarmHostImpl::CallbackList<IVideoDeviceEvents*, IARMGroupVideoDevice> IarmHostImpl::s_videoDeviceListeners;
IarmHostImpl::CallbackList<IVideoOutputPortEvents*, IARMGroupVideoPort> IarmHostImpl::s_videoPortListeners;
IarmHostImpl::CallbackList<IAudioOutputPortEvents*, IARMGroupAudioPort> IarmHostImpl::s_audioPortListeners;
IarmHostImpl::CallbackList<ICompositeInEvents*, IARMGroupComposite> IarmHostImpl::s_compositeListeners;

IarmHostImpl::~IarmHostImpl()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    s_videoDeviceListeners.Release();
    s_videoPortListeners.Release();
    s_audioPortListeners.Release();
	s_compositeListeners.Release();
}

template <typename T, typename F>
/* static */ void IarmHostImpl::Dispatch(const std::list<T*>& listeners, F&& fn)
{
    std::stringstream ss;
    std::lock_guard<std::mutex> lock(s_mutex);

    for (auto* listener : listeners) {
        auto start = std::chrono::steady_clock::now();

        fn(listener);

        auto end     = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        ss << "\t client =" << listener << ", elapsed = " << elapsed.count() << " ms\n";
    }

    INT_INFO("%s Dispatch done to %zu listeners\n%s", typeid(T).name(), listeners.size(), ss.str().c_str());
}

uint32_t IarmHostImpl::Register(IVideoDeviceEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_videoDeviceListeners.Register(listener);
}

uint32_t IarmHostImpl::UnRegister(IVideoDeviceEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_videoDeviceListeners.UnRegister(listener);
}

// Dispatcher for IARMGroupVideoDevice
/* static */ void IarmHostImpl::Dispatch(std::function<void(IVideoDeviceEvents* listener)>&& fn)
{
    Dispatch(s_videoDeviceListeners, std::move(fn));
}

uint32_t IarmHostImpl::Register(IVideoOutputPortEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_videoPortListeners.Register(listener);
}

uint32_t IarmHostImpl::UnRegister(IVideoOutputPortEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_videoPortListeners.UnRegister(listener);
}

// Dispatcher for IARMGroupVideoPort
/* static */ void IarmHostImpl::Dispatch(std::function<void(IVideoOutputPortEvents* listener)>&& fn)
{
    Dispatch(s_videoPortListeners, std::move(fn));
}

uint32_t IarmHostImpl::Register(IAudioOutputPortEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_audioPortListeners.Register(listener);
}

uint32_t IarmHostImpl::UnRegister(IAudioOutputPortEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_audioPortListeners.UnRegister(listener);
}

// Dispatcher for IARMGroupAudioPort
/* static */ void IarmHostImpl::Dispatch(std::function<void(IAudioOutputPortEvents* listener)>&& fn)
{
    Dispatch(s_audioPortListeners, std::move(fn));
}

uint32_t IarmHostImpl::Register(ICompositeInEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_compositeListeners.Register(listener);
}

uint32_t IarmHostImpl::UnRegister(ICompositeInEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_compositeListeners.UnRegister(listener);
}

// Dispatcher for IARMGroupComposite
/* static */ void IarmHostImpl::Dispatch(std::function<void(ICompositeInEvents* listener)>&& fn)
{
    Dispatch(s_compositeListeners, std::move(fn));
}

} // namespace device
