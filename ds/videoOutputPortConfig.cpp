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
#include "manager.hpp"


#include <thread>
#include <mutex>
#include <iostream>
#include <string.h>
using namespace std;

#define DEBUG 1 // Using for dumpconfig 

typedef struct videoPortConfigs
{
	const dsVideoPortTypeConfig_t  *pKConfigs;
	int *pKVideoPortConfigs_size;
	const dsVideoPortPortConfig_t  *pKPorts;
	int *pKVideoPortPorts_size;
	dsVideoPortResolution_t *pKResolutionsSettings;
	int *pKResolutionsSettings_size;
}videoPortConfigs_t;

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


void dumpconfig(videoPortConfigs_t *config)
{
	INT_INFO("%d:%s: Entering function\n", __LINE__, __func__);
	INT_INFO("%d:%s: pKConfigs = %p\n", __LINE__, __func__, config->pKConfigs);
	INT_INFO("%d:%s: pKConfigSize pointer %p = %d\n", __LINE__, __func__, config->pKVideoPortConfigs_size, *(config->pKVideoPortConfigs_size));
	INT_INFO("%d:%s: pKPorts = %p\n", __LINE__, __func__, config->pKPorts);
	INT_INFO("%d:%s: pKPortSize pointer %p = %d\n", __LINE__, __func__, config->pKVideoPortPorts_size, *(config->pKVideoPortPorts_size));
	INT_INFO("%d:%s: pKResolutionsSettings = %p\n", __LINE__, __func__, config->pKResolutionsSettings);
	INT_INFO("%d:%s: pKResolutionsSettingsSize pointer %p = %d\n", __LINE__, __func__, config->pKResolutionsSettings_size, *(config->pKResolutionsSettings_size));

	INT_INFO("\n\n=========================================================================================================================\n\n");
	if(config->pKConfigs != NULL && *(config->pKVideoPortConfigs_size) != -1 && 
	   config->pKPorts != NULL && *(config->pKVideoPortPorts_size) != -1 &&
	   config->pKResolutionsSettings != NULL && *(config->pKResolutionsSettings_size) != -1	)
	{
		INT_INFO("\n\n####################################################################### \n\n");
		INT_INFO("%d:%s: Dumping Resolutions Settings\n", __LINE__, __func__);
		for (size_t i = 0; i < *(config->pKResolutionsSettings_size); i++) {
			dsVideoPortResolution_t *resolution = &(config->pKResolutionsSettings[i]);
			INT_INFO("%d:%s: resolution->name = %s\n", __LINE__, __func__, resolution->name);
			INT_INFO("%d:%s: resolution->pixelResolution= %d\n", __LINE__, __func__, resolution->pixelResolution);
			INT_INFO("%d:%s: resolution->aspectRatio= %d\n", __LINE__, __func__, resolution->aspectRatio);
			INT_INFO("%d:%s: resolution->stereoScopicMode= %d\n", __LINE__, __func__, resolution->stereoScopicMode);
			INT_INFO("%d:%s: resolution->frameRate= %d\n", __LINE__, __func__, resolution->frameRate);
			INT_INFO("%d:%s: resolution->interlaced= %d\n", __LINE__, __func__, resolution->interlaced);

		}
		INT_INFO("\n\n####################################################################### \n\n");
		INT_INFO("\n\n####################################################################### \n\n");
		INT_INFO("%d:%s: Dumping Video Port Configurations\n", __LINE__, __func__);
		for (size_t i = 0; i < *(config->pKVideoPortConfigs_size); i++)
		{
			const dsVideoPortTypeConfig_t *typeCfg = &(config->pKConfigs[i]);

			INT_INFO("%d:%s: typeCfg->typeId = %d\n", __LINE__, __func__, typeCfg->typeId);
			INT_INFO("%d:%s: typeCfg->name = %s\n", __LINE__, __func__, typeCfg->name);
			INT_INFO("%d:%s: typeCfg->dtcpSupported= %d\n", __LINE__, __func__, typeCfg->dtcpSupported);
			INT_INFO("%d:%s: typeCfg->hdcpSupported = %d\n", __LINE__, __func__, typeCfg->hdcpSupported);
			INT_INFO("%d:%s: typeCfg->restrictedResollution = %d\n", __LINE__, __func__, typeCfg->restrictedResollution);
			INT_INFO("%d:%s: typeCfg->numSupportedResolutions= %lu\n", __LINE__, __func__, typeCfg->numSupportedResolutions);

			INT_INFO("%d:%s: typeCfg->supportedResolutions = %p\n", __LINE__, __func__, typeCfg->supportedResolutions);
			INT_INFO("%d:%s: typeCfg->supportedResolutions->name = %s\n", __LINE__, __func__, typeCfg->supportedResolutions->name);
			INT_INFO("%d:%s: typeCfg->supportedResolutions->pixelResolution= %d\n", __LINE__, __func__, typeCfg->supportedResolutions->pixelResolution);
			INT_INFO("%d:%s: typeCfg->supportedResolutions->aspectRatio= %d\n", __LINE__, __func__, typeCfg->supportedResolutions->aspectRatio);
			INT_INFO("%d:%s: typeCfg->supportedResolutions->stereoScopicMode= %d\n", __LINE__, __func__, typeCfg->supportedResolutions->stereoScopicMode);
			INT_INFO("%d:%s: typeCfg->supportedResolutions->frameRate= %d\n", __LINE__, __func__, typeCfg->supportedResolutions->frameRate);
			INT_INFO("%d:%s: typeCfg->supportedResolutions->interlaced= %d\n", __LINE__, __func__, typeCfg->supportedResolutions->interlaced);
		}
		INT_INFO("\n\n####################################################################### \n\n");

		INT_INFO("\n\n####################################################################### \n\n");
		INT_INFO("%d:%s: Dumping Video Port Connections\n", __LINE__, __func__);
		for (size_t i = 0; i < *(config->pKVideoPortPorts_size); i++) {
			const dsVideoPortPortConfig_t *port = &(config->pKPorts[i]);
			INT_INFO("%d:%s: port->id.type = %d\n", __LINE__, __func__, port->id.type);
			INT_INFO("%d:%s: port->id.index = %d\n", __LINE__, __func__, port->id.index);
			INT_INFO("%d:%s: port->connectedAOP.type = %d\n", __LINE__, __func__, port->connectedAOP.type);
			INT_INFO("%d:%s: port->connectedAOP.index = %d\n", __LINE__, __func__, port->connectedAOP.index);
			INT_INFO("%d:%s: port->defaultResolution = %s\n", __LINE__, __func__, port->defaultResolution);
		}
		INT_INFO("\n\n####################################################################### \n\n");
	}
	else
	{
		INT_ERROR("%d:%s: pKConfigs or pKPorts or pKResolutionsSettings is NULL, and pKVideoPortConfigs_size  or pKVideoPortPorts_size or  pKResolutionsSettings_size is -1\n", __LINE__, __func__);
	}
	INT_INFO("\n\n=========================================================================================================================\n\n");
	INT_INFO("%d:%s: Exit function\n", __LINE__, __func__);
}

