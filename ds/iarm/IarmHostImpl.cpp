#include <algorithm>
#include <mutex>

#include "IarmHostImpl.hpp"

#include "dsMgr.h"
/*#include "dsTypes.h"*/
#include "dslogger.h"

#include "libIBus.h"

using IVideoDeviceEvents = device::Host::IVideoDeviceEvents;
using IVideoPortEvents = device::Host::IVideoPortEvents;

namespace device {

struct EventHandlerMapping {
    IARM_EventId_t eventId;
    IARM_EventHandler_t handler;
};

// IARM c to c++ shim (all methods are static)
// Thread safety to be ensured by the caller
class IarmHostPriv {

public:
    // ------------------------------------- video device events -----------------------------------
    static bool RegisterVideoDeviceEvents()
    {
        s_isVideoDeviceEventsRegistered = RegisterIarmEvents(s_videoDeviceHandlers);

        return s_isVideoDeviceEventsRegistered;
    }

    static bool UnRegisterVideoDeviceEvents()
    {
        s_isVideoDeviceEventsRegistered = !UnregisterIarmEvents(s_videoDeviceHandlers);

        return !s_isVideoDeviceEventsRegistered;
    }

    static bool isVideoDeviceEventsRegistered() { return s_isVideoDeviceEventsRegistered; }

    static void iarmDisplayFrameratePreChangeHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE received %s %d", owner, eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            std::string framerate(eventData->data.DisplayFrameRateChange.framerate, sizeof(eventData->data.DisplayFrameRateChange.framerate));

            IarmHostImpl::DispatchVideoDeviceEvents([&framerate](IVideoDeviceEvents* listener) {
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
            std::string framerate(eventData->data.DisplayFrameRateChange.framerate, sizeof(eventData->data.DisplayFrameRateChange.framerate));

            IarmHostImpl::DispatchVideoDeviceEvents([&framerate](IVideoDeviceEvents* listener) {
                listener->OnDisplayFrameratePostChange(framerate);
            });
        } else {
            INT_ERROR("Invalid data received for display framerate post-change");
        }
    }

    // ------------------------ video port events ------------------------------
    static bool RegisterVideoPortEvents()
    {
        s_isVideoPortEventsRegistered = RegisterIarmEvents(s_videoPortHandlers);
        return s_isVideoPortEventsRegistered;
    }

    static bool UnRegisterVideoPortEvents()
    {
        s_isVideoPortEventsRegistered = !UnregisterIarmEvents(s_videoPortHandlers);
        return !s_isVideoPortEventsRegistered;
    }

    static bool isVideoPortEventsRegistered() { return s_isVideoPortEventsRegistered; }

