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


#include "dslogger.h"
#include <stdarg.h>

#define MAX_LOG_BUFF 500

DS_LogCb logCb = NULL;

void DS_RegisterForLog(DS_LogCb cb)
{
    logCb = cb;
}

int ds_log(int priority,const char *format, ...)
{
    char tmp_buff[MAX_LOG_BUFF];
    va_list args;
    va_start(args, format);
    vsnprintf(tmp_buff,MAX_LOG_BUFF-1,format, args);
    va_end(args);
    if(logCb != NULL)
    {
        logCb(priority,tmp_buff);
    }
    else
    {
        return printf(tmp_buff);
    }
    return 0;
}




/** @} */
/** @} */
