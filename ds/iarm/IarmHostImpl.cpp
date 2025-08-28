#include <chrono>
#include <mutex>
#include <sstream>
#include <cstring>

#include "IarmHostImpl.hpp"

#include "dsMgr.h"
#include "dslogger.h"

#include "libIBus.h"

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

inline bool isValidOwner(const char* owner)
{
    if (std::string(IARM_BUS_DSMGR_NAME) != std::string(owner)) {
        INT_ERROR("Invalid owner %s, expected %s", owner, IARM_BUS_DSMGR_NAME);
        return false;
    }
    return true;
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
        INT_INFO("IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

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
        INT_INFO("IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

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

    static void iarmZoomSettingsChangedHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_ZOOM_SETTINGS received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            dsVideoZoom_t zoomSetting = static_cast<dsVideoZoom_t>(eventData->data.dfc.zoomsettings);

            IarmHostImpl::Dispatch([zoomSetting](IVideoDeviceEvents* listener) {
                listener->OnZoomSettingsChanged(zoomSetting);
            });
        } else {
            INT_ERROR("Invalid data received for zoom settings change");
        }
    };

    static constexpr EventHandlerMapping handlers[] = {
        { IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE,  &IARMGroupVideoDevice::iarmDisplayFrameratePreChangeHandler  },
        { IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, &IARMGroupVideoDevice::iarmDisplayFrameratePostChangeHandler },
        { IARM_BUS_DSMGR_EVENT_ZOOM_SETTINGS,               &IARMGroupVideoDevice::iarmZoomSettingsChangedHandler        },
    };
};

class IARMGroupVideoOutputPort {
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
    static void iarmResolutionPreChangeHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_RES_PRECHANGE received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

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

    static void iarmResolutionPostChangeHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_RES_POSTCHANGE received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

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

    static void iarmHDCPStatusChangeHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_HDCP_STATUS received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            dsHdcpStatus_t hdcpStatus = static_cast<dsHdcpStatus_t>(eventData->data.hdmi_hdcp.hdcpStatus);
            IarmHostImpl::Dispatch([hdcpStatus](IVideoOutputPortEvents* listener) {
                listener->OnHDCPStatusChange(hdcpStatus);
            });
        } else {
            INT_ERROR("Invalid data received for HDCP status change");
        }
    }

    static void iarmVideoFormatUpdateHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_VIDEO_FORMAT_UPDATE received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

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
        { IARM_BUS_DSMGR_EVENT_RES_PRECHANGE,       &IARMGroupVideoOutputPort::iarmResolutionPreChangeHandler  },
        { IARM_BUS_DSMGR_EVENT_RES_POSTCHANGE,      &IARMGroupVideoOutputPort::iarmResolutionPostChangeHandler },
        { IARM_BUS_DSMGR_EVENT_HDCP_STATUS,         &IARMGroupVideoOutputPort::iarmHDCPStatusChangeHandler     },
        { IARM_BUS_DSMGR_EVENT_VIDEO_FORMAT_UPDATE, &IARMGroupVideoOutputPort::iarmVideoFormatUpdateHandler    },
    };
};

class IARMGroupAudioOutputPort {
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
    static void iarmAssociatedAudioMixingChangedHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_ASSOCIATED_AUDIO_MIXING_CHANGED received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

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

    static void iarmAudioFaderControlChangedHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_FADER_CONTROL_CHANGED received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

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

    static void iarmAudioPrimaryLanguageChangedHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_PRIMARY_LANGUAGE_CHANGED received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

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

    static void iarmAudioSecondaryLanguageChangedHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_SECONDARY_LANGUAGE_CHANGED received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

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

    static void iarmAudioOutHotPlugHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_OUT_HOTPLUG received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

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

    static void iarmDolbyAtmosCapabilitiesChangedHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_ATMOS_CAPS_CHANGED received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

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
    static void iarmAudioPortStateChangedHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_PORT_STATE received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

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

    static void iarmAudioModeEventHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_MODE received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

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

    static void iarmAudioLevelChangedEventHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_LEVEL_CHANGED received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            int audioLevel = eventData->data.AudioLevelInfo.level;

