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
 * COM-RPC client used by libds in place of the legacy dsMgr IARM calls.
 *
 * Modelled on PowerController: RPC::SmartInterfaceType tracks activation of the
 * "org.rdk.DeviceSettings" plugin and is meant for non-Thunder processes, and the
 * PIDFileMonitor (ported from PowerController) re-establishes the COM-RPC channel
 * when Thunder restarts.
 *
 * Lifecycle: dsSmartInterface_Start() / dsSmartInterface_Stop() are called from device::Manager::Initialize()
 * and DeInitialize(); nothing is opened or started before that.
 */

#ifdef DEVICESETTINGS_PLUGIN_ENABLED

// System headers first: Thunder's core.h pulls <sys/inotify.h> in inside its own namespace, which would
// hide the global inotify declarations used by the PID monitor (PowerController uses the same ordering).
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "dsSmartInterfaceHandler.h"
#include "dsclientlogger.h"

#ifndef MODULE_NAME
#define MODULE_NAME DSSmartInterfaceHandler
#endif

#include <core/core.h>
#include <com/com.h>
#include <plugins/plugins.h>

#include <interfaces/IDeviceSettings.h>
#include <interfaces/IDeviceSettingsDisplay.h>
#include <interfaces/IDeviceSettingsVideoPort.h>

#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

MODULE_NAME_DECLARATION(dsSmartInterfaceHandler)

