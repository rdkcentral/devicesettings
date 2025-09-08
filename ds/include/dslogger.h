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
* @defgroup ds
* @{
**/


#ifndef _DS_LOGGER_H_
#define _DS_LOGGER_H_

#include <cstring>

#include "dsregisterlog.h"

int ds_log(int priority, const char* fileName, int lineNum, const char *format, ...);

#define INFO_LEVEL   0
#define WARN_LEVEL   1
#define ERROR_LEVEL  2
#define DEBUG_LEVEL  3

// Helper to extract filename from full path
// E.g. "/path/to/file.cpp" -> "file.cpp"
// IMPORTANT: This will work for Unix style paths only
static inline const char* fileName(const char* path) {
    const char* slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

#ifndef DS_LOG_LEVEL
#define DS_LOG_LEVEL ERROR_LEVEL
#endif

#define INT_INFO(FORMAT, ...)       ds_log(INFO_LEVEL, fileName(__FILE__), __LINE__, FORMAT,  ##__VA_ARGS__ )
#define INT_WARN(FORMAT, ...)       ds_log(WARN_LEVEL, fileName(__FILE__), __LINE__, FORMAT,  ##__VA_ARGS__ )
#define INT_ERROR(FORMAT, ...)      ds_log(ERROR_LEVEL, fileName(__FILE__), __LINE__, FORMAT,  ##__VA_ARGS__ )

// conditionally enable debug logs, based on DS_LOG_LEVEL
#if DS_LOG_LEVEL >= DEBUG_LEVEL
#define INT_DEBUG(FORMAT, ...)      ds_log(DEBUG_LEVEL, fileName(__FILE__), __LINE__, FORMAT,  ##__VA_ARGS__ )
#else
#define INT_DEBUG(FORMAT, ...)      do {} while(0)
#endif

#endif



/** @} */
/** @} */