            IarmHostImpl::Dispatch([audioLevel](IAudioOutputPortEvents* listener) {
                listener->OnAudioLevelChangedEvent(audioLevel);
            });
        } else {
            INT_ERROR("Invalid data received for audio level change");
        }
    };

    static void iarmAudioFormatUpdateHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_AUDIO_FORMAT_UPDATE received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

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
        { IARM_BUS_DSMGR_EVENT_AUDIO_ASSOCIATED_AUDIO_MIXING_CHANGED, &IARMGroupAudioOutputPort::iarmAssociatedAudioMixingChangedHandler  },
        { IARM_BUS_DSMGR_EVENT_AUDIO_FADER_CONTROL_CHANGED,           &IARMGroupAudioOutputPort::iarmAudioFaderControlChangedHandler      },
        { IARM_BUS_DSMGR_EVENT_AUDIO_PRIMARY_LANGUAGE_CHANGED,        &IARMGroupAudioOutputPort::iarmAudioPrimaryLanguageChangedHandler   },
        { IARM_BUS_DSMGR_EVENT_AUDIO_SECONDARY_LANGUAGE_CHANGED,      &IARMGroupAudioOutputPort::iarmAudioSecondaryLanguageChangedHandler },
        { IARM_BUS_DSMGR_EVENT_AUDIO_OUT_HOTPLUG,                     &IARMGroupAudioOutputPort::iarmAudioOutHotPlugHandler               },
        { IARM_BUS_DSMGR_EVENT_ATMOS_CAPS_CHANGED,                    &IARMGroupAudioOutputPort::iarmDolbyAtmosCapabilitiesChangedHandler },
        { IARM_BUS_DSMGR_EVENT_AUDIO_PORT_STATE,                      &IARMGroupAudioOutputPort::iarmAudioPortStateChangedHandler         },
        { IARM_BUS_DSMGR_EVENT_AUDIO_MODE,                            &IARMGroupAudioOutputPort::iarmAudioModeEventHandler                },
        { IARM_BUS_DSMGR_EVENT_AUDIO_LEVEL_CHANGED,                   &IARMGroupAudioOutputPort::iarmAudioLevelChangedEventHandler        },
        { IARM_BUS_DSMGR_EVENT_AUDIO_FORMAT_UPDATE,                   &IARMGroupAudioOutputPort::iarmAudioFormatUpdateHandler             },
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
    static void iarmCompositeInHotPlugHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    { 
        INT_INFO("IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (eventData) {
            dsCompositeInPort_t compositePort = eventData->data.composite_in_connect.port;
            bool isConnected = eventData->data.composite_in_connect.isPortConnected;
            IarmHostImpl::Dispatch([compositePort, isConnected](ICompositeInEvents* listener) {
                 listener->OnCompositeInHotPlug(compositePort,isConnected);
            });
        } else {
            INT_ERROR("Invalid data received for Composite Status Handler in iarmCompositeInHotPlugHandler");
        }
    }

    static void iarmCompositeInSignalStatusHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }
        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (eventData) {
            dsCompositeInPort_t compositePort = eventData->data.composite_in_sig_status.port;
            dsCompInSignalStatus_t compositeSigStatus = eventData->data.composite_in_sig_status.status;
            IarmHostImpl::Dispatch([compositePort,compositeSigStatus](ICompositeInEvents* listener) {
                listener->OnCompositeInSignalStatus(compositePort,compositeSigStatus);
            });
        } else {
            INT_ERROR("Invalid data received for Composite Status Handler in iarmCompositeInSignalStatusHandler");
        }
    }

    static void iarmCompositeInStatusHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }
	IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (eventData) {
            dsCompositeInPort_t compositePort = eventData->data.composite_in_status.port;
            bool isPresented = eventData->data.composite_in_status.isPresented;
	    IarmHostImpl::Dispatch([compositePort,isPresented](ICompositeInEvents* listener) {
            listener->OnCompositeInStatus(compositePort,isPresented);
            });
        } else {
            INT_ERROR("Invalid data received for Composite Status Handler in iarmCompositeInStatusHandler");
        }
    }


    static void iarmCompositeInVideoModeUpdateHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE received owner = %s, eventId = %d", owner, eventId);
        if (!isValidOwner(owner)) {
            return;
        }
        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (eventData) {
            dsCompositeInPort_t compositePort = eventData->data.composite_in_video_mode.port;
                        dsVideoPortResolution_t videoResolution{};
                        memset(videoResolution.name, 0, sizeof(videoResolution.name));
                        videoResolution.pixelResolution = eventData->data.composite_in_video_mode.resolution.pixelResolution;
                        videoResolution.interlaced = eventData->data.composite_in_video_mode.resolution.interlaced;
                        videoResolution.frameRate = eventData->data.composite_in_video_mode.resolution.frameRate;
                        IarmHostImpl::Dispatch([compositePort,videoResolution](ICompositeInEvents* listener) {
                             listener->OnCompositeInVideoModeUpdate(compositePort,videoResolution);
            });
        } else {
            INT_ERROR("Invalid data received for Composite Video Mode Update in iarmCompositeInVideoModeUpdateHandler");
        }
     }