// Logging. Every line carries the module tag, the thread id and the function, e.g.
//   [DSSmartInterface][1234] Connect: COM-RPC channel opened ...
// INFO  : lifecycle, connection and plugin activation changes, handle (re)resolution
// WARN  : recoverable problems (Thunder / plugin not reachable yet, slow calls, retries)
// ERROR : failed calls and unexpected conditions
// TRACE : one line per COM-RPC call, only when the DS_SMART_INTERFACE_TRACE environment
//         variable is set (these calls can be frequent, e.g. per decrypt in Widevine)
#define SI_LOG(LEVEL, fmt, ...) LEVEL("[DSSmartInterface][%d] %s: " fmt "\n", currentThreadId(), __func__, ##__VA_ARGS__)
#define SI_INFO(fmt, ...) SI_LOG(INT_INFO, fmt, ##__VA_ARGS__)
#define SI_WARN(fmt, ...) SI_LOG(INT_WARN, fmt, ##__VA_ARGS__)
#define SI_ERROR(fmt, ...) SI_LOG(INT_ERROR, fmt, ##__VA_ARGS__)
#define SI_TRACE(fmt, ...)                       \
    do {                                         \
        if (traceEnabled()) {                    \
            SI_LOG(INT_INFO, "TRACE " fmt, ##__VA_ARGS__); \
        }                                        \
    } while (0)

using namespace WPEFramework;

namespace {

using VideoPortIf = Exchange::IDeviceSettingsVideoPort;
using DisplayIf   = Exchange::IDeviceSettingsDisplay;

const char kCallsign[]        = "org.rdk.DeviceSettings";
const char kPidFile[]         = "/tmp/wpeframework.pid";
const uint32_t kOpenTimeoutMs = 2000;

#define INVALID_PID (-1)
#define INVALID_FD (-1)
#define POLL_TIMEOUT (2000)

// ---- HAL (ds*_t) <-> entservices-apis enum mapping -------------------------------------------
// The DeviceSettings plugin API declares its own enums. Where they are value-for-value identical a
// static_cast is used and the asserts below make the build fail if either header ever drifts.
// The frame rate differs (the HAL also has 59 fps and 23 fps), see frameRateToDs().
// dsVideoPortType_t <-> VideoPortIf::VideoPort
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_RF) == static_cast<int>(VideoPortIf::DS_VIDEO_PORT_TYPE_RF), "dsVIDEOPORT_TYPE_RF != DS_VIDEO_PORT_TYPE_RF");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_BB) == static_cast<int>(VideoPortIf::DS_VIDEO_PORT_TYPE_BB), "dsVIDEOPORT_TYPE_BB != DS_VIDEO_PORT_TYPE_BB");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_SVIDEO) == static_cast<int>(VideoPortIf::DS_VIDEO_PORT_TYPE_SVIDEO), "dsVIDEOPORT_TYPE_SVIDEO != DS_VIDEO_PORT_TYPE_SVIDEO");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_1394) == static_cast<int>(VideoPortIf::DS_VIDEO_PORT_TYPE_1394), "dsVIDEOPORT_TYPE_1394 != DS_VIDEO_PORT_TYPE_1394");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_DVI) == static_cast<int>(VideoPortIf::DS_VIDEO_PORT_TYPE_DVI), "dsVIDEOPORT_TYPE_DVI != DS_VIDEO_PORT_TYPE_DVI");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_COMPONENT) == static_cast<int>(VideoPortIf::DS_VIDEO_PORT_TYPE_COMPONENT), "dsVIDEOPORT_TYPE_COMPONENT != DS_VIDEO_PORT_TYPE_COMPONENT");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_HDMI) == static_cast<int>(VideoPortIf::DS_VIDEO_PORT_TYPE_HDMI), "dsVIDEOPORT_TYPE_HDMI != DS_VIDEO_PORT_TYPE_HDMI");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_HDMI_INPUT) == static_cast<int>(VideoPortIf::DS_VIDEO_PORT_TYPE_HDMI_INPUT), "dsVIDEOPORT_TYPE_HDMI_INPUT != DS_VIDEO_PORT_TYPE_HDMI_INPUT");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_INTERNAL) == static_cast<int>(VideoPortIf::DS_VIDEO_PORT_TYPE_INTERNAL), "dsVIDEOPORT_TYPE_INTERNAL != DS_VIDEO_PORT_TYPE_INTERNAL");
// dsVideoPortType_t <-> DisplayIf::DisplayPortType
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_RF) == static_cast<int>(DisplayIf::DS_DISPLAY_PORT_TYPE_RF), "dsVIDEOPORT_TYPE_RF != DS_DISPLAY_PORT_TYPE_RF");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_BB) == static_cast<int>(DisplayIf::DS_DISPLAY_PORT_TYPE_BB), "dsVIDEOPORT_TYPE_BB != DS_DISPLAY_PORT_TYPE_BB");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_SVIDEO) == static_cast<int>(DisplayIf::DS_DISPLAY_PORT_TYPE_SVIDEO), "dsVIDEOPORT_TYPE_SVIDEO != DS_DISPLAY_PORT_TYPE_SVIDEO");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_1394) == static_cast<int>(DisplayIf::DS_DISPLAY_PORT_TYPE_1394), "dsVIDEOPORT_TYPE_1394 != DS_DISPLAY_PORT_TYPE_1394");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_DVI) == static_cast<int>(DisplayIf::DS_DISPLAY_PORT_TYPE_DVI), "dsVIDEOPORT_TYPE_DVI != DS_DISPLAY_PORT_TYPE_DVI");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_COMPONENT) == static_cast<int>(DisplayIf::DS_DISPLAY_PORT_TYPE_COMPONENT), "dsVIDEOPORT_TYPE_COMPONENT != DS_DISPLAY_PORT_TYPE_COMPONENT");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_HDMI) == static_cast<int>(DisplayIf::DS_DISPLAY_PORT_TYPE_HDMI), "dsVIDEOPORT_TYPE_HDMI != DS_DISPLAY_PORT_TYPE_HDMI");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_HDMI_INPUT) == static_cast<int>(DisplayIf::DS_DISPLAY_PORT_TYPE_HDMI_INPUT), "dsVIDEOPORT_TYPE_HDMI_INPUT != DS_DISPLAY_PORT_TYPE_HDMI_INPUT");
static_assert(static_cast<int>(dsVIDEOPORT_TYPE_INTERNAL) == static_cast<int>(DisplayIf::DS_DISPLAY_PORT_TYPE_INTERNAL), "dsVIDEOPORT_TYPE_INTERNAL != DS_DISPLAY_PORT_TYPE_INTERNAL");
// dsHdcpStatus_t <-> VideoPortIf::HDCPStatus
static_assert(static_cast<int>(dsHDCP_STATUS_UNPOWERED) == static_cast<int>(VideoPortIf::DS_HDCP_STATUS_UNPOWERED), "dsHDCP_STATUS_UNPOWERED != DS_HDCP_STATUS_UNPOWERED");
static_assert(static_cast<int>(dsHDCP_STATUS_UNAUTHENTICATED) == static_cast<int>(VideoPortIf::DS_HDCP_STATUS_UNAUTHENTICATED), "dsHDCP_STATUS_UNAUTHENTICATED != DS_HDCP_STATUS_UNAUTHENTICATED");
static_assert(static_cast<int>(dsHDCP_STATUS_AUTHENTICATED) == static_cast<int>(VideoPortIf::DS_HDCP_STATUS_AUTHENTICATED), "dsHDCP_STATUS_AUTHENTICATED != DS_HDCP_STATUS_AUTHENTICATED");
static_assert(static_cast<int>(dsHDCP_STATUS_AUTHENTICATIONFAILURE) == static_cast<int>(VideoPortIf::DS_HDCP_STATUS_AUTHENTICATIONFAILURE), "dsHDCP_STATUS_AUTHENTICATIONFAILURE != DS_HDCP_STATUS_AUTHENTICATIONFAILURE");
static_assert(static_cast<int>(dsHDCP_STATUS_INPROGRESS) == static_cast<int>(VideoPortIf::DS_HDCP_STATUS_INPROGRESS), "dsHDCP_STATUS_INPROGRESS != DS_HDCP_STATUS_INPROGRESS");
static_assert(static_cast<int>(dsHDCP_STATUS_PORTDISABLED) == static_cast<int>(VideoPortIf::DS_HDCP_STATUS_PORTDISABLED), "dsHDCP_STATUS_PORTDISABLED != DS_HDCP_STATUS_PORTDISABLED");
// dsHdcpProtocolVersion_t <-> VideoPortIf::HDCPProtocolVersion
static_assert(static_cast<int>(dsHDCP_VERSION_1X) == static_cast<int>(VideoPortIf::DS_HDCP_VERSION_1X), "dsHDCP_VERSION_1X != DS_HDCP_VERSION_1X");
static_assert(static_cast<int>(dsHDCP_VERSION_2X) == static_cast<int>(VideoPortIf::DS_HDCP_VERSION_2X), "dsHDCP_VERSION_2X != DS_HDCP_VERSION_2X");
// dsVideoAspectRatio_t <-> DisplayIf::DisplayVideoAspectRatio
static_assert(static_cast<int>(dsVIDEO_ASPECT_RATIO_4x3) == static_cast<int>(DisplayIf::DS_DISPLAY_ASPECT_RATIO_4X3), "dsVIDEO_ASPECT_RATIO_4x3 != DS_DISPLAY_ASPECT_RATIO_4X3");
static_assert(static_cast<int>(dsVIDEO_ASPECT_RATIO_16x9) == static_cast<int>(DisplayIf::DS_DISPLAY_ASPECT_RATIO_16X9), "dsVIDEO_ASPECT_RATIO_16x9 != DS_DISPLAY_ASPECT_RATIO_16X9");
// dsVideoStereoScopicMode_t <-> DisplayIf::DisplayInVideoStereoScopicMode
static_assert(static_cast<int>(dsVIDEO_SSMODE_UNKNOWN) == static_cast<int>(DisplayIf::DS_DISPLAY_SSMODE_UNKNOWN), "dsVIDEO_SSMODE_UNKNOWN != DS_DISPLAY_SSMODE_UNKNOWN");
static_assert(static_cast<int>(dsVIDEO_SSMODE_2D) == static_cast<int>(DisplayIf::DS_DISPLAY_SSMODE_2D), "dsVIDEO_SSMODE_2D != DS_DISPLAY_SSMODE_2D");
static_assert(static_cast<int>(dsVIDEO_SSMODE_3D_SIDE_BY_SIDE) == static_cast<int>(DisplayIf::DS_DISPLAY_SSMODE_3D_SIDE_BY_SIDE), "dsVIDEO_SSMODE_3D_SIDE_BY_SIDE != DS_DISPLAY_SSMODE_3D_SIDE_BY_SIDE");
static_assert(static_cast<int>(dsVIDEO_SSMODE_3D_TOP_AND_BOTTOM) == static_cast<int>(DisplayIf::DS_DISPLAY_SSMODE_3D_TOP_AND_BOTTOM), "dsVIDEO_SSMODE_3D_TOP_AND_BOTTOM != DS_DISPLAY_SSMODE_3D_TOP_AND_BOTTOM");
// dsVideoFrameRate_t <-> DisplayIf::DisplayInVideoFrameRate
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_UNKNOWN) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_UNKNOWN), "dsVIDEO_FRAMERATE_UNKNOWN != DS_DISPLAY_FRAMERATE_UNKNOWN");
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_24) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_24), "dsVIDEO_FRAMERATE_24 != DS_DISPLAY_FRAMERATE_24");
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_25) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_25), "dsVIDEO_FRAMERATE_25 != DS_DISPLAY_FRAMERATE_25");
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_30) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_30), "dsVIDEO_FRAMERATE_30 != DS_DISPLAY_FRAMERATE_30");
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_60) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_60), "dsVIDEO_FRAMERATE_60 != DS_DISPLAY_FRAMERATE_60");
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_23dot98) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_23_98), "dsVIDEO_FRAMERATE_23dot98 != DS_DISPLAY_FRAMERATE_23_98");
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_29dot97) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_29_97), "dsVIDEO_FRAMERATE_29dot97 != DS_DISPLAY_FRAMERATE_29_97");
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_50) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_50), "dsVIDEO_FRAMERATE_50 != DS_DISPLAY_FRAMERATE_50");
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_59dot94) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_59_94), "dsVIDEO_FRAMERATE_59dot94 != DS_DISPLAY_FRAMERATE_59_94");
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_100) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_100), "dsVIDEO_FRAMERATE_100 != DS_DISPLAY_FRAMERATE_100");
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_119dot88) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_119_88), "dsVIDEO_FRAMERATE_119dot88 != DS_DISPLAY_FRAMERATE_119_88");
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_120) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_120), "dsVIDEO_FRAMERATE_120 != DS_DISPLAY_FRAMERATE_120");
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_200) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_200), "dsVIDEO_FRAMERATE_200 != DS_DISPLAY_FRAMERATE_200");
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_239dot76) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_239_76), "dsVIDEO_FRAMERATE_239dot76 != DS_DISPLAY_FRAMERATE_239_76");
static_assert(static_cast<int>(dsVIDEO_FRAMERATE_240) == static_cast<int>(DisplayIf::DS_DISPLAY_FRAMERATE_240), "dsVIDEO_FRAMERATE_240 != DS_DISPLAY_FRAMERATE_240");

