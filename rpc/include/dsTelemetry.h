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

#ifndef __DS_TELEMETRY_H__
#define __DS_TELEMETRY_H__

/**
* @defgroup devicesettings
* @{
* @defgroup dsTelemetry
* @{
**/

#include <telemetry_busmessage_sender.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TELEMENTRY_EVENT_STRING(marker, value) \
    do { \
        t2_event_s((char*)marker, (char*)value); \
    } while(0)

#define TELEMENTRY_EVENT_FLOAT(marker, value) \
    do { \
        t2_event_f((char*)marker, (double)value); \
    } while(0)

#define TELEMENTRY_EVENT_INT(marker, value) \
    do { \
        t2_event_d((char*)marker, (int)value); \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* __DS_TELEMETRY_H__ */

/**
* @}
* @}
**/
