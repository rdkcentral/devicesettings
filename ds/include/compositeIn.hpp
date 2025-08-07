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

/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright ARRIS Enterprises, Inc. 2015.
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
 * limitations under
*/


/**
* @defgroup devicesettings
* @{
* @defgroup ds
* @{
**/


#ifndef _DS_COMPOSITEIN_HPP_
#define _DS_COMPOSITEIN_HPP_

#include <stdint.h>
#include "dsTypes.h"

/**
 * @file compositeIn.hpp
 * @brief Structures and classes for COMPOSITE Input are defined here
 * @ingroup compositeIn
 */

static const int8_t COMPOSITE_IN_PORT_NONE = -1;

namespace device
{


/**
 * @class CompositeInput
 * @brief This class manages COMPOSITE Input
 */
class CompositeInput
{

public:
    struct IEvent {
            // @brief Composite In Hotplug event
            // @text onCompositeInHotPlug
            // @param port: Port of the hotplug
            // @param isConnected: Is it connected (true) or not(false)
            virtual void OnCompositeInHotPlug(dsCompositeInPort_t port, bool isConnected) { };
 
            // @brief Composite In Signal status
            // @text onCompositeInSignalStatus
            // @param port: Port of the hotplug
            // @param signalStatus: Signal status
            virtual void OnCompositeInSignalStatus(dsCompositeInPort_t port, dsCompInSignalStatus_t signalStatus) { };
 
            // @brief Composite In status
            // @text onCompositeInStatus
            // @param activePort: Active port
            // @param isPresented: is it presented to user
            virtual void OnCompositeInStatus(dsCompositeInPort_t activePort, bool isPresented) { };


            // @brief Composite In Video Mode Update
            // @text OnCompositeInVideoModeUpdate
            // @param activePort: Active port
            // @param videoResolution: See DisplayVideoPortResolution
            virtual void OnCompositeInVideoModeUpdate(dsCompositeInPort_t activePort, dsVideoPortResolution_t videoResolution) = 0;
	    };

    uint32_t Register(IEvent *listener);
    uint32_t UnRegister(IEvent *listener);
    
    static CompositeInput & getInstance();

    uint8_t getNumberOfInputs        () const;
    bool    isPresented              () const;
    bool    isActivePort             (int8_t Port) const;
    int8_t  getActivePort            () const;
    bool    isPortConnected          (int8_t Port) const;
    void    selectPort               (int8_t Port) const;
    void    scaleVideo               (int32_t x, int32_t y, int32_t width, int32_t height) const;

private:
    CompositeInput ();           /* default constructor */
    virtual ~CompositeInput ();  /* destructor */
};


}   /* namespace device */


#endif /* _DS_COMPOSITEIN_HPP_ */


/** @} */
/** @} */
