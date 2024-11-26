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
* @defgroup rpc
* @{
**/


#include "dsserverlogger.h"
#include <stdarg.h>
#include <unistd.h>

#define MAX_LOG_BUFF 500

#ifdef DSMGR_LOGGER_ENABLED

int b_rdk_logger_enabled = 0;

#endif

DSServer_LogCb logCb = NULL;

void DSServer_RegisterForLog(DSServer_LogCb cb)
{
    logCb = cb;
}

#ifdef DSMGR_LOGGER_ENABLED
void dsServer_Rdklogger_Init()
{
    const char* PdebugConfigFile = NULL;
    const char* DSMGR_DEBUG_ACTUAL_PATH    = "/etc/debug.ini";
    const char* DSMGR_DEBUG_OVERRIDE_PATH  = "/opt/debug.ini";

        /* Init the logger */
    if (access(DSMGR_DEBUG_OVERRIDE_PATH, F_OK) != -1 ) {
        PdebugConfigFile = DSMGR_DEBUG_OVERRIDE_PATH;
    }
    else {
        PdebugConfigFile = DSMGR_DEBUG_ACTUAL_PATH;
    }

    if (rdk_logger_init(PdebugConfigFile) == 0) {
       b_rdk_logger_enabled = 1;
    }
}
#endif

int ds_server_log(int priority,const char *format, ...)
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
