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

#include <cstdio>
#include <cstdarg>

#include "dslogger.h"

#include <sys/syscall.h>   // for SYS_gettid
#include <unistd.h>        // for syscall

#define unlikely(x) (__builtin_expect(!!(x), 0))
#define MAX_LOG_BUFF 1024
#define kFormatMessageSize (MAX_LOG_BUFF - 128)

DS_LogCb logCb = NULL;

void DS_RegisterForLog(DS_LogCb cb)
{
    logCb = cb;
}

int ds_log(LogLevel priority, const char* fileName, int lineNum, const char *func, const char *format, ...)
{
    char formatted[MAX_LOG_BUFF] = {'\0'};
    enum LogLevel {INFO_LEVEL = 0, WARN_LEVEL, ERROR_LEVEL, DEBUG_LEVEL, TRACE_LEVEL};
    const char *levelMap[] = { "INFO", "WARN", "ERROR", "DEBUG", "TRACE"};

    if (!func || !fileName || !format)
    {
        return -1;
    }

    va_list argptr;
    va_start(argptr, format);
    vsnprintf(formatted, kFormatMessageSize, format, argptr);
    va_end(argptr);

    if (nullptr != logCb) {
        logCb(priority, formatted);
    } else {
        fprintf(stderr, "[DS][%d] %s [%s:%d] %s: %s \n",
                (int)syscall(SYS_gettid),
                levelMap[static_cast<int>(priority)],
                fileName,
                lineNum,
                func,
                formatted);
        fflush(stderr);
    }
    return 0;
}

/** @} */
/** @} */
