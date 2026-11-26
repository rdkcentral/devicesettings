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
 * @file testWidevineDS.cpp
 * @brief On-target validation of the DSMGR (libds) APIs used by the Widevine CDM.
 *
 * The Widevine CDM (oemcrypto_engine_rdk.cpp) uses libds like this, and this utility does the same:
 *   - IARM_Bus_Init / IARM_Bus_Connect, device::Manager::Initialize()
 *   - IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, HDMI_HOTPLUG / HDCP_STATUS)
 *   - device::Host::getVideoOutputPorts() and, per VideoOutputPort: getName, getType().getName(),
 *     isEnabled, isActive, isDisplayConnected, isContentProtected, getHDCPStatus, getHDCPProtocol,
 *     getHDCPReceiverProtocol, getHDCPCurrentProtocol, GetHdmiPreference / SetHdmiPreference,
 *     enable / disable (analog "cable output" toggle)
 *   - the HDCP capability derivation of CryptoEngine::config_current_hdcp_capability() and
 *     config_maximum_hdcp_capability(), re-implemented here so the result can be checked
 *
 * It works against both the legacy dsMgr (IARM) and the DeviceSettings plugin (COM-RPC through
 * DSSmartInterfaceHandler), because it only uses the public libds API and IARM, exactly like Widevine.
 *
 * Every API call is checked (no exception, value in range) and timed. The exit code is 0 when no check
 * FAILed (SKIP is not a failure), 1 otherwise.
 *
 * Usage:  testWidevineDS [--iterations N] [--wait-events SEC [--expect-events]] [--set-pref] [--toggle-cable]
 *                        [--threads N] [--reinit N] [--soak SEC]
 *   --iterations N    repeat the read-only API sequence N times and print latency statistics (default 1)
 *   --wait-events S   wait S seconds for IARM HDMI_HOTPLUG / HDCP_STATUS events (do a hot plug or reboot the
 *                     TV meanwhile); the payload of every event is validated and, like Widevine, the ports are
 *                     read again afterwards and compared with the event (connected state, HDCP status)
 *   --expect-events   with --wait-events: FAIL instead of SKIP when no event arrived
 *   --set-pref        also run Widevine's EnableHDCPV2_2 / RevertHDCP sequence (SetHdmiPreference 2X, then
 *                     MAX) and restore the original preference afterwards. Changes the HDMI preference.
 *   --toggle-cable    also run Widevine's cable output toggle (disable / enable RF, Baseband, SVideo and
 *                     Component ports). Briefly disables analog outputs.
 *   --threads N       run the read-only sequence from N threads at once (sessions + IARM callback thread)
 *   --reinit N        N times Manager::DeInitialize() / Initialize() (engine restart), ports re-checked each time
 *   --soak S          poll once a second for S seconds, logging OK / FAILED per round; restart the DeviceSettings
 *                     plugin or WPEFramework meanwhile to check that libds recovers (needs healthy last rounds)
 *
 * Built with the other programs by sample/Makefile (needs C++11) and installed as /usr/bin/testWidevineDS.
 */

/**
* @defgroup devicesettings
* @{
* @defgroup test
* @{
**/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "host.hpp"
#include "manager.hpp"
#include "videoOutputPort.hpp"
#include "videoOutputPortType.hpp"
#include "list.hpp"
#include "exception.hpp"
#include "unsupportedOperationException.hpp"

#include "dsError.h"
#include "dsTypes.h"
#include "dsDisplay.h"

#include "libIARM.h"
#include "libIBus.h"
#include "dsMgr.h"

namespace {

typedef std::chrono::steady_clock Clock;

const char* kIarmClientName = "WidevineDSCheck";

// ---- options --------------------------------------------------------------------------------------

struct Options {
    int iterations;
    int waitEventsSec;
    bool expectEvents;
    bool setPref;
    bool toggleCable;
    int threads;
    int reinit;
    int soakSec;

    Options()
        : iterations(1)
        , waitEventsSec(0)
        , expectEvents(false)
        , setPref(false)
        , toggleCable(false)
        , threads(0)
        , reinit(0)
        , soakSec(0)
    {
    }
};

void usage(const char* prog)
{
    printf("Usage: %s [--iterations N] [--wait-events SEC [--expect-events]] [--set-pref] [--toggle-cable]\n", prog);
    printf("          [--threads N] [--reinit N] [--soak SEC]\n");
    printf("  --iterations N    repeat the read-only API sequence N times, print latency statistics (default 1)\n");
    printf("  --wait-events S   wait S seconds for IARM HDMI_HOTPLUG / HDCP_STATUS events; validates each payload and,\n");
    printf("                    like Widevine, re-reads the ports after the events and compares them with the payload\n");
    printf("  --expect-events   with --wait-events: FAIL (instead of SKIP) when no event arrived\n");
    printf("  --set-pref        run Widevine's SetHdmiPreference 2X / MAX sequence (changes the preference, restored)\n");
    printf("  --toggle-cable    run Widevine's analog cable output disable / enable (briefly disables analog ports)\n");
    printf("  --threads N       run the read-only sequence from N threads at once (Widevine sessions + IARM thread)\n");
    printf("  --reinit N        N times Manager::DeInitialize() / Initialize() (Widevine engine restart), then re-check\n");
    printf("  --soak S          poll the APIs once a second for S seconds and log OK / FAILED per round; restart the\n");
    printf("                    DeviceSettings plugin or WPEFramework meanwhile to check that libds recovers\n");
}

bool parseOptions(int argc, char* argv[], Options& opt)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            opt.iterations = atoi(argv[++i]);
            if (opt.iterations < 1) {
                opt.iterations = 1;
            }
        } else if (strcmp(argv[i], "--wait-events") == 0 && i + 1 < argc) {
            opt.waitEventsSec = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--expect-events") == 0) {
            opt.expectEvents = true;
        } else if (strcmp(argv[i], "--set-pref") == 0) {
            opt.setPref = true;
        } else if (strcmp(argv[i], "--toggle-cable") == 0) {
            opt.toggleCable = true;
        } else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
            opt.threads = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--reinit") == 0 && i + 1 < argc) {
            opt.reinit = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--soak") == 0 && i + 1 < argc) {
            opt.soakSec = atoi(argv[++i]);
        } else {
            return false;
        }
    }
    return true;
}