int currentThreadId()
{
    return static_cast<int>(syscall(SYS_gettid));
}

bool traceEnabled()
{
    static const bool enabled = (getenv("DS_SMART_INTERFACE_TRACE") != nullptr);
    return enabled;
}

long long elapsedMs(const std::chrono::steady_clock::time_point& since)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - since).count();
}

// A call taking longer than this is reported even when it succeeds.
const long long kSlowCallMs = 1000;

dsError_t invalidParam(const char* what)
{
    SI_WARN("%s - invalid parameter", what);
    return dsERR_INVALID_PARAM;
}

dsError_t toDsError(const Core::hresult rc)
{
    if (rc == Core::ERROR_NONE) {
        return dsERR_NONE;
    }
    if (rc == Core::ERROR_NOT_SUPPORTED) {
        return dsERR_OPERATION_NOT_SUPPORTED; // libds turns this into UnsupportedOperationException
    }
    return (rc == Core::ERROR_UNAVAILABLE) ? dsERR_INVALID_STATE : dsERR_GENERAL;
}

// C API must never let an exception escape into the C callers (dsVideoPort.c ...).
template <typename F>
dsError_t Guard(const char* what, F fn)
{
    try {
        return fn();
    } catch (const std::exception& e) {
        SI_ERROR("%s - exception caught: %s, returning dsERR_GENERAL", what, e.what());
    } catch (...) {
        SI_ERROR("%s - unknown exception caught, returning dsERR_GENERAL", what);
    }
    return dsERR_GENERAL;
}

// Takes the raw API value (VideoFrameRate or DisplayInVideoFrameRate, same numbering). The plugin copies the
// HAL value as it is, so the HAL 59 fps / 23 fps values reach us as 15 (== API MAX) and 16.
dsVideoFrameRate_t frameRateToDs(const int in)
{
    if (in >= static_cast<int>(dsVIDEO_FRAMERATE_UNKNOWN) && in <= static_cast<int>(dsVIDEO_FRAMERATE_240)) {
        return static_cast<dsVideoFrameRate_t>(in);
    }
    if (in == static_cast<int>(dsVIDEO_FRAMERATE_59)) {
        return dsVIDEO_FRAMERATE_59;
    }
    if (in == static_cast<int>(dsVIDEO_FRAMERATE_23)) {
        return dsVIDEO_FRAMERATE_23;
    }
    SI_WARN("unexpected API frame rate value %d, reported as unknown", in);
    return dsVIDEO_FRAMERATE_UNKNOWN;
}

dsVideoPortResolution_t toDs(const DisplayIf::DisplayVideoPortResolution& in)
{
    dsVideoPortResolution_t out;
    memset(&out, 0, sizeof(out));
    strncpy(out.name, in.name.c_str(), sizeof(out.name) - 1);
    // the plugin copies the HAL enum values as they are, so cast them back
    out.pixelResolution  = static_cast<dsVideoResolution_t>(in.pixelResolution);
    out.aspectRatio      = static_cast<dsVideoAspectRatio_t>(in.aspectRatio);
    out.stereoScopicMode = static_cast<dsVideoStereoScopicMode_t>(in.stereoScopicMode);
    out.frameRate        = frameRateToDs(static_cast<int>(in.frameRate));
    out.interlaced       = in.interlaced;
    return out;
}

/**
 * Ported from PowerController. Watches the Thunder PID file so a Thunder restart is
 * noticed, and doubles as a periodic ticker (every POLL_TIMEOUT ms) so a missed
 * inotify event, or Thunder not yet being up, never leaves the channel dead.
 *
 * Differences from the PowerController version: the parent directory is watched
 * (the PID file may not exist yet, or be deleted and recreated), it can be started
 * again after Stop(), and it keeps ticking even when inotify is unavailable.
 */
class PIDFileMonitor {
public:
    using ChangedCb = std::function<void(const int pid)>;
    using TickCb    = std::function<void()>;

    PIDFileMonitor(const std::string& pidFile, ChangedCb changed, TickCb tick)
        : _pidFile(pidFile)
        , _dir(parseDir(pidFile))
        , _name(parseName(pidFile))
        , _changed(std::move(changed))
        , _tick(std::move(tick))
        , _fd(INVALID_FD)
        , _wd(INVALID_FD)
        , _wakeupFd(INVALID_FD)
        , _shutdown(false)
    {
    }

    ~PIDFileMonitor()
    {
        Stop();
    }

    PIDFileMonitor(const PIDFileMonitor&) = delete;
    PIDFileMonitor& operator=(const PIDFileMonitor&) = delete;

    bool Running() const
    {
        return _thread && _thread->joinable();
    }

    int PID() const
    {
        return readPID();
    }

    void Run()
    {
        if (Running()) {
            return;
        }

        _shutdown = false;
        _wakeupFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (_wakeupFd < 0) {
            // not fatal, Stop() then waits for the poll timeout instead of being woken
            SI_ERROR("eventfd creation failed, err: %s", strerror(errno));
            _wakeupFd = INVALID_FD;
        }

        try {
            _thread.reset(new std::thread(&PIDFileMonitor::monitorThread, this));
            SI_INFO("started, watching %s for file %s, periodic check every %d ms", _dir.c_str(), _name.c_str(), POLL_TIMEOUT);
        } catch (const std::exception& e) {
            SI_ERROR("failed to start thread: %s", e.what());
            _thread.reset();
            closeFd(_wakeupFd);
        }
    }

    void Stop()
    {
        if (!_thread) {
            return;
        }
        if (_thread->get_id() == std::this_thread::get_id()) {
            SI_WARN("Stop() called from the monitor thread itself, ignored");
            return; // never join ourselves
        }
        SI_INFO("stopping");

        _shutdown = true;
        if (INVALID_FD != _wakeupFd) {
            const uint64_t val = 1;
            if (write(_wakeupFd, &val, sizeof(val)) < 0) {
                SI_ERROR("eventfd write failed, err: %s", strerror(errno));
            }
        }

        if (_thread->joinable()) {
            _thread->join();
        }
        _thread.reset();
        closeFd(_wakeupFd);
        SI_INFO("stopped");
    }

private:
    static std::string parseDir(const std::string& path)
    {
        const size_t slash = path.find_last_of('/');
        return (slash == std::string::npos) ? std::string(".") : (slash == 0 ? std::string("/") : path.substr(0, slash));
    }

