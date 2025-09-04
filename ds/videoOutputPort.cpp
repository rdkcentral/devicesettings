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


#include "videoOutputPort.hpp"
#include "videoResolution.hpp"
#include "audioOutputPortType.hpp"
#include "videoOutputPortType.hpp"
#include "videoOutputPortConfig.hpp"
#include "audioOutputPortConfig.hpp"
#include "host.hpp"
#include "dslogger.h"
#include "dsVideoPort.h"
#include "dsDisplay.h"
#include "edid-parser.hpp"

#include "safec_lib.h"

#include "illegalArgumentException.hpp"
#include "unsupportedOperationException.hpp"

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <thread>
#include <chrono>
#include "dsInternal.h"
using namespace std;


/**
 * @file videoOutputPort.cpp
 * @brief VideoOutputPort objects are instantiated by the Device Settings module upon initialization.
 * Applications do not need to create any such objects on its own.
 * References to the preallocated objects can be retrieved by applications via Host::getVideoOutputPort(const std::string &name).
 * Each VideoOutputPort is associated with an instance of VideoOutputPortType.
 */

namespace device {

const char * VideoOutputPort::kPropertyResolution = ".resolution";

enum {
    READ_EDID_RETRY_MS = 500,
    READ_EDID_RETRY_TOTAL_MS = 2000
};

/**
 * @addtogroup dssettingsvidoutportapi
 * @{
 */
/**
 * @fn  VideoOutputPort::getInstance(int id)
 * @brief This API is used to get the instance of the video output port based on the port id returned by the getsupported videooutput port.
 *
 * @param[in] id port id
 *
 * @return Reference to the instance of the port id
 */
VideoOutputPort & VideoOutputPort::getInstance(int id)
{
	return VideoOutputPortConfig::getInstance().getPort(id);
}


/**
 * @fn  VideoOutputPort::getInstance(const std::string &name)
 * @brief This API is used to get the instance of the video output port based on the port name returned by the getsupported videooutput port.
 *
 * @param[in] name Name of the port
 *
 * @return Reference to the instance of the name of the port
 */
VideoOutputPort & VideoOutputPort::getInstance(const std::string &name)
{
	return VideoOutputPortConfig::getInstance().getPort(name);

}


/**
 * @fn VideoOutputPort::VideoOutputPort(const int type, const int index, const int id, int audioPortId, const std::string &resolution)
 * @brief This function is a parameterised constructor for videooutputport. It initialises the data members of
 * VideoOutputPort instance with the parameters passed. It also updates the details about the video port
 * to indicate if the video port is enabled or not and also if its connected to the display or not. An
 * IllegalArgumentException is thrown if any error occurs while accessing the information about the video
 * port.
 *
 * @param[in] type Type of video output port (HDMI).
 * @param[in] index Index of video output port (0,1,...)
 * @param[in] id Videooutput port id.
 * @param[in] audioPortId AudioOutput Porttype id.
 * @param[in] resolution Supported videooutput port resolution.
 *
 * @return None
 */
VideoOutputPort::VideoOutputPort(const int type, const int index, const int id, int audioPortId, const std::string &resolution) :
										_type(type), _index(index), _id(id),
										_handle(-1), _enabled(true), _contentProtected(false),
										_displayConnected(false), _aPortId(audioPortId),
										_defaultResolution(resolution),
										_resolution(resolution),
										_display(*this)
{
	dsError_t ret = dsGetVideoPort((dsVideoPortType_t)_type, _index, &_handle);

	{
		std::stringstream out;
		out << getType().getName() << _index;
		_name = std::string(out.str());
	}

	if (ret == dsERR_NONE) {
		bool enabled = false;
		ret = dsIsVideoPortEnabled(_handle, &enabled);
		if (ret == dsERR_NONE) {
			_enabled = enabled;
			_contentProtected = false;

			bool connected = false;
			ret = dsIsDisplayConnected(_handle, &connected);
			if (ret == dsERR_NONE) {
				_displayConnected = connected;
			}
			else {
				throw IllegalArgumentException();
			}
		}
		else {
		}
	}
	else {
		throw IllegalArgumentException();
	}
}

bool VideoOutputPort::setScartParameter(const std::string parameter, const std::string value)
{
	return false;
}

int VideoOutputPort::getVideoEOTF() const
{
    dsHDRStandard_t videoEotf = dsHDRSTANDARD_NONE;
    dsError_t ret = dsGetVideoEOTF(_handle, &videoEotf);

    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }

    return (int)videoEotf;
}

int VideoOutputPort::getMatrixCoefficients() const
{
    dsDisplayMatrixCoefficients_t matrixCoefficients = dsDISPLAY_MATRIXCOEFFICIENT_UNKNOWN;
    dsError_t ret = dsGetMatrixCoefficients(_handle, &matrixCoefficients);

    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }

    return (int)matrixCoefficients;
}

int VideoOutputPort::getColorDepth() const
{
    unsigned int colorDepth = 0;
    dsError_t ret = dsGetColorDepth(_handle, &colorDepth);

    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }

    return (int)colorDepth;
}

int VideoOutputPort::getColorSpace() const
{
    dsDisplayColorSpace_t colorSpace = dsDISPLAY_COLORSPACE_UNKNOWN;
    dsError_t ret = dsGetColorSpace(_handle, &colorSpace);

    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }

    return (int)colorSpace;
}

int VideoOutputPort::getQuantizationRange() const
{
    dsDisplayQuantizationRange_t quantizationRange = dsDISPLAY_QUANTIZATIONRANGE_UNKNOWN;
    dsError_t ret = dsGetQuantizationRange(_handle, &quantizationRange);

    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }

    return (int)quantizationRange;
}

void VideoOutputPort::getCurrentOutputSettings(int &videoEOTF, int &matrixCoefficients, int &colorSpace, int &colorDepth, int &quantizationRange) const
{
    dsHDRStandard_t _videoEOTF = dsHDRSTANDARD_NONE;
    dsDisplayMatrixCoefficients_t _matrixCoefficients = dsDISPLAY_MATRIXCOEFFICIENT_UNKNOWN;
    dsDisplayColorSpace_t _colorSpace = dsDISPLAY_COLORSPACE_UNKNOWN;
    unsigned int _colorDepth = 0;
    dsDisplayQuantizationRange_t _quantizationRange = dsDISPLAY_QUANTIZATIONRANGE_UNKNOWN;
    dsError_t ret = dsGetCurrentOutputSettings(_handle, &_videoEOTF, &_matrixCoefficients, &_colorSpace, &_colorDepth, &_quantizationRange);

    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }

    videoEOTF = _videoEOTF;
    matrixCoefficients = _matrixCoefficients;
    colorSpace = _colorSpace;
    colorDepth = _colorDepth;
    quantizationRange = _quantizationRange;
}

/**
 * @fn VideoOutputPort::~VideoOutputPort()
 * @brief This is a default destructor of class VideoOutputPort.
 *
 * @return None.
 */
VideoOutputPort::~VideoOutputPort()
{
}


/**
 * @fn const VideoOutputPortType & VideoOutputPort::getType() const
 * @brief This API is used to get the type of the video output port.
 * A type of the video output port represent the general capabilities of the port.
 *
 * @return An instance to the type of the video output port
 */
const VideoOutputPortType & VideoOutputPort::getType() const
{
	return VideoOutputPortConfig::getInstance().getPortType(_type);
}


/**
 * @fn AudioOutputPort &VideoOutputPort::getAudioOutputPort()
 * @brief This API is used to get the audio output port connected to the video output port.
 * This connection is established during library initialization and cannot be modified.
 * An Video port is connected to at most one Audio port. An Audio port however can be connected to multiple Video ports.
 * UnsupportedOperationException will be thrown if the this Video Output Port is not connected to any Audio Output Port (i.e. on a Video-only device)
 *
 * @return Reference to the audio output port connected
 */