// ---- result reporting -----------------------------------------------------------------------------

enum State { PASS, FAIL, SKIP };

struct Outcome {
    State state;
    std::string name;
    std::string detail;
};

std::vector<Outcome> g_results;
std::mutex g_lock; // results, statistics and console output are shared with the worker threads
std::atomic<bool> g_quiet(false); // later iterations / worker threads: only failures are recorded

const char* stateName(State s)
{
    return (s == PASS) ? "PASS" : ((s == FAIL) ? "FAIL" : "SKIP");
}

void record(State s, const std::string& name, const char* fmt, ...) __attribute__((format(printf, 3, 4)));

void record(State s, const std::string& name, const char* fmt, ...)
{
    if (g_quiet && s != FAIL) {
        return;
    }
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    Outcome o;
    o.state  = s;
    o.name   = name;
    o.detail = buffer;

    std::lock_guard<std::mutex> guard(g_lock);
    g_results.push_back(o);
    printf("[%s] %-42s %s\n", stateName(s), name.c_str(), buffer);
    fflush(stdout);
}

// ---- latency statistics ---------------------------------------------------------------------------

struct Stat {
    double minMs;
    double maxMs;
    double sumMs;
    int count;

    Stat()
        : minMs(1e12)
        , maxMs(0)
        , sumMs(0)
        , count(0)
    {
    }
};

std::map<std::string, Stat> g_stats;

double elapsedMs(const Clock::time_point& since)
{
    return std::chrono::duration<double, std::milli>(Clock::now() - since).count();
}

void addStat(const std::string& api, double ms)
{
    std::lock_guard<std::mutex> guard(g_lock);
    Stat& s = g_stats[api];
    if (ms < s.minMs) {
        s.minMs = ms;
    }
    if (ms > s.maxMs) {
        s.maxMs = ms;
    }
    s.sumMs += ms;
    s.count++;
}

// ---- API check helper -----------------------------------------------------------------------------

// Runs one libds call, times it and validates the value. All libds APIs used by Widevine can throw
// device::Exception; isActive() throws UnsupportedOperationException when RxSense is not supported.
bool checkApi(const std::string& scope, const char* api, const std::function<int()>& fn,
              const std::function<bool(int)>& valid, int& value, bool unsupportedIsSkip)
{
    const std::string name = scope + "." + api;
    std::string error;
    bool unsupported = false;
    bool ok          = false;

    const Clock::time_point t0 = Clock::now();
    try {
        value = fn();
        ok    = true;
    } catch (const device::UnsupportedOperationException& e) {
        unsupported = true;
        error       = e.what();
    } catch (const device::Exception& e) {
        char code[32];
        snprintf(code, sizeof(code), " (code %d)", e.getCode());
        error = std::string(e.what()) + code;
    } catch (const std::exception& e) {
        error = e.what();
    } catch (...) {
        error = "unknown exception";
    }
    const double took = elapsedMs(t0);
    addStat(api, took);

    if (ok) {
        if (valid(value)) {
            record(PASS, name, "%d (%.1f ms)", value, took);
            return true;
        }
        record(FAIL, name, "value %d is out of range (%.1f ms)", value, took);
        return false;
    }
    if (unsupported && unsupportedIsSkip) {
        record(SKIP, name, "not supported: %s", error.c_str());
        return false;
    }
    record(FAIL, name, "exception: %s (%.1f ms)", error.c_str(), took);
    return false;
}

bool anyValue(int) { return true; }
bool isBool(int v) { return v == 0 || v == 1; }
bool validHdcpStatus(int v) { return v >= dsHDCP_STATUS_UNPOWERED && v < dsHDCP_STATUS_MAX; }
bool validHdcpProtocol(int v) { return v >= dsHDCP_VERSION_1X && v <= dsHDCP_VERSION_MAX; }

// ---- IARM events (same handler shape as CryptoEngine::dsHdmiEventHandler) -------------------------

std::atomic<int> g_hotplugEvents(0);
std::atomic<int> g_hdcpEvents(0);
std::atomic<int> g_badEvents(0);
std::atomic<int> g_eventSeq(0);     // bumped after every valid event
std::atomic<int> g_lastHotplug(-1); // last dsDisplayEvent_t seen
std::atomic<int> g_lastHdcp(-1);    // last dsHdcpStatus_t seen
std::atomic<int> g_lastKind(0);     // 1 = last event was HDMI_HOTPLUG, 2 = HDCP_STATUS

void iarmEventHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
{
    if (owner == NULL || strcmp(owner, IARM_BUS_DSMGR_NAME) != 0) {
        printf("  [event] unexpected owner '%s', event %d\n", owner ? owner : "(null)", static_cast<int>(eventId));
        g_badEvents++;
        return;
    }
    if (data == NULL || len < sizeof(int)) {
        printf("  [event] event %d without usable payload (len %zu)\n", static_cast<int>(eventId), len);
        g_badEvents++;
        return;
    }

    const IARM_Bus_DSMgr_EventData_t* eventData = static_cast<const IARM_Bus_DSMgr_EventData_t*>(data);
    switch (eventId) {
    case IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG: {
        const int event = eventData->data.hdmi_hpd.event;
        const bool good = (event == dsDISPLAY_EVENT_CONNECTED || event == dsDISPLAY_EVENT_DISCONNECTED);
        printf("  [event] HDMI_HOTPLUG  event=%d (%s)%s\n", event,
               (event == dsDISPLAY_EVENT_CONNECTED) ? "connected" : ((event == dsDISPLAY_EVENT_DISCONNECTED) ? "disconnected" : "?"),
               good ? "" : "  <-- INVALID value");
        if (good) {
            g_lastHotplug = event;
            g_lastKind    = 1;
            g_hotplugEvents++;
            g_eventSeq++;
        } else {
            g_badEvents++;
        }
        break;
    }
    case IARM_BUS_DSMGR_EVENT_HDCP_STATUS: {
        const int status = eventData->data.hdmi_hdcp.hdcpStatus;
        const bool good  = validHdcpStatus(status);
        printf("  [event] HDCP_STATUS   status=%d%s\n", status, good ? "" : "  <-- INVALID value");
        if (good) {
            g_lastHdcp = status;
            g_lastKind = 2;
            g_hdcpEvents++;
            g_eventSeq++;
        } else {
            g_badEvents++;
        }
        break;
    }
    default:
        printf("  [event] unexpected DSMGR event %d\n", static_cast<int>(eventId));
        g_badEvents++;
        break;
    }
    fflush(stdout);
}

// ---- Widevine HDCP capability derivation ----------------------------------------------------------
// Copy of the logic in oemcrypto_engine_rdk.cpp (CryptoEngine::getHDCPVersion,
// config_current_hdcp_capability, config_maximum_hdcp_capability), values of OEMCrypto_HDCP_Capability.

enum WvHdcp {
    WV_HDCP_NONE              = 0,
    WV_HDCP_V1                = 1,
    WV_HDCP_V2_2              = 4,
    WV_HDCP_NO_DIGITAL_OUTPUT = 0xff
};

const char* wvName(WvHdcp v)
{
    switch (v) {
    case WV_HDCP_NONE:              return "HDCP_NONE";
    case WV_HDCP_V1:                return "HDCP_V1";
    case WV_HDCP_V2_2:              return "HDCP_V2_2";
    case WV_HDCP_NO_DIGITAL_OUTPUT: return "HDCP_NO_DIGITAL_OUTPUT";
    }
    return "?";
}

WvHdcp wvHdcpVersion(device::VideoOutputPort& port, bool current)
{
    WvHdcp retVal                              = WV_HDCP_NONE;
    bool isConnected                           = false;
    bool isHDCPEnabled                         = true;
    dsHdcpProtocolVersion_t hdcpReceiverProtocol = dsHDCP_VERSION_MAX;
    dsHdcpProtocolVersion_t hdcpCurrentProtocol  = dsHDCP_VERSION_MAX;

    try {
        isConnected = port.isDisplayConnected();
        (void)port.getHDCPProtocol();
        if (isConnected) {
            isHDCPEnabled        = port.isContentProtected();
            hdcpReceiverProtocol = static_cast<dsHdcpProtocolVersion_t>(port.getHDCPReceiverProtocol());
            hdcpCurrentProtocol  = static_cast<dsHdcpProtocolVersion_t>(port.getHDCPCurrentProtocol());
        } else {
            isHDCPEnabled = false;
        }
    } catch (const std::exception&) {
        // Widevine logs and goes on with the defaults
    }

    if (isHDCPEnabled) {
        if (current) {
            retVal = (hdcpCurrentProtocol == dsHDCP_VERSION_2X) ? WV_HDCP_V2_2 : WV_HDCP_V1;
        } else {
            retVal = (hdcpReceiverProtocol == dsHDCP_VERSION_2X) ? WV_HDCP_V2_2 : WV_HDCP_V1;
        }
    }
    if (retVal == WV_HDCP_NONE) {
        retVal = WV_HDCP_V1;
    }
    return retVal;
}