private:
    static constexpr EventHandlerMapping handlers[] = {
        { IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_HOTPLUG,                  &IARMGroupComposite::iarmCompositeInHotPlugHandler          },
        { IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_SIGNAL_STATUS,            &IARMGroupComposite::iarmCompositeInSignalStatusHandler     },
        { IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_STATUS,                   &IARMGroupComposite::iarmCompositeInStatusHandler           },
        { IARM_BUS_DSMGR_EVENT_COMPOSITE_IN_VIDEO_MODE_UPDATE,        &IARMGroupComposite::iarmCompositeInVideoModeUpdateHandler  },
    };
};


class IARMGroupDisplay {
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
    static void iarmDisplayDisplayRxSense(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_RX_SENSE received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }
        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (eventData) {
            dsDisplayEvent_t displayStatusEvent = static_cast<dsDisplayEvent_t>(eventData->data.hdmi_rxsense.status);
            IarmHostImpl::Dispatch([displayStatusEvent](IDisplayEvents* listener) {
            listener->OnDisplayRxSense(displayStatusEvent);
            });
        } else {
            INT_ERROR("Invalid data received for Composite Status Handler in iarmDisplayDisplayRxSense");
        }
    }

    static void iarmDisplayHDCPStatusChange(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_HDCP_STATUS received owner = %s, eventId = %d", owner, eventId);
        if (!isValidOwner(owner)) {
            return;
        }
        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (eventData) {
            dsHdcpStatus_t hdcpStatus = static_cast<dsHdcpStatus_t>(eventData->data.hdmi_hdcp.hdcpStatus);
            IarmHostImpl::Dispatch([hdcpStatus](IDisplayEvents* listener) {
                /* To check Parameter Required or Not*/
		listener->OnDisplayHDCPStatus();
            });
        } else {
            INT_ERROR("Invalid data received for Composite Video Mode Update in iarmDisplayHDCPStatusChange");
        }
     }

private:
    static constexpr EventHandlerMapping handlers[] = {
        { IARM_BUS_DSMGR_EVENT_RX_SENSE,                  &IARMGroupDisplay::iarmDisplayDisplayRxSense       },
        { IARM_BUS_DSMGR_EVENT_HDCP_STATUS,               &IARMGroupDisplay::iarmDisplayHDCPStatusChange     },
    };
};

class IARMGroupDisplayDevice {
public:
    static bool RegisterIarmEvents()
    {
        IARM_Result_t result = IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG,
            &IARMGroupDisplayDevice::iarmDisplayHDMIHotPlugHandler);

        if (result != IARM_RESULT_SUCCESS) {
            INT_ERROR("Failed to register IARM event handler for IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG");
        }
        return (result == IARM_RESULT_SUCCESS);
    }

    static bool UnRegisterIarmEvents()
    {
        IARM_Result_t result = IARM_Bus_UnRegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG);
        if (result != IARM_RESULT_SUCCESS) {
            INT_ERROR("Failed to unregister IARM event handler for IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG");
        }
        return (result == IARM_RESULT_SUCCESS);
    }

