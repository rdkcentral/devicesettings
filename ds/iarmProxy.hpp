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


#ifndef IARMPROXY_H_
#define IARMPROXY_H_

#include "libIBus.h"

namespace device {

class IARMProxy {
public:
    void registerPowerEventHandler(IARM_EventHandler_t _eventHandler);
	void UnRegisterPowerEventHandler();
	static IARMProxy& getInstance(void);
private:
	IARMProxy();
	virtual ~IARMProxy();
    IARMProxy (const IARMProxy&);
    IARMProxy& operator=(const IARMProxy&);
};

}

#endif /* IARMPROXY_H_ */


/** @} */
/** @} */
