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
        cout << "[DsMgr] Copying _supportedResolutions to tmpsupportedResolutions, size: " << _supportedResolutions.size() << endl;
        for (size_t idx = 0; idx < _supportedResolutions.size(); idx++) {
            const VideoResolution& res = _supportedResolutions[idx];
            cout << "[DsMgr] [" << idx << "] " << res.getName() 
                 << " pixelRes=" << res.getPixelResolution().getId()
                 << " aspectRatio=" << res.getAspectRatio().getId()
                 << " frameRate=" << res.getFrameRate().getId() << endl;
        }
        for (const VideoResolution& resolution : _supportedResolutions) {
            tmpsupportedResolutions.push_back(resolution);
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
        cout << "[DsMgr] _supportedResolutions cache after update, size: " << _supportedResolutions.size() << endl;
		for (size_t idx = 0; idx < _supportedResolutions.size(); idx++) {
			const VideoResolution& res = _supportedResolutions[idx];
			cout << "[DsMgr] [" << idx << "] " << res.getName() 
			     << " pixelRes=" << res.getPixelResolution().getId()
			     << " aspectRatio=" << res.getAspectRatio().getId()
			     << " frameRate=" << res.getFrameRate().getId() << endl;
		}
	}
	return supportedResolutions;
}


void dumpconfig(videoPortConfigs_t *config)
{
    if (nullptr == config) {
        INT_ERROR("Video config is NULL");
        return;
    }
    if ( -1 == access("/opt/dsMgrDumpDeviceConfigs", F_OK) ) {
        INT_INFO("Dumping of Device configs is disabled");
        return;
    }
    INT_INFO("\n=============== Starting to Dump VideoPort Configs ===============\n");

    int configSize = -1, portSize = -1, resolutionSize = -1;
    if (( nullptr != config->pKConfigs ) && ( nullptr != config->pKPorts ) && ( nullptr != config->pKResolutionsSettings ))
    {
        configSize = (config->pKVideoPortConfigs_size) ? *(config->pKVideoPortConfigs_size) : -1;
        portSize = (config->pKVideoPortPorts_size) ? *(config->pKVideoPortPorts_size) : -1;
        resolutionSize = (config->pKResolutionsSettings_size) ? *(config->pKResolutionsSettings_size) : -1;
        INT_INFO("pKConfigs = %p", config->pKConfigs);
        INT_INFO("pKConfigSize pointer %p = %d", config->pKVideoPortConfigs_size, configSize);
        INT_INFO("pKPorts = %p", config->pKPorts);
        INT_INFO("pKPortSize pointer %p = %d", config->pKVideoPortPorts_size, portSize);
        INT_INFO("pKResolutionsSettings = %p", config->pKResolutionsSettings);
        INT_INFO("pKResolutionsSettingsSize pointer %p = %d", config->pKResolutionsSettings_size, resolutionSize);

        INT_INFO("\n\n############### Dumping Video Resolutions Settings ############### \n\n");

        for (int i = 0; i < resolutionSize; i++) {
            dsVideoPortResolution_t *resolution = &(config->pKResolutionsSettings[i]);
            INT_INFO("resolution->name = %s", resolution->name);
            INT_INFO("resolution->pixelResolution= %d", resolution->pixelResolution);
            INT_INFO("resolution->aspectRatio= %d", resolution->aspectRatio);
            INT_INFO("resolution->stereoScopicMode= %d", resolution->stereoScopicMode);
            INT_INFO("resolution->frameRate= %d", resolution->frameRate);
            INT_INFO("resolution->interlaced= %d", resolution->interlaced);
        }

        INT_INFO("\n ############### Dumping Video Port Configurations ############### \n");

        for (int i = 0; i < configSize; i++)
        {
            const dsVideoPortTypeConfig_t *typeCfg = &(config->pKConfigs[i]);
            INT_INFO("typeCfg->typeId = %d", typeCfg->typeId);
            INT_INFO("typeCfg->name = %s", (typeCfg->name) ? typeCfg->name : "NULL");
            INT_INFO("typeCfg->dtcpSupported= %d", typeCfg->dtcpSupported);
            INT_INFO("typeCfg->hdcpSupported = %d", typeCfg->hdcpSupported);
            INT_INFO("typeCfg->restrictedResollution = %d", typeCfg->restrictedResollution);
            INT_INFO("typeCfg->numSupportedResolutions= %lu", typeCfg->numSupportedResolutions);

            INT_INFO("typeCfg->supportedResolutions = %p", typeCfg->supportedResolutions);
            INT_INFO("typeCfg->supportedResolutions->name = %s", (typeCfg->supportedResolutions->name) ? typeCfg->supportedResolutions->name : "NULL");
            INT_INFO("typeCfg->supportedResolutions->pixelResolution= %d", typeCfg->supportedResolutions->pixelResolution);
            INT_INFO("typeCfg->supportedResolutions->aspectRatio= %d", typeCfg->supportedResolutions->aspectRatio);
            INT_INFO("typeCfg->supportedResolutions->stereoScopicMode= %d", typeCfg->supportedResolutions->stereoScopicMode);
            INT_INFO("typeCfg->supportedResolutions->frameRate= %d", typeCfg->supportedResolutions->frameRate);
            INT_INFO("typeCfg->supportedResolutions->interlaced= %d", typeCfg->supportedResolutions->interlaced);
        }
        INT_INFO("\n############### Dumping Video Port Connections ###############\n");

        for (int i = 0; i < portSize; i++) {
            const dsVideoPortPortConfig_t *portCfg = &(config->pKPorts[i]);
            INT_INFO("portCfg->id.type = %d", portCfg->id.type);
            INT_INFO("portCfg->id.index = %d", portCfg->id.index);
            INT_INFO("portCfg->connectedAOP.type = %d", portCfg->connectedAOP.type);
            INT_INFO("portCfg->connectedAOP.index = %d", portCfg->connectedAOP.index);
            INT_INFO("portCfg->defaultResolution = %s", (portCfg->defaultResolution) ? portCfg->defaultResolution : "NULL");
        }
    }
    else
    {
        INT_ERROR("pKConfigs or pKPorts or pKResolutionsSettings is NULL");
    }
    INT_INFO("\n=============== Dump VideoPort Configs done ===============\n");
    INT_INFO("Exit function");
}

