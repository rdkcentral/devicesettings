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


#include "frameRate.hpp"
#include "illegalArgumentException.hpp"
#include "videoOutputPortConfig.hpp"
#include "dsTypes.h"
#include "dsUtl.h"
#include "dslogger.h"
#include <cmath>
#include <map>
#include <limits>
#include <cstdio>
#include <unistd.h>

namespace {
    typedef struct _frameRateInfo
    {
        std::string name;
        float value;
    }
    FrameRateInfo;

    std::map<dsVideoFrameRate_t, FrameRateInfo> _frameRates;

    inline bool isValid(int id) {
        bool valid = false;
        valid = dsVideoPortFrameRate_isValid(id);
        // now check if the frame rate id is supported in the map, as some devices may not support all frame rates defined in dsVideoFrameRate_t enum
        if (valid) {
            auto it = _frameRates.find(static_cast<dsVideoFrameRate_t>(id));
            if (it == _frameRates.end()) {
                valid = false;
                INT_WARN("Frame rate id: %d is not supported in the map", id);
            }
        }
        return valid;
    }
}

namespace device {
    // initializing with UNKNOWN, actual value will be set in initializeFrameRates() function
    int FrameRate::kUnknown              = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k24                   = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k25                   = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k30                   = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k60                   = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k23dot98              = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k29dot97              = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k50                   = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k59dot94              = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k100                  = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k119dot88             = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k120                  = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k200                  = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k239dot76             = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k240                  = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k59                   = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::k23                   = dsVIDEO_FRAMERATE_UNKNOWN;
    int FrameRate::kMax                  = dsVIDEO_FRAMERATE_MAX;

    void initializeFrameRates() {
        _frameRates.clear();
        _frameRates.insert({dsVIDEO_FRAMERATE_UNKNOWN, {"Unknown", 0}});

        INT_INFO("Initializing frame rates map with supported frame rates, count: %d", dsVIDEO_FRAMERATE_MAX);

        for (size_t i = dsVIDEO_FRAMERATE_UNKNOWN + 1; i < dsVIDEO_FRAMERATE_MAX; i++) {
            FrameRateInfo framerateInfo;
            framerateInfo.name.clear();
            framerateInfo.value = 0;

            switch (i) {
                case dsVIDEO_FRAMERATE_24: {
                    framerateInfo.value = 24.0;
                    framerateInfo.name = "24";
                    FrameRate::k24 = dsVIDEO_FRAMERATE_24;
                }
                break;
                case dsVIDEO_FRAMERATE_25: {
                    framerateInfo.value = 25.0;
                    framerateInfo.name = "25";
                    FrameRate::k25 = dsVIDEO_FRAMERATE_25;
                }
                break;
                case dsVIDEO_FRAMERATE_30: {
                    framerateInfo.value = 30.0;
                    framerateInfo.name = "30";
                    FrameRate::k30 = dsVIDEO_FRAMERATE_30;
                }
                break;
                case dsVIDEO_FRAMERATE_60: {
                    framerateInfo.value = 60.0;
                    framerateInfo.name = "60";
                    FrameRate::k60 = dsVIDEO_FRAMERATE_60;
                }
                break;
                case dsVIDEO_FRAMERATE_23dot98: {
                    framerateInfo.value = 23.98;
                    framerateInfo.name = "23.98";
                    FrameRate::k23dot98 = dsVIDEO_FRAMERATE_23dot98;
                }
                break;
                case dsVIDEO_FRAMERATE_29dot97: {
                    framerateInfo.value = 29.97;
                    framerateInfo.name = "29.97";
                    FrameRate::k29dot97 = dsVIDEO_FRAMERATE_29dot97;
                }
                break;
                case dsVIDEO_FRAMERATE_50: {
                    framerateInfo.value = 50.0;
                    framerateInfo.name = "50";
                    FrameRate::k50 = dsVIDEO_FRAMERATE_50;
                }
                break;
                case dsVIDEO_FRAMERATE_59dot94: {
                    framerateInfo.value = 59.94;
                    framerateInfo.name = "59.94";
                    FrameRate::k59dot94 = dsVIDEO_FRAMERATE_59dot94;
                }
                break;
                case dsVIDEO_FRAMERATE_100: {
                    framerateInfo.value = 100.0;
                    framerateInfo.name = "100";
                    FrameRate::k100 = dsVIDEO_FRAMERATE_100;
                }
                break;
                case dsVIDEO_FRAMERATE_119dot88: {
                    framerateInfo.value = 119.88;
                    framerateInfo.name = "119.88";
                    FrameRate::k119dot88 = dsVIDEO_FRAMERATE_119dot88;
                }
                break;
                case dsVIDEO_FRAMERATE_120: {
                    framerateInfo.value = 120.0;
                    framerateInfo.name = "120";
                    FrameRate::k120 = dsVIDEO_FRAMERATE_120;
                }
                break;
                case dsVIDEO_FRAMERATE_200: {
                    framerateInfo.value = 200.0;
                    framerateInfo.name = "200";
                    FrameRate::k200 = dsVIDEO_FRAMERATE_200;
                }
                break;
                case dsVIDEO_FRAMERATE_239dot76: {
                    framerateInfo.value = 239.76;
                    framerateInfo.name = "239.76";
                    FrameRate::k239dot76 = dsVIDEO_FRAMERATE_239dot76;
                }
                break;
                case dsVIDEO_FRAMERATE_240: {
                    framerateInfo.value = 240.0;
                    framerateInfo.name = "240";
                    FrameRate::k240 = dsVIDEO_FRAMERATE_240;
                }
                break;
            #if 0 // dsVIDEO_FRAMERATE_59 and dsVIDEO_FRAMERATE_23 are not supported in all devices, this will be enabled once all devices support them
                case dsVIDEO_FRAMERATE_59: {
                    framerateInfo.value = 59.0;
                    framerateInfo.name = "59";
                    FrameRate::k59 = dsVIDEO_FRAMERATE_59;
                }
                break;
                case dsVIDEO_FRAMERATE_23: {
                    framerateInfo.value = 23.0;
                    framerateInfo.name = "23";
                    FrameRate::k23 = dsVIDEO_FRAMERATE_23;
                }
                break;
            #endif
                default: {
                    framerateInfo.name = "Unknown";
                    framerateInfo.value = 0;
                    INT_WARN("Invalid frame rate id: %d", static_cast<int>(i));
                }
                break;
            }

            INT_INFO("Frame rate id: %d, name: %s, value: %f", static_cast<int>(i), framerateInfo.name.c_str(), framerateInfo.value);
            // inserting the frame rate info in the map with frame rate id as key and FrameRateInfo struct as value
            _frameRates.insert({static_cast<dsVideoFrameRate_t>(i), framerateInfo});
        }
    }