void VideoOutputPortConfig::load()
{
	static int configSize, portSize, resolutionSize, invalid_size = -1;
	videoPortConfigs_t configuration = {0};
	const char* searchVaribles[] = {
        "kVideoPortConfigs",
        "kVideoPortConfigs_size",
        "kVideoPortPorts",
        "kVideoPortPorts_size",
		"kResolutionsSettings",
		"kResolutionsSettings_size"
    };
	bool ret = false;

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

		INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[0]);
		ret = searchConfigs(searchVaribles[0], (void **)&configuration.pKConfigs);
		ret= false;
		INT_INFO("make default ret = false to read fallback\n);
		if(ret == true)
		{
			INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[1]);
			ret = searchConfigs(searchVaribles[1], (void **)&configuration.pKVideoPortConfigs_size);
			if(ret == false)
			{
				INT_ERROR("%s is not defined\n", searchVaribles[1]);
				configuration.pKVideoPortConfigs_size = &invalid_size;
			}
			INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[2]);
			ret = searchConfigs(searchVaribles[2], (void **)&configuration.pKPorts);
			if(ret == false)
			{
				INT_ERROR("%s is not defined\n", searchVaribles[2]);
			}
			INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[3]);
			ret = searchConfigs(searchVaribles[3], (void **)&configuration.pKVideoPortPorts_size);
			if(ret == false)
			{
				INT_ERROR("%s is not defined\n", searchVaribles[3]);
				configuration.pKVideoPortPorts_size = &invalid_size;
			}
			// Resolutions
			ret = searchConfigs(searchVaribles[4], (void **)&configuration.pKResolutionsSettings);
			if(ret == false)
			{
				INT_ERROR("%s is not defined\n", searchVaribles[4]);
			}
			INT_INFO("%d:%s: Calling  searchConfigs( %s)\n", __LINE__, __func__, searchVaribles[5]);
			ret = searchConfigs(searchVaribles[5], (void **)&configuration.pKResolutionsSettings_size);
			if(ret == false)
			{
				INT_ERROR("%s is not defined\n", searchVaribles[5]);
				configuration.pKResolutionsSettings_size = &invalid_size;
			}
		}
		else
		{
			INT_ERROR("Read Old Configs\n");
			configuration.pKConfigs = kConfigs;
			configSize = dsUTL_DIM(kConfigs);
			configuration.pKVideoPortConfigs_size = &configSize;
			configuration.pKPorts = kPorts;
			portSize = dsUTL_DIM(kPorts);
			configuration.pKVideoPortPorts_size = &portSize;
			configuration.pKResolutionsSettings = kResolutions;
			resolutionSize = dsUTL_DIM(kResolutions);
			configuration.pKResolutionsSettings_size = &resolutionSize;
			INT_INFO("configuration.pKConfigs =%p, *(configuration.pKVideoPortConfigs_size) = %d\n", configuration.pKConfigs, *(configuration.pKVideoPortConfigs_size));
			INT_INFO("configuration.pKPorts =%p, *(configuration.pKVideoPortPorts_size) = %d\n", configuration.pKPorts, *(configuration.pKVideoPortPorts_size));
			INT_INFO("configuration.pKResolutionsSettings =%p, *(configuration.pKResolutionsSettings_size) = %d\n", configuration.pKResolutionsSettings, *(configuration.pKResolutionsSettings_size));
		}

		if((configuration.pKConfigs != NULL) && (configuration.pKVideoPortConfigs_size != NULL) &&
		   (configuration.pKPorts != NULL) && (configuration.pKVideoPortPorts_size != NULL) &&
		   (configuration.pKResolutionsSettings != NULL) && (configuration.pKResolutionsSettings_size != NULL))
		{

			#if DEBUG
			dumpconfig(&configuration);
			#endif
			/* Initialize a set of supported resolutions
		 	*
		 	*/
			for (size_t i = 0; i < *(configuration.pKResolutionsSettings_size); i++) {
				dsVideoPortResolution_t *resolution = &(configuration.pKResolutionsSettings[i]);
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
			for (size_t i = 0; i < *(configuration.pKVideoPortConfigs_size); i++)
			{
				const dsVideoPortTypeConfig_t *typeCfg = &(configuration.pKConfigs[i]);
				VideoOutputPortType &vPortType = VideoOutputPortType::getInstance(typeCfg->typeId);
				vPortType.enable();
				vPortType.setRestrictedResolution(typeCfg->restrictedResollution);
			}

			/*
		 	* set up ports based on kPorts[]
		 	*/
			for (size_t i = 0; i < *(configuration.pKVideoPortPorts_size); i++) {
				const dsVideoPortPortConfig_t *port = &(configuration.pKPorts[i]);

				_vPorts.push_back(
						VideoOutputPort((port->id.type), port->id.index, i,
								AudioOutputPortType::getInstance(configuration.pKPorts[i].connectedAOP.type).getPort(configuration.pKPorts[i].connectedAOP.index).getId(),
								std::string(port->defaultResolution)));

				_vPortTypes.at(port->id.type).addPort(_vPorts.at(i));

			}
		}
		else
		{
			cout << "Video Outport Configs or Ports or Resolutions is NULL. ..."<<endl;
			throw Exception("Failed to load video outport config");
		}
	}
	catch (...) {
		cout << "VIdeo Outport Exception Thrown. ..."<<endl;
        throw Exception("Failed to load video outport config");
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
