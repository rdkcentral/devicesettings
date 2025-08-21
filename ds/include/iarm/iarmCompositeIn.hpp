
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


#ifndef _DS_IARM_COMPOSITEIN_HPP_
#define _DS_IARM_COMPOSITEIN_HPP_

#include <stdint.h>
#include "dsTypes.h"
#include <vector>


#pragma once
#include "compositeIn.hpp"
#include "dsMgr.h"
#include "libIARM.h"
#include "libIBus.h"
#include <mutex>
#include <vector>

class IarmCompositeInput  {
public:
    IarmCompositeInput();
    ~IarmCompositeInput();

    uint32_t Register(device::CompositeInput::IEvent* listener);
    uint32_t UnRegister(device::CompositeInput::IEvent* listener);

private:
    // Event Handlers
    static void OnCompositeInHotPlugHandler(const char* owner, IARM_EventId_t eventId, void *data, size_t len);
    static void OnCompositeInSignalStatusHandler(const char* owner, IARM_EventId_t eventId, void *data, size_t len);
    static void OnCompositeInStatusHandler(const char* owner, IARM_EventId_t eventId, void *data, size_t len);
    static void OnCompositeInVideoModeUpdateHandler(const char* owner, IARM_EventId_t eventId, void *data, size_t len);

    template<typename F>
    static void Dispatch(F&& fn);

    struct EventHandlerMapping {
        IARM_EventId_t eventId;
        IARM_EventHandler_t handler;
    };

    static const EventHandlerMapping eventHandlers[];
    static const char* OWNER_NAME;

    static std::mutex s_mutex;
    static std::vector<device::CompositeInput::IEvent*> compInListener;
};

#endif /* _DS_IARM_COMPOSITEIN_HPP_ */


/** @} */
/** @} */