    static std::string parseName(const std::string& path)
    {
        const size_t slash = path.find_last_of('/');
        return (slash == std::string::npos) ? path : path.substr(slash + 1);
    }

    static void closeFd(int& fd)
    {
        if (INVALID_FD != fd) {
            close(fd);
            fd = INVALID_FD;
        }
    }

    static char* trim(char* str)
    {
        while (isspace(static_cast<unsigned char>(*str))) {
            str++;
        }
        if (*str == '\0') {
            return str;
        }
        char* end = str + strlen(str) - 1;
        while (end > str && isspace(static_cast<unsigned char>(*end))) {
            end--;
        }
        *(end + 1) = '\0';
        return str;
    }

    int readPID() const
    {
        int pid = INVALID_PID;
        FILE* file = fopen(_pidFile.c_str(), "r");

        if (nullptr != file) {
            char buffer[64] = { 0 };
            if (nullptr != fgets(buffer, sizeof(buffer), file)) {
                char* text = trim(buffer);
                char* end  = nullptr;
                errno      = 0;
                const long value = strtol(text, &end, 10);
                // empty file (being rewritten), garbage or out of range is not a PID
                if (!errno && end != text && value > 0 && value <= INT_MAX) {
                    pid = static_cast<int>(value);
                }
            }
            fclose(file);
        }
        return pid;
    }

    void notifyChanged()
    {
        const int pid = readPID();
        if (INVALID_PID != pid) {
            SI_INFO("%s updated, pid=%d", _pidFile.c_str(), pid);
            try {
                _changed(pid);
            } catch (...) {
                SI_ERROR("exception in change callback");
            }
        } else {
            // truncated before being rewritten is normal, the next close-write event follows
            SI_WARN("%s changed but holds no valid PID (yet)", _pidFile.c_str());
        }
    }

    void tick()
    {
        try {
            _tick();
        } catch (...) {
            SI_ERROR("exception in tick callback");
        }
    }

    // returns false if the inotify fd must no longer be used
    bool handleInotify()
    {
        alignas(struct inotify_event) char buffer[4096];

        for (;;) {
            const ssize_t bytesRead = read(_fd, buffer, sizeof(buffer));
            if (bytesRead < 0) {
                if (errno == EAGAIN || errno == EINTR) {
                    return true;
                }
                SI_ERROR("inotify read failed, err: %s", strerror(errno));
                return false;
            }
            if (bytesRead == 0) {
                return true;
            }

            bool pidFileEvent = false;
            for (char* ptr = buffer; ptr < buffer + bytesRead;) {
                const struct inotify_event* event = reinterpret_cast<const struct inotify_event*>(ptr);
                if (event->mask & IN_Q_OVERFLOW) {
                    pidFileEvent = true; // events were lost, re-read to be safe
                } else if (event->len > 0 && _name == event->name) {
                    pidFileEvent = true;
                }
                ptr += sizeof(struct inotify_event) + event->len;
            }

            if (pidFileEvent) {
                notifyChanged();
            }
        }
    }

    void monitorThread()
    {
        _fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
        if (_fd < 0) {
            SI_ERROR("inotify_init failed, err: %s, falling back to periodic check", strerror(errno));
            _fd = INVALID_FD;
        } else {
            _wd = inotify_add_watch(_fd, _dir.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO);
            if (_wd >= 0) {
                SI_INFO("inotify watch on %s active (fd %d, wd %d)", _dir.c_str(), _fd, _wd);
            }
            if (_wd < 0) {
                SI_ERROR("inotify_add_watch(%s) failed, err: %s, falling back to periodic check", _dir.c_str(), strerror(errno));
                _wd = INVALID_FD;
                closeFd(_fd);
            }
        }

        struct pollfd pfds[2];
        pfds[0].fd      = _wakeupFd; // a negative fd is ignored by poll()
        pfds[0].events  = POLLIN;
        pfds[0].revents = 0;
        pfds[1].fd      = _fd;
        pfds[1].events  = POLLIN;
        pfds[1].revents = 0;

        auto lastTick = std::chrono::steady_clock::now();

        while (!_shutdown) {
            const int res = poll(pfds, 2, POLL_TIMEOUT);

            if (res < 0) {
                if (errno == EINTR) {
                    continue;
                }
                SI_ERROR("poll failed, err: %s", strerror(errno));
                break;
            }

            if (res > 0) {
                if (pfds[0].revents & POLLIN) {
                    SI_INFO("wakeup received, leaving monitor loop");
                    break; // wakeup, shutdown requested
                }
                if (pfds[1].revents & POLLIN) {
                    if (!handleInotify()) {
                        SI_WARN("inotify no longer usable, continuing with the periodic check only");
                        pfds[1].fd = INVALID_FD; // keep ticking without inotify
                    }
                } else if (pfds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    SI_WARN("inotify fd error (revents 0x%x), continuing with the periodic check only", pfds[1].revents);
                    pfds[1].fd = INVALID_FD;
                }
            }

            // busy directories must not starve the ticker
            const auto now = std::chrono::steady_clock::now();
            if (now - lastTick >= std::chrono::milliseconds(POLL_TIMEOUT)) {
                lastTick = now;
                tick();
            }
        }

        if (INVALID_FD != _wd && INVALID_FD != _fd) {
            inotify_rm_watch(_fd, _wd);
        }
        _wd = INVALID_FD;
        closeFd(_fd);
    }

private:
    const std::string _pidFile;
    const std::string _dir;
    const std::string _name;
    ChangedCb _changed;
    TickCb _tick;

    int _fd;
    int _wd;
    int _wakeupFd;
    std::atomic<bool> _shutdown;
    std::unique_ptr<std::thread> _thread;
};

class DSSmartInterfaceHandler : public RPC::SmartInterfaceType<Exchange::IDeviceSettings> {
    using BaseClass     = RPC::SmartInterfaceType<Exchange::IDeviceSettings>;

    // libds clients cache the handle returned by GetVideoPort / GetDisplay. The plugin
    // handle is only valid for one plugin lifetime, so clients get a stable token
    // instead and the plugin handle is re-resolved whenever the plugin (re)activated.
    enum class HandleKind { VideoPort, Display };

    struct HandleEntry {
        HandleKind kind;
        int type;
        int index;
        int32_t handle;
        uint32_t epoch;
        bool resolved;
    };

    DSSmartInterfaceHandler()
        : BaseClass()
        , _started(false)
        , _connected(false)
        , _pid(INVALID_PID)
        , _connectFailures(0)
        , _unavailableCount(0)
        , _epoch(0)
        , _videoPort(nullptr)
        , _display(nullptr)
        , _pidMonitor(kPidFile,
              std::bind(&DSSmartInterfaceHandler::PidChanged, this, std::placeholders::_1),
              std::bind(&DSSmartInterfaceHandler::Tick, this))
    {
    }

public:
    DSSmartInterfaceHandler(const DSSmartInterfaceHandler&) = delete;
    DSSmartInterfaceHandler& operator=(const DSSmartInterfaceHandler&) = delete;