AudioOutputPort &VideoOutputPort::getAudioOutputPort()
{
	return AudioOutputPortConfig::getInstance().getPort(_aPortId);
}


/**
 * @fn  const VideoResolution & VideoOutputPort::getResolution()
 * @brief This API is used to get the current video resolution output from the video output port.
 * Upon library initialization, the resolution from the persistence storage will be loaded as the initial Resolution for the video output port.
 *
 * @return Reference to the output resolution
 */
const VideoResolution & VideoOutputPort::getResolution() 
{
	dsVideoPortResolution_t resolution;
	memset(&resolution, 0, sizeof(resolution));
	
	dsGetResolution(_handle,&resolution);

	/*Copy onto Temp - Safe*/
	std::string temp( resolution.name,strlen(resolution.name));
	_resolution = string(std::move(temp));
	
	return VideoResolution::getInstance(_resolution,true);
}


/**
 * @fn  const VideoOutputPort::getDefaultResolution() const
 * @brief This API is used to get the default resolution supported by the video output port.
 *
 * @return Reference to the default output resolution
 */
const VideoResolution & VideoOutputPort::getDefaultResolution() const
{
	return VideoResolution::getInstance(_defaultResolution,true);
}


/**
 * @fn const VideoOutputPort::Display & VideoOutputPort::getDisplay()
 * @brief This API is used to get the display device information currently connected to the output port.
 * Application can use isDisplayConnected() to query the display connection status before trying to get an instance of the display.
 *
 * @return Returns a reference to the connected display device. If the vedio port is not connected to the display then
 * it throws an IllegalStateException.
 */
const VideoOutputPort::Display &VideoOutputPort::getDisplay() 
{
	if (isDisplayConnected()) {
		dsVideoAspectRatio_t aspect;
		dsDisplayEDID_t *edid = (dsDisplayEDID_t*)malloc(sizeof(dsDisplayEDID_t));
          	if (edid == NULL) {
                  	throw Exception(dsERR_RESOURCE_NOT_AVAILABLE);
    		}
        	memset(edid, 0, sizeof(*edid));
		dsError_t dsError = dsERR_NONE;
		if (_display._handle != 0) {
			dsGetDisplayAspectRatio(_display._handle, &aspect);
			_display._aspectRatio = aspect;
			dsError = dsGetEDID(_display._handle, edid);
			if(dsError == dsERR_NONE)
			{
			 _display._productCode = edid->productCode;
			 _display._serialNumber = edid->serialNumber;
			 _display._manufacturerYear = edid->manufactureYear;
			 _display._manufacturerWeek = edid->manufactureWeek;
			 _display._hdmiDeviceType = edid->hdmiDeviceType;
			 /* The EDID  physical Address */
			_display._physicalAddressA = edid->physicalAddressA;
			_display._physicalAddressB = edid->physicalAddressB;
			_display._physicalAddressC = edid->physicalAddressC;
			_display._physicalAddressD = edid->physicalAddressD;
			_display._isDeviceRepeater = edid->isRepeater;


			}
			else
			{
				printf("VideoOutputPort::Display::dsGetEDID has dsError: %d\r\n", dsError);
			}
            free(edid);
			return _display;
		}
		else {
            if(edid)
            {
                free(edid);
            }
			throw Exception(dsERR_INVALID_PARAM);
		}
	}
	else {
		throw Exception(dsERR_INVALID_STATE);
	}
}


/**
 * @fn bool VideoOutputPort::isDisplayConnected() const
 * @brief This API is used to Check if the port is currently connected to any display device.
 *
 * If ret is not equal to dsERR_NONE, it will throw the ret to exception handler and it will pass message as "No message for this Exception".
 *
 * @return True or False
 * @retval 1 if display is connected
 * @retval 0 if display is not connected
 */