void VideoOutputPortConfig::load(videoPortConfigs_t* dynamicVideoPortConfigs)
{
    int configSize, portSize, resolutionSize;
    videoPortConfigs_t configuration = {0};

    INT_INFO("Enter function");
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

        INT_INFO("Using '%s' config", dynamicVideoPortConfigs ? "dynamic" : "static");
        if ( nullptr != dynamicVideoPortConfigs )
        {
            configuration = *dynamicVideoPortConfigs;
            configSize = (configuration.pKVideoPortConfigs_size) ? *(configuration.pKVideoPortConfigs_size) : -1;
            portSize = (configuration.pKVideoPortPorts_size) ? *(configuration.pKVideoPortPorts_size) : -1;
            resolutionSize = (configuration.pKResolutionsSettings_size) ? *(configuration.pKResolutionsSettings_size) : -1;
        }
        else {
            configuration.pKConfigs = kConfigs;
            configSize = dsUTL_DIM(kConfigs);
            configuration.pKVideoPortConfigs_size = &configSize;
            configuration.pKPorts = kPorts;
            portSize = dsUTL_DIM(kPorts);
            configuration.pKVideoPortPorts_size = &portSize;
            configuration.pKResolutionsSettings = kResolutions;
            resolutionSize = dsUTL_DIM(kResolutions);
            configuration.pKResolutionsSettings_size = &resolutionSize;
        }

        INT_INFO("VideoPort Config[%p] ConfigSize[%d] Port[%p] PortSize[%d] Resolutions[%p] ResolutionsSize[%d]",
            configuration.pKConfigs,
            configSize,
            configuration.pKPorts,
            portSize,
            configuration.pKResolutionsSettings,
            resolutionSize);

        dumpconfig(&configuration);

        if (( nullptr != configuration.pKConfigs ) && ( nullptr != configuration.pKPorts) && ( nullptr != configuration.pKResolutionsSettings))
        {
            /* Initialize a set of supported resolutions */
            for (int i = 0; i < resolutionSize; i++) {
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
            for (int i = 0; i < configSize; i++)
            {
                const dsVideoPortTypeConfig_t *typeCfg = &(configuration.pKConfigs[i]);
                VideoOutputPortType &vPortType = VideoOutputPortType::getInstance(typeCfg->typeId);
                vPortType.enable();
                vPortType.setRestrictedResolution(typeCfg->restrictedResollution);
            }

            /*
            * set up ports based on kPorts[]
            */
            for (int i = 0; i < portSize; i++) {
                const dsVideoPortPortConfig_t *portCfg = &(configuration.pKPorts[i]);
                if (nullptr == portCfg->defaultResolution) {
                    INT_ERROR("defaultResolution is NULL at %d,  using empty string...", i);
                }
                _vPorts.push_back(
                        VideoOutputPort((portCfg->id.type), portCfg->id.index, i,
                                AudioOutputPortType::getInstance(portCfg->connectedAOP.type).getPort(portCfg->connectedAOP.index).getId(),
                                (portCfg->defaultResolution) ? std::string(portCfg->defaultResolution) : std::string("")));
                _vPortTypes.at(portCfg->id.type).addPort(_vPorts.at(i));
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
    INT_INFO("Exit function");
}

void VideoOutputPortConfig::getVideoPortResolutions(int *pResolutionCount, dsVideoPortResolution_t *pResolutions)
{
    if (nullptr == pResolutionCount) {
        INT_ERROR("pResolutionCount is NULL");
        return;
    }
    
    std::lock_guard<std::mutex> lock(gSupportedResolutionsMutex);
    *pResolutionCount = _supportedResolutions.size();
    
    if (nullptr != pResolutions) {
        // Reverse logic: Create pResolutions from _supportedResolutions
        for (size_t i = 0; i < _supportedResolutions.size(); i++) {
            const VideoResolution& resolution = _supportedResolutions[i];
            
            // Copy resolution name
            strncpy(pResolutions[i].name, resolution.getName().c_str(), sizeof(pResolutions[i].name) - 1);
            pResolutions[i].name[sizeof(pResolutions[i].name) - 1] = '\0';
            
            // Populate resolution properties
            pResolutions[i].pixelResolution = (dsVideoResolution_t)resolution.getPixelResolution().getId();
            pResolutions[i].aspectRatio = (dsVideoAspectRatio_t)resolution.getAspectRatio().getId();
            pResolutions[i].stereoScopicMode = (dsVideoStereoScopicMode_t)resolution.getStereoscopicMode().getId();
            pResolutions[i].frameRate = (dsVideoFrameRate_t)resolution.getFrameRate().getId();
            pResolutions[i].interlaced = resolution.isInterlaced();
        }
        INT_INFO("Populated %zu video resolutions to pResolutions", _supportedResolutions.size());
    }
}

void VideoOutputPortConfig::getVideoPortVPorts(int *pPortCount, dsVideoPortPortConfig_t *pPorts)
{
    if (nullptr == pPortCount) {
        INT_ERROR("pPortCount is NULL");
        return;
    }
    
    *pPortCount = _vPorts.size();
    
    if (nullptr != pPorts) {
        // Reverse logic: Create pPorts from _vPorts
        for (size_t i = 0; i < _vPorts.size(); i++) {
            VideoOutputPort& port = _vPorts.at(i);
            
            // Populate port ID
            pPorts[i].id.type = (dsVideoPortType_t)port.getType().getId();
            pPorts[i].id.index = port.getIndex();
            
            // Populate connected audio output port
            AudioOutputPort& audioPort = port.getAudioOutputPort();
            pPorts[i].connectedAOP.type = (dsAudioPortType_t)audioPort.getType().getId();
            pPorts[i].connectedAOP.index = audioPort.getIndex();
            
            // Get default resolution name - pointer to persistent string in VideoResolution object
            const VideoResolution& defaultRes = port.getDefaultResolution();
            pPorts[i].defaultResolution = defaultRes.getName().c_str();
        }
        INT_INFO("Populated %zu video ports to pPorts", _vPorts.size());
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

extern "C" void dsGetVideoPortResolutions(int *pResolutionCount, dsVideoPortResolution_t *pResolutions)
{
	device::VideoOutputPortConfig::getInstance().getVideoPortResolutions(pResolutionCount, pResolutions);
}

extern "C" void dsGetVideoPortVPorts(int *pPortCount, dsVideoPortPortConfig_t *pPorts)
{
	device::VideoOutputPortConfig::getInstance().getVideoPortVPorts(pPortCount, pPorts);
}

/** @} */
/** @} */