    void deinitializeFrameRates() {
        FrameRate::k24 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k25 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k30 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k60 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k23dot98 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k29dot97 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k50 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k59dot94 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k100 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k119dot88 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k120 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k200 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k239dot76 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k240 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k59 = dsVIDEO_FRAMERATE_UNKNOWN;
        FrameRate::k23 = dsVIDEO_FRAMERATE_UNKNOWN;
        _frameRates.clear();
    }

    const FrameRate & FrameRate::getInstance(int id)
    {
        if (::isValid(id)) {
            return VideoOutputPortConfig::getInstance().getFrameRate(id);
        }
        else {
            INT_ERROR("Frame rate id: %d is not valid", id);
            throw IllegalArgumentException();
        }
    }

    const FrameRate & FrameRate::getInstance(const std::string &name)
    {
        for (const auto& frameRate : _frameRates) {
            if (frameRate.second.name == name) {
                return VideoOutputPortConfig::getInstance().getFrameRate(frameRate.first);
            }
        }
        INT_ERROR("Frame rate name: %s is not valid", name.c_str());
        throw IllegalArgumentException();
    }

    FrameRate::FrameRate(int id)
    {
        if (::isValid(id)) {
            auto it = _frameRates.find(static_cast<dsVideoFrameRate_t>(id));
            if (it != _frameRates.end()) {
                _value = it->second.value;
                _name = it->second.name;
                _id = id;
                INT_INFO("Creating FrameRate with id: %d, name: %s, value: %f", _id, _name.c_str(), _value);
            }
            else {
                // this should never happen as we are already validating the id, but adding this check to avoid potential crash in case of map lookup failure
                INT_ERROR("Frame rate id: %d is valid but not found in the map", id);
                throw IllegalArgumentException();
            }
        }
        else {
            INT_ERROR("Frame rate id: %d is not valid", id);
            throw IllegalArgumentException();
        }

    }

    FrameRate::FrameRate(float value) : _value(value){
        for (const auto& frameRate : _frameRates) {
            // check if the value matches with any of the supported frame rates in the map, if found initialize the FrameRate object with corresponding name and id
            if (std::fabs(frameRate.second.value - value) < std::numeric_limits<float>::epsilon()) {
                _id = frameRate.first;
                _name = frameRate.second.name;
                INT_INFO("Creating FrameRate with value: %f, name: %s, id: %d", _value, _name.c_str(), _id);
                return;
            }
        }
        INT_ERROR("Frame rate value: %f is not valid", value);
        throw IllegalArgumentException();
    }

    FrameRate::~FrameRate() {
    }
}

/** @} */
/** @} */