bool VideoOutputPort::isDisplayConnected() const
{
    bool connected = false;
    dsError_t ret = dsIsDisplayConnected(_handle, &connected);
	if (ret != dsERR_NONE) {
		throw Exception(ret);
	}
    return connected;
}


/**
 * @fn VideoOutputPort::isEnabled() const
 * @brief This API is used to check whether this Video output port is enabled or not.
 *
 * @return None
 */
bool VideoOutputPort::isEnabled() const
{
	bool enabled = false;
	dsError_t ret = dsIsVideoPortEnabled(_handle, &enabled);
	if (ret != dsERR_NONE) {
		throw Exception(ret);
	}
	return enabled;
}


/**
 * @fn VideoOutputPort::isActive() const
 * @brief This API is used to check if Port is connected to active port of Sink device.
 *
 * @return None
 */
bool VideoOutputPort::isActive() const
{
	bool active = false;
	dsError_t ret = dsIsVideoPortActive(_handle, &active);
	if (ret != dsERR_NONE) {
        if (ret == dsERR_OPERATION_NOT_SUPPORTED) {
            throw UnsupportedOperationException("RxSense is Not Supported");
        }
		throw Exception(ret);
	}
	return active;
}



/**
 * @fn VideoOutputPort::isDynamicResolutionSupported() const
 * @brief This API is used to check whether the video output port supports the dynamic super resolution or not.
 *
 * @return True or False
 * @retval True If video output port supports Dynamic super resolution
 * @retval False If video output port doesnot support Dynamic super resolution
 */
bool VideoOutputPort::isDynamicResolutionSupported() const
{
	return VideoOutputPortType::getInstance(_type).isDynamicResolutionsSupported();
}


/**
 * @fn VideoOutputPort::setResolution(const std::string &resolutionName)
 * @brief This API is used to set the output resolution of the port by ID or its Name.
 * The specified resolution must be one of the resolutions supported by the port.
 * The list of supported resolutions can be retrieved via VideoOutputPortType:: getSupportedResolutions(); IllegalArgumentException will be thrown
 * if the specified name or ID of the resolution is not recognized.
 *
 * @param[in] resolutionName Name of the Resoultion
 * @param[in] persist Flag to inform whether or not to persist the resolution
 * @param[in] isIgnoreEdid Flag to inform whether to ignore the EDID data or not
 *
 * @return None
 */
void VideoOutputPort::setResolution(const std::string &resolutionName, bool persist/* = true*/, bool isIgnoreEdid/* = false*/)
{
        printf("ResOverride VideoOutputPort::setResolution resolutionName:%s persist:%d isIgnoreEdid:%d line:%d\r\n", resolutionName.c_str(), persist, isIgnoreEdid, __LINE__);
	if (0 && resolutionName.compare(_resolution) == 0) {
		return;
	}

	VideoResolution newResolution = VideoResolution::getInstance(resolutionName, isIgnoreEdid);

	dsVideoPortResolution_t resolution;
        memset(&resolution, 0, sizeof(resolution));
	resolution.aspectRatio 		= (dsVideoAspectRatio_t)		newResolution.getAspectRatio().getId();
	resolution.frameRate 		= (dsVideoFrameRate_t) 	 	newResolution.getFrameRate().getId();
	resolution.interlaced 		= (bool)					newResolution.isInterlaced();
	resolution.pixelResolution 	= (dsVideoResolution_t)	newResolution.getPixelResolution().getId();
	resolution.stereoScopicMode = (dsVideoStereoScopicMode_t)newResolution.getStereoscopicMode().getId();
        strncpy(resolution.name,resolutionName.c_str(),sizeof(resolution.name));
        resolution.name[sizeof(resolution.name) - 1] = '\0';  // Ensure null-termination
	
        dsError_t ret = dsVideoPortSetResolution(_handle, &resolution, persist);

	if (ret != dsERR_NONE) {
		throw Exception(ret);
	}
	
	_resolution = newResolution.getName();
}