    // The object is created on first use but does nothing (no channel, no thread) until
    // Start(). It is never destroyed: SmartInterfaceType must not be torn down while
    // COM-RPC threads may still reference it.
    static DSSmartInterfaceHandler& Instance()
    {
        static DSSmartInterfaceHandler* instance = new DSSmartInterfaceHandler();
        static std::once_flag exitHook;
        std::call_once(exitHook, [] {
            std::atexit(&DSSmartInterfaceHandler::AtExit);
            SI_INFO("instance created, callsign=%s, thunder pid file=%s, open timeout=%u ms, trace=%s", kCallsign, kPidFile,
                    kOpenTimeoutMs, traceEnabled() ? "on" : "off (set DS_SMART_INTERFACE_TRACE to enable)");
        });
        return *instance;
    }

    // ---- lifecycle ----------------------------------------------------------------

    void Start()
    {
        std::lock_guard<std::mutex> guard(_lifecycleLock);
        if (_started) {
            SI_TRACE("already started");
            return;
        }
        SI_INFO("starting, opening COM-RPC channel to %s and starting the Thunder PID monitor", kCallsign);
        _started = true;
        _pid     = _pidMonitor.PID();
        const uint32_t rc = Connect(); // best effort, the monitor keeps retrying if Thunder is not up yet
        _pidMonitor.Run();
        SI_INFO("started, thunder pid=%d, channel %s, plugin %s", _pid.load(), (rc == Core::ERROR_NONE) ? "open" : "NOT open yet (monitor will retry)",
                BaseClass::IsOperational() ? "active" : "not active yet");
    }

    void Stop()
    {
        std::lock_guard<std::mutex> guard(_lifecycleLock);
        if (!_started) {
            SI_TRACE("not started");
            return;
        }
        SI_INFO("stopping, thunder pid=%d", _pid.load());
        _started = false;
        _pidMonitor.Stop(); // first, so it cannot reconnect behind our back
        Disconnect();
        SI_INFO("stopped");
    }

    bool Started() const
    {
        return _started;
    }

    // Called by SmartInterfaceType when the plugin is activated / deactivated.
    void Operational(const bool upAndRunning) override
    {
        InvalidateInterfaces();

        if (upAndRunning) {
            _unavailableCount = 0; // log the next "plugin not available" again
        }
        SI_INFO("DeviceSettings plugin %s, cached interfaces dropped, handle epoch now %u", upAndRunning ? "ACTIVATED" : "DEACTIVATED", EpochSnapshot());
    }

    // ---- video port ---------------------------------------------------------------

    dsError_t GetVideoPort(dsVideoPortType_t type, int index, intptr_t* token)
    {
        if (!dsVideoPortType_isValid(type)) {
            SI_ERROR("invalid video port type %d (index %d)", static_cast<int>(type), index);
            return dsERR_INVALID_PARAM;
        }
        return GetHandle(HandleKind::VideoPort, static_cast<int>(type), index, token);
    }

    dsError_t GetDisplay(dsVideoPortType_t type, int index, intptr_t* token)
    {
        if (!dsVideoPortType_isValid(type)) {
            SI_ERROR("invalid display port type %d (index %d)", static_cast<int>(type), index);
            return dsERR_INVALID_PARAM;
        }
        return GetHandle(HandleKind::Display, static_cast<int>(type), index, token);
    }

    dsError_t EnableVideoPort(intptr_t token, bool enabled)
    {
        return CallPort("EnableVideoPort", token, [&](VideoPortIf* vp, int32_t handle) {
            return vp->EnableVideoPort(handle, enabled);
        });
    }

    dsError_t SetHdmiPreference(intptr_t token, dsHdcpProtocolVersion_t version)
    {
        return CallPort("SetHDMIPreference", token, [&](VideoPortIf* vp, int32_t handle) {
            return vp->SetHDMIPreference(handle, static_cast<VideoPortIf::HDCPProtocolVersion>(version));
        });
    }

    // ---- display ------------------------------------------------------------------

    dsError_t GetEDID(intptr_t token, dsDisplayEDID_t* out)
    {
        DisplayIf::DisplayEDID edid = DisplayIf::DisplayEDID();
        DisplayIf::IDSVideoPortResolutionIterator* iterator = nullptr;
        const dsError_t ret = CallDisplay("GetDisplayEdid", token, [&](DisplayIf* d, int32_t handle) {
            return d->GetDisplayEdid(handle, edid, iterator);
        });
        if (ret != dsERR_NONE) {
            return ret;
        }

        memset(out, 0, sizeof(*out));
        out->productCode      = edid.productCode;
        out->serialNumber     = edid.serialNumber;
        out->manufactureYear  = edid.manufactureYear;
        out->manufactureWeek  = edid.manufactureWeek;
        out->hdmiDeviceType   = edid.hdmiDeviceType;
        out->isRepeater       = edid.isRepeater;
        out->physicalAddressA = edid.physicalAddressA;
        out->physicalAddressB = edid.physicalAddressB;
        out->physicalAddressC = edid.physicalAddressC;
        out->physicalAddressD = edid.physicalAddressD;
        strncpy(out->monitorName, edid.monitorName.c_str(), sizeof(out->monitorName) - 1);

        int count = 0;
        if (iterator != nullptr) {
            DisplayIf::DisplayVideoPortResolution resolution;
            iterator->Reset(0);
            while (count < dsEEDID_MAX_VIDEO_CODE && iterator->Next(resolution)) {
                out->suppResolutionList[count++] = toDs(resolution);
            }
            iterator->Release();
        }
        out->numOfSupportedResolution = count;
        return dsERR_NONE;
    }

    dsError_t QueryBool(const char* what, intptr_t token, bool* out,
                        Core::hresult (VideoPortIf::*fn)(const int32_t, bool&))
    {
        bool value = false;
        const dsError_t ret = CallPort(what, token, [&](VideoPortIf* vp, int32_t handle) {
            return (vp->*fn)(handle, value);
        });
        if (ret == dsERR_NONE) {
            *out = value;
        }
        return ret;
    }

    template <typename E, typename D>
    dsError_t QueryEnum(const char* what, intptr_t token, D* out,
                        Core::hresult (VideoPortIf::*fn)(const int32_t, E&))
    {
        E value = E();
        const dsError_t ret = CallPort(what, token, [&](VideoPortIf* vp, int32_t handle) {
            return (vp->*fn)(handle, value);
        });
        if (ret == dsERR_NONE) {
            *out = static_cast<D>(value);
        }
        return ret;
    }

    dsError_t Ready() const
    {
        if (!_started) {
            return dsERR_NOT_INITIALIZED;
        }
        return BaseClass::IsOperational() ? dsERR_NONE : dsERR_INVALID_STATE;
    }

private:
    static void AtExit()
    {
        // only our own thread has to be gone before static destruction starts
        SI_INFO("process exiting, stopping the Thunder PID monitor");
        Instance()._pidMonitor.Stop();
    }

