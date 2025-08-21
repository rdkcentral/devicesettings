/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 2025 Management
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

#include "dsMgr.h"
#include "dsRpc.h"
#include "dslogger.h"

#include "iarm/VideoDevicePriv.h"

using IEvent = device::VideoDevice::IEvent;

namespace iarm {

VideoDevicePriv::EventHandlerMapping VideoDevicePriv::eventHandlers[] = {
    {IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE,
     &VideoDevicePriv::OnDisplayFrameratePreChangeHandler},
    {IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE,
     &VideoDevicePriv::OnDisplayFrameratePostChangeHandler},
};

const char *VideoDevicePriv::to_string(IARM_EventId_t eventId) {
  switch (eventId) {
  case IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE:
    return "IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE";
  case IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE:
    return "IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE";
  default:
    return "Unknown Event";
  }
}

VideoDevicePriv::VideoDevicePriv() {}

VideoDevicePriv::~VideoDevicePriv() {}

void VideoDevicePriv::SubscribeForIarmEvents() {
  IARM_Result_t status = IARM_RESULT_INVALID_STATE;
  for (const auto &eh : eventHandlers) {
    status = IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, eh.eventId,
                                           eh.handler);
    if (IARM_RESULT_SUCCESS != status) {
      INT_ERROR("Failed to register event handler for %s, status: %d",
                to_string(eh.eventId), status);
    }
  }
}

VideoDevicePriv::OnDisplayFrameratePreChangeHandler(const char *,
                                                    IARM_EventId_t eventId,
                                                    void *data, size_t len) {
  Dispatch([&](IEvent *listener) {
    listener->OnDisplayFrameratePreChangeHandler(frameRate);
  });
}

uint32_t VideoDevicePriv::Register(IEvent *listener) {
  {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = std::find(s_listeners.begin(), s_listeners.end(), listener);

    bool found = (it != s_listeners.end());

    if (found) {
      // Listener already registered, no need to add again
      return 0;
    }

    if (it != s_listeners.end()) {
      // Listener already registered, no need to add again
      return 0;
    }
    s_listeners.push_back(listener);
  }
  return 0;
}

uint32_t VideoDevicePriv::UnRegister(IEvent *listener) {
  auto it = std::find(s_listeners.begin(), s_listeners.end(), listener);

  if (it != s_listeners.end()) {
    s_listeners.erase(it);
  } else {
  }

  return 0;
}

} // namespace iarm