void VideoOutputPort::setPreferredColorDepth(const unsigned int colordepth, bool persist/* = true*/)
{
    dsError_t ret = dsVideoPortSetPreferredColorDepth (_handle, (dsDisplayColorDepth_t)colordepth, persist);

    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }
}

unsigned int VideoOutputPort::getPreferredColorDepth(bool persist/* = true*/)
{
    dsDisplayColorDepth_t colordepth = dsDISPLAY_COLORDEPTH_AUTO;
    dsError_t ret = dsVideoPortGetPreferredColorDepth (_handle,&colordepth, persist);
    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }

    return (unsigned int)colordepth;
}

void VideoOutputPort::getColorDepthCapabilities (unsigned int *capabilities) const
{
    dsColorDepthCapabilities (_handle, capabilities);
}

/**
 * @fn VideoOutputPort::setDisplayConnected(const bool connected)
 * @brief This API is used to set the video output port display to be connected.
 *
 * @param connected True if display is connected  or False if display is not connencted
 *
 * @return None
 */
void VideoOutputPort::setDisplayConnected(const bool connected)
{
	_displayConnected = connected;
	throw UnsupportedOperationException();
}


/**
 * @fn bool VideoOutputPort::isContentProtected() const
 * @brief This API is used to Check if the port or the content output on the port has DTCP or HDCP in use.
 *
 * @param  None
 *
 * @return True or False
 * @retval 1 if content from this port is protected by DTCP or HDCP.Otherwise
 * @retval 0 if content from this port is not protected by DTCP or HDCP
 */
bool VideoOutputPort::isContentProtected() const
{
    bool isProtected = false;
    dsIsHDCPEnabled (_handle, &isProtected);
    return isProtected;
}


/**
 * @fn VideoOutputPort::enable()
 * @brief This API is used to enable the video output port.
 *
 * @return None
 */
void VideoOutputPort::enable()
{
	if (dsEnableVideoPort(_handle, true) == dsERR_NONE)
	{
		_enabled = true;
	}
}


/**
 * @fn VideoOutputPort::disable()
 * @brief This API is used to disable the video output port.
 *
 * @return None
 */
void VideoOutputPort::disable()
{
	if (dsEnableVideoPort(_handle, false) == dsERR_NONE)
	{
		_enabled = false;
	}
}


/**
 * @fn VideoOutputPort::Display::Display(VideoOutputPort &vPort)
 * @brief This function is used to get the display handle of a given video device.
 *
 * @param[in] vPort Video port handle associated with connected display.
 * @return None.
 */
VideoOutputPort::Display::Display(VideoOutputPort &vPort) :
                                                          _productCode(0), _serialNumber(0),
                                                          _manufacturerYear(0), _manufacturerWeek(0),
                                                          _hdmiDeviceType(true), _isSurroundCapable(false),
                                                          _isDeviceRepeater(false), _aspectRatio(0),
                                                          _physicalAddressA(1), _physicalAddressB(0),
                                                          _physicalAddressC(0), _physicalAddressD(0),
                                                          _handle(-1)
{
	dsError_t ret = dsERR_NONE;
	ret = dsGetDisplay((dsVideoPortType_t)vPort.getType().getId(), vPort.getIndex(), &_handle);
	if (ret != dsERR_NONE) {
		throw Exception(ret);
	}
}

/**
 * @fn bool VideoOutputPort::Display::hasSurround(void) const
 * @brief This function returns true if connected display supports surround audio 
 *
 * @param None.
 * @return true if the connected sink has surround capability.
 */