    // ---- channel handling (mirrors PowerController::Connect / Disconnect) ---------

    uint32_t Connect()
    {
        std::lock_guard<std::mutex> guard(_connLock);
        if (_connected) {
            return Core::ERROR_NONE;
        }

        const auto begin = std::chrono::steady_clock::now();
        uint32_t rc      = Core::ERROR_GENERAL;
        try {
            // may call Operational(true) synchronously when the plugin is already active
            rc = BaseClass::Open(kOpenTimeoutMs, BaseClass::Connector(), kCallsign);
        } catch (const std::exception& e) {
            SI_ERROR("exception opening COM-RPC channel: %s", e.what());
        } catch (...) {
            SI_ERROR("unknown exception opening COM-RPC channel");
        }

        if (rc == Core::ERROR_NONE) {
            _connected = true;
            const unsigned failures = _connectFailures.exchange(0);
            SI_INFO("COM-RPC channel opened in %lld ms, callsign=%s, plugin %s%s", elapsedMs(begin), kCallsign,
                    BaseClass::IsOperational() ? "active" : "not active yet (waiting for activation)",
                    failures != 0 ? ", after earlier failed attempts" : "");
        } else {
            // the monitor retries every POLL_TIMEOUT ms, do not flood the log while Thunder is down
            const unsigned failures = ++_connectFailures;
            if (failures == 1 || (failures % 15) == 0) {
                SI_WARN("COM-RPC channel open failed (%s, rc=%u) after %lld ms, attempt %u, is Thunder running? will keep retrying",
                        Core::ErrorToString(rc), rc, elapsedMs(begin), failures);
            } else {
                SI_TRACE("COM-RPC channel open failed (rc=%u), attempt %u", rc, failures);
            }
        }
        return rc;
    }

    void Disconnect()
    {
        std::lock_guard<std::mutex> guard(_connLock);
        if (_connected) {
            const auto begin = std::chrono::steady_clock::now();
            try {
                // calls Operational(false) when the plugin was active
                BaseClass::Close(Core::infinite);
            } catch (...) {
                SI_ERROR("exception closing COM-RPC channel");
            }
            _connected = false;
            SI_INFO("COM-RPC channel closed in %lld ms", elapsedMs(begin));
        } else {
            SI_TRACE("channel already closed");
        }
        // Operational(false) is not delivered if the plugin was already gone
        InvalidateInterfaces();
    }

    // Monitor thread: Thunder restarted, drop the dead channel and open a new one.
    void PidChanged(const int pid)
    {
        if (!_started || pid == _pid) {
            return;
        }
        SI_WARN("Thunder PID changed, old: %d, new: %d - Thunder restarted, re-establishing the COM-RPC channel", _pid.load(), pid);
        _pid = pid;
        Disconnect();
        const uint32_t rc = Connect();
        SI_INFO("re-establish after Thunder restart: %s (rc=%u)", (rc == Core::ERROR_NONE) ? "channel open" : "channel NOT open, will retry", rc);
    }

    // Monitor thread, every POLL_TIMEOUT: safety net for a missed inotify event and
    // for Thunder not being reachable yet.
    void Tick()
    {
        if (!_started) {
            return;
        }
        const int pid = _pidMonitor.PID();
        if (INVALID_PID != pid && pid != _pid) {
            SI_WARN("Thunder PID change found by periodic check (inotify event missed?), old: %d, new: %d", _pid.load(), pid);
            PidChanged(pid);
        } else if (!_connected) {
            SI_TRACE("periodic check: channel not open, retrying (thunder pid=%d)", pid);
            (void)Connect();
        } else {
            SI_TRACE("periodic check: channel open, thunder pid=%d, plugin %s", pid, BaseClass::IsOperational() ? "active" : "not active");
        }
    }

    // ---- interface cache ----------------------------------------------------------

    uint32_t EpochSnapshot()
    {
        std::lock_guard<std::mutex> guard(_lock);
        return _epoch;
    }

    // Drops the cached proxies; plugin handles become stale and are resolved again on next use.
    void InvalidateInterfaces()
    {
        VideoPortIf* videoPort = nullptr;
        DisplayIf* display     = nullptr;
        {
            std::lock_guard<std::mutex> guard(_lock);
            _epoch++;
            videoPort                = _videoPort;
            display                  = _display;
            _videoPort               = nullptr;
            _display                 = nullptr;
        }
        SI_TRACE("dropping cached interfaces: videoPort=%s display=%s", videoPort ? "yes" : "no", display ? "yes" : "no");
        if (videoPort != nullptr) {
            videoPort->Release();
        }
        if (display != nullptr) {
            display->Release();
        }
    }

    // Returns an AddRef'd sub-interface (caller must Release) or nullptr when the
    // plugin is not active. Never holds _lock across a COM-RPC call or Interface():
    // Operational() runs under the monitor lock and takes _lock, so the reverse
    // order would deadlock.
    template <typename T>
    T* Acquire(T*& cache)
    {
        uint32_t epoch = 0;
        {
            std::lock_guard<std::mutex> guard(_lock);
            if (cache != nullptr) {
                cache->AddRef();
                return cache;
            }
            epoch = _epoch;
        }

        Exchange::IDeviceSettings* root = BaseClass::Interface();
        if (root == nullptr) {
            SI_TRACE("IDeviceSettings root interface not available (plugin not active)");
            return nullptr;
        }
        const auto begin = std::chrono::steady_clock::now();
        T* fresh         = root->QueryInterface<T>();
        root->Release();
        if (fresh == nullptr) {
            SI_ERROR("QueryInterface<0x%08x> on the DeviceSettings plugin failed after %lld ms", static_cast<unsigned>(T::ID), elapsedMs(begin));
            return nullptr;
        }

        std::lock_guard<std::mutex> guard(_lock);
        if (epoch != _epoch) {
            SI_WARN("plugin cycled while acquiring interface 0x%08x (epoch %u -> %u), handing it out uncached", static_cast<unsigned>(T::ID), epoch, _epoch);
            return fresh; // plugin cycled meanwhile, hand out without caching
        }
        if (cache != nullptr) {
            fresh->Release();
            fresh = cache;
        } else {
            cache = fresh;
            SI_INFO("acquired DeviceSettings sub-interface 0x%08x in %lld ms (epoch %u), cached for further calls", static_cast<unsigned>(T::ID), elapsedMs(begin), epoch);
        }
        fresh->AddRef();
        return fresh;
    }