private:
    static void iarmDisplayHDMIHotPlugHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            dsDisplayEvent_t displayEvent = static_cast<dsDisplayEvent_t>(eventData->data.hdmi_hpd.event);

            IarmHostImpl::Dispatch([displayEvent](IDisplayDeviceEvents* listener) {
                listener->OnDisplayHDMIHotPlug(displayEvent);
            });
        } else {
            INT_ERROR("Invalid data received for HDMI (out) hot plug change");
        }
    }
}; // IARMGroupDisplayDevice

class IARMGroupHdmiIn {
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
    static void iarmHDMIInEventHotPlugHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            dsHdmiInPort_t port = eventData->data.hdmi_in_connect.port);
            bool isConnected = eventData->data.hdmi_in_connect.isPortConnected;

            IarmHostImpl::Dispatch([port, isConnected](IHDMIInEvents* listener) {
                listener->OnHDMIInEventHotPlug(port, isConnected);
            });
        } else {
            INT_ERROR("Invalid data received for HdmiIn hot plug");
        }
    };

    static void iarmHDMIInEventSignalStatusHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            dsHdmiInPort_t port = eventData->data.hdmi_in_sig_status.port;
            dsHdmiInSignalStatus_t sigStatus = eventData->data.hdmi_in_sig_status.status;

            IarmHostImpl::Dispatch([port, sigStatus](IHDMIInEvents* listener) {
                listener->OnHDMIInEventSignalStatus(port, sigStatus);
            });
        } else {
            INT_ERROR("Invalid data received for HdmiIn signal status");
        }
    };

    static void iarmHDMIInEventStatusHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            dsHdmiInPort_t activePort = eventData->data.hdmi_in_status.port;
            bool isPresented = eventData->data.hdmi_in_status.isPresented;

            IarmHostImpl::Dispatch([activePort, isPresented](IHDMIInEvents* listener) {
                listener->OnHDMIInEventStatus(activePort, isPresented);
            });
        } else {
            INT_ERROR("Invalid data received for HdmiIn event status");
        }
    };

    static void iarmHDMIInVideoModeUpdateHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            dsHdmiInPort_t port = eventData->data.hdmi_in_video_mode.port;
            dsVideoPortResolution_t& res;
            res.pixelResolution = eventData->data.hdmi_in_video_mode.resolution.pixelResolution;
            res.interlaced = eventData->data.hdmi_in_video_mode.resolution.interlaced;
            res.frameRate = eventData->data.hdmi_in_video_mode.resolution.frameRate;

            IarmHostImpl::Dispatch([port, &res](IHDMIInEvents* listener) {
                listener->OnHDMIInVideoModeUpdate(port, res);
            });
        } else {
            INT_ERROR("Invalid data received for HdmiIn video mode update");
        }
    };

    static void iarmHDMIInAllmStatusHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_HDMI_IN_ALLM_STATUS received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (eventData) {
            dsHdmiInPort_t port = eventData->data.hdmi_in_allm_mode.port;
            bool allmStatus = eventData->data.hdmi_in_allm_mode.allm_mode;

            IarmHostImpl::Dispatch([port, allmStatus](IHDMIInEvents* listener) {
                listener->OnHDMIInAllmStatus(port, allmStatus);
            });
        } else {
            INT_ERROR("Invalid data received for HdmiIn allm status");
        }
    };

    static void iarmHDMIInVRRStatusHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_HDMI_IN_VRR_STATUS received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
        if (eventData) {
            dsHdmiInPort_t port = eventData->data.hdmi_in_vrr_mode.port;
            dsVRRType_t vrrType = eventData->data.hdmi_in_vrr_mode.vrr_type;

            IarmHostImpl::Dispatch([port, vrrType](IHDMIInEvents* listener) {
                listener->OnHDMIInVRRStatus(port, vrrType);
            });

        } else {
            INT_ERROR("Invalid data received for HdmiIn vrr status");
        }
    };

    static void iarmHDMIInAVIContentTypeHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_HDMI_IN_AVI_CONTENT_TYPE received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            dsHdmiInPort_t port = eventData->data.hdmi_in_content_type.port;
            dsAviContentType_t type = eventData->data.hdmi_in_content_type.aviContentType;

            IarmHostImpl::Dispatch([port, type](IHDMIInEvents* listener) {
                listener->OnHDMIInAVIContentType(port, type);
            });
        } else {
            INT_ERROR("Invalid data received for HdmiIn avi content type");
        }
    };

    static void iarmHDMIInAVLatencyHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_HDMI_IN_AV_LATENCY received owner = %s, eventId = %d", owner, eventId);

        if (!isValidOwner(owner)) {
            return;
        }

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            int audioDelay = eventData->data.hdmi_in_av_latency.audio_output_delay;
            int videoDelay = eventData->data.hdmi_in_av_latency.video_latency;

            IarmHostImpl::Dispatch([audioDelay, videoDelay](IHDMIInEvents* listener) {
                listener->OnHDMIInAVLatency(audioDelay, videoDelay);
            });
        } else {
            INT_ERROR("Invalid data received for HdmiIn av latency");
        }
    };