    static void iarmResolutionPreChangeHandler(const char*, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_RES_PRECHANGE received, eventId = %d", eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            int width = eventData->data.resn.width;
            int height = eventData->data.resn.height;

            IarmHostImpl::DispatchVideoPortEvents([width, height](IVideoPortEvents* listener) {
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
            int width = eventData->data.resn.width;
            int height = eventData->data.resn.height;

            IarmHostImpl::DispatchVideoPortEvents([width, height](IVideoPortEvents* listener) {
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

    static void iarmVideoFormatUpdateHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        INT_INFO("IARM_BUS_DSMGR_EVENT_VIDEO_FORMAT_UPDATE received %s %d", owner, eventId);

        IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;

        if (eventData) {
            dsHDRStandard_t videoFormat = eventData->data.VideoFormatInfo.videoFormat;

            IarmHostImpl::DispatchVideoPortEvents([videoFormat](IVideoPortEvents* listener) {
                listener->OnVideoFormatUpdate(videoFormat);
            });
        } else {
            INT_ERROR("Invalid data received for video format update");
        }
    }
    static void iarmHDCPProtocolChangeStatusHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len) { }

private:
    static bool s_isVideoDeviceEventsRegistered;
    static bool s_isVideoPortEventsRegistered;

    static constexpr EventHandlerMapping s_videoDeviceHandlers[] = {
        { IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, &IarmHostPriv::iarmDisplayFrameratePreChangeHandler },
        { IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, &IarmHostPriv::iarmDisplayFrameratePostChangeHandler },
    };

    static constexpr EventHandlerMapping s_videoPortHandlers[] = {
        { IARM_BUS_DSMGR_EVENT_RES_PRECHANGE, &IarmHostPriv::iarmResolutionPreChangeHandler },
        { IARM_BUS_DSMGR_EVENT_RES_POSTCHANGE, &IarmHostPriv::iarmResolutionPostChangeHandler },
        { IARM_BUS_DSMGR_EVENT_HDCP_STATUS, &IarmHostPriv::iarmHDCPStatusChangeHandler },
        { IARM_BUS_DSMGR_EVENT_VIDEO_FORMAT_UPDATE, &IarmHostPriv::iarmVideoFormatUpdateHandler },
        // {TODO: ?? , &IarmHostPriv::iarmHDCPProtocolChangeStatusHandler},
    };

    template <size_t N>
    static bool RegisterIarmEvents(const EventHandlerMapping (&handlers)[N])
    {
        bool subscribed = true;

        for (const auto& eh : handlers) {
            if (IARM_RESULT_SUCCESS != IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, eh.eventId, eh.handler)) {
                INT_ERROR("Failed to register IARM event handler for %d", eh.eventId);
                subscribed = false;
            }
        }

        if (!subscribed) {
            // in case of failure / partial failure
            // we should unregister any handlers that were registered
            UnregisterIarmEvents(handlers);
        }

        return subscribed;
    }

    template <size_t N>
    static bool UnregisterIarmEvents(const EventHandlerMapping (&handlers)[N])
    {
        bool unsubscribed = true;

        for (const auto& eh : handlers) {
            if (IARM_RESULT_SUCCESS != IARM_Bus_UnRegisterEventHandler(IARM_BUS_DSMGR_NAME, eh.eventId)) {
                INT_ERROR("Failed to unregister IARM event handler for %d", eh.eventId);
                unsubscribed = false;
            }
        }
        return unsubscribed;
    }
};

// static data
bool IarmHostPriv::s_isVideoDeviceEventsRegistered { false };
bool IarmHostPriv::s_isVideoPortEventsRegistered { false };

std::mutex IarmHostImpl::s_mutex;
std::list<IVideoDeviceEvents*> IarmHostImpl::s_videoDeviceListeners;
std::list<IVideoPortEvents*> IarmHostImpl::s_videoPortListeners;

IarmHostImpl::~IarmHostImpl()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    IarmHostPriv::UnRegisterVideoPortEvents();
    IarmHostPriv::UnRegisterVideoDeviceEvents();

    s_videoPortListeners.clear();
    s_videoDeviceListeners.clear();
}

uint32_t IarmHostImpl::Register(IVideoDeviceEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    // NULL check is already handled by the caller

    // Check if listener is already registered
    if (!IarmHostPriv::isVideoDeviceEventsRegistered()) {
        if (!IarmHostPriv::RegisterVideoDeviceEvents()) {
            INT_ERROR("Failed to subscribe to IARM Video device events");
            return 1; // Error: Failed to register video device events
        }
    }

    auto it = std::find(s_videoDeviceListeners.begin(), s_videoDeviceListeners.end(), listener);
    if (it != s_videoDeviceListeners.end()) {
        // Listener already registered
        INT_ERROR("IVideoDeviceEvent %p is already registered", listener);
        return 0;
    }

    s_videoDeviceListeners.push_back(listener);
    INT_INFO("IVideoDeviceEvent %p registered", listener);
    return 0;
}

template <typename T>
uint32_t IarmHostImpl::Register(std::list<T*>& listeners, T* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    // NULL check is already handled by the caller

    // Check if listener is already registered
    auto it = std::find(listeners.begin(), listeners.end(), listener);
    if (it != listeners.end()) {
        // Listener already registered
        INT_ERROR("%p is already registered", listener);
        return 0;
    }

    listeners.push_back(listener);
    INT_INFO("%p registered", listener);
    return 0;
}

uint32_t IarmHostImpl::UnRegister(IVideoDeviceEvents* listener)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    auto it = std::find(s_videoDeviceListeners.begin(), s_videoDeviceListeners.end(), listener);

    if (it == s_videoDeviceListeners.end()) {
        // Listener not found
        INT_ERROR("IVideoDeviceEvent %p is not registered", listener);
        return 1; // Error: Listener not found
    }

    s_videoDeviceListeners.erase(it);
    INT_INFO("IVideoDeviceEvent %p unregistered", listener);

    if (s_videoDeviceListeners.empty()) {
        // No more listeners, unregister from IARM
        if (!IarmHostPriv::UnRegisterVideoDeviceEvents()) {
            INT_ERROR("Failed to unsubscribe from IARM Video device events");
        }
    }

    return 0;
}

template <typename F>
void IarmHostImpl::DispatchVideoDeviceEvents(F&& fn)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    for (auto* listener : s_videoDeviceListeners) {
        fn(listener);
    }
}

template <typename F>
void IarmHostImpl::DispatchVideoPortEvents(F&& fn)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    for (auto* listener : s_videoPortListeners) {
        fn(listener);
    }
}

uint32_t IarmHostImpl::Register(IVideoPortEvents* listener)
{
    return 0;
}

uint32_t IarmHostImpl::UnRegister(IVideoPortEvents* listener)
{
    return 0;
}
} // namespace device
