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


#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include "dsInternal.h"

#include "dsTypes.h"
#include "dsVideoPortSettings.h"


const char* dsHdmiStatusFile="/tmp/ds_hdmi_status.bin";
const char* dsHdmiTagHotplug="hotplug";
const char* dsHdmiTagHdcp="hdcp_status";
const char* dsHdmiTagHdcpVerison="hdcp_version";

const char* dsHdmiConnected="CONNECTED";
const char* dsHdmiDisconnected="DISCONNECTED";

const char* dsHdcpUnpowered="UNPOWERED";
const char* dsHdcpUnauthenticated="UNAUTHENTICATED";
const char* dsHdcpAuthenticated="AUTHENTICATED";
const char* dsHdcpAuthenticationfailure="AUTHENTICATIONFAILURE";
const char* dsHdcpInprogress="INPROGRESS";
const char* dsHdcpPortdisabeld="PORTDISABLED";

const char* dsHdcpVersion2X="VERSION_2X";
const char* dsHdcpVersion1X="VERSION_1X";


dsHdcpStatus_t dsReadHdcpStatus() {
    dsHdcpStatus_t hdcpStatus = dsHDCP_STATUS_UNPOWERED;
    std::ifstream statusFile(dsHdmiStatusFile);
    std::string line;
    if (!statusFile.is_open()) {
        std::cerr << __func__ << __LINE__ <<"Failed to open status file." << std::endl;
        return hdcpStatus;
    }

    while (std::getline(statusFile, line)) {
        if (line.find(dsHdmiTagHdcp) == 0) {
            std::string status = line.substr(strlen(dsHdmiTagHdcp) + 1);
            if (status == dsHdcpUnpowered) hdcpStatus = dsHDCP_STATUS_UNPOWERED;
            else if (status == dsHdcpUnauthenticated) hdcpStatus = dsHDCP_STATUS_UNAUTHENTICATED;
            else if (status == dsHdcpAuthenticated) hdcpStatus = dsHDCP_STATUS_AUTHENTICATED;
            else if (status == dsHdcpAuthenticationfailure) hdcpStatus = dsHDCP_STATUS_AUTHENTICATIONFAILURE;
            else if (status == dsHdcpInprogress) hdcpStatus = dsHDCP_STATUS_INPROGRESS;
            else if (status == dsHdcpPortdisabeld) hdcpStatus = dsHDCP_STATUS_PORTDISABLED;
            return hdcpStatus; // Found the status, exit loop
        }
    }
    return hdcpStatus;
}

dsHdcpProtocolVersion_t dsReadHdcpVersion() {
    dsHdcpProtocolVersion_t hdcpVersion = dsHDCP_VERSION_1X;
    std::ifstream statusFile(dsHdmiStatusFile);
    std::string line;
    if (!statusFile.is_open()) {
        std::cerr << __func__ << __LINE__<< "Failed to open status file." << std::endl;
        return hdcpVersion;
    }

    while (std::getline(statusFile, line)) {
        if (line.find(dsHdmiTagHdcpVerison) == 0) {
            std::string version = line.substr(strlen(dsHdmiTagHdcpVerison) + 1);
            hdcpVersion = (version == dsHdcpVersion2X) ? dsHDCP_VERSION_2X : dsHDCP_VERSION_1X;
            return hdcpVersion; // Found the version, exit loop
        }
    }
    return hdcpVersion;
}

bool dsReadHotplugStatus() {
	bool connected = true;
    std::ifstream statusFile(dsHdmiStatusFile);
    std::string line;
    if (!statusFile.is_open()) {
        std::cerr << __func__ << __LINE__<< "Failed to open status file." << std::endl;
        return connected;
    }
    while (std::getline(statusFile, line)) {
        if (line.find(dsHdmiTagHotplug) == 0) {
            std::string status = line.substr(strlen(dsHdmiTagHotplug) + 1);
            connected = (status == dsHdmiConnected);
            return connected; // Found the status, exit loop
        }
    }
    return connected;
}

void readStatus() {

    bool connected = dsReadHotplugStatus();
    dsHdcpStatus_t hdcpStatus = dsReadHdcpStatus();
    dsHdcpProtocolVersion_t hdcpVersion = dsReadHdcpVersion();

    std::cout << "Client read:\n"
              << "hotplug=" << (connected ? "CONNECTED" : "DISCONNECTED") << "\n"
              << "hdcp=" << hdcpStatus << "\n"
              << "hdcp_version=" << (hdcpVersion == dsHDCP_VERSION_1X ? "VERSION_1X" : "VERSION_2X") << std::endl;
}

int main() {

    readStatus();

    return 0;
}