WvHdcp wvCurrentCapability(device::List<device::VideoOutputPort>& ports, WvHdcp previous)
{
    WvHdcp retVal = WV_HDCP_NO_DIGITAL_OUTPUT;

    for (size_t i = 0; i < ports.size(); i++) {
        device::VideoOutputPort& port = ports.at(i);

        bool isConnected = false;
        int hdcpReason   = dsHDCP_STATUS_UNPOWERED;
        try {
            isConnected = port.isDisplayConnected();
            hdcpReason  = port.getHDCPStatus();
        } catch (...) {
        }
        if (hdcpReason == dsHDCP_STATUS_UNAUTHENTICATED || hdcpReason == dsHDCP_STATUS_INPROGRESS) {
            return previous; // authentication in progress, keep the previous level
        } else if (hdcpReason == dsHDCP_STATUS_AUTHENTICATIONFAILURE) {
            return WV_HDCP_NONE;
        }

        bool isEnabled = false;
        try {
            isEnabled = port.isEnabled();
        } catch (...) {
        }
        if (isEnabled && isConnected && port.getType().getName() == "HDMI") {
            WvHdcp value = WV_HDCP_NONE;
            try {
                value = port.isContentProtected() ? wvHdcpVersion(port, true) : WV_HDCP_NONE;
            } catch (...) {
            }
            if (value < retVal) {
                retVal = value;
            }
        }
    }
    return retVal;
}

WvHdcp wvMaximumCapability(device::List<device::VideoOutputPort>& ports)
{
    WvHdcp retVal = WV_HDCP_NO_DIGITAL_OUTPUT;

    for (size_t i = 0; i < ports.size(); i++) {
        device::VideoOutputPort& port = ports.at(i);

        bool isConnected = false;
        bool isEnabled   = false;
        try {
            isConnected = port.isDisplayConnected();
        } catch (...) {
        }
        try {
            isEnabled = port.isEnabled();
        } catch (...) {
        }
        if (isEnabled && isConnected && port.getType().getName() == "HDMI") {
            const WvHdcp value = port.isContentProtected() ? wvHdcpVersion(port, false) : WV_HDCP_NONE;
            if (value > retVal || retVal == WV_HDCP_NO_DIGITAL_OUTPUT) {
                retVal = value;
            }
        }
    }
    return retVal;
}

// ---- test steps -----------------------------------------------------------------------------------

bool isAnalogPort(const std::string& type)
{
    return type == "RF" || type == "Baseband" || type == "SVideo" || type == "Component";
}

// Read-only part: everything CryptoEngine calls on a VideoOutputPort while deriving the HDCP level.
void checkPort(device::VideoOutputPort& port)
{
    const std::string name = port.getName();
    const std::string type = port.getType().getName();
    int v                  = 0;

    if (!g_quiet) {
        printf("\n--- port %s (type %s, id %d) ---\n", name.c_str(), type.c_str(), port.getType().getId());
    }
    if (name.empty() || type.empty()) {
        record(FAIL, name + ".identity", "empty port name or type name");
    }

    checkApi(name, "isEnabled", [&]() -> int { return port.isEnabled() ? 1 : 0; }, isBool, v, false);
    checkApi(name, "isActive", [&]() -> int { return port.isActive() ? 1 : 0; }, isBool, v, true);

    bool connected = false;
    if (checkApi(name, "isDisplayConnected", [&]() -> int { return port.isDisplayConnected() ? 1 : 0; }, isBool, v, false)) {
        connected = (v == 1);
    }

    checkApi(name, "getType.isHDCPSupported", [&]() -> int { return port.getType().isHDCPSupported() ? 1 : 0; }, isBool, v, false);

    // setVideoOutputPorts() reads these two on every port, whatever its type or connection state
    checkApi(name, "isContentProtected", [&]() -> int { return port.isContentProtected() ? 1 : 0; }, isBool, v, false);
    checkApi(name, "getHDCPStatus", [&]() -> int { return port.getHDCPStatus(); }, validHdcpStatus, v, false);

    if (type != "HDMI") {
        record(SKIP, name + ".hdcp", "HDCP protocol / preference APIs are only used by Widevine on HDMI ports");
        return;
    }

    checkApi(name, "getHDCPProtocol", [&]() -> int { return port.getHDCPProtocol(); }, validHdcpProtocol, v, false);
    checkApi(name, "GetHdmiPreference", [&]() -> int { return port.GetHdmiPreference(); }, validHdcpProtocol, v, false);

    if (connected) {
        checkApi(name, "getHDCPReceiverProtocol", [&]() -> int { return port.getHDCPReceiverProtocol(); }, validHdcpProtocol, v, false);
        checkApi(name, "getHDCPCurrentProtocol", [&]() -> int { return port.getHDCPCurrentProtocol(); }, validHdcpProtocol, v, false);
    } else {
        record(SKIP, name + ".connectedOnly", "display not connected: receiver / current protocol skipped");
    }
}

// Widevine keeps a copy of the port list (mVideoPorts_ = getVideoOutputPorts()) and copies single ports by
// value (ToggleCableOutput). The copies must behave like the originals.
void checkCopies(device::List<device::VideoOutputPort>& ports)
{
    try {
        device::List<device::VideoOutputPort> listCopy = ports;
        if (listCopy.size() != ports.size()) {
            record(FAIL, "ports.copy", "list copy has %zu ports, original %zu", listCopy.size(), ports.size());
            return;
        }
        for (size_t i = 0; i < listCopy.size(); i++) {
            device::VideoOutputPort portCopy = listCopy.at(i);
            device::VideoOutputPort& orig    = ports.at(i);
            if (portCopy.getName() != orig.getName() || portCopy.isDisplayConnected() != orig.isDisplayConnected()
                || portCopy.isEnabled() != orig.isEnabled() || portCopy.getHDCPStatus() != orig.getHDCPStatus()) {
                record(FAIL, "ports.copy", "copy of %s differs from the original", orig.getName().c_str());
                return;
            }
        }
        record(PASS, "ports.copy", "list copy and per-port copies of %zu port(s) behave like the originals", ports.size());
    } catch (const std::exception& e) {
        record(FAIL, "ports.copy", "exception: %s", e.what());
    } catch (...) {
        record(FAIL, "ports.copy", "unknown exception");
    }
}