private:
    static constexpr EventHandlerMapping handlers[] = {
        { IARM_BUS_DSMGR_EVENT_HDMI_IN_HOTPLUG,                 &IARMGroupHdmiIn::iarmHDMIInEventHotPlugHandler },
        { IARM_BUS_DSMGR_EVENT_HDMI_IN_SIGNAL_STATUS,           &IARMGroupHdmiIn::iarmHDMIInEventSignalStatusHandler },
        { IARM_BUS_DSMGR_EVENT_HDMI_IN_STATUS,                  &IARMGroupHdmiIn::iarmHDMIInEventStatusHandler },
        { IARM_BUS_DSMGR_EVENT_HDMI_IN_VIDEO_MODE_UPDATE,       &IARMGroupHdmiIn::iarmHDMIInVideoModeUpdateHandler },
        { IARM_BUS_DSMGR_EVENT_HDMI_IN_ALLM_STATUS,             &IARMGroupHdmiIn::iarmHDMIInAllmStatusHandler },
        { IARM_BUS_DSMGR_EVENT_HDMI_IN_VRR_STATUS,              &IARMGroupHdmiIn::iarmHDMIInVRRStatusHandler },
        { IARM_BUS_DSMGR_EVENT_HDMI_IN_AVI_CONTENT_TYPE,        &IARMGroupHdmiIn::iarmHDMIInAVIContentTypeHandler },
        { IARM_BUS_DSMGR_EVENT_HDMI_IN_AV_LATENCY,              &IARMGroupHdmiIn::iarmHDMIInAVLatencyHandler }
    };
}; // IARMGroupHdmiIn

// static data
constexpr EventHandlerMapping IARMGroupHdmiIn::handlers[];
constexpr EventHandlerMapping IARMGroupVideoDevice::handlers[];
constexpr EventHandlerMapping IARMGroupVideoOutputPort::handlers[];
constexpr EventHandlerMapping IARMGroupAudioOutputPort::handlers[];
constexpr EventHandlerMapping IARMGroupComposite::handlers[];
constexpr EventHandlerMapping IARMGroupDisplay::handlers[];

std::mutex IarmHostImpl::s_mutex;
IarmHostImpl::CallbackList<IHDMIInEvents*, IARMGroupHdmiIn> IarmHostImpl::s_hdmiInListeners;
IarmHostImpl::CallbackList<IVideoDeviceEvents*, IARMGroupVideoDevice> IarmHostImpl::s_videoDeviceListeners;
IarmHostImpl::CallbackList<IVideoDeviceEvents*, IARMGroupVideoDevice> IarmHostImpl::s_videoDeviceListeners;
IarmHostImpl::CallbackList<IVideoOutputPortEvents*, IARMGroupVideoOutputPort> IarmHostImpl::s_videoOutputPortListeners;
IarmHostImpl::CallbackList<IAudioOutputPortEvents*, IARMGroupAudioOutputPort> IarmHostImpl::s_audioOutputPortListeners;
IarmHostImpl::CallbackList<ICompositeInEvents*, IARMGroupComposite> IarmHostImpl::s_compositeListeners;
IarmHostImpl::CallbackList<IDisplayEvents*, IARMGroupDisplay> IarmHostImpl::s_displayListeners;
IarmHostImpl::CallbackList<IDisplayDeviceEvents*, IARMGroupDisplayDevice> IarmHostImpl::s_displayDeviceListeners;

