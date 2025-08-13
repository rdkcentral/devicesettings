
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


#ifndef _DS_DISPLAYCONNECTIONCHANGELISTENER_H_
#define _DS_DISPLAYCONNECTIONCHANGELISTENER_H_

#include "videoOutputPort.hpp"

namespace device {

class DisplayConnectionChangeListener {
public:

    	struct IDisplayHDMIHotPlugEvent{

            // @brief Display HDMI Hot plug event
            // @text onDisplayHDMIHotPlug
            // @param displayEvent: DS_DISPLAY_EVENT_CONNECTED or DS_DISPLAY_EVENT_DISCONNECTED
            virtual void OnDisplayHDMIHotPlug(DisplayEvent displayEvent) = 0;
    	};
    
    	uint32_t Register(IDisplayHDMIHotPlugEvent*listener);
    	uint32_t UnRegister(IDisplayHDMIHotPlugEvent *listener);



    	struct IEvent{

            // @brief Display RX Sense event
            // @text onDisplayRxSense
            // @param displayEvent: DS_DISPLAY_RXSENSE_ON or DS_DISPLAY_RXSENSE_OFF
            virtual void OnDisplayRxSense(DisplayEvent displayEvent) = 0;
 
            // @brief Display HDCP Status
            // @text OnDisplayHDCPStatus
            virtual void OnDisplayHDCPStatus() = 0;

    	};
    
    	uint32_t Register(IEvent *listener);
    	uint32_t UnRegister(IEvent *listener);


	DisplayConnectionChangeListener() {}
	virtual ~DisplayConnectionChangeListener() {}
        virtual void displayConnectionChanged(VideoOutputPort &port, int newConnectionStatus) = 0;
        
};

}

#endif /* DISPLAYCONNECTIONCHANGELISTENER_H_ */


/** @} */
/** @} */