// Two enumerations must give the same ports and the same state (handles are cached by libds / Widevine).
void checkRepeatable()
{
    try {
        device::List<device::VideoOutputPort> a = device::Host::getInstance().getVideoOutputPorts();
        device::List<device::VideoOutputPort> b = device::Host::getInstance().getVideoOutputPorts();
        if (a.size() != b.size()) {
            record(FAIL, "ports.repeatable", "port count differs between two enumerations: %zu vs %zu", a.size(), b.size());
            return;
        }
        for (size_t i = 0; i < a.size(); i++) {
            device::VideoOutputPort& pa = a.at(i);
            device::VideoOutputPort& pb = b.at(i);
            if (pa.getName() != pb.getName()) {
                record(FAIL, "ports.repeatable", "port %zu name differs: '%s' vs '%s'", i, pa.getName().c_str(), pb.getName().c_str());
                return;
            }
            if (pa.isDisplayConnected() != pb.isDisplayConnected()) {
                record(FAIL, "ports.repeatable", "%s isDisplayConnected differs between two enumerations", pa.getName().c_str());
                return;
            }
        }
        record(PASS, "ports.repeatable", "%zu port(s) identical in two enumerations", a.size());
    } catch (const std::exception& e) {
        record(FAIL, "ports.repeatable", "exception: %s", e.what());
    } catch (...) {
        record(FAIL, "ports.repeatable", "unknown exception");
    }
}

// Widevine: EnableHDCPV2_2() sets HDMI preference 2X, RevertHDCP() sets MAX (= auto).
void checkHdmiPreference(device::List<device::VideoOutputPort>& ports)
{
    bool found = false;
    for (size_t i = 0; i < ports.size(); i++) {
        device::VideoOutputPort& port = ports.at(i);
        if (port.getType().getName() != "HDMI") {
            continue;
        }
        found                  = true;
        const std::string name = port.getName();
        int original           = dsHDCP_VERSION_MAX;
        int v                  = 0;

        if (!checkApi(name, "pref.read", [&]() -> int { return port.GetHdmiPreference(); }, validHdcpProtocol, original, false)) {
            continue;
        }
        if (checkApi(name, "pref.set2X", [&]() -> int { return port.SetHdmiPreference(dsHDCP_VERSION_2X) ? 1 : 0; }, isBool, v, false)) {
            if (v != 1) {
                record(FAIL, name + ".pref.set2X.result", "SetHdmiPreference(2X) returned false");
            }
            int back = -1;
            if (checkApi(name, "pref.readBack2X", [&]() -> int { return port.GetHdmiPreference(); }, validHdcpProtocol, back, false) && back != dsHDCP_VERSION_2X) {
                record(FAIL, name + ".pref.readBack2X.value", "preference is %d after setting 2X", back);
            }
        }
        if (checkApi(name, "pref.revertMAX", [&]() -> int { return port.SetHdmiPreference(dsHDCP_VERSION_MAX) ? 1 : 0; }, isBool, v, false)) {
            int back = -1;
            if (checkApi(name, "pref.readBackMAX", [&]() -> int { return port.GetHdmiPreference(); }, validHdcpProtocol, back, false) && back != dsHDCP_VERSION_MAX) {
                record(FAIL, name + ".pref.readBackMAX.value", "preference is %d after reverting to MAX", back);
            }
        }
        if (original != dsHDCP_VERSION_MAX) {
            checkApi(name, "pref.restore", [&]() -> int { return port.SetHdmiPreference(static_cast<dsHdcpProtocolVersion_t>(original)) ? 1 : 0; }, isBool, v, false);
        }
    }
    if (!found) {
        record(SKIP, "pref", "no HDMI port");
    }
}

// Widevine: ToggleCableOutput() disables / enables the analog ports.
void checkCableToggle(device::List<device::VideoOutputPort>& ports)
{
    bool found = false;
    for (size_t i = 0; i < ports.size(); i++) {
        device::VideoOutputPort& port = ports.at(i);
        const std::string type        = port.getType().getName();
        if (!isAnalogPort(type)) {
            continue;
        }
        found                  = true;
        const std::string name = port.getName();
        int v                  = 0;

        if (checkApi(name, "cable.disable", [&]() -> int { port.disable(); return 1; }, anyValue, v, false)) {
            int enabled = -1;
            if (checkApi(name, "cable.isEnabledAfterDisable", [&]() -> int { return port.isEnabled() ? 1 : 0; }, isBool, enabled, false) && enabled != 0) {
                record(FAIL, name + ".cable.disable.state", "port still enabled after disable()");
            }
        }
        if (checkApi(name, "cable.enable", [&]() -> int { port.enable(); return 1; }, anyValue, v, false)) {
            int enabled = -1;
            if (checkApi(name, "cable.isEnabledAfterEnable", [&]() -> int { return port.isEnabled() ? 1 : 0; }, isBool, enabled, false) && enabled != 1) {
                record(FAIL, name + ".cable.enable.state", "port not enabled after enable()");
            }
        }
    }
    if (!found) {
        record(SKIP, "cable", "no RF / Baseband / SVideo / Component port, Widevine logs 'Unable to locate cable output'");
    }
}