IarmHostImpl::~IarmHostImpl()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    s_hdmiInListeners.Release();
    s_videoDeviceListeners.Release();
    s_videoOutputPortListeners.Release();
    s_audioOutputPortListeners.Release();
    s_compositeListeners.Release();
    s_displayListeners.Release();
    s_displayDeviceListeners.Release();
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

dsError_t IarmHostImpl::Register(IHDMIInEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_hdmiInListeners.Register(listener);
}

dsError_t IarmHostImpl::UnRegister(IHDMIInEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_hdmiInListeners.UnRegister(listener);
}

// Dispatcher for IHDMIInEvents
/* static */ void IarmHostImpl::Dispatch(std::function<void(IHDMIInEvents* listener)>&& fn)
{
    Dispatch(s_hdmiInListeners, std::move(fn));
}

dsError_t IarmHostImpl::Register(IVideoDeviceEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_videoDeviceListeners.Register(listener);
}

dsError_t IarmHostImpl::UnRegister(IVideoDeviceEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_videoDeviceListeners.UnRegister(listener);
}

// Dispatcher for IARMGroupVideoDevice
/* static */ void IarmHostImpl::Dispatch(std::function<void(IVideoDeviceEvents* listener)>&& fn)
{
    Dispatch(s_videoDeviceListeners, std::move(fn));
}

dsError_t IarmHostImpl::Register(IVideoOutputPortEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_videoOutputPortListeners.Register(listener);
}

dsError_t IarmHostImpl::UnRegister(IVideoOutputPortEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_videoOutputPortListeners.UnRegister(listener);
}

// Dispatcher for IVideoOutputPortEvents
/* static */ void IarmHostImpl::Dispatch(std::function<void(IVideoOutputPortEvents* listener)>&& fn)
{
    Dispatch(s_videoOutputPortListeners, std::move(fn));
}

dsError_t IarmHostImpl::Register(IAudioOutputPortEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_audioOutputPortListeners.Register(listener);
}

dsError_t IarmHostImpl::UnRegister(IAudioOutputPortEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_audioOutputPortListeners.UnRegister(listener);
}

// Dispatcher for IAudioOutputPortEvents
/* static */ void IarmHostImpl::Dispatch(std::function<void(IAudioOutputPortEvents* listener)>&& fn)
{
    Dispatch(s_audioOutputPortListeners, std::move(fn));
}

dsError_t  IarmHostImpl::Register(ICompositeInEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_compositeListeners.Register(listener);
}

dsError_t  IarmHostImpl::UnRegister(ICompositeInEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_compositeListeners.UnRegister(listener);
}

// Dispatcher for IARMGroupComposite
/* static */ void IarmHostImpl::Dispatch(std::function<void(ICompositeInEvents* listener)>&& fn)
{
    Dispatch(s_compositeListeners, std::move(fn));
}

dsError_t  IarmHostImpl::Register(IDisplayEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_displayListeners.Register(listener);
}

dsError_t  IarmHostImpl::UnRegister(IDisplayEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_displayListeners.UnRegister(listener);
}

// Dispatcher for IARMGroupComposite
/* static */ void IarmHostImpl::Dispatch(std::function<void(IDisplayEvents* listener)>&& fn)
{
    Dispatch(s_displayListeners, std::move(fn));
}
dsError_t IarmHostImpl::Register(IDisplayDeviceEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_displayDeviceListeners.Register(listener);
}

dsError_t IarmHostImpl::UnRegister(IDisplayDeviceEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_displayDeviceListeners.UnRegister(listener);
}

// Dispatcher for IDisplayDeviceEvents
/* static */ void IarmHostImpl::Dispatch(std::function<void(IDisplayDeviceEvents* listener)>&& fn)
{
    Dispatch(s_displayDeviceListeners, std::move(fn));
}

} // namespace device
