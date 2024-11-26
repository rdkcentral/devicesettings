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


#include "videoOutputPortType.hpp"
#include "videoOutputPortConfig.hpp"
#include "audioOutputPortConfig.hpp"
#include "illegalArgumentException.hpp"
#include "dsVideoPortSettings.h"
#include "dsVideoResolutionSettings.h"
#include "dsDisplay.h"
#include "dsVideoPort.h"
#include "dsUtl.h"
#include "dsError.h"
#include "illegalArgumentException.hpp"
#include "list.hpp"
#include "videoResolution.hpp"
#include "dslogger.h"
#include "host.hpp"


#include <thread>
#include <mutex>
#include <iostream>
#include <string.h>
using namespace std;

namespace device {

static VideoResolution* defaultVideoResolution;
static std::mutex gSupportedResolutionsMutex;
VideoOutputPortConfig::VideoOutputPortConfig() {
	// TODO Auto-generated constructor stub
	defaultVideoResolution = new   VideoResolution(
				0, /* id */
				std::string("720p"),
				dsVIDEO_PIXELRES_1280x720,
				dsVIDEO_ASPECT_RATIO_16x9,
				dsVIDEO_SSMODE_2D,
				dsVIDEO_FRAMERATE_59dot94,
				_PROGRESSIVE);

}

VideoOutputPortConfig::~VideoOutputPortConfig() {
	// TODO Auto-generated destructor stub
	delete defaultVideoResolution;
}

VideoOutputPortConfig & VideoOutputPortConfig::getInstance() {
    static VideoOutputPortConfig _singleton;
	return _singleton;
}

const PixelResolution &VideoOutputPortConfig::getPixelResolution(int id) const
{
	return _vPixelResolutions.at(id);
}

const AspectRatio &VideoOutputPortConfig::getAspectRatio(int id) const
{
	return _vAspectRatios.at(id);
}

const StereoScopicMode &VideoOutputPortConfig::getSSMode(int id) const
{
	return _vStereoScopieModes.at(id);
}

const VideoResolution &VideoOutputPortConfig::getVideoResolution (int id) const
{
    {std::lock_guard<std::mutex> lock(gSupportedResolutionsMutex);
	if (id < _supportedResolutions.size()){
		return _supportedResolutions.at(id);
	}
	else {
		cout<<"returns default resolution 720p"<<endl;
		//If id not found return the 720p default resolution.
		return  *defaultVideoResolution;
	}
    }
}

const FrameRate &VideoOutputPortConfig::getFrameRate(int id) const
{
	return _vFrameRates.at(id);
}

VideoOutputPortType &VideoOutputPortConfig::getPortType(int id)
{
	return _vPortTypes.at(id);
}

VideoOutputPort &VideoOutputPortConfig::getPort(int id)
{
	return _vPorts.at(id);
}

VideoOutputPort &VideoOutputPortConfig::getPort(const std::string & name)
{
	for (size_t i = 0; i < _vPorts.size(); i++) {
		if (name.compare(_vPorts.at(i).getName()) == 0) {
			return _vPorts.at(i);
		}
	}

	throw IllegalArgumentException();
}

List<VideoOutputPort> VideoOutputPortConfig::getPorts()
{
	List <VideoOutputPort> rPorts;

	for (size_t i = 0; i < _vPorts.size(); i++) {
		rPorts.push_back(_vPorts.at(i));
	}

	return rPorts;
}

List<VideoOutputPortType>  VideoOutputPortConfig::getSupportedTypes()
{
	List<VideoOutputPortType> supportedTypes;
	for (std::vector<VideoOutputPortType>::const_iterator it = _vPortTypes.begin(); it != _vPortTypes.end(); it++) {
		if (it->isEnabled()) {
			supportedTypes.push_back(*it);
		}
	}

	return supportedTypes;
}

List<VideoResolution>  VideoOutputPortConfig::getSupportedResolutions(bool isIgnoreEdid)
{
	List<VideoResolution> supportedResolutions;
	std::vector<VideoResolution> tmpsupportedResolutions;
	int isDynamicList = 0;
	dsError_t dsError = dsERR_NONE;
	intptr_t _handle = 0;  //CID:98922 - Uninit
	bool force_disable_4K = true;
	
	printf ("\nResOverride VideoOutputPortConfig::getSupportedResolutions isIgnoreEdid:%d\n", isIgnoreEdid);
	if (!isIgnoreEdid) {
	    try {
                std::string strVideoPort = device::Host::getInstance().getDefaultVideoPortName();
		device::VideoOutputPort vPort = VideoOutputPortConfig::getInstance().getPort(strVideoPort.c_str());
		if (vPort.isDisplayConnected())
		{
			dsDisplayEDID_t *edid = (dsDisplayEDID_t*)malloc(sizeof(dsDisplayEDID_t));
                  	if (edid == NULL) {
       		 		throw Exception(dsERR_RESOURCE_NOT_AVAILABLE);
    			}
			
			/*Initialize the struct*/
			memset(edid, 0, sizeof(*edid));

			edid->numOfSupportedResolution = 0;
			dsGetDisplay((dsVideoPortType_t)vPort.getType().getId(), vPort.getIndex(), &_handle);
			dsError = dsGetEDID(_handle, edid);
			if(dsError != dsERR_NONE)
			{
				cout <<" dsGetEDID failed so setting edid->numOfSupportedResolution to 0"<< endl;
				edid->numOfSupportedResolution = 0;
			}
	
			cout <<" EDID Num of Resolution ......."<< edid->numOfSupportedResolution << endl;	
			for (size_t i = 0; i < edid->numOfSupportedResolution; i++)
			{
				dsVideoPortResolution_t *resolution = &edid->suppResolutionList[i];
				isDynamicList = 1;

				tmpsupportedResolutions.push_back(
							VideoResolution(
							i, /* id */
							std::string(resolution->name),
							resolution->pixelResolution,
							resolution->aspectRatio,
							resolution->stereoScopicMode,
							resolution->frameRate,
							resolution->interlaced));
			}

			free(edid);
		}
	    }catch (...)
		{
			isDynamicList = 0;
			cout << "VideoOutputPortConfig::getSupportedResolutions Thrown. Exception..."<<endl;
		}
	}
	//If isIgnoreEdid is true isDynamicList is zero. Edid logic is skipped.
	if (0 == isDynamicList )
	{
		size_t numResolutions = dsUTL_DIM(kResolutions);
		for (size_t i = 0; i < numResolutions; i++) 
		{
			dsVideoPortResolution_t *resolution = &kResolutions[i];
			tmpsupportedResolutions.push_back(
					VideoResolution(
					i, /* id */
					std::string(resolution->name),
					resolution->pixelResolution,
					resolution->aspectRatio,
					resolution->stereoScopicMode,
					resolution->frameRate,
					resolution->interlaced));
		}
	}
	if (!isIgnoreEdid) {
	    try {
			dsGetForceDisable4KSupport(_handle, &force_disable_4K);
	    }
	    catch(...)
	    {
		cout<<"Failed to get status of forceDisable4K!"<<endl;
	    }
	    for (std::vector<VideoResolution>::iterator it = tmpsupportedResolutions.begin(); it != tmpsupportedResolutions.end(); it++) {
		if (it->isEnabled()) {
			if((true == force_disable_4K) && (((it->getName() == "2160p60") || (it->getName() == "2160p30"))))
			{
				continue;
			}
			supportedResolutions.push_back(*it);
		}
	    }
	} else {
	    for (std::vector<VideoResolution>::iterator it = tmpsupportedResolutions.begin(); it != tmpsupportedResolutions.end(); it++) {
		    supportedResolutions.push_back(*it);
	    }
	}
	{std::lock_guard<std::mutex> lock(gSupportedResolutionsMutex);
		cout<<"_supportedResolutions cache updated"<<endl;
		_supportedResolutions.clear ();
		for (VideoResolution resolution : tmpsupportedResolutions){
			_supportedResolutions.push_back(resolution);
		}
	}
	return supportedResolutions;
}



void VideoOutputPortConfig::load()
{
	try {
		/*
		 * Load Constants First.
		 */
		for (size_t i = 0; i < dsVIDEO_PIXELRES_MAX; i++) {
			_vPixelResolutions.push_back(PixelResolution(i));
		}
		for (size_t i = 0; i < dsVIDEO_ASPECT_RATIO_MAX; i++) {
			_vAspectRatios.push_back(AspectRatio(i));
		}
		for (size_t i = 0; i < dsVIDEO_SSMODE_MAX; i++) {
			_vStereoScopieModes.push_back(StereoScopicMode(i));
		}
		for (size_t i = 0; i < dsVIDEO_FRAMERATE_MAX; i++) {
			_vFrameRates.push_back(FrameRate((int)i));
		}

		for (size_t i = 0; i < dsVIDEOPORT_TYPE_MAX; i++) {
			_vPortTypes.push_back(VideoOutputPortType((int)i));
		}

		/* Initialize a set of supported resolutions
		 *
		 */
		size_t numResolutions = dsUTL_DIM(kResolutions);
		for (size_t i = 0; i < numResolutions; i++) {
			dsVideoPortResolution_t *resolution = &kResolutions[i];
			{std::lock_guard<std::mutex> lock(gSupportedResolutionsMutex);
				_supportedResolutions.push_back(
									VideoResolution(
										i, /* id */
										std::string(resolution->name),
										resolution->pixelResolution,
										resolution->aspectRatio,
										resolution->stereoScopicMode,
										resolution->frameRate,
										resolution->interlaced));
			}
		}


	/*
	 * Initialize Video portTypes (Only Enable POrts)
	 * and its port instances (curr resolution)
	 */
		for (size_t i = 0; i < dsUTL_DIM(kConfigs); i++)
		{
			const dsVideoPortTypeConfig_t *typeCfg = &kConfigs[i];
			VideoOutputPortType &vPortType = VideoOutputPortType::getInstance(typeCfg->typeId);
			vPortType.enable();
			vPortType.setRestrictedResolution(typeCfg->restrictedResollution);
		}

		/*
		 * set up ports based on kPorts[]
		 */
		for (size_t i = 0; i < dsUTL_DIM(kPorts); i++) {
			const dsVideoPortPortConfig_t *port = &kPorts[i];

			_vPorts.push_back(
					VideoOutputPort((port->id.type), port->id.index, i,
							AudioOutputPortType::getInstance(kPorts[i].connectedAOP.type).getPort(kPorts[i].connectedAOP.index).getId(),
							std::string(port->defaultResolution)));

			_vPortTypes.at(port->id.type).addPort(_vPorts.at(i));

		}

	}
	catch (...) {
		cout << "VIdeo Outport Exception Thrown. ..."<<endl;
	}

}

void VideoOutputPortConfig::release()
  {
	try {
              _vPixelResolutions.clear();
              _vAspectRatios.clear();
              _vStereoScopieModes.clear();
              _vFrameRates.clear();
              _vPortTypes.clear();                            
              {std::lock_guard<std::mutex> lock(gSupportedResolutionsMutex);
                      _supportedResolutions.clear();
              }
              _vPorts.clear();
	}
	catch (const Exception &e) {
		throw e;
	}
  }
}


/** @} */
/** @} */