bool VideoOutputPort::Display::hasSurround(void) const
{
    printf("VideoOutputPort::Display::hasSurround\r\n");

    dsError_t ret = dsERR_NONE;
    bool surround = false;
    intptr_t vopHandle = 0;
    ret = dsGetVideoPort(dsVIDEOPORT_TYPE_HDMI, 0, &vopHandle);
    if (ret == dsERR_NONE) {
	ret = dsIsDisplaySurround(vopHandle, &surround);
	printf("VideoOutputPort::Display::hasSurround ret %d\r\n", ret);
    }

    if (ret != dsERR_NONE) {
                surround = false;
		printf("VideoOutputPort::Display::hasSurround return default value false");
    }

    return surround;
}
  
/**
 * @fn bool VideoOutputPort::Display::getSurroundMode(void) const
 * @brief This function returns the supports surround audio Mode
 *
 * @param None.
 * @return enum for the surround capability.
 */
int VideoOutputPort::Display::getSurroundMode(void) const
{
    printf("VideoOutputPort::Display::getSurroundMode\r\n");

    dsError_t ret = dsERR_NONE;
    int surroundMode = dsSURROUNDMODE_NONE;
    intptr_t vopHandle = 0;
    ret = dsGetVideoPort(dsVIDEOPORT_TYPE_HDMI, 0, &vopHandle);
    if (ret == dsERR_NONE) {
	ret = dsGetSurroundMode(vopHandle, &surroundMode);
	printf("VideoOutputPort::Display::getSurroundMode ret %d\r\n", ret);
    }

    if (ret != dsERR_NONE) {
                surroundMode = dsSURROUNDMODE_NONE;
		printf("VideoOutputPort::Display::getSurroundMode return default value dsSURROUNDMODE_NONE");
    }

    return surroundMode;
}


/**
 * @fn void VideoOutputPort::Display::getEDIDBytes(std::vector<uint8_t> &edid) const
 * @brief This function is used to get the EDID information of the connected video display.
 * After it gets the EDID information , it clears the old edid information and inserts it.
 * The EDID bytes are verified before returning them. If invalid EDID bytes are detected, the
 * EDID bytes are read again after READ_EDID_RETRY_MS millis. This repeats for a maximum
 * of READ_EDID_RETRY_TOTAL_MS millis.
 * If ret is not equal to dsERR_NONE, it will throw the ret to exception handler and it will pass message as "dsGetEDIDBytes failed".
 * If invalid EDID is read, even after retries, it throws exception with message "EDID verification failed"
 * If EDID bytes length > 1024 it throws exception with message "EDID length > 1024"
 *
 * @param edid The EDID raw buffer of the display. The HAL implementation should
 *                      malloc() the buffer and return it to the application. The
 *                      application is required to free() the buffer after using it;
 *                      If HDMI is not connected  no data should be returned,
 *                      and the API returns dsERR_INVALID_STATE.
 * @return None.
 */
void VideoOutputPort::Display::getEDIDBytes(std::vector<uint8_t> &edid) const
{
    using namespace std::chrono;

    printf("VideoOutputPort::Display::getEDIDBytes \r\n");

    dsError_t ret = dsERR_NONE;
    int length = 0;

    unsigned char edidBytes[512] = {0};

    const auto reading_edid_start = system_clock::now();
    const char* exceptionstr = "";
    bool retry_get_edid;
    do {
        retry_get_edid = false;
        const auto get_edid_bytes_start = system_clock::now();
        ret = dsGetEDIDBytes(_handle, edidBytes, &length);

        printf("VideoOutputPort::Display::getEDIDBytes has ret %d\r\n", ret);
        if (ret == dsERR_NONE) {
            if (length <= 1024) {
                printf("VideoOutputPort::Display::getEDIDBytes has %d bytes\r\n", length);
                if (edid_parser::EDID_STATUS_OK == edid_parser::EDID_Verify(edidBytes, length)) {
                    edid.clear();
                    edid.insert(edid.begin(), edidBytes, edidBytes + length);
                } else {
                    ret = dsERR_GENERAL;
                    exceptionstr = "EDID verification failed";
                    auto now = system_clock::now();
                    if (now + milliseconds(READ_EDID_RETRY_MS) < reading_edid_start + milliseconds(READ_EDID_RETRY_TOTAL_MS)) {
                        std::this_thread::sleep_for(milliseconds(READ_EDID_RETRY_MS) - (now - get_edid_bytes_start));
                        retry_get_edid = true;
                    }
                }
            } else {
                ret = dsERR_OPERATION_NOT_SUPPORTED;
                exceptionstr = "EDID length > 1024";
            }
        } else {
            exceptionstr = "dsGetEDIDBytes failed";
        }
    } while (retry_get_edid);

    if (ret != dsERR_NONE) {
        throw Exception(ret, exceptionstr);
    }
}


