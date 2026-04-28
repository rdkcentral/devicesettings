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
 * @file frameRate.hpp
 * @brief This file defines FrameRate class to manage the video frame rate types.
 */



/**
* @defgroup devicesettings
* @{
* @defgroup ds
* @{
**/


#ifndef _DS_FRAMERATE_HPP_
#define _DS_FRAMERATE_HPP_

#include "dsConstant.hpp"
#include <string>

namespace device {
    void initializeFrameRates();
    void deinitializeFrameRates();

/**
 * @class FrameRate
 * @brief This class extends DSConstant to handle the video frame rate.
 * @ingroup devicesettingsclass
 */
class FrameRate : public DSConstant {
	float _value;                //!< Indicates the supported frame rate value in fps.
public:
    static int kUnknown;   //!< Indicates video frame rate of unknown type.
    static int k24;        //!< Indicates video frame rate of 24 fps.
    static int k25;        //!< Indicates video frame rate of 25 fps.
    static int k30;        //!< Indicates video frame rate of 30 fps.
    static int k60;        //!< Indicates video frame rate of 60 fps.
    static int k23dot98;   //!< Indicates video frame rate of 23.98 fps.
    static int k29dot97;   //!< Indicates video frame rate of 29.97 fps.
    static int k50;        //!< Indicates video frame rate of 50 fps.
    static int k59dot94;   //!< Indicates video frame rate of 59.94 fps.
    static int k100;       //!< Indicates video frame rate of 100 fps.
    static int k119dot88;  //!< Indicates video frame rate of 119.88 fps.
    static int k120;       //!< Indicates video frame rate of 120 fps.
    static int k200;       //!< Indicates video frame rate of 200 fps.
    static int k239dot76;  //!< Indicates video frame rate of 239.76 fps.
    static int k240;       //!< Indicates video frame rate of 240 fps.
    static int k59;        //!< Indicates video frame rate of 59 fps.
    static int k23;        //!< Indicates video frame rate of 23 fps.
    static int kMax;       //!< Indicates maximum number of frame rates supported.

	static const FrameRate & getInstance(int id);
	static const FrameRate & getInstance(const std::string &name);

	FrameRate(float value);
	FrameRate(int id);
	virtual ~FrameRate();
};

}

#endif /* _DS_FRAMERATE_HPP_ */


/** @} */
/** @} */
