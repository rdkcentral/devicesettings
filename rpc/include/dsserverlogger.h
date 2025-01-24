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


#ifndef _DS_SERVER_LOGGER_H_
#define _DS_SERVER_LOGGER_H_

#include <stdio.h>
#include "dsserverregisterlog.h"

int ds_server_log(int priority,const char *format, ...);

#if (defined(DSMGR_LOGGER_ENABLED))
#include "rdk_debug.h"

void dsServer_Rdklogger_Init();

#define INT_ERROR(FORMAT, ...)       LOG_ERROR(PREFIX(FORMAT), __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define INT_WARNING(FORMAT,  ...)       LOG_WARNING(PREFIX(FORMAT),  __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define INT_INFO(FORMAT,  ...)       LOG_INFO(PREFIX(FORMAT),  __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define INT_DEBUG(FORMAT, ...)       LOG_DEBUG(PREFIX(FORMAT), __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define INT_TRACE(FORMAT, ...)       LOG_TRACE(PREFIX(FORMAT), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define PREFIX(FORMAT)  "%d\t: %s - " FORMAT

extern int b_rdk_logger_enabled;


#define LOG_DEBUG(FORMAT, ...);       if(b_rdk_logger_enabled) {\
RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DSMGR", FORMAT , __VA_ARGS__);\
}\
else\
{\
printf(FORMAT, __VA_ARGS__);\
}

#define LOG_ERROR(FORMAT, ...);       if(b_rdk_logger_enabled) {\
RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DSMGR", FORMAT , __VA_ARGS__);\
}\
else\
{\
printf(FORMAT, __VA_ARGS__);\
}

#define LOG_INFO(FORMAT, ...);        if(b_rdk_logger_enabled) {\
RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DSMGR", FORMAT , __VA_ARGS__);\
}\
else\
{\
printf(FORMAT, __VA_ARGS__);\
}

#define LOG_WARNING(FORMAT, ...);     if(b_rdk_logger_enabled) {\
RDK_LOG(RDK_LOG_WARN, "LOG.RDK.DSMGR", FORMAT , __VA_ARGS__);\
}\
else\
{\
printf(FORMAT, __VA_ARGS__);\
}

#else

#define INT_DEBUG(FORMAT, ...);        printf(FORMAT, ##__VA_ARGS__)
#define INT_ERROR(FORMAT, ...);        printf(FORMAT, ##__VA_ARGS__)
#define INT_INFO(FORMAT, ...);         printf(FORMAT, ##__VA_ARGS__)
#define INT_WARNING(FORMAT, ...);      printf(FORMAT, ##__VA_ARGS__)

#endif

#endif



/** @} */
/** @} */