/**
 * @fn VideoOutputPort::Display::~Display()
 * @brief This function is a default destructor of class Display.
 *
 * @return None.
 */
VideoOutputPort::Display::~Display()
{
}


int VideoOutputPort::getHDCPStatus() 
{
    dsHdcpStatus_t hdcpStatus;
    int _hdcpStatus = 0;

    dsError_t ret = dsGetHDCPStatus(_handle, &hdcpStatus);
    
    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }

    _hdcpStatus = hdcpStatus;
    return _hdcpStatus;
}

int VideoOutputPort::getHDCPProtocol()
{
    dsHdcpProtocolVersion_t hdcpProtocol;
    dsError_t ret = dsGetHDCPProtocol(_handle, &hdcpProtocol);

    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }

    return hdcpProtocol;
}

int VideoOutputPort::getHDCPReceiverProtocol()
{
    dsHdcpProtocolVersion_t hdcpProtocol;

    dsError_t ret = dsGetHDCPReceiverProtocol(_handle, &hdcpProtocol);

    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }

    return hdcpProtocol;
}

int VideoOutputPort::getHDCPCurrentProtocol()
{
    dsHdcpProtocolVersion_t hdcpProtocol;

    dsError_t ret = dsGetHDCPCurrentProtocol(_handle, &hdcpProtocol);

    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }

    return hdcpProtocol;
}

/**
 * @fn  const void VideoOutputPort::getTVHDRCapabilities(int *capabilities) const
 * @brief This API is used to get HDR format supported by TV.
 *
 * @return HDR capabilities supported by TV
 */
void VideoOutputPort::getTVHDRCapabilities(int *capabilities) const
{
    dsGetTVHDRCapabilities(_handle, capabilities);
}

/**
 * @fn  const void VideoOutputPort::getSupportedTvResolutions(int *resolutions) const
 * @brief This API is used to get TV supported Video Resolutions.
 *
 * @return Resolutions supported by TV
 */
void VideoOutputPort::getSupportedTvResolutions(int *resolutions) const
{
    dsError_t ret = dsSupportedTvResolutions(_handle,resolutions);
    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }
}

int VideoOutputPort::forceDisable4KSupport(bool disable)
{
	dsSetForceDisable4KSupport(_handle, disable);
	return 0;
}

/**
 * @fn  const int VideoOutputPort::setForceHDRMode(dsHDRStandard_t mode)
 * @brief This API is used to set/reset force HDR mode.
 *
 * @return bool status
 */
bool VideoOutputPort::setForceHDRMode(dsHDRStandard_t mode)
{
    return dsSetForceHDRMode(_handle, mode) == dsERR_NONE;
}

/**
 * @fn  const void VideoOutputPort::IsOutputHDR()
 * @brief This API is used to check if Video output is HDR or not.
 *
 * @return True if video output is HDR
 */
bool VideoOutputPort::IsOutputHDR()
{
    bool hdr;
    dsIsOutputHDR(_handle, &hdr);
    return hdr;
}

/**
 * @fn  const void VideoOutputPort::ResetOutputToSDR()
 * @brief This API is used to reset the video output to SDR if it is HDR.
 *
 * @return None
 */