// What Widevine does after an event: it drops its port cache and reads the ports again. The fresh state has
// to agree with the event payload, otherwise DSController broadcast something that does not match the HAL.
void verifyEventState()
{
    const int hotplug = g_lastHotplug.load();
    const int hdcp    = g_lastHdcp.load();
    const bool hdcpLast = (g_lastKind.load() == 2); // an older HDCP status may legitimately have changed since

    try {
        device::List<device::VideoOutputPort> fresh = device::Host::getInstance().getVideoOutputPorts();
        bool checked = false;
        for (size_t i = 0; i < fresh.size(); i++) {
            device::VideoOutputPort& port = fresh.at(i);
            if (port.getType().getName() != "HDMI") {
                continue;
            }
            checked = true;
            if (hotplug >= 0) {
                const bool connected = port.isDisplayConnected();
                const bool expected  = (hotplug == dsDISPLAY_EVENT_CONNECTED);
                record((connected == expected) ? PASS : FAIL, "events.state.hotplug",
                       "%s: event says %s, isDisplayConnected()=%d", port.getName().c_str(),
                       expected ? "connected" : "disconnected", connected ? 1 : 0);
            }
            if (hdcpLast && hdcp >= 0) {
                const int status = port.getHDCPStatus();
                record((status == hdcp) ? PASS : FAIL, "events.state.hdcp", "%s: event says %d, getHDCPStatus()=%d",
                       port.getName().c_str(), hdcp, status);
            }
        }
        if (!checked) {
            record(SKIP, "events.state", "no HDMI port to compare with the events");
        }
    } catch (const std::exception& e) {
        record(FAIL, "events.state", "exception while re-reading the ports: %s", e.what());
    } catch (...) {
        record(FAIL, "events.state", "unknown exception while re-reading the ports");
    }
}

void waitForEvents(const Options& opt)
{
    printf("\n--- waiting %d s for IARM HDMI_HOTPLUG / HDCP_STATUS events (do an HDMI hot plug now) ---\n", opt.waitEventsSec);

    int checkedSeq              = 0;
    const Clock::time_point t0 = Clock::now();
    while (elapsedMs(t0) < opt.waitEventsSec * 1000.0) {
        usleep(100 * 1000);
        const int seq = g_eventSeq.load();
        if (seq == checkedSeq) {
            continue;
        }
        usleep(500 * 1000); // let a burst of events settle, only the final state is comparable
        if (seq != g_eventSeq.load()) {
            continue;
        }
        checkedSeq = seq;
        verifyEventState();
    }
    if (checkedSeq != g_eventSeq.load()) {
        usleep(500 * 1000);
        verifyEventState();
    }

    const int hotplug = g_hotplugEvents.load();
    const int hdcp    = g_hdcpEvents.load();
    const int bad     = g_badEvents.load();

    if (bad != 0) {
        record(FAIL, "events.payload", "%d event(s) with an invalid owner / payload / value", bad);
    } else {
        record(PASS, "events.payload", "no invalid event received");
    }
    const State missing = opt.expectEvents ? FAIL : SKIP;
    if (hotplug > 0) {
        record(PASS, "events.HDMI_HOTPLUG", "%d event(s) received", hotplug);
    } else {
        record(missing, "events.HDMI_HOTPLUG", "no event received in %d s", opt.waitEventsSec);
    }
    if (hdcp > 0) {
        record(PASS, "events.HDCP_STATUS", "%d event(s) received", hdcp);
    } else {
        record(missing, "events.HDCP_STATUS", "no event received in %d s", opt.waitEventsSec);
    }
}

size_t failCount()
{
    std::lock_guard<std::mutex> guard(g_lock);
    size_t n = 0;
    for (size_t i = 0; i < g_results.size(); i++) {
        if (g_results[i].state == FAIL) {
            n++;
        }
    }
    return n;
}

// Widevine runs the capability queries from several session threads while the IARM thread delivers events.
void checkThreads(const Options& opt)
{
    const int rounds = (opt.iterations > 1) ? opt.iterations : 20;
    printf("\n--- %d threads x %d rounds of the read-only sequence ---\n", opt.threads, rounds);

    const size_t failsBefore = failCount();
    const Clock::time_point t0 = Clock::now();
    g_quiet = true;

    std::vector<std::thread> workers;
    for (int t = 0; t < opt.threads; t++) {
        workers.push_back(std::thread([rounds]() {
            try {
                WvHdcp previous = WV_HDCP_NO_DIGITAL_OUTPUT;
                for (int r = 0; r < rounds; r++) {
                    device::List<device::VideoOutputPort> list = device::Host::getInstance().getVideoOutputPorts();
                    for (size_t i = 0; i < list.size(); i++) {
                        checkPort(list.at(i));
                    }
                    previous = wvCurrentCapability(list, previous);
                    (void)wvMaximumCapability(list);
                }
            } catch (const std::exception& e) {
                record(FAIL, "threads.worker", "exception: %s", e.what());
            } catch (...) {
                record(FAIL, "threads.worker", "unknown exception");
            }
        }));
    }
    for (size_t t = 0; t < workers.size(); t++) {
        workers[t].join();
    }
    g_quiet = false;

    const size_t failed = failCount() - failsBefore;
    record((failed == 0) ? PASS : FAIL, "threads", "%d threads x %d rounds in %.0f ms, %zu failed check(s)", opt.threads, rounds,
           elapsedMs(t0), failed);
}