    template <typename T, typename F>
    dsError_t CallInterface(const char* what, T*& cache, F fn)
    {
        if (!_started) {
            SI_ERROR("%s - refused, not started (device::Manager::Initialize was not called)", what);
            return dsERR_NOT_INITIALIZED;
        }
        T* itf = Acquire(cache);
        if (itf == nullptr) {
            // Widevine may poll while the plugin is down: first occurrence and then every 50th
            const unsigned refused = ++_unavailableCount;
            if (refused == 1 || (refused % 50) == 0) {
                SI_ERROR("%s - DeviceSettings plugin not available (channel %s), %u call(s) refused since it went away", what,
                         _connected ? "open" : "closed", refused);
            } else {
                SI_TRACE("%s - DeviceSettings plugin not available", what);
            }
            return dsERR_INVALID_STATE;
        }

        const auto begin       = std::chrono::steady_clock::now();
        const Core::hresult rc = fn(itf);
        const long long took   = elapsedMs(begin);
        itf->Release();

        if (rc != Core::ERROR_NONE) {
            SI_ERROR("%s failed, rc=%u (%s) after %lld ms", what, rc, Core::ErrorToString(rc), took);
        } else if (took >= kSlowCallMs) {
            SI_WARN("%s succeeded but was slow: %lld ms", what, took);
        } else {
            SI_TRACE("%s ok (%lld ms)", what, took);
        }
        return toDsError(rc);
    }

    template <typename F>
    dsError_t CallVideoPort(const char* what, F fn)
    {
        return CallInterface(what, _videoPort, fn);
    }

    dsError_t FetchVideoPortHandle(int type, int index, int32_t& handle)
    {
        return CallVideoPort("GetVideoPort", [&](VideoPortIf* vp) {
            return vp->GetVideoPort(static_cast<VideoPortIf::VideoPort>(type), index, handle);
        });
    }

    dsError_t FetchDisplayHandle(int type, int index, int32_t& handle)
    {
        return CallInterface("GetDisplay", _display, [&](DisplayIf* d) {
            return d->GetDisplay(static_cast<DisplayIf::DisplayPortType>(type), index, handle);
        });
    }

    static const char* KindName(const HandleKind kind)
    {
        return (kind == HandleKind::VideoPort) ? "video port" : "display";
    }

    dsError_t FetchHandle(const HandleKind kind, const int type, const int index, int32_t& handle)
    {
        return (kind == HandleKind::VideoPort) ? FetchVideoPortHandle(type, index, handle) : FetchDisplayHandle(type, index, handle);
    }

    // Finds / creates the token for (kind, type, index) and resolves it once, so an
    // unavailable plugin is reported to the caller right away.
    dsError_t GetHandle(const HandleKind kind, const int type, const int index, intptr_t* token)
    {
        if (!_started) {
            SI_ERROR("%s handle requested (type %d, index %d) but not started (device::Manager::Initialize was not called)", KindName(kind), type, index);
            return dsERR_NOT_INITIALIZED;
        }

        size_t slot = 0;
        {
            std::lock_guard<std::mutex> guard(_lock);
            for (; slot < _handles.size(); slot++) {
                if (_handles[slot].kind == kind && _handles[slot].type == type && _handles[slot].index == index) {
                    break;
                }
            }
            if (slot == _handles.size()) {
                HandleEntry entry = { kind, type, index, -1, 0, false };
                _handles.push_back(entry);
                SI_INFO("new %s token %zu for type %d, index %d", KindName(kind), slot + 1, type, index);
            }
        }

        int32_t handle = -1;
        const dsError_t ret = ResolveHandle(kind, static_cast<intptr_t>(slot + 1), handle);
        if (ret == dsERR_NONE) {
            *token = static_cast<intptr_t>(slot + 1);
        } else {
            SI_ERROR("%s handle for type %d, index %d could not be resolved, ret=%d", KindName(kind), type, index, static_cast<int>(ret));
        }
        return ret;
    }

    // token -> current plugin handle of the expected kind, fetched again if the plugin restarted since.
    // Tokens are unique across kinds, a video port token is never accepted for a display call or the reverse.
    dsError_t ResolveHandle(const HandleKind expected, const intptr_t token, int32_t& handle)
    {
        HandleEntry entry = { expected, 0, 0, -1, 0, false };
        uint32_t epoch    = 0;
        {
            std::lock_guard<std::mutex> guard(_lock);
            if (token < 1 || static_cast<size_t>(token) > _handles.size()) {
                SI_ERROR("unknown handle %ld (%zu known), was it obtained with GetVideoPort / GetDisplay?", static_cast<long>(token), _handles.size());
                return dsERR_INVALID_PARAM;
            }
            entry = _handles[static_cast<size_t>(token) - 1];
            epoch = _epoch;
            if (entry.kind == expected && entry.resolved && entry.epoch == epoch) {
                handle = entry.handle;
                return dsERR_NONE;
            }
        }

        if (entry.kind != expected) {
            SI_ERROR("handle %ld is a %s handle but a %s handle is required", static_cast<long>(token), KindName(entry.kind), KindName(expected));
            return dsERR_INVALID_PARAM;
        }

        if (entry.resolved) {
            SI_INFO("%s token %ld (type %d, index %d) is stale (plugin handle %d from epoch %u, now %u), resolving again", KindName(entry.kind),
                    static_cast<long>(token), entry.type, entry.index, entry.handle, entry.epoch, epoch);
        }
        int32_t fresh       = -1;
        const dsError_t ret = FetchHandle(entry.kind, entry.type, entry.index, fresh);
        if (ret != dsERR_NONE) {
            return ret;
        }

        {
            std::lock_guard<std::mutex> guard(_lock);
            if (_epoch == epoch) { // otherwise the plugin cycled again, do not cache
                HandleEntry& slot = _handles[static_cast<size_t>(token) - 1];
                slot.handle       = fresh;
                slot.epoch        = epoch;
                slot.resolved     = true;
                SI_INFO("%s token %ld (type %d, index %d) -> plugin handle %d (epoch %u)", KindName(entry.kind), static_cast<long>(token),
                        entry.type, entry.index, fresh, epoch);
            } else {
                SI_WARN("plugin cycled while resolving %s token %ld (epoch %u -> %u), handle %d not cached", KindName(entry.kind),
                        static_cast<long>(token), epoch, _epoch, fresh);
            }
        }
        handle = fresh;
        return dsERR_NONE;
    }

    template <typename F>
    dsError_t CallPort(const char* what, const intptr_t token, F fn)
    {
        int32_t handle = -1;
        const dsError_t ret = ResolveHandle(HandleKind::VideoPort, token, handle);
        if (ret != dsERR_NONE) {
            return ret;
        }
        return CallVideoPort(what, [&](VideoPortIf* vp) { return fn(vp, handle); });
    }

    template <typename F>
    dsError_t CallDisplay(const char* what, const intptr_t token, F fn)
    {
        int32_t handle = -1;
        const dsError_t ret = ResolveHandle(HandleKind::Display, token, handle);
        if (ret != dsERR_NONE) {
            return ret;
        }
        return CallInterface(what, _display, [&](DisplayIf* d) { return fn(d, handle); });
    }

private:
    std::mutex _lifecycleLock; // Start / Stop
    std::atomic<bool> _started;

    std::mutex _connLock; // Connect / Disconnect, never taken by Operational()
    std::atomic<bool> _connected;
    std::atomic<int> _pid; // Thunder PID the current channel belongs to
    std::atomic<unsigned> _connectFailures;   // consecutive failed Open() attempts, for log throttling
    std::atomic<unsigned> _unavailableCount;  // calls refused since the plugin went away, for log throttling

