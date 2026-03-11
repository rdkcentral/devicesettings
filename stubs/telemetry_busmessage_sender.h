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
 * @file telemetry_busmessage_sender.h
 * @brief Stub for native/CI builds. Delegates to the T2 mock from
 *        entservices-testframework (Tests/mocks/Telemetry.h).
 *        Production (Yocto) builds use the real header supplied by the
 *        telemetry package via DEPENDS = " telemetry" in the .bb recipe.
 */

#ifndef __TELEMETRY_BUSMESSAGE_SENDER_H__
#define __TELEMETRY_BUSMESSAGE_SENDER_H__

/* Pull in T2ERROR enum + t2_init / t2_uninit / t2_event_s / t2_event_d
 * function-pointer declarations from the shared testframework mock. */
#include "Telemetry.h"

#endif /* __TELEMETRY_BUSMESSAGE_SENDER_H__ */