// Widevine's engine restart: Manager::DeInitialize() closes the plugin channel, Initialize() opens it again.
void checkReinit(const Options& opt)
{
    printf("\n--- %d x Manager::DeInitialize / Initialize ---\n", opt.reinit);

    size_t expectedPorts = 0;
    try {
        expectedPorts = device::Host::getInstance().getVideoOutputPorts().size();
    } catch (...) {
    }

    for (int n = 1; n <= opt.reinit; n++) {
        const Clock::time_point t0 = Clock::now();
        try {
            device::Manager::DeInitialize();
            device::Manager::Initialize();
        } catch (const std::exception& e) {
            record(FAIL, "reinit", "cycle %d: exception: %s", n, e.what());
            return;
        } catch (...) {
            record(FAIL, "reinit", "cycle %d: unknown exception", n);
            return;
        }
        addStat("reinit.cycle", elapsedMs(t0));

        try {
            device::List<device::VideoOutputPort> list = device::Host::getInstance().getVideoOutputPorts();
            if (list.size() != expectedPorts) {
                record(FAIL, "reinit", "cycle %d: %zu port(s) after restart, expected %zu", n, list.size(), expectedPorts);
                return;
            }
            const bool quiet = g_quiet;
            g_quiet          = true;
            for (size_t i = 0; i < list.size(); i++) {
                checkPort(list.at(i));
            }
            g_quiet = quiet;
        } catch (const std::exception& e) {
            record(FAIL, "reinit", "cycle %d: ports unusable after restart: %s", n, e.what());
            return;
        } catch (...) {
            record(FAIL, "reinit", "cycle %d: ports unusable after restart", n);
            return;
        }
    }
    record(PASS, "reinit", "%d restart cycle(s), ports usable after each", opt.reinit);
}

// Polls once a second without recording failures, so a plugin / WPEFramework restart in the middle shows up as a
// few FAILED rounds followed by OK rounds again. The check passes when the last rounds are healthy.
void soak(const Options& opt)
{
    printf("\n--- soak %d s: restart the DeviceSettings plugin / WPEFramework meanwhile ---\n", opt.soakSec);

    int failedRounds = 0;
    int okInARow     = 0;
    for (int s = 0; s < opt.soakSec; s++) {
        std::string error;
        bool ok = false;
        try {
            device::List<device::VideoOutputPort> list = device::Host::getInstance().getVideoOutputPorts();
            for (size_t i = 0; i < list.size(); i++) {
                device::VideoOutputPort& port = list.at(i);
                (void)port.isDisplayConnected();
                (void)port.getHDCPStatus();
            }
            ok = (list.size() != 0);
            if (!ok) {
                error = "no port returned";
            }
        } catch (const std::exception& e) {
            error = e.what();
        } catch (...) {
            error = "unknown exception";
        }
        if (ok) {
            okInARow++;
        } else {
            okInARow = 0;
            failedRounds++;
        }
        printf("  [soak] t=%3ds %s%s%s\n", s + 1, ok ? "OK" : "FAILED", ok ? "" : ": ", error.c_str());
        fflush(stdout);
        sleep(1);
    }
    const bool healthy = (okInARow >= 3) || (opt.soakSec < 3 && failedRounds == 0);
    record(healthy ? PASS : FAIL, "soak", "%d of %d round(s) failed, %d healthy round(s) at the end", failedRounds, opt.soakSec, okInARow);
}

void printSummary()
{
    int pass = 0, fail = 0, skip = 0;
    for (size_t i = 0; i < g_results.size(); i++) {
        (g_results[i].state == PASS) ? pass++ : ((g_results[i].state == FAIL) ? fail++ : skip++);
    }

    if (!g_stats.empty()) {
        printf("\n=== API latency (ms) ===\n%-28s %6s %9s %9s %9s\n", "api", "calls", "min", "avg", "max");
        for (std::map<std::string, Stat>::const_iterator it = g_stats.begin(); it != g_stats.end(); ++it) {
            const Stat& s = it->second;
            printf("%-28s %6d %9.2f %9.2f %9.2f\n", it->first.c_str(), s.count, s.minMs, s.sumMs / s.count, s.maxMs);
        }
    }
    if (fail != 0) {
        printf("\n=== FAILED checks ===\n");
        for (size_t i = 0; i < g_results.size(); i++) {
            if (g_results[i].state == FAIL) {
                printf("  %-42s %s\n", g_results[i].name.c_str(), g_results[i].detail.c_str());
            }
        }
    }
    printf("\n=== testWidevineDS: %d passed, %d failed, %d skipped ===\n", pass, fail, skip);
}

} // namespace