    std::mutex _lock;     // protects everything below up to _pidMonitor
    uint32_t _epoch;      // bumped on every plugin (de)activation / channel reset

    VideoPortIf* _videoPort;
    DisplayIf* _display;
    std::vector<HandleEntry> _handles; // token (index + 1) -> port, resolved plugin handle per epoch

    PIDFileMonitor _pidMonitor; // declared last: its thread uses the members above
};

} // namespace

extern "C" {

dsError_t dsSmartInterface_Start(void)
{
    return Guard("Start", []() -> dsError_t {
        DSSmartInterfaceHandler::Instance().Start();
        return dsERR_NONE;
    });
}

dsError_t dsSmartInterface_Stop(void)
{
    return Guard("Stop", []() -> dsError_t {
        DSSmartInterfaceHandler::Instance().Stop();
        return dsERR_NONE;
    });
}

dsError_t dsSmartInterface_Init(void)
{
    return Guard("Init", []() -> dsError_t { return DSSmartInterfaceHandler::Instance().Ready(); });
}

dsError_t dsSmartInterface_GetVideoPort(dsVideoPortType_t type, int index, intptr_t* handle)
{
    if (handle == nullptr) {
        return invalidParam(__func__);
    }
    return Guard("GetVideoPort", [&]() -> dsError_t { return DSSmartInterfaceHandler::Instance().GetVideoPort(type, index, handle); });
}

dsError_t dsSmartInterface_IsVideoPortEnabled(intptr_t handle, bool* enabled)
{
    if (enabled == nullptr) {
        return invalidParam(__func__);
    }
    return Guard("IsVideoPortEnabled", [&]() -> dsError_t {
        return DSSmartInterfaceHandler::Instance().QueryBool("IsVideoPortEnabled", handle, enabled, &VideoPortIf::IsVideoPortEnabled);
    });
}

dsError_t dsSmartInterface_EnableVideoPort(intptr_t handle, bool enabled)
{
    return Guard("EnableVideoPort", [&]() -> dsError_t { return DSSmartInterfaceHandler::Instance().EnableVideoPort(handle, enabled); });
}

dsError_t dsSmartInterface_IsDisplayConnected(intptr_t handle, bool* connected)
{
    if (connected == nullptr) {
        return invalidParam(__func__);
    }
    return Guard("IsDisplayConnected", [&]() -> dsError_t {
        return DSSmartInterfaceHandler::Instance().QueryBool("IsVideoPortDisplayConnected", handle, connected, &VideoPortIf::IsVideoPortDisplayConnected);
    });
}

dsError_t dsSmartInterface_IsVideoPortActive(intptr_t handle, bool* active)
{
    if (active == nullptr) {
        return invalidParam(__func__);
    }
    return Guard("IsVideoPortActive", [&]() -> dsError_t {
        return DSSmartInterfaceHandler::Instance().QueryBool("IsVideoPortActive", handle, active, &VideoPortIf::IsVideoPortActive);
    });
}

dsError_t dsSmartInterface_IsHDCPEnabled(intptr_t handle, bool* enabled)
{
    if (enabled == nullptr) {
        return invalidParam(__func__);
    }
    return Guard("IsHDCPEnabled", [&]() -> dsError_t {
        return DSSmartInterfaceHandler::Instance().QueryBool("IsHDCPEnabledOnVideoPort", handle, enabled, &VideoPortIf::IsHDCPEnabledOnVideoPort);
    });
}

dsError_t dsSmartInterface_GetHDCPStatus(intptr_t handle, dsHdcpStatus_t* status)
{
    if (status == nullptr) {
        return invalidParam(__func__);
    }
    return Guard("GetHDCPStatus", [&]() -> dsError_t {
        return DSSmartInterfaceHandler::Instance().QueryEnum("GetHDCPStatusOnVideoPort", handle, status, &VideoPortIf::GetHDCPStatusOnVideoPort);
    });
}

dsError_t dsSmartInterface_GetHDCPProtocol(intptr_t handle, dsHdcpProtocolVersion_t* version)
{
    if (version == nullptr) {
        return invalidParam(__func__);
    }
    return Guard("GetHDCPProtocol", [&]() -> dsError_t {
        return DSSmartInterfaceHandler::Instance().QueryEnum("GetHDCPProtocolVersionOnVideoPort", handle, version, &VideoPortIf::GetHDCPProtocolVersionOnVideoPort);
    });
}

dsError_t dsSmartInterface_GetHDCPReceiverProtocol(intptr_t handle, dsHdcpProtocolVersion_t* version)
{
    if (version == nullptr) {
        return invalidParam(__func__);
    }
    return Guard("GetHDCPReceiverProtocol", [&]() -> dsError_t {
        return DSSmartInterfaceHandler::Instance().QueryEnum("GetHDCPReceiverProtocolVersionOnVideoPort", handle, version, &VideoPortIf::GetHDCPReceiverProtocolVersionOnVideoPort);
    });
}

dsError_t dsSmartInterface_GetHDCPCurrentProtocol(intptr_t handle, dsHdcpProtocolVersion_t* version)
{
    if (version == nullptr) {
        return invalidParam(__func__);
    }
    return Guard("GetHDCPCurrentProtocol", [&]() -> dsError_t {
        return DSSmartInterfaceHandler::Instance().QueryEnum("GetHDCPCurrentProtocolVersionOnVideoPort", handle, version, &VideoPortIf::GetHDCPCurrentProtocolVersionOnVideoPort);
    });
}

dsError_t dsSmartInterface_GetHdmiPreference(intptr_t handle, dsHdcpProtocolVersion_t* version)
{
    if (version == nullptr) {
        return invalidParam(__func__);
    }
    return Guard("GetHdmiPreference", [&]() -> dsError_t {
        return DSSmartInterfaceHandler::Instance().QueryEnum("GetHDMIPreference", handle, version, &VideoPortIf::GetHDMIPreference);
    });
}

dsError_t dsSmartInterface_SetHdmiPreference(intptr_t handle, dsHdcpProtocolVersion_t version)
{
    return Guard("SetHdmiPreference", [&]() -> dsError_t { return DSSmartInterfaceHandler::Instance().SetHdmiPreference(handle, version); });
}

dsError_t dsSmartInterface_GetDisplay(dsVideoPortType_t type, int index, intptr_t* handle)
{
    if (handle == nullptr) {
        return invalidParam(__func__);
    }
    return Guard("GetDisplay", [&]() -> dsError_t { return DSSmartInterfaceHandler::Instance().GetDisplay(type, index, handle); });
}

dsError_t dsSmartInterface_GetEDID(intptr_t handle, dsDisplayEDID_t* edid)
{
    if (edid == nullptr) {
        return invalidParam(__func__);
    }
    return Guard("GetEDID", [&]() -> dsError_t { return DSSmartInterfaceHandler::Instance().GetEDID(handle, edid); });
}

} // extern "C"

#endif /* DEVICESETTINGS_PLUGIN_ENABLED */