void VideoOutputPort::ResetOutputToSDR()
{
    bool hdr;
    hdr = IsOutputHDR();

    if(hdr == true){
        dsResetOutputToSDR();
    }
    else {
        printf("ds: %s: Allready SDR\n",__FUNCTION__);
    }
}

/**
 * @fn bool VideoOutputPort::SetHdmiPreference(dsHdcpProtocolVersion_t hdcpProtocol)
 * @brief This API is used to set the Preferred HDMI Protocol
 *
 * @param enum dsHdcpProtocolVersion
 *  dsHDCP_VERSION_1X = 0,            < HDCP Protocol version 1.x
 *  dsHDCP_VERSION_2X,                < HDCP Protocol version 2.x
 *  dsHDCP_VERSION_MAX                < Maximum index for HDCP protocol.
 *
 * @return true if no error.
 */

bool VideoOutputPort::SetHdmiPreference(dsHdcpProtocolVersion_t hdcpProtocol)
{
    dsError_t ret = dsSetHdmiPreference(_handle, &hdcpProtocol);
    if (ret != dsERR_NONE) {
        return false;
    }

    return true;
}

/**
 * @fn bool VideoOutputPort::GetHdmiPreference()
 * @brief This API is used to get the HDMI Preference
 *
 * @return int value corresponding to : enum dsHdcpProtocolVersion
 *  dsHDCP_VERSION_1X = 0,            < HDCP Protocol version 1.x
 *  dsHDCP_VERSION_2X,                < HDCP Protocol version 2.x
 *  dsHDCP_VERSION_MAX                < Maximum index for HDCP protocol.
 *
 */

int VideoOutputPort::GetHdmiPreference()
{
    dsHdcpProtocolVersion_t hdcpProtocol;
    dsError_t ret = dsGetHdmiPreference(_handle, &hdcpProtocol);

    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }

    return hdcpProtocol;
}

bool VideoOutputPort::SetStandbyVideoState(const string port , const bool enable)
{
    dsError_t ret = dsSetStandbyVideoState(port, enable);
    if (ret != dsERR_NONE) {
        return false;
    }
    return true;

}

bool VideoOutputPort::GetStandbyVideoState(const string port, bool isEnabled)
{
    dsError_t ret = dsGetStandbyVideoState(port, isEnabled);
    if (ret != dsERR_NONE) {
        return false;
    }
    return true;

}

bool VideoOutputPort::SetAvPortState(const uint8_t powerState)
{
    dsError_t ret = dsSetAvPortState(powerState);
    if (ret != dsERR_NONE) {
        return false;
    }
    return true;

}

/**
 * @fn void setAllmEnabled(bool enable); 
 * @brief Enables/Disables ALLM mode for connected HDMI display.
 */
void VideoOutputPort::Display::setAllmEnabled(bool enable) const
{
    printf("VideoOutputPort::Display::setAllmEnabled \r\n");
    dsError_t ret = dsSetAllmEnabled(_handle,enable);
    if (ret != dsERR_NONE) {
     throw Exception(ret);
    }
}

/**
 * @fn void setAVIContentType(dsAviContentType_t contentType);
 * @brief Sets HDMI AVI content type signalling.
 */
void VideoOutputPort::Display::setAVIContentType(dsAviContentType_t contentType) const
{
    printf("VideoOutputPort::Display::setAVIContentType \r\n");
    dsError_t ret = dsSetAVIContentType(_handle,contentType);
    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }
}

/**
 * @fn void setAVIScanInformation(dsAVIScanInformation_t scanInfo);
 * @brief ets HDMI AVI scan info signalling.
 */
void VideoOutputPort::Display::setAVIScanInformation(dsAVIScanInformation_t scanInfo) const
{
    printf("VideoOutputPort::Display::setAVIScanInformation \r\n");
    dsError_t ret = dsSetAVIScanInformation(_handle,scanInfo);
    if (ret != dsERR_NONE) {
        throw Exception(ret);
    }
}

}

/** @} */ //End of Doxygen tag


/** @} */
/** @} */