int main(int argc, char* argv[])
{
    Options opt;
    if (!parseOptions(argc, argv, opt)) {
        usage(argv[0]);
        return 2;
    }

    // 1. IARM, as CryptoEngine::initialize_iarm_bus(); "already initialized / connected" is not an error
    IARM_Result_t iarm = IARM_Bus_Init(kIarmClientName);
    record((iarm == IARM_RESULT_SUCCESS) ? PASS : SKIP, "iarm.init", "IARM_Bus_Init rc=%d%s", static_cast<int>(iarm),
           (iarm == IARM_RESULT_SUCCESS) ? "" : " (already initialized?)");
    iarm = IARM_Bus_Connect();
    record((iarm == IARM_RESULT_SUCCESS) ? PASS : SKIP, "iarm.connect", "IARM_Bus_Connect rc=%d%s", static_cast<int>(iarm),
           (iarm == IARM_RESULT_SUCCESS) ? "" : " (already connected?)");

    // 2. device::Manager::Initialize(), nothing else can work without it
    {
        const Clock::time_point t0 = Clock::now();
        try {
            device::Manager::Initialize();
            record(PASS, "Manager::Initialize", "done in %.1f ms", elapsedMs(t0));
        } catch (const std::exception& e) {
            record(FAIL, "Manager::Initialize", "exception after %.1f ms: %s", elapsedMs(t0), e.what());
            printSummary();
            return 1;
        } catch (...) {
            record(FAIL, "Manager::Initialize", "unknown exception after %.1f ms", elapsedMs(t0));
            printSummary();
            return 1;
        }
    }

    // 3. events, registered like CryptoEngine does
    iarm = IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG, iarmEventHandler);
    record((iarm == IARM_RESULT_SUCCESS) ? PASS : FAIL, "iarm.register.HDMI_HOTPLUG", "rc=%d", static_cast<int>(iarm));
    iarm = IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDCP_STATUS, iarmEventHandler);
    record((iarm == IARM_RESULT_SUCCESS) ? PASS : FAIL, "iarm.register.HDCP_STATUS", "rc=%d", static_cast<int>(iarm));

    // 4. the video output ports, CryptoEngine::setVideoOutputPorts()
    device::List<device::VideoOutputPort> ports;
    try {
        const Clock::time_point t0 = Clock::now();
        ports                      = device::Host::getInstance().getVideoOutputPorts();
        addStat("getVideoOutputPorts", elapsedMs(t0));
        if (ports.size() == 0) {
            record(FAIL, "Host::getVideoOutputPorts", "no video output port returned");
        } else {
            record(PASS, "Host::getVideoOutputPorts", "%zu port(s) in %.1f ms", ports.size(), elapsedMs(t0));
        }
    } catch (const std::exception& e) {
        record(FAIL, "Host::getVideoOutputPorts", "exception: %s", e.what());
    } catch (...) {
        record(FAIL, "Host::getVideoOutputPorts", "unknown exception");
    }

    if (ports.size() != 0) {
        checkRepeatable();
        checkCopies(ports);

        // 5. per port APIs and the derived Widevine HDCP levels, repeated for --iterations
        WvHdcp previous = WV_HDCP_NO_DIGITAL_OUTPUT;
        for (int it = 0; it < opt.iterations; it++) {
            g_quiet = (it > 0);
            if (opt.iterations > 1 && !g_quiet) {
                printf("\n##### iteration 1 of %d (details), the others only report failures #####\n", opt.iterations);
            }
            for (size_t i = 0; i < ports.size(); i++) {
                checkPort(ports.at(i));
            }

            const Clock::time_point t0 = Clock::now();
            const WvHdcp current       = wvCurrentCapability(ports, previous);
            addStat("wv.currentHdcpCapability", elapsedMs(t0));
            const WvHdcp maximum = wvMaximumCapability(ports);
            previous             = current;
            record(PASS, "wv.hdcpCapability", "current=%s maximum=%s", wvName(current), wvName(maximum));
            if (current != WV_HDCP_NO_DIGITAL_OUTPUT && maximum != WV_HDCP_NO_DIGITAL_OUTPUT && current > maximum) {
                record(FAIL, "wv.hdcpCapability.order", "current level %s is higher than the maximum %s", wvName(current), wvName(maximum));
            }
        }
        g_quiet = false;

        // 6. optional, state changing sequences
        if (opt.setPref) {
            printf("\n--- HDMI preference (EnableHDCPV2_2 / RevertHDCP) ---\n");
            checkHdmiPreference(ports);
        }
        if (opt.toggleCable) {
            printf("\n--- cable output toggle ---\n");
            checkCableToggle(ports);
        }
        if (opt.threads > 0) {
            checkThreads(opt);
        }
    }

    // 7. optional, events, then the long running / lifecycle checks (they re-read the ports themselves)
    if (opt.waitEventsSec > 0) {
        waitForEvents(opt);
    }
    if (opt.soakSec > 0) {
        soak(opt);
    }
    if (opt.reinit > 0) {
        checkReinit(opt);
    }

    // 8. clean up
    IARM_Bus_UnRegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG);
    IARM_Bus_UnRegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDCP_STATUS);
    try {
        device::Manager::DeInitialize();
        record(PASS, "Manager::DeInitialize", "done");
    } catch (const std::exception& e) {
        record(FAIL, "Manager::DeInitialize", "exception: %s", e.what());
    } catch (...) {
        record(FAIL, "Manager::DeInitialize", "unknown exception");
    }
    IARM_Bus_Disconnect();
    IARM_Bus_Term();

    printSummary();
    for (size_t i = 0; i < g_results.size(); i++) {
        if (g_results[i].state == FAIL) {
            return 1;
        }
    }
    return 0;
}

/** @} */
/** @} */
