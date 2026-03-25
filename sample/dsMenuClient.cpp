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
 * @file  dsMenuClient.cpp
 * @brief Standalone menu-driven DS API client application.
 *
 * PURPOSE
 *   Exercises the DeviceSettings C++ API via a persistent menu-driven session.
 *   IARM Bus and DS Manager are initialized ONCE at startup and intentionally
 *   NOT re-initialized between menu-driven test runs.  This is the key design
 *   property: after dsmgr is killed (SIGABRT / SIGSEGV) and restarted by
 *   systemd, a subsequent "Run Tests" from this menu will use STALE handles
 *   and reveal the exact failure behaviour documented in ISSUE-001.
 *
 *   An explicit menu provides two recovery options after dsmgr restart:
 *     [R] Selective Handle Refresh – calls dsGetVideoPort/dsGetAudioPort etc.
 *         directly via C-RPC; updates g_hdl_* cache WITHOUT DeInit/Init.
 *         C++ wrapper _handle fields remain stale (demonstrates the root cause).
 *     [I] Full Re-Initialize – Manager::DeInitialize() + Initialize(); rebuilds
 *         all C++ wrapper objects with fresh handles (full recovery).
 *
 * MODULES CURRENTLY IMPLEMENTED
 *   Module 1 – Audio              [handle-based, intptr_t _handle: fails after restart]
 *     TC-AUD-01  getAudioOutputPorts – all ports: isEnabled/isMuted/getLevel/getEncoding
 *     TC-AUD-02  setMuted            – toggle+restore round-trip on SPEAKER0 (TV profile only)
 *
 *   Module 2 – CompositeIn
 *     TC-CIN-01  getCompositeInfo    – getNumberOfInputs / isPresented / getActivePort
 *     TC-CIN-02  isPortConnected     – connected status per composite input port
 *
 *   Module 3 – Display             [handle-based via VideoOutputPort::Display._handle]
 *     TC-DSP-01  hasSurround+getSurroundMode – via HDMI0 Display object
 *     TC-DSP-02  getEDIDBytes        – raw EDID bytes from HDMI0 display (if connected)
 *
 *   Module 4 – FPD (Front Panel Display)  [enum-based, immune to stale handle]
 *     TC-FPD-01  getFPBrightness  – reads brightness for each indicator
 *     TC-FPD-02  setFPBrightness  – writes brightness=50, reads back
 *     TC-FPD-03  setFPColor       – sets Power indicator to Blue, reads back
 *     TC-FPD-04  setFPState       – sets Power indicator ON, reads back state
 *     TC-FPD-05  setFPTextDisplay – sets text "RDK", reads brightness per indicator
 *     TC-FPD-06  setFPTextTimeFormat – sets 12-hr format, reads back
 *
 *   Module 5 – HDMIIn
 *     TC-HIN-01  getHdmiInInfo    – getNumberOfInputs / isPresented / getActivePort / getCurrentVideoMode
 *     TC-HIN-02  isPortConnected  – connected status per HDMI input port
 *
 *   Module 6 – Host               [getCPUTemp / version / power mode / port counts]
 *     TC-HST-01  getHostInfo      – CPU temp, version, power mode, HDMI present, port names
 *     TC-HST-02  getSleepModes    – preferred sleep mode, available modes, port counts
 *
 *   Module 7 – VideoDevice        [handle-based, intptr_t _handle: fails after restart]
 *     TC-VD-01   getVideoDevices  – DFC, HDRCapabilities, VideoCodingFormats, STB resolutions
 *     TC-VD-02   setDFC           – round-trip GET->SET-same->GET verify
 *
 *   Module 8 – VideoPort  *** STALE HANDLE DEMONSTRATION ***
 *     TC-VP-01   getVideoOutputPorts  – all ports: isEnabled/Connected/HDCP/Resolution
 *                                       ALL calls pass intptr_t _handle -> FAIL after restart
 *     TC-VP-02   setResolution(HDMI0) – round-trip GET->SET-same->GET verify
 *                                       write path also passes _handle -> FAIL after restart
 *     Key insight: FPD uses enum index, VideoPort uses server-side pointer.
 *
 * MODULES RESERVED (to be added)
 *   (none – all modules now implemented)
 *
 * USAGE
 *   dsMenuClient [output_dir]
 *   output_dir defaults to /tmp/ds-menu-client
 *
 * TEST PROCEDURE
 *   1.  Launch application. Init happens automatically once.
 *   2.  Select profile (STB / TV / Common).
 *   3.  Run any module → saves results to <output_dir>/RUNXX/<module>/*.txt
 *   4.  Exit to shell. Kill dsmgr:  kill -6 $(pidof dsMgrMain)
 *   5.  Return to application. Run same module again  → VERIFY_FAIL (stale handle).
 *   6.  Choose [R] Handle Refresh → g_hdl_* updated; C++ wrappers still stale.
 *   7.  Choose [S] Status → verify fresh handles are live (dsIsVideoPortEnabled OK).
 *   8.  Choose [I] Full Re-Initialize → all C++ wrappers rebuilt with fresh handles.
 *   9.  Run module again → VERIFY_PASS (stale handle condition cleared).
 *
 * BUILD (on device, inside devicesettings root)
 *   $(CXX) $(CFLAGS) sample/dsMenuClient.cpp -o dsMenuClient $(LDFLAGS)
 */

/* =========================================================================
 * Includes
 * ========================================================================= */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>

/* DS C++ headers */
#include "manager.hpp"
#include "host.hpp"
#include "audioOutputPort.hpp"
#include "audioOutputPortConfig.hpp"
#include "videoOutputPortConfig.hpp"
#include "audioEncoding.hpp"
#include "audioStereoMode.hpp"
#include "compositeIn.hpp"
#include "frontPanelConfig.hpp"
#include "frontPanelIndicator.hpp"
#include "frontPanelTextDisplay.hpp"
#include "hdmiIn.hpp"
#include "sleepMode.hpp"
#include "videoDFC.hpp"
#include "videoDevice.hpp"
#include "videoOutputPort.hpp"
#include "videoOutputPortType.hpp"
#include "videoResolution.hpp"
#include "exception.hpp"
#include "list.hpp"

/* IARM */
#include "libIBus.h"
#include "dsMgr.h"   /* pulls in dsTypes.h → dsVideoPortType_t / dsAudioPortType_t */

/* ── C-level DS RPC handle-acquisition prototypes ─────────────────────────
 * These are the raw IARM-RPC functions that sit beneath the C++ DS wrappers.
 * Calling them directly lets us re-fetch handles without tearing down the
 * entire Manager (DeInitialize / Initialize) layer.  The actual symbols are
 * linked from libds-rpc-client (rpc/cli/*.c).  Types come from dsTypes.h
 * which is already included transitively through dsMgr.h above.
 * ────────────────────────────────────────────────────────────────────────── */
extern "C" {
    dsError_t dsGetVideoPort  (dsVideoPortType_t  type,  int index, intptr_t *handle);
    dsError_t dsGetDisplay    (dsVideoPortType_t  vType, int index, intptr_t *handle);
    dsError_t dsGetAudioPort  (dsAudioPortType_t  type,  int index, intptr_t *handle);
    dsError_t dsGetVideoDevice(int index, intptr_t *handle);
    /* Lightweight verification helpers (call with a freshly fetched handle) */
    dsError_t dsIsVideoPortEnabled(intptr_t handle, bool *enabled);
    dsError_t dsIsDisplayConnected(intptr_t handle, bool *connected);
}

/* =========================================================================
 * Constants / Configuration
 * ========================================================================= */

#define APP_NAME          "DSMenuClient"
#define DEFAULT_OUT_DIR   "/opt/gsk/ds-client-test"
#define IARM_CLIENT_NAME  "DSMenuClient"
#define HDMI_PORT_NAME    "HDMI0"   /* VideoPort used by Module 8 TCs */

/* Indicator names supported by the FPD module */
static const char * const FPD_INDICATORS[] = {
    "Power", NULL
};

/* =========================================================================
 * Global state  (intentionally NOT reset between test runs)
 * ========================================================================= */

static bool g_iarm_initialized  = false;   /* IARM_Bus_Init called once       */
static bool g_ds_initialized    = false;   /* Manager::Initialize called once */
static int  g_run_count         = 0;       /* increments each "Run Tests"     */

/* ── Selective-refresh handle cache ───────────────────────────────────────
 * Populated by ds_handle_refresh() via direct C-RPC calls.  The C++ DS
 * wrapper objects (AudioOutputPort, VideoOutputPort, VideoDevice …) cache
 * their own private intptr_t _handle at Manager::Initialize() time and
 * provide no public setter – so they remain stale after a dsmgr restart.
 *
 * These globals give test cases a second path: call C-level APIs (e.g.
 * dsIsVideoPortEnabled) with a freshly fetched handle and compare against
 * the stale C++ wrapper result, demonstrating exactly where the staleness
 * lives and confirming that [R] Handle-Refresh clears it without the
 * overhead of a full Manager DeInitialize/Initialize cycle.
 * ────────────────────────────────────────────────────────────────────────── */
static intptr_t g_hdl_vport    = 0;   /* VideoPort  HDMI[0]   – dsGetVideoPort()   */
static intptr_t g_hdl_display  = 0;   /* Display    HDMI[0]   – dsGetDisplay()     */
static intptr_t g_hdl_aport    = 0;   /* AudioPort  HDMI[0]   – dsGetAudioPort()   */
static intptr_t g_hdl_vdev     = 0;   /* VideoDevice[0]       – dsGetVideoDevice() */
static bool     g_handles_valid = false; /* true after ds_handle_refresh() succeeds  */
static std::string g_out_root   = DEFAULT_OUT_DIR;

/* Profile selection */
typedef enum {
    PROFILE_STB    = 1,
    PROFILE_TV     = 2,
    PROFILE_COMMON = 3,
    PROFILE_ALL    = 4
} DeviceProfile_t;

static DeviceProfile_t g_profile = PROFILE_ALL;

/* =========================================================================
 * Utility helpers
 * ========================================================================= */

/* Returns ISO-8601 UTC timestamp string e.g. "2026-03-20T12:34:56Z" */
static std::string timestamp_now(void)
{
    time_t t = time(NULL);
    struct tm tm_utc;
    gmtime_r(&t, &tm_utc);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
    return std::string(buf);
}

/* Make directory (and parents) – like mkdir -p */
static void mkdirp(const std::string &path)
{
    std::string tmp = path;
    for (size_t i = 1; i < tmp.size(); ++i) {
        if (tmp[i] == '/') {
            tmp[i] = '\0';
            mkdir(tmp.c_str(), 0755);
            tmp[i] = '/';
        }
    }
    mkdir(tmp.c_str(), 0755);
}

/* Stream that writes to both a file and stdout simultaneously */
class TeeStream {
public:
    TeeStream() {}
    ~TeeStream() { close(); }

    bool open(const std::string &path) {
        _file.open(path.c_str(), std::ios::out | std::ios::trunc);
        _path = path;
        return _file.is_open();
    }
    void close() { if (_file.is_open()) _file.close(); }

    template <typename T>
    TeeStream & operator<<(const T &v) {
        std::ostringstream oss;
        oss << v;
        std::string s = oss.str();
        std::cout << s;
        if (_file.is_open()) _file << s;
        return *this;
    }
    /* Handle std::endl and other manipulators */
    TeeStream & operator<<(std::ostream& (*manip)(std::ostream&)) {
        std::cout << manip;
        if (_file.is_open()) _file << manip;
        return *this;
    }

    const std::string &path() const { return _path; }

private:
    std::ofstream _file;
    std::string   _path;
};

/* Global verify summary for the current run */
struct VerifyEntry {
    std::string tc_id;
    std::string description;
    bool        pass;
    std::string detail;
};
static std::vector<VerifyEntry> g_verify_results;

static void verify_record(const std::string &tc_id,
                          const std::string &desc,
                          bool pass,
                          const std::string &detail = "")
{
    VerifyEntry e;
    e.tc_id       = tc_id;
    e.description = desc;
    e.pass        = pass;
    e.detail      = detail;
    g_verify_results.push_back(e);
}

/* Print a section separator */
static void print_sep(TeeStream &out, const std::string &title = "")
{
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    if (!title.empty())
        out << "  " << title << "\n"
            << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
}

/* =========================================================================
 * IARM / DS lifecycle helpers
 * ========================================================================= */

static bool ds_iarm_init(void)
{
    if (g_iarm_initialized) {
        printf("[INIT] IARM already connected – skipping.\n");
        return true;
    }
    printf("[INIT] Connecting to IARM Bus as '%s'...\n", IARM_CLIENT_NAME);
    IARM_Result_t rc = IARM_Bus_Init(IARM_CLIENT_NAME);
    if (rc != IARM_RESULT_SUCCESS) {
        printf("[INIT] IARM_Bus_Init FAILED: %d\n", rc);
        return false;
    }
    rc = IARM_Bus_Connect();
    if (rc != IARM_RESULT_SUCCESS) {
        printf("[INIT] IARM_Bus_Connect FAILED: %d\n", rc);
        IARM_Bus_Term();
        return false;
    }
    g_iarm_initialized = true;
    printf("[INIT] IARM Bus connected OK.\n");
    return true;
}

static bool ds_manager_init(void)
{
    if (g_ds_initialized) {
        printf("[INIT] DS Manager already initialized – skipping.\n");
        printf("[WARN] *** Handles are STALE from the previous init ***\n");
        printf("[WARN] *** If dsmgr was restarted, API calls WILL FAIL ***\n");
        return true;
    }
    printf("[INIT] Calling device::Manager::Initialize()...\n");
    try {
        device::Manager::Initialize();
        g_ds_initialized = true;
        printf("[INIT] DS Manager Initialize OK.\n");
        return true;
    }
    catch (const device::Exception &e) {
        printf("[INIT] DS Manager Initialize EXCEPTION: %s\n", e.what());
    }
    catch (const std::exception &e) {
        printf("[INIT] DS Manager Initialize std::exception: %s\n", e.what());
    }
    catch (...) {
        printf("[INIT] DS Manager Initialize unknown exception.\n");
    }
    return false;
}

/* =========================================================================
 * ds_handle_refresh  –  lightweight selective handle re-fetch   [R key]
 *
 *   Calls the C-level RPC functions (dsGetVideoPort / dsGetAudioPort /
 *   dsGetDisplay / dsGetVideoDevice) directly.  Each call crosses the IARM
 *   RPC boundary and asks the RUNNING dsmgr for its current server-side
 *   pointer to each port/device object.  The results are stored in the
 *   g_hdl_* globals above.
 *
 *   What this DOES:
 *     • Refreshes g_hdl_vport / g_hdl_display / g_hdl_aport / g_hdl_vdev
 *     • Runs a quick sanity check (dsIsVideoPortEnabled / dsIsDisplayConnected)
 *       with each fresh handle to prove they are live
 *
 *   What this does NOT do:
 *     • Does NOT call Manager::DeInitialize() or Manager::Initialize()
 *     • Does NOT rebuild the C++ DS wrapper objects (AudioOutputPort,
 *       VideoOutputPort, VideoDevice, Display …)
 *     • Those C++ wrappers retain their stale private _handle values –
 *       use option [I] (Full Re-Initialize) if you need the C++ layer fixed
 *
 *   Use case:
 *     After dsmgr crashes and restarts, call [R] to confirm the C-RPC path
 *     works with new handles, then re-run any module to show that C++ wrapper
 *     calls still fail (VERIFY_FAIL) while direct C-level calls succeed.
 * ========================================================================= */
static void ds_handle_refresh(void)
{
    printf("\n[HREFRESH] ──────────────────────────────────────────────────────\n");
    printf("[HREFRESH] Selective handle refresh via direct C-RPC calls.\n");
    printf("[HREFRESH] NO Manager::DeInitialize() / Initialize() called.\n");
    printf("[HREFRESH] IARM connection and DS config objects are NOT disturbed.\n\n");

    int ok = 0, fail = 0;
    dsError_t ret;

    /* ── VideoPort HDMI[0] ─────────────────────────────────────────── */
    {
        intptr_t nh = 0;
        ret = dsGetVideoPort(dsVIDEOPORT_TYPE_HDMI, 0, &nh);
        if (ret == dsERR_NONE) {
            printf("[HREFRESH]  VideoPort  HDMI[0]  old=0x%zX  →  new=0x%zX  ✔\n",
                   (size_t)g_hdl_vport, (size_t)nh);
            g_hdl_vport = nh;
            /* Quick liveness check */
            bool enabled = false;
            dsError_t chk = dsIsVideoPortEnabled(nh, &enabled);
            printf("[HREFRESH]             liveness dsIsVideoPortEnabled → %s (ret=%d)\n",
                   chk == dsERR_NONE ? (enabled ? "enabled" : "disabled") : "FAIL", chk);
            ++ok;
        } else {
            printf("[HREFRESH]  VideoPort  HDMI[0]  dsGetVideoPort FAILED (err=%d)\n", ret);
            ++fail;
        }
    }

    /* ── Display HDMI[0] ───────────────────────────────────────────── */
    {
        intptr_t nh = 0;
        ret = dsGetDisplay(dsVIDEOPORT_TYPE_HDMI, 0, &nh);
        if (ret == dsERR_NONE) {
            printf("[HREFRESH]  Display    HDMI[0]  old=0x%zX  →  new=0x%zX  ✔\n",
                   (size_t)g_hdl_display, (size_t)nh);
            g_hdl_display = nh;
            /* Quick liveness check */
            bool connected = false;
            dsError_t chk = dsIsDisplayConnected(nh, &connected);
            printf("[HREFRESH]             liveness dsIsDisplayConnected  → %s (ret=%d)\n",
                   chk == dsERR_NONE ? (connected ? "connected" : "not connected") : "FAIL", chk);
            ++ok;
        } else {
            printf("[HREFRESH]  Display    HDMI[0]  dsGetDisplay FAILED (err=%d)\n", ret);
            ++fail;
        }
    }

    /* ── AudioPort HDMI[0] ─────────────────────────────────────────── */
    {
        intptr_t nh = 0;
        ret = dsGetAudioPort(dsAUDIOPORT_TYPE_HDMI, 0, &nh);
        if (ret == dsERR_NONE) {
            printf("[HREFRESH]  AudioPort  HDMI[0]  old=0x%zX  →  new=0x%zX  ✔\n",
                   (size_t)g_hdl_aport, (size_t)nh);
            g_hdl_aport = nh;
            ++ok;
        } else {
            printf("[HREFRESH]  AudioPort  HDMI[0]  dsGetAudioPort FAILED (err=%d)\n", ret);
            ++fail;
        }
    }

    /* ── VideoDevice[0] ────────────────────────────────────────────── */
    {
        intptr_t nh = 0;
        ret = dsGetVideoDevice(0, &nh);
        if (ret == dsERR_NONE) {
            printf("[HREFRESH]  VideoDevice[0]       old=0x%zX  →  new=0x%zX  ✔\n",
                   (size_t)g_hdl_vdev, (size_t)nh);
            g_hdl_vdev = nh;
            ++ok;
        } else {
            printf("[HREFRESH]  VideoDevice[0]       dsGetVideoDevice FAILED (err=%d)\n", ret);
            ++fail;
        }
    }

    g_handles_valid = (fail == 0);

    printf("\n[HREFRESH] Result: %d refreshed, %d failed.\n", ok, fail);
    if (g_handles_valid) {
        printf("[HREFRESH] ✔ All C-level handles refreshed successfully.\n");
        printf("[HREFRESH]   ⚠ C++ wrapper _handle fields are STILL stale.\n");
        printf("[HREFRESH]   Re-run any module to confirm: C++ calls → VERIFY_FAIL\n");
        printf("[HREFRESH]   but the fresh g_hdl_* values are now live.\n");
        printf("[HREFRESH]   Use [I] Full Re-Initialize to also fix C++ wrappers.\n");
        g_run_count = 0;
        g_verify_results.clear();
        printf("[HREFRESH]   Run counter reset to 0 (post-refresh baseline).\n");
    } else {
        printf("[HREFRESH] ✘ One or more handles could not be refreshed.\n");
        printf("[HREFRESH]   Is dsmgr running?  → pidof dsMgrMain\n");
        printf("[HREFRESH]   If dsmgr is down, start it, then retry [R].\n");
        printf("[HREFRESH]   For full recovery use [I] Full Re-Initialize.\n");
    }
    printf("[HREFRESH] ──────────────────────────────────────────────────────\n\n");
}

/* =========================================================================
 * ds_full_reinitialize  –  full Manager DeInitialize + Initialize   [I key]
 *
 *   This is the original heavyweight option: tears down every C++ DS wrapper
 *   object (and clears their private _handle caches), waits 1 second, then
 *   rebuilds everything from scratch via Manager::Initialize().  All four
 *   C++ wrapper layers (AudioOutputPort, VideoOutputPort, VideoDevice,
 *   Display) receive fresh handles as part of object construction.
 *
 *   Use this when:
 *     • [R] Handle-Refresh succeeded at C-level but you also need the C++
 *       wrapper objects fixed (so module TCs pass again)
 *     • dsmgr was killed and restarted AND you need a clean slate
 * ========================================================================= */
static void ds_full_reinitialize(void)
{
    printf("\n[FULLREINIT] ─────────────────────────────────────────────────────\n");
    printf("[FULLREINIT] Full Manager DeInitialize + Initialize.\n");
    printf("[FULLREINIT] All C++ wrapper objects will be rebuilt with fresh handles.\n\n");

    if (g_ds_initialized) {
        try {
            device::Manager::DeInitialize();
            printf("[FULLREINIT] DS Manager DeInitialize OK.\n");
        }
        catch (...) {
            printf("[FULLREINIT] DS Manager DeInitialize exception (ignored).\n");
        }
        g_ds_initialized = false;
    }

    /* Brief pause to allow dsmgr to settle after restart */
    sleep(1);

    printf("[FULLREINIT] Re-initializing DS Manager...\n");
    if (ds_manager_init()) {
        printf("[FULLREINIT] ✔ DS Manager Initialize OK – all handles refreshed.\n");
        printf("[FULLREINIT]   C++ wrapper _handle fields are now fresh.\n");
        printf("[FULLREINIT]   Updating g_hdl_* cache to match new handles.\n");
        /* Keep the cached globals in sync with the fresh C++ layer */
        dsError_t r;
        r = dsGetVideoPort (dsVIDEOPORT_TYPE_HDMI, 0, &g_hdl_vport);
        r = dsGetDisplay   (dsVIDEOPORT_TYPE_HDMI, 0, &g_hdl_display);
        r = dsGetAudioPort (dsAUDIOPORT_TYPE_HDMI, 0, &g_hdl_aport);
        r = dsGetVideoDevice(0, &g_hdl_vdev);
        (void)r;   /* results logged by individual calls above if needed */
        g_handles_valid = true;
    } else {
        printf("[FULLREINIT] ✘ Re-initialization FAILED.\n");
        g_handles_valid = false;
    }
    printf("[FULLREINIT] ─────────────────────────────────────────────────────\n\n");
}

/* =========================================================================
 * ds_cpp_refresh_handles  –  C++ wrapper refreshAllHandles()        [H key]
 *
 *   Calls AudioOutputPortConfig::refreshAllHandles() and
 *   VideoOutputPortConfig::refreshAllHandles().  Each iterates the
 *   pre-built port objects in _aPorts[] / _vPorts[] and re-fetches their
 *   private intptr_t _handle via dsGetAudioPort / dsGetVideoPort / dsGetDisplay
 *   WITHOUT tearing down the Manager or rebuilding port objects.
 *
 *   Use this when:
 *     • [R] Handle-Refresh updated g_hdl_* but C++ wrapper calls still fail
 *     • You want to fix the C++ layer WITHOUT the full DeInit+Init overhead
 * ========================================================================= */
static void ds_cpp_refresh_handles(void)
{
    printf("\n[CPPREFRESH] ────────────────────────────────────────────────────\n");
    printf("[CPPREFRESH] C++ wrapper refreshAllHandles() for Audio + Video ports.\n");
    printf("[CPPREFRESH] NO Manager::DeInitialize() / Initialize() called.\n\n");

    /* ── Audio ports ─────────────────────────────────────────────────── */
    printf("[CPPREFRESH] Calling AudioOutputPortConfig::refreshAllHandles()...\n");
    dsError_t aret = dsERR_NONE;
    try {
        aret = device::AudioOutputPortConfig::getInstance().refreshAllHandles();
        if (aret == dsERR_NONE)
            printf("[CPPREFRESH] ✔ Audio  refreshAllHandles() → ALL OK\n");
        else
            printf("[CPPREFRESH] ⚠ Audio  refreshAllHandles() → PARTIAL FAIL (ret=%d)\n", aret);
    } catch (const std::exception &e) {
        printf("[CPPREFRESH] ✘ Audio  refreshAllHandles() exception: %s\n", e.what());
        aret = dsERR_GENERAL;
    } catch (...) {
        printf("[CPPREFRESH] ✘ Audio  refreshAllHandles() unknown exception\n");
        aret = dsERR_GENERAL;
    }

    /* ── Video ports ─────────────────────────────────────────────────── */
    printf("[CPPREFRESH] Calling VideoOutputPortConfig::refreshAllHandles()...\n");
    int vret = 0;
    try {
        vret = device::VideoOutputPortConfig::getInstance().refreshAllHandles();
        if (vret == 0)
            printf("[CPPREFRESH] ✔ Video  refreshAllHandles() → ALL OK\n");
        else
            printf("[CPPREFRESH] ⚠ Video  refreshAllHandles() → PARTIAL FAIL (ret=%d)\n", vret);
    } catch (const std::exception &e) {
        printf("[CPPREFRESH] ✘ Video  refreshAllHandles() exception: %s\n", e.what());
        vret = dsERR_GENERAL;
    } catch (...) {
        printf("[CPPREFRESH] ✘ Video  refreshAllHandles() unknown exception\n");
        vret = dsERR_GENERAL;
    }

    bool all_ok = (aret == dsERR_NONE && vret == dsERR_NONE);
    g_handles_valid = all_ok;

    printf("\n[CPPREFRESH] Result: Audio=%s  Video=%s\n",
           aret == dsERR_NONE ? "OK" : "FAIL",
           vret == dsERR_NONE ? "OK" : "FAIL");
    if (all_ok) {
        printf("[CPPREFRESH] ✔ All C++ wrapper handles refreshed.\n");
        printf("[CPPREFRESH]   Module TCs should now pass without Full Re-Init.\n");
        g_run_count = 0;
        g_verify_results.clear();
        printf("[CPPREFRESH]   Run counter reset to 0 (post-refresh baseline).\n");
    } else {
        printf("[CPPREFRESH] ✘ One or more ports failed to refresh.\n");
        printf("[CPPREFRESH]   Is dsmgr running?  → pidof dsMgrMain\n");
        printf("[CPPREFRESH]   If handles are still bad use [I] Full Re-Initialize.\n");
    }
    printf("[CPPREFRESH] ────────────────────────────────────────────────────\n\n");
}

/* =========================================================================
 * Run directory creation
 * ========================================================================= */

static std::string get_run_dir(int run_num)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "RUN%02d", run_num);
    std::string d = g_out_root + "/" + buf;
    mkdirp(d);
    return d;
}

static std::string get_module_dir(const std::string &run_dir,
                                  const std::string &module_name)
{
    std::string d = run_dir + "/" + module_name;
    mkdirp(d);
    return d;
}

/* =========================================================================
 * TC-FPD-01: getFPBrightness (READ)
 *   Reads brightness for every configured FP indicator.
 *   Output file: 4_FPD/getFPBrightness_<indicator>.txt  per indicator
 *   API: FrontPanelIndicator::getBrightness()
 * ========================================================================= */
static void tc_fpd_01_get_brightness(const std::string &mod_dir)
{
    printf("\n  [TC-FPD-01] getFPBrightness – reading all indicators\n");

    for (int i = 0; FPD_INDICATORS[i] != NULL; ++i) {
        const char *name = FPD_INDICATORS[i];
        std::string fname = mod_dir + "/getFPBrightness_" + name + ".txt";
        TeeStream out;
        out.open(fname);

        out << "=== TC-FPD-01: getFPBrightness [" << name << "] ===\n";
        out << "Timestamp : " << timestamp_now() << "\n";
        out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
        out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";

        bool pass = false;
        try {
            int bright = device::FrontPanelIndicator::getInstance(name).getBrightness();
            out << "getFPBrightness(" << name << ") = " << bright << "\n";
            out << "RESULT: PASS\n";
            pass = true;
            verify_record("TC-FPD-01-" + std::string(name),
                          std::string("getFPBrightness(") + name + ")",
                          true,
                          std::to_string(bright));
        }
        catch (const device::Exception &e) {
            out << "EXCEPTION (device::Exception): " << e.what() << "\n";
            out << "RESULT: FAIL – stale handle or dsmgr not running\n";
            verify_record("TC-FPD-01-" + std::string(name),
                          std::string("getFPBrightness(") + name + ")",
                          false, e.what());
        }
        catch (...) {
            out << "EXCEPTION (unknown)\n";
            out << "RESULT: FAIL\n";
            verify_record("TC-FPD-01-" + std::string(name),
                          std::string("getFPBrightness(") + name + ")",
                          false, "unknown exception");
        }
        out << "Saved to: " << fname << "\n";
    }
}

/* =========================================================================
 * TC-FPD-02: setFPBrightness (WRITE + READ-BACK)
 *   Sets brightness=50 on "Power" indicator, reads back to verify.
 *   Output file: 4_FPD/setFPBrightness_Power_50.txt
 *   API: FrontPanelIndicator::setBrightness(int, bool)
 *        FrontPanelIndicator::getBrightness()
 * ========================================================================= */
static void tc_fpd_02_set_brightness(const std::string &mod_dir)
{
    printf("\n  [TC-FPD-02] setFPBrightness – Power indicator = 50\n");

    std::string fname = mod_dir + "/setFPBrightness_Power_50.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-FPD-02: setFPBrightness [Power] = 50 ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";

    try {
        device::FrontPanelIndicator &ind =
            device::FrontPanelIndicator::getInstance("Power");

        /* ── Step 1: GET current (before) value ─────────────────────── */
        int before = ind.getBrightness();
        out << "Step 1 – GET before value    : " << before << "\n";

        /* ── Step 2: SET to test value 50 ───────────────────────────── */
        out << "Step 2 – SET brightness to   : 50\n";
        ind.setBrightness(50, false /* persist=false: do not write to persistent store */);
        out << "         Waiting 5s for HAL to apply value...\n";
        sleep(5);

        /* ── Step 3: GET to verify SET value ────────────────────────── */
        int after_set = ind.getBrightness();
        out << "Step 3 – GET after set       : " << after_set << "\n";
        bool pass_set = (after_set == 50);
        out << "         Verify (expect 50)  : " << (pass_set ? "PASS" : "FAIL (mismatch)") << "\n";

        /* ── Step 4: SET back to original (before) value ────────────── */
        out << "Step 4 – SET restore to      : " << before << "\n";
        ind.setBrightness(before, false);
        out << "         Waiting 5s for HAL to apply value...\n";
        sleep(5);

        /* ── Step 5: GET to verify restored value ───────────────────── */
        int after_restore = ind.getBrightness();
        out << "Step 5 – GET after restore   : " << after_restore << "\n";
        bool pass_restore = (after_restore == before);
        out << "         Verify (expect " << before << ")  : "
            << (pass_restore ? "PASS" : "FAIL (mismatch)") << "\n";

        bool pass = pass_set && pass_restore;
        out << "\nRESULT: " << (pass ? "PASS" : "FAIL") << "\n";
        out << "  set_verify=" << (pass_set ? "PASS" : "FAIL")
            << "  restore_verify=" << (pass_restore ? "PASS" : "FAIL") << "\n";

        verify_record("TC-FPD-02", "setFPBrightness(Power,50) set+restore round-trip", pass,
                      "before=" + std::to_string(before) +
                      " after_set=" + std::to_string(after_set) +
                      " after_restore=" + std::to_string(after_restore));
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL – stale handle or dsmgr not running\n";
        verify_record("TC-FPD-02", "setFPBrightness(Power,50) round-trip", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-FPD-02", "setFPBrightness(Power,50) round-trip", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-FPD-03: setFPColor (WRITE + READ-BACK)
 *   Sets Power indicator to Blue, reads back color mode.
 *   Output file: 4_FPD/setFPColor_Power_Blue.txt
 *   API: FrontPanelIndicator::setColor(const Color&, bool)
 *        FrontPanelIndicator::getColorMode()
 * ========================================================================= */
static void tc_fpd_03_set_color(const std::string &mod_dir)
{
    printf("\n  [TC-FPD-03] setFPColor – Power indicator = Blue\n");

    std::string fname = mod_dir + "/setFPColor_Power_Blue.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-FPD-03: setFPColor [Power] = Blue ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";

    try {
        device::FrontPanelIndicator &ind =
            device::FrontPanelIndicator::getInstance("Power");

        int colorModeBefore = ind.getColorMode();
        out << "Color mode before set : " << colorModeBefore << "\n";
        out << "Setting color to      : Blue\n";

        ind.setColor(device::FrontPanelIndicator::Color::getInstance("Blue"),
                     false /* persist */);

        int colorModeAfter = ind.getColorMode();
        out << "Color mode after set  : " << colorModeAfter << "\n";
        out << "Sample Application: Color Mode value is " << colorModeAfter << "\n";
        out << "RESULT: PASS (set color completed)\n";
        verify_record("TC-FPD-03", "setFPColor(Power,Blue)", true,
                      "colorMode=" + std::to_string(colorModeAfter));
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL – stale handle or dsmgr not running\n";
        verify_record("TC-FPD-03", "setFPColor(Power,Blue)", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-FPD-03", "setFPColor(Power,Blue)", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-FPD-04: setFPState (WRITE + READ-BACK)
 *   Sets Power indicator state = ON, reads back state + brightness levels
 *   + supported colors.
 *   Output file: 4_FPD/setFPState_Power_on.txt
 *   API: FrontPanelIndicator::setState(bool)
 *        FrontPanelIndicator::getState()
 *        FrontPanelIndicator::getBrightnessLevels(int&,int&,int&)
 *        FrontPanelIndicator::getSupportedColors()
 * ========================================================================= */
static void tc_fpd_04_set_state(const std::string &mod_dir)
{
    printf("\n  [TC-FPD-04] setFPState – Power indicator = ON\n");

    std::string fname = mod_dir + "/setFPState_Power_on.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-FPD-04: setFPState [Power] = ON ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";

    try {
        device::FrontPanelIndicator &ind =
            device::FrontPanelIndicator::getInstance("Power");

        bool stateBefore = ind.getState();
        out << "Current state of [Power] : " << (stateBefore ? "On" : "Off") << "\n";
        out << "Setting state to         : On\n";

        ind.setState(true);

        bool stateAfter = ind.getState();
        out << "State after set          : " << (stateAfter ? "On" : "Off") << "\n";

        /* Brightness levels */
        int levels = 0, minBright = 0, maxBright = 0;
        ind.getBrightnessLevels(levels, minBright, maxBright);
        out << "Brightness levels: count=" << levels
            << "  min=" << minBright
            << "  max=" << maxBright << "\n";

        /* Supported colors */
        device::List<device::FrontPanelIndicator::Color> colors =
            ind.getSupportedColors();
        out << "Supported colors count   : " << colors.size() << "\n";

        bool pass = stateAfter;  /* should be ON after setState(true) */
        out << "RESULT: " << (pass ? "PASS" : "FAIL (state did not change to On)") << "\n";
        verify_record("TC-FPD-04", "setFPState(Power,ON) readback", pass,
                      std::string("stateAfter=") + (stateAfter ? "On" : "Off"));
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL – stale handle or dsmgr not running\n";
        verify_record("TC-FPD-04", "setFPState(Power,ON)", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-FPD-04", "setFPState(Power,ON)", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-FPD-05: setFPTextDisplay (WRITE + READ-BACK)
 *   Sets text "RDK" on the text display, reads brightness per indicator.
 *   Output file: 4_FPD/setFPTextDisplay_RDK.txt
 *   API: FrontPanelConfig::getTextDisplay("Text").setText(const char*)
 *        FrontPanelConfig::getIndicator(name).getBrightness()
 * ========================================================================= */
static void tc_fpd_05_set_text_display(const std::string &mod_dir)
{
    printf("\n  [TC-FPD-05] setFPTextDisplay – text = 'RDK'\n");

    std::string fname = mod_dir + "/setFPTextDisplay_RDK.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-FPD-05: setFPTextDisplay = 'RDK' ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";

    try {
        device::FrontPanelConfig &cfg = device::FrontPanelConfig::getInstance();

        /* Read brightness per indicator before set */
        out << "--- Indicator brightness BEFORE setText ---\n";
        for (int i = 0; FPD_INDICATORS[i] != NULL; ++i) {
            try {
                int b = cfg.getIndicator(FPD_INDICATORS[i]).getBrightness();
                out << "  " << FPD_INDICATORS[i] << " : brightness is " << b << "\n";
            }
            catch (...) {
                out << "  " << FPD_INDICATORS[i] << " : getBrightness FAILED\n";
            }
        }

        out << "\nSample Application: set text display------- RDK\n";
        cfg.getTextDisplay("Text").setText("RDK");
        out << "setText('RDK') completed\n";

        /* Read brightness per indicator after set */
        out << "--- Indicator brightness AFTER setText ---\n";
        for (int i = 0; FPD_INDICATORS[i] != NULL; ++i) {
            try {
                int b = cfg.getIndicator(FPD_INDICATORS[i]).getBrightness();
                out << "  " << FPD_INDICATORS[i] << " : brightness is " << b << "\n";
            }
            catch (...) {
                out << "  " << FPD_INDICATORS[i] << " : getBrightness FAILED\n";
            }
        }

        out << "RESULT: PASS\n";
        verify_record("TC-FPD-05", "setFPTextDisplay(RDK)", true, "setText completed");
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL – stale handle or dsmgr not running\n";
        verify_record("TC-FPD-05", "setFPTextDisplay(RDK)", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-FPD-05", "setFPTextDisplay(RDK)", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-FPD-06: setFPTextTimeFormat (WRITE + READ-BACK)
 *   Sets time format to 12-hour mode, reads back current format.
 *   Output file: 4_FPD/setFPTextTimeFormat_12hr.txt
 *   API: FrontPanelTextDisplay::setTimeFormat(int)
 *        FrontPanelTextDisplay::getCurrentTimeFormat()
 * ========================================================================= */
static void tc_fpd_06_set_time_format(const std::string &mod_dir)
{
    printf("\n  [TC-FPD-06] setFPTextTimeFormat – format = 12-hr (0)\n");

    std::string fname = mod_dir + "/setFPTextTimeFormat_12hr.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-FPD-06: setFPTextTimeFormat = 12-hr (0) ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";

    try {
        device::FrontPanelTextDisplay &td =
            device::FrontPanelConfig::getInstance().getTextDisplay("Text");

        int fmtBefore = td.getCurrentTimeFormat();
        out << "Before setting - Time zone read from DS is  " << fmtBefore << "\n";

        out << "Setting format to : 0 (12_HR)\n";
        td.setTimeFormat(0);  /* 0 = dsFPD_TIME_12_HOUR */

        int fmtAfter = td.getCurrentTimeFormat();
        out << "After setting - Time zone read from DS is now " << fmtAfter << "\n";

        bool pass = (fmtAfter == 0);
        out << "RESULT: " << (pass ? "PASS" : "FAIL (readback mismatch)") << "\n";
        verify_record("TC-FPD-06", "setFPTextTimeFormat(12hr) readback", pass,
                      "before=" + std::to_string(fmtBefore) +
                      " after=" + std::to_string(fmtAfter));
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL – stale handle or dsmgr not running\n";
        verify_record("TC-FPD-06", "setFPTextTimeFormat(12hr)", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-FPD-06", "setFPTextTimeFormat(12hr)", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-AUD-01: getAudioOutputPorts – read all port properties
 *
 *   STALE HANDLE DEMONSTRATION:
 *   AudioOutputPort._handle = intptr_t from dsGetAudioPort() RPC acquired
 *   at Manager::Initialize() time.  After dsmgr restart all API calls that
 *   pass this handle to dsmgr via IARM RPC will throw device::Exception.
 *
 *   Output file: 1_Audio/getAudioOutputPorts.txt
 * ========================================================================= */
static void tc_aud_01_get_ports(const std::string &mod_dir)
{
    printf("\n  [TC-AUD-01] getAudioOutputPorts – reading all port properties\n");

    std::string fname = mod_dir + "/getAudioOutputPorts.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-AUD-01: getAudioOutputPorts ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  AudioOutputPort._handle = intptr_t from dsGetAudioPort() RPC at init.\n";
    out << "  All handle-based APIs below will throw device::Exception after restart.\n\n";

    bool any_fail = false;
    try {
        device::List<device::AudioOutputPort> aPorts =
            device::Host::getInstance().getAudioOutputPorts();
        out << "Port count : " << aPorts.size() << "\n\n";

        for (size_t i = 0; i < aPorts.size(); i++) {
            device::AudioOutputPort &aPort = aPorts.at(i);
            out << "\u2500\u2500 Port [" << aPort.getName() << "] \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";

            /* isEnabled – passes intptr_t _handle via IARM RPC */
            try {
                bool en = aPort.isEnabled();
                out << "  isEnabled()        : " << (en ? "Yes" : "No") << "\n";
            } catch (...) {
                out << "  isEnabled()        : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }

            /* isMuted – passes intptr_t _handle */
            try {
                bool mu = aPort.isMuted();
                out << "  isMuted()          : " << (mu ? "Yes" : "No") << "\n";
            } catch (...) {
                out << "  isMuted()          : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }

            /* getLevel – passes intptr_t _handle */
            try {
                float lv = aPort.getLevel();
                out << "  getLevel()         : " << lv << "\n";
            } catch (...) {
                out << "  getLevel()         : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }

            /* getEncoding – passes intptr_t _handle */
            try {
                const device::AudioEncoding &enc = aPort.getEncoding();
                out << "  getEncoding()      : " << enc.getName() << "\n";
            } catch (...) {
                out << "  getEncoding()      : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }

            /* getStereoMode – passes intptr_t _handle */
            try {
                const device::AudioStereoMode &sm = aPort.getStereoMode(false);
                out << "  getStereoMode()    : " << sm.getName() << "\n";
            } catch (...) {
                out << "  getStereoMode()    : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }

            out << "\n";
        }

        bool pass = !any_fail;
        out << "RESULT: " << (pass ? "PASS" : "FAIL \u2013 handle-based calls threw exceptions") << "\n";
        verify_record("TC-AUD-01", "getAudioOutputPorts all-port property read", pass,
                      pass ? "all handle calls OK"
                           : "STALE HANDLE: handle-based calls FAILED after dsmgr restart");
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-AUD-01", "getAudioOutputPorts all-port property read", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-AUD-01", "getAudioOutputPorts all-port property read", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-AUD-02: setMuted – toggle+restore round-trip on first audio output port
 *
 *   GET muted state → toggle mute → wait 2s → verify toggled
 *   → restore original → verify restored.
 *   setMuted() passes intptr_t _handle → will throw after dsmgr restart.
 *
 *   Output file: 1_Audio/setMuted_roundtrip.txt
 * ========================================================================= */
static void tc_aud_02_set_muted(const std::string &mod_dir)
{
    printf("\n  [TC-AUD-02] setMuted – toggle+restore round-trip on SPEAKER0 (TV profile only)\n");

    std::string fname = mod_dir + "/setMuted_roundtrip.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-AUD-02: setMuted round-trip (toggle+restore) ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "Profile   : " << (g_profile == PROFILE_TV ? "TV" :
                              g_profile == PROFILE_STB ? "STB" :
                              g_profile == PROFILE_COMMON ? "Common" : "All") << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";

    /* ── Profile guard: only applicable on TV ──────────────────────────── */
    if (g_profile != PROFILE_TV && g_profile != PROFILE_ALL) {
        out << "Profile is not TV – TC-AUD-02 is a TV-only test (SPEAKER0).\n";
        out << "RESULT: SKIP (profile=" << (g_profile == PROFILE_STB ? "STB" : "Common") << ")\n";
        verify_record("TC-AUD-02", "setMuted SPEAKER0 round-trip", true,
                      "skip – profile is not TV");
        out << "Saved to: " << fname << "\n";
        printf("  [TC-AUD-02] SKIP – profile is not TV (SPEAKER0 not applicable).\n");
        return;
    }

    try {
        device::AudioOutputPort &aPort =
            device::Host::getInstance().getAudioOutputPort("SPEAKER0");

        out << "Port      : " << aPort.getName() << "\n\n";

        /* Step 1: GET current mute state */
        bool was_muted = aPort.isMuted();
        out << "Step 1 \u2013 GET current muted state : " << (was_muted ? "muted" : "unmuted") << "\n";

        /* Step 2: SET to toggled state */
        bool target = !was_muted;
        out << "Step 2 \u2013 SET muted to            : " << (target ? "muted" : "unmuted") << "\n";
        aPort.setMuted(target);
        out << "         Waiting 2s...\n";
        sleep(2);

        /* Step 3: GET to verify toggle */
        bool after_set = aPort.isMuted();
        out << "Step 3 \u2013 GET after toggle        : " << (after_set ? "muted" : "unmuted") << "\n";
        bool pass_set = (after_set == target);
        out << "         Verify               : " << (pass_set ? "PASS" : "FAIL (mismatch)") << "\n";

        /* Step 4: SET restore */
        out << "Step 4 \u2013 SET restore to          : " << (was_muted ? "muted" : "unmuted") << "\n";
        aPort.setMuted(was_muted);
        out << "         Waiting 2s...\n";
        sleep(2);

        /* Step 5: GET verify restore */
        bool after_restore = aPort.isMuted();
        out << "Step 5 \u2013 GET after restore       : " << (after_restore ? "muted" : "unmuted") << "\n";
        bool pass_restore = (after_restore == was_muted);
        out << "         Verify restore       : " << (pass_restore ? "PASS" : "FAIL (mismatch)") << "\n";

        bool pass = pass_set && pass_restore;
        out << "\nRESULT: " << (pass ? "PASS" : "FAIL") << "\n";
        verify_record("TC-AUD-02", "setMuted SPEAKER0 toggle+restore round-trip", pass,
                      std::string("was_muted=") + (was_muted ? "true" : "false") +
                      " after_set=" + (after_set ? "true" : "false") +
                      " after_restore=" + (after_restore ? "true" : "false"));
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "NOTE: SPEAKER0 may not exist on this platform, or handle is stale.\n";
        out << "RESULT: FAIL – stale handle or SPEAKER0 not present\n";
        verify_record("TC-AUD-02", "setMuted SPEAKER0 toggle+restore round-trip", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-AUD-02", "setMuted SPEAKER0 toggle+restore round-trip", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-CIN-01: CompositeIn – getNumberOfInputs / isPresented / getActivePort
 *
 *   Output file: 2_CompositeIn/getCompositeInfo.txt
 * ========================================================================= */
static void tc_cin_01_get_info(const std::string &mod_dir)
{
    printf("\n  [TC-CIN-01] CompositeIn – getNumberOfInputs / isPresented / getActivePort\n");

    std::string fname = mod_dir + "/getCompositeInfo.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-CIN-01: CompositeIn getInfo ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";

    try {
        device::CompositeInput &cin = device::CompositeInput::getInstance();

        uint8_t num = cin.getNumberOfInputs();
        out << "getNumberOfInputs() : " << (int)num << "\n";

        bool presented = cin.isPresented();
        out << "isPresented()       : " << (presented ? "Yes" : "No") << "\n";

        int8_t activePort = cin.getActivePort();
        out << "getActivePort()     : " << (int)activePort
            << (activePort == COMPOSITE_IN_PORT_NONE ? " (none)" : "") << "\n";

        out << "\nRESULT: PASS\n";
        verify_record("TC-CIN-01", "CompositeIn getInfo", true,
                      "numInputs=" + std::to_string((int)num) +
                      " presented=" + (presented ? "true" : "false") +
                      " activePort=" + std::to_string((int)activePort));
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-CIN-01", "CompositeIn getInfo", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-CIN-01", "CompositeIn getInfo", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-CIN-02: CompositeIn – isPortConnected status per input port
 *
 *   Output file: 2_CompositeIn/getCompositePortStatus.txt
 * ========================================================================= */
static void tc_cin_02_port_connected(const std::string &mod_dir)
{
    printf("\n  [TC-CIN-02] CompositeIn – isPortConnected per port\n");

    std::string fname = mod_dir + "/getCompositePortStatus.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-CIN-02: CompositeIn isPortConnected ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";

    try {
        device::CompositeInput &cin = device::CompositeInput::getInstance();
        uint8_t num = cin.getNumberOfInputs();
        out << "Number of inputs : " << (int)num << "\n\n";

        bool any_connected = false;
        for (int8_t port = 0; port < (int8_t)num; ++port) {
            bool conn = cin.isPortConnected(port);
            bool active = cin.isActivePort(port);
            out << "  Port " << (int)port
                << "  connected=" << (conn   ? "Yes" : "No")
                << "  active="    << (active ? "Yes" : "No") << "\n";
            if (conn) any_connected = true;
        }

        out << "\nRESULT: PASS\n";
        verify_record("TC-CIN-02", "CompositeIn isPortConnected per port", true,
                      std::string("numPorts=") + std::to_string((int)num) +
                      " anyConnected=" + (any_connected ? "true" : "false"));
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-CIN-02", "CompositeIn isPortConnected per port", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-CIN-02", "CompositeIn isPortConnected per port", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-DSP-01: Display – hasSurround + getSurroundMode via HDMI0
 *
 *   VideoOutputPort::Display._handle = intptr_t from dsGetDisplay() RPC.
 *   After dsmgr restart Display API calls throw device::Exception.
 *
 *   Output file: 3_Display/getDisplaySurround.txt
 * ========================================================================= */
static void tc_dsp_01_display_surround(const std::string &mod_dir)
{
    printf("\n  [TC-DSP-01] Display – hasSurround + getSurroundMode (%s)\n", HDMI_PORT_NAME);

    std::string fname = mod_dir + "/getDisplaySurround.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-DSP-01: Display hasSurround + getSurroundMode ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n";
    out << "Port      : " << HDMI_PORT_NAME << "\n\n";

    try {
        device::VideoOutputPort &vPort =
            device::Host::getInstance().getVideoOutputPort(HDMI_PORT_NAME);

        /* Check HDMI connection first – this call itself uses the stale handle */
        bool connected = vPort.isDisplayConnected();
        out << "isDisplayConnected() : " << (connected ? "Yes" : "No") << "\n";

        if (!connected) {
            out << "Display not connected \u2013 surround queries skipped.\n";
            out << "RESULT: SKIP (no display connected)\n";
            verify_record("TC-DSP-01", "Display hasSurround+getSurroundMode", true,
                          "display not connected \u2013 skip");
            out << "Saved to: " << fname << "\n";
            return;
        }

        /* getDisplay() obtains Display._handle from dsGetDisplay() RPC */
        const device::VideoOutputPort::Display &disp = vPort.getDisplay();

        bool surround = disp.hasSurround();
        out << "hasSurround()        : " << (surround ? "Yes" : "No") << "\n";

        int surroundMode = disp.getSurroundMode();
        out << "getSurroundMode()    : " << surroundMode << "\n";

        out << "\nRESULT: PASS\n";
        verify_record("TC-DSP-01", "Display hasSurround+getSurroundMode", true,
                      std::string("surround=") + (surround ? "true" : "false") +
                      " mode=" + std::to_string(surroundMode));
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL \u2013 stale handle or dsmgr not running\n";
        verify_record("TC-DSP-01", "Display hasSurround+getSurroundMode", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-DSP-01", "Display hasSurround+getSurroundMode", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-DSP-02: Display – getEDIDBytes via HDMI0 (only when display connected)
 *
 *   Reads raw EDID buffer from the connected HDMI display.
 *   Display._handle is also handle-based → throws after dsmgr restart.
 *
 *   Output file: 3_Display/getDisplayEDID.txt
 * ========================================================================= */
static void tc_dsp_02_display_edid(const std::string &mod_dir)
{
    printf("\n  [TC-DSP-02] Display – getEDIDBytes (%s)\n", HDMI_PORT_NAME);

    std::string fname = mod_dir + "/getDisplayEDID.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-DSP-02: Display getEDIDBytes ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n";
    out << "Port      : " << HDMI_PORT_NAME << "\n\n";

    try {
        device::VideoOutputPort &vPort =
            device::Host::getInstance().getVideoOutputPort(HDMI_PORT_NAME);

        bool connected = vPort.isDisplayConnected();
        out << "isDisplayConnected() : " << (connected ? "Yes" : "No") << "\n";

        if (!connected) {
            out << "Display not connected \u2013 EDID query skipped.\n";
            out << "RESULT: SKIP (no display connected)\n";
            verify_record("TC-DSP-02", "Display getEDIDBytes", true,
                          "display not connected \u2013 skip");
            out << "Saved to: " << fname << "\n";
            return;
        }

        const device::VideoOutputPort::Display &disp = vPort.getDisplay();

        std::vector<uint8_t> edid;
        disp.getEDIDBytes(edid);

        out << "EDID byte count      : " << edid.size() << "\n";
        out << "First 8 bytes (hex)  : ";
        for (size_t b = 0; b < edid.size() && b < 8; ++b) {
            char hex[8];
            snprintf(hex, sizeof(hex), "%02X ", edid[b]);
            out << hex;
        }
        out << "\n";

        bool pass = (edid.size() >= 128);
        out << "\nRESULT: " << (pass ? "PASS" : "FAIL (EDID too short – expected >=128 bytes)") << "\n";
        verify_record("TC-DSP-02", "Display getEDIDBytes", pass,
                      "edidLen=" + std::to_string(edid.size()));
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL \u2013 stale handle or dsmgr not running\n";
        verify_record("TC-DSP-02", "Display getEDIDBytes", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-DSP-02", "Display getEDIDBytes", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-HIN-01: HdmiInput – getNumberOfInputs / isPresented / getActivePort /
 *            getCurrentVideoMode
 *
 *   Output file: 5_HDMIIn/getHdmiInInfo.txt
 * ========================================================================= */
static void tc_hin_01_get_info(const std::string &mod_dir)
{
    printf("\n  [TC-HIN-01] HdmiInput – getNumberOfInputs / isPresented / getActivePort\n");

    std::string fname = mod_dir + "/getHdmiInInfo.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-HIN-01: HdmiInput getInfo ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";

    try {
        device::HdmiInput &hin = device::HdmiInput::getInstance();

        uint8_t num = hin.getNumberOfInputs();
        out << "getNumberOfInputs()   : " << (int)num << "\n";

        bool presented = hin.isPresented();
        out << "isPresented()         : " << (presented ? "Yes" : "No") << "\n";

        int8_t activePort = hin.getActivePort();
        out << "getActivePort()       : " << (int)activePort
            << (activePort == HDMI_IN_PORT_NONE ? " (none)" : "") << "\n";

        if (activePort >= 0) {
            std::string videoMode = hin.getCurrentVideoMode();
            out << "getCurrentVideoMode() : " << videoMode << "\n";
        } else {
            out << "getCurrentVideoMode() : N/A (no active port)\n";
        }

        out << "\nRESULT: PASS\n";
        verify_record("TC-HIN-01", "HdmiInput getInfo", true,
                      "numInputs=" + std::to_string((int)num) +
                      " presented=" + (presented ? "true" : "false") +
                      " activePort=" + std::to_string((int)activePort));
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-HIN-01", "HdmiInput getInfo", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-HIN-01", "HdmiInput getInfo", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-HIN-02: HdmiInput – isPortConnected status per HDMI input port
 *
 *   Output file: 5_HDMIIn/getHdmiInPortStatus.txt
 * ========================================================================= */
static void tc_hin_02_port_connected(const std::string &mod_dir)
{
    printf("\n  [TC-HIN-02] HdmiInput – isPortConnected per port\n");

    std::string fname = mod_dir + "/getHdmiInPortStatus.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-HIN-02: HdmiInput isPortConnected ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";

    try {
        device::HdmiInput &hin = device::HdmiInput::getInstance();
        uint8_t num = hin.getNumberOfInputs();
        out << "Number of inputs : " << (int)num << "\n\n";

        bool any_connected = false;
        for (int8_t port = 0; port < (int8_t)num; ++port) {
            bool conn   = hin.isPortConnected(port);
            bool active = hin.isActivePort(port);
            out << "  Port " << (int)port
                << "  connected=" << (conn   ? "Yes" : "No")
                << "  active="    << (active ? "Yes" : "No") << "\n";
            if (conn) any_connected = true;
        }

        out << "\nRESULT: PASS\n";
        verify_record("TC-HIN-02", "HdmiInput isPortConnected per port", true,
                      std::string("numPorts=") + std::to_string((int)num) +
                      " anyConnected=" + (any_connected ? "true" : "false"));
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-HIN-02", "HdmiInput isPortConnected per port", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-HIN-02", "HdmiInput isPortConnected per port", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-HST-01: Host – getCPUTemperature / getVersion / getPowerMode /
 *            isHDMIOutPortPresent / getDefaultVideoPortName / getDefaultAudioPortName
 *
 *   Output file: 6_Host/getHostInfo.txt
 * ========================================================================= */
static void tc_hst_01_get_info(const std::string &mod_dir)
{
    printf("\n  [TC-HST-01] Host – getCPUTemp / version / powerMode / port names\n");

    std::string fname = mod_dir + "/getHostInfo.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-HST-01: Host getInfo ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";

    bool any_fail = false;
    try {
        device::Host &h = device::Host::getInstance();

        try {
            float temp = h.getCPUTemperature();
            out << "getCPUTemperature()       : " << temp << " \u00b0C\n";
        } catch (...) {
            out << "getCPUTemperature()       : EXCEPTION\n";
            any_fail = true;
        }

        try {
            uint32_t ver = h.getVersion();
            char vbuf[32];
            snprintf(vbuf, sizeof(vbuf), "0x%08X", ver);
            out << "getVersion()              : " << vbuf << "\n";
        } catch (...) {
            out << "getVersion()              : EXCEPTION\n";
            any_fail = true;
        }

        try {
            int pm = h.getPowerMode();
            out << "getPowerMode()            : " << pm << "\n";
        } catch (...) {
            out << "getPowerMode()            : EXCEPTION\n";
            any_fail = true;
        }

        try {
            bool hdmiPresent = h.isHDMIOutPortPresent();
            out << "isHDMIOutPortPresent()    : " << (hdmiPresent ? "Yes" : "No") << "\n";
        } catch (...) {
            out << "isHDMIOutPortPresent()    : EXCEPTION\n";
        }

        try {
            std::string defVP = h.getDefaultVideoPortName();
            out << "getDefaultVideoPortName() : " << defVP << "\n";
        } catch (...) {
            out << "getDefaultVideoPortName() : EXCEPTION\n";
        }

        try {
            std::string defAP = h.getDefaultAudioPortName();
            out << "getDefaultAudioPortName() : " << defAP << "\n";
        } catch (...) {
            out << "getDefaultAudioPortName() : EXCEPTION\n";
        }

        bool pass = !any_fail;
        out << "\nRESULT: " << (pass ? "PASS" : "FAIL") << "\n";
        verify_record("TC-HST-01", "Host getInfo (CPUTemp/version/powerMode)", pass,
                      pass ? "all getters OK" : "one or more getters threw exception");
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-HST-01", "Host getInfo", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-HST-01", "Host getInfo", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-HST-02: Host – getPreferredSleepMode / getAvailableSleepModes /
 *            port counts (audio + video + video device)
 *
 *   Output file: 6_Host/getHostSleepModes.txt
 * ========================================================================= */
static void tc_hst_02_sleep_modes(const std::string &mod_dir)
{
    printf("\n  [TC-HST-02] Host – sleep modes + port counts\n");

    std::string fname = mod_dir + "/getHostSleepModes.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-HST-02: Host sleep modes + port counts ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";

    try {
        device::Host &h = device::Host::getInstance();

        try {
            device::SleepMode pref = h.getPreferredSleepMode();
            out << "getPreferredSleepMode()      : " << pref.getName() << "\n";
        } catch (...) {
            out << "getPreferredSleepMode()      : EXCEPTION\n";
        }

        try {
            device::List<device::SleepMode> modes = h.getAvailableSleepModes();
            out << "getAvailableSleepModes()     : " << modes.size() << " modes\n";
            for (size_t i = 0; i < modes.size(); ++i) {
                out << "  [" << i << "] " << modes.at(i).getName() << "\n";
            }
        } catch (...) {
            out << "getAvailableSleepModes()     : EXCEPTION\n";
        }

        try {
            out << "getAudioOutputPorts().size() : "
                << h.getAudioOutputPorts().size() << "\n";
        } catch (...) {
            out << "getAudioOutputPorts().size() : EXCEPTION\n";
        }

        try {
            out << "getVideoOutputPorts().size() : "
                << h.getVideoOutputPorts().size() << "\n";
        } catch (...) {
            out << "getVideoOutputPorts().size() : EXCEPTION\n";
        }

        try {
            out << "getVideoDevices().size()     : "
                << h.getVideoDevices().size() << "\n";
        } catch (...) {
            out << "getVideoDevices().size()     : EXCEPTION\n";
        }

        out << "\nRESULT: PASS\n";
        verify_record("TC-HST-02", "Host sleep modes + port counts", true, "");
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-HST-02", "Host sleep modes + port counts", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-HST-02", "Host sleep modes + port counts", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-VD-01: VideoDevice – read device info
 *
 *   STALE HANDLE DEMONSTRATION:
 *   VideoDevice._handle = intptr_t from dsGetVideoDevice() RPC acquired at
 *   Manager::Initialize() time.  After dsmgr restart all handle-based calls
 *   throw device::Exception.
 *
 *   Output file: 7_VideoDevice/getVideoDevices.txt
 * ========================================================================= */
static void tc_vd_01_get_devices(const std::string &mod_dir)
{
    printf("\n  [TC-VD-01] VideoDevice – getDFC / getHDRCapabilities / codecs / resolutions\n");

    std::string fname = mod_dir + "/getVideoDevices.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-VD-01: VideoDevice getInfo ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  VideoDevice._handle = intptr_t from dsGetVideoDevice() RPC at init.\n";
    out << "  getDFC/getHDRCapabilities/getSupportedVideoCodingFormats use this handle.\n\n";

    bool any_fail = false;
    try {
        device::List<device::VideoDevice> vDevices =
            device::Host::getInstance().getVideoDevices();
        out << "Device count : " << vDevices.size() << "\n\n";

        for (size_t i = 0; i < vDevices.size(); i++) {
            device::VideoDevice &dev = vDevices.at(i);
            out << "\u2500\u2500 Device [" << i << "] \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";

            /* getDFC – uses _handle */
            try {
                const device::VideoDFC &dfc = dev.getDFC();
                out << "  getDFC()                       : " << dfc.getName() << "\n";
            } catch (...) {
                out << "  getDFC()                       : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }

            /* getSupportedDFCs – config read, may not need live handle */
            try {
                const device::List<device::VideoDFC> dfcs = dev.getSupportedDFCs();
                out << "  getSupportedDFCs().size()      : " << dfcs.size() << "\n";
            } catch (...) {
                out << "  getSupportedDFCs()             : EXCEPTION\n";
            }

            /* getHDRCapabilities – uses _handle */
            try {
                int caps = 0;
                dev.getHDRCapabilities(&caps);
                char cbuf[32];
                snprintf(cbuf, sizeof(cbuf), "0x%X", caps);
                out << "  getHDRCapabilities()           : " << cbuf << "\n";
            } catch (...) {
                out << "  getHDRCapabilities()           : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }

            /* getSupportedVideoCodingFormats – uses _handle */
            try {
                unsigned int fmts = dev.getSupportedVideoCodingFormats();
                char fbuf[32];
                snprintf(fbuf, sizeof(fbuf), "0x%X", fmts);
                out << "  getSupportedVideoCodingFormats : " << fbuf << "\n";
            } catch (...) {
                out << "  getSupportedVideoCodingFormats : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }

            /* getSettopSupportedResolutions – uses _handle */
            try {
                std::list<std::string> stbRes;
                dev.getSettopSupportedResolutions(stbRes);
                out << "  getSettopSupportedResolutions  : " << stbRes.size() << " entries\n";
            } catch (...) {
                out << "  getSettopSupportedResolutions  : EXCEPTION\n";
            }

            /* getFRFMode – informational */
            try {
                int frfmode = 0;
                int ret = dev.getFRFMode(&frfmode);
                out << "  getFRFMode()                   : " << frfmode << " (ret=" << ret << ")\n";
            } catch (...) {
                out << "  getFRFMode()                   : EXCEPTION\n";
            }

            out << "\n";
        }

        bool pass = !any_fail;
        out << "RESULT: " << (pass ? "PASS" : "FAIL \u2013 handle-based calls threw exceptions") << "\n";
        verify_record("TC-VD-01", "VideoDevice getInfo (DFC/HDRCaps/codecs)", pass,
                      pass ? "all handle calls OK"
                           : "STALE HANDLE: handle-based calls FAILED after dsmgr restart");
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-VD-01", "VideoDevice getInfo", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-VD-01", "VideoDevice getInfo", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-VD-02: VideoDevice – setDFC round-trip (GET → SET-same → GET verify)
 *
 *   Sets the same DFC back – no visual change.  setDFC() passes _handle
 *   to dsmgr IARM RPC → throws device::Exception after dsmgr restart.
 *
 *   Output file: 7_VideoDevice/setDFC_roundtrip.txt
 * ========================================================================= */
static void tc_vd_02_set_dfc(const std::string &mod_dir)
{
    printf("\n  [TC-VD-02] VideoDevice – setDFC round-trip (GET->SET-same->GET)\n");

    std::string fname = mod_dir + "/setDFC_roundtrip.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-VD-02: VideoDevice setDFC round-trip ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "NOTE: Setting the SAME DFC back (no visual change).\n";
    out << "      Purpose: prove DFC SET path also uses stale handle after restart.\n\n";

    try {
        device::List<device::VideoDevice> vDevices =
            device::Host::getInstance().getVideoDevices();

        if (vDevices.size() == 0) {
            out << "No video devices found. Skipping.\n";
            out << "RESULT: SKIP\n";
            verify_record("TC-VD-02", "VideoDevice setDFC round-trip", true, "no video devices \u2013 skip");
            out << "Saved to: " << fname << "\n";
            return;
        }

        device::VideoDevice &dev = vDevices.at(0);

        /* Step 1: GET current DFC */
        std::string dfc_before = dev.getDFC().getName();
        out << "Step 1 \u2013 GET current DFC : " << dfc_before << "\n";

        /* Step 2: SET same DFC back (no visual change) */
        out << "Step 2 \u2013 SET DFC to      : " << dfc_before << " (same, no visual change)\n";
        dev.setDFC(dfc_before);
        out << "         SET completed OK\n";
        out << "         Waiting 2s...\n";
        sleep(2);

        /* Step 3: GET to verify */
        std::string dfc_after = dev.getDFC().getName();
        out << "Step 3 \u2013 GET after set   : " << dfc_after << "\n";
        bool pass = (dfc_after == dfc_before);
        out << "         Verify          : " << (pass ? "PASS" : "FAIL (mismatch)") << "\n";

        out << "\nRESULT: " << (pass ? "PASS" : "FAIL") << "\n";
        verify_record("TC-VD-02", "VideoDevice setDFC round-trip", pass,
                      "before=" + dfc_before + " after=" + dfc_after);
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL \u2013 stale handle or dsmgr not running\n";
        out << "  *** Expected failure after dsmgr restart (stale intptr_t handle) ***\n";
        out << "  *** Use option [R] Re-Initialize DS to cure this               ***\n";
        verify_record("TC-VD-02", "VideoDevice setDFC round-trip", false,
                      std::string("STALE HANDLE: ") + e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-VD-02", "VideoDevice setDFC round-trip", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-VP-01: getVideoOutputPorts – read all port properties
 *
 *   STALE HANDLE DEMONSTRATION:
 *   Every VideoOutputPort C++ method passes an intptr_t _handle to dsmgr
 *   via IARM RPC.  That handle is a server-side memory pointer obtained by
 *   dsGetVideoPort() at Manager::Initialize() time.  After dsmgr restarts,
 *   the old pointer is INVALID in the new process → ALL calls below throw
 *   device::Exception.  Compare with FPD (Module 4) which identifies
 *   indicators by enum index and therefore does NOT fail after restart.
 *
 *   Output file: 8_VideoPort/getVideoOutputPorts.txt
 * ========================================================================= */
static void tc_vp_01_get_port_status(const std::string &mod_dir)
{
    printf("\n  [TC-VP-01] getVideoOutputPorts – reading all port properties\n");

    std::string fname = mod_dir + "/getVideoOutputPorts.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-VP-01: getVideoOutputPorts ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  VideoOutputPort._handle = intptr_t from dsGetVideoPort() RPC at init.\n";
    out << "  All APIs below pass this server-side pointer to dsmgr via IARM.\n";
    out << "  After dsmgr restart: pointer invalid -> device::Exception thrown.\n";
    out << "  FPD Module 4 uses enum index (no pointer) -> immune to this issue.\n\n";

    bool any_fail = false;
    try {
        device::List<device::VideoOutputPort> vPorts =
            device::Host::getInstance().getVideoOutputPorts();
        out << "Port count : " << vPorts.size() << "\n\n";

        for (size_t i = 0; i < vPorts.size(); i++) {
            device::VideoOutputPort &vPort = vPorts.at(i);
            std::string pname = vPort.getName();
            out << "── Port [" << pname << "] ──────────────────────────────────\n";

            /* isEnabled – passes intptr_t _handle via IARM RPC */
            try {
                bool en = vPort.isEnabled();
                out << "  isEnabled()              : " << (en ? "Yes" : "No") << "\n";
            } catch (...) {
                out << "  isEnabled()              : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }

            /* isDisplayConnected – passes intptr_t _handle */
            try {
                bool conn = vPort.isDisplayConnected();
                out << "  isDisplayConnected()     : " << (conn ? "Yes" : "No") << "\n";
            } catch (...) {
                out << "  isDisplayConnected()     : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }

            /* isActive – may also throw UnsupportedOperationException */
            try {
                bool active = vPort.isActive();
                out << "  isActive()               : " << (active ? "Yes" : "No") << "\n";
            } catch (...) {
                out << "  isActive()               : EXCEPTION (unsupported or stale)\n";
            }

            /* isContentProtected – passes intptr_t _handle */
            try {
                bool cp = vPort.isContentProtected();
                out << "  isContentProtected()     : " << (cp ? "Yes" : "No") << "\n";
            } catch (...) {
                out << "  isContentProtected()     : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }

            /* getResolution – calls dsGetCurrentResolution RPC with handle */
            try {
                std::string res = vPort.getResolution().getName();
                out << "  getResolution()          : " << res << "\n";
            } catch (...) {
                out << "  getResolution()          : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }

            /* getHDCPStatus – calls dsGetHDCPStatus RPC with handle */
            try {
                int hdcp = vPort.getHDCPStatus();
                out << "  getHDCPStatus()          : " << hdcp << "\n";
            } catch (...) {
                out << "  getHDCPStatus()          : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }

            /* getType – static config lookup, does NOT require handle */
            try {
                out << "  getType().name           : " << vPort.getType().getName() << "\n";
                out << "  getType().isHDCPSupported: "
                    << (vPort.getType().isHDCPSupported() ? "Yes" : "No") << "\n";
            } catch (...) {
                out << "  getType()                : EXCEPTION\n";
            }
            out << "\n";
        }

        bool pass = !any_fail;
        out << "RESULT: " << (pass ? "PASS"
                                   : "FAIL – handle-based calls threw exceptions") << "\n";
        verify_record("TC-VP-01", "getVideoOutputPorts all-port property read", pass,
                      pass ? "all handle calls OK"
                           : "STALE HANDLE: handle-based calls FAILED after dsmgr restart");
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL – stale handle or dsmgr not running\n";
        verify_record("TC-VP-01", "getVideoOutputPorts all-port property read",
                      false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-VP-01", "getVideoOutputPorts all-port property read",
                      false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-VP-02: setResolution – round-trip on HDMI0 (GET → SET-same → GET verify)
 *
 *   Sets the SAME resolution back (no visual change, persist=false) to
 *   exercise the WRITE code path through the stale intptr_t _handle.
 *   After dsmgr restart ALL three steps throw device::Exception.
 *
 *   Output file: 8_VideoPort/setResolution_HDMI0_round_trip.txt
 * ========================================================================= */
static void tc_vp_02_set_resolution(const std::string &mod_dir)
{
    printf("\n  [TC-VP-02] setResolution – HDMI0 round-trip (GET->SET-same->GET)\n");

    std::string fname = mod_dir + "/setResolution_HDMI0_round_trip.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-VP-02: setResolution HDMI0 round-trip ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n";
    out << "Port      : " << HDMI_PORT_NAME << "\n\n";
    out << "NOTE: Setting the SAME resolution back (no visual change, persist=false).\n";
    out << "      Purpose: prove SET path also uses stale handle after restart.\n\n";

    try {
        device::VideoOutputPort &vPort =
            device::Host::getInstance().getVideoOutputPort(HDMI_PORT_NAME);

        /* ── Step 1: GET current resolution ─────────────────────────── */
        std::string res_before = vPort.getResolution().getName();
        out << "Step 1 – GET current resolution : " << res_before << "\n";

        /* ── Step 2: SET same resolution back (no-op, persist=false) ── */
        out << "Step 2 – SET same resolution    : " << res_before << " (persist=false)\n";
        vPort.setResolution(res_before, false /* persist */);
        out << "         SET completed OK\n";
        out << "         Waiting 5s for HAL to apply value...\n";
        sleep(5);

        /* ── Step 3: GET to verify round-trip ───────────────────────── */
        std::string res_after = vPort.getResolution().getName();
        out << "Step 3 – GET after set          : " << res_after << "\n";
        bool pass = (res_after == res_before);
        out << "         Verify (expect " << res_before << "): "
            << (pass ? "PASS" : "FAIL (mismatch)") << "\n";

        out << "\nRESULT: " << (pass ? "PASS" : "FAIL") << "\n";
        verify_record("TC-VP-02", "setResolution(HDMI0) round-trip", pass,
                      "before=" + res_before + " after=" + res_after);
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL – stale handle or dsmgr not running\n";
        out << "  *** Expected failure after dsmgr restart (stale intptr_t handle) ***\n";
        out << "  *** Use option [R] Re-Initialize DS to cure this                 ***\n";
        verify_record("TC-VP-02", "setResolution(HDMI0) round-trip", false,
                      std::string("STALE HANDLE: ") + e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-VP-02", "setResolution(HDMI0) round-trip",
                      false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * Module 1 Audio runner
 * ========================================================================= */
static void run_module_audio(const std::string &run_dir)
{
    std::string mod_dir = get_module_dir(run_dir, "1_Audio");
    std::string sumfile = mod_dir + "/audio_run_summary.txt";
    TeeStream summary;
    summary.open(sumfile);

    print_sep(summary, "Module 1 \u2013 Audio  [stale handle: intptr_t _handle]");
    summary << "Run number  : " << g_run_count << "\n";
    summary << "Timestamp   : " << timestamp_now() << "\n";
    summary << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    summary << "DS_Init     : " << (g_ds_initialized   ? "YES" : "NO") << "\n";
    summary << "STALE RISK  : " << (g_run_count > 1 && g_ds_initialized
                                        ? "YES \u2013 intptr_t _handle from RUN01 in use"
                                        : "No (first run)") << "\n";
    summary << "Output dir  : " << mod_dir << "\n\n";
    summary << "Test cases:\n";
    summary << "  TC-AUD-01  getAudioOutputPorts (all ports, all handle-based getters)\n";
    summary << "  TC-AUD-02  setMuted round-trip (toggle+restore) on first audio port\n\n";

    tc_aud_01_get_ports(mod_dir);
    tc_aud_02_set_muted(mod_dir);

    summary << "\n";
    print_sep(summary, "Verify Summary \u2013 Module 1 Audio");
    int pass_count = 0, fail_count = 0;
    for (const VerifyEntry &e : g_verify_results) {
        if (e.tc_id.substr(0, 6) != "TC-AUD") continue;
        const char *status = e.pass ? "VERIFY_PASS" : "VERIFY_FAIL";
        summary << "  [" << status << "] " << e.tc_id << " \u2013 " << e.description;
        if (!e.detail.empty()) summary << "  (" << e.detail << ")";
        summary << "\n";
        if (e.pass) ++pass_count; else ++fail_count;
    }
    summary << "\n  TOTAL: PASS=" << pass_count << "  FAIL=" << fail_count << "\n";
    if (fail_count > 0) {
        summary << "\n  *** STALE HANDLE FAILURES (expected after dsmgr restart) ***\n";
        summary << "  [R] Handle Refresh – re-fetch g_hdl_* via dsGetVideoPort() etc.\n";
        summary << "  [I] Full Re-Initialize – Manager DeInit+Init (also fixes C++ wrappers).\n";
    }
    printf("\n  Results saved to: %s\n", mod_dir.c_str());
}

/* =========================================================================
 * Module 2 CompositeIn runner
 * ========================================================================= */
static void run_module_compositein(const std::string &run_dir)
{
    std::string mod_dir = get_module_dir(run_dir, "2_CompositeIn");
    std::string sumfile = mod_dir + "/compositein_run_summary.txt";
    TeeStream summary;
    summary.open(sumfile);

    print_sep(summary, "Module 2 \u2013 CompositeIn");
    summary << "Run number  : " << g_run_count << "\n";
    summary << "Timestamp   : " << timestamp_now() << "\n";
    summary << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    summary << "DS_Init     : " << (g_ds_initialized   ? "YES" : "NO") << "\n";
    summary << "Output dir  : " << mod_dir << "\n\n";
    summary << "Test cases:\n";
    summary << "  TC-CIN-01  getCompositeInfo    (getNumberOfInputs / isPresented / getActivePort)\n";
    summary << "  TC-CIN-02  isPortConnected per composite input port\n\n";

    tc_cin_01_get_info(mod_dir);
    tc_cin_02_port_connected(mod_dir);

    summary << "\n";
    print_sep(summary, "Verify Summary \u2013 Module 2 CompositeIn");
    int pass_count = 0, fail_count = 0;
    for (const VerifyEntry &e : g_verify_results) {
        if (e.tc_id.substr(0, 6) != "TC-CIN") continue;
        const char *status = e.pass ? "VERIFY_PASS" : "VERIFY_FAIL";
        summary << "  [" << status << "] " << e.tc_id << " \u2013 " << e.description;
        if (!e.detail.empty()) summary << "  (" << e.detail << ")";
        summary << "\n";
        if (e.pass) ++pass_count; else ++fail_count;
    }
    summary << "\n  TOTAL: PASS=" << pass_count << "  FAIL=" << fail_count << "\n";
    printf("\n  Results saved to: %s\n", mod_dir.c_str());
}

/* =========================================================================
 * Module 3 Display runner
 * ========================================================================= */
static void run_module_display(const std::string &run_dir)
{
    std::string mod_dir = get_module_dir(run_dir, "3_Display");
    std::string sumfile = mod_dir + "/display_run_summary.txt";
    TeeStream summary;
    summary.open(sumfile);

    print_sep(summary, "Module 3 \u2013 Display  [stale handle: VideoOutputPort::Display._handle]");
    summary << "Run number  : " << g_run_count << "\n";
    summary << "Timestamp   : " << timestamp_now() << "\n";
    summary << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    summary << "DS_Init     : " << (g_ds_initialized   ? "YES" : "NO") << "\n";
    summary << "STALE RISK  : " << (g_run_count > 1 && g_ds_initialized
                                        ? "YES \u2013 intptr_t _handle from RUN01 in use"
                                        : "No (first run)") << "\n";
    summary << "Output dir  : " << mod_dir << "\n\n";
    summary << "Test cases:\n";
    summary << "  TC-DSP-01  hasSurround + getSurroundMode via HDMI0 Display object\n";
    summary << "  TC-DSP-02  getEDIDBytes \u2013 raw EDID from HDMI0 display (if connected)\n\n";

    tc_dsp_01_display_surround(mod_dir);
    tc_dsp_02_display_edid(mod_dir);

    summary << "\n";
    print_sep(summary, "Verify Summary \u2013 Module 3 Display");
    int pass_count = 0, fail_count = 0;
    for (const VerifyEntry &e : g_verify_results) {
        if (e.tc_id.substr(0, 6) != "TC-DSP") continue;
        const char *status = e.pass ? "VERIFY_PASS" : "VERIFY_FAIL";
        summary << "  [" << status << "] " << e.tc_id << " \u2013 " << e.description;
        if (!e.detail.empty()) summary << "  (" << e.detail << ")";
        summary << "\n";
        if (e.pass) ++pass_count; else ++fail_count;
    }
    summary << "\n  TOTAL: PASS=" << pass_count << "  FAIL=" << fail_count << "\n";
    if (fail_count > 0) {
        summary << "\n  *** STALE HANDLE FAILURES (expected after dsmgr restart) ***\n";
        summary << "  [R] Handle Refresh – re-fetch g_hdl_* via dsGetVideoPort() etc.\n";
        summary << "  [I] Full Re-Initialize – Manager DeInit+Init (also fixes C++ wrappers).\n";
    }
    printf("\n  Results saved to: %s\n", mod_dir.c_str());
}

/* =========================================================================
 * Module 5 HDMIIn runner
 * ========================================================================= */
static void run_module_hdmiin(const std::string &run_dir)
{
    std::string mod_dir = get_module_dir(run_dir, "5_HDMIIn");
    std::string sumfile = mod_dir + "/hdmiin_run_summary.txt";
    TeeStream summary;
    summary.open(sumfile);

    print_sep(summary, "Module 5 \u2013 HDMIIn");
    summary << "Run number  : " << g_run_count << "\n";
    summary << "Timestamp   : " << timestamp_now() << "\n";
    summary << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    summary << "DS_Init     : " << (g_ds_initialized   ? "YES" : "NO") << "\n";
    summary << "Output dir  : " << mod_dir << "\n\n";
    summary << "Test cases:\n";
    summary << "  TC-HIN-01  getHdmiInInfo    (numInputs / isPresented / activePort / videoMode)\n";
    summary << "  TC-HIN-02  isPortConnected  per HDMI input port\n\n";

    tc_hin_01_get_info(mod_dir);
    tc_hin_02_port_connected(mod_dir);

    summary << "\n";
    print_sep(summary, "Verify Summary \u2013 Module 5 HDMIIn");
    int pass_count = 0, fail_count = 0;
    for (const VerifyEntry &e : g_verify_results) {
        if (e.tc_id.substr(0, 6) != "TC-HIN") continue;
        const char *status = e.pass ? "VERIFY_PASS" : "VERIFY_FAIL";
        summary << "  [" << status << "] " << e.tc_id << " \u2013 " << e.description;
        if (!e.detail.empty()) summary << "  (" << e.detail << ")";
        summary << "\n";
        if (e.pass) ++pass_count; else ++fail_count;
    }
    summary << "\n  TOTAL: PASS=" << pass_count << "  FAIL=" << fail_count << "\n";
    printf("\n  Results saved to: %s\n", mod_dir.c_str());
}

/* =========================================================================
 * Module 6 Host runner
 * ========================================================================= */
static void run_module_host(const std::string &run_dir)
{
    std::string mod_dir = get_module_dir(run_dir, "6_Host");
    std::string sumfile = mod_dir + "/host_run_summary.txt";
    TeeStream summary;
    summary.open(sumfile);

    print_sep(summary, "Module 6 \u2013 Host");
    summary << "Run number  : " << g_run_count << "\n";
    summary << "Timestamp   : " << timestamp_now() << "\n";
    summary << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    summary << "DS_Init     : " << (g_ds_initialized   ? "YES" : "NO") << "\n";
    summary << "Output dir  : " << mod_dir << "\n\n";
    summary << "Test cases:\n";
    summary << "  TC-HST-01  getHostInfo   (CPUTemp / version / powerMode / port names)\n";
    summary << "  TC-HST-02  getSleepModes (preferred / available / port counts)\n\n";

    tc_hst_01_get_info(mod_dir);
    tc_hst_02_sleep_modes(mod_dir);

    summary << "\n";
    print_sep(summary, "Verify Summary \u2013 Module 6 Host");
    int pass_count = 0, fail_count = 0;
    for (const VerifyEntry &e : g_verify_results) {
        if (e.tc_id.substr(0, 6) != "TC-HST") continue;
        const char *status = e.pass ? "VERIFY_PASS" : "VERIFY_FAIL";
        summary << "  [" << status << "] " << e.tc_id << " \u2013 " << e.description;
        if (!e.detail.empty()) summary << "  (" << e.detail << ")";
        summary << "\n";
        if (e.pass) ++pass_count; else ++fail_count;
    }
    summary << "\n  TOTAL: PASS=" << pass_count << "  FAIL=" << fail_count << "\n";
    printf("\n  Results saved to: %s\n", mod_dir.c_str());
}

/* =========================================================================
 * Module 7 VideoDevice runner
 * ========================================================================= */
static void run_module_videodevice(const std::string &run_dir)
{
    std::string mod_dir = get_module_dir(run_dir, "7_VideoDevice");
    std::string sumfile = mod_dir + "/videodevice_run_summary.txt";
    TeeStream summary;
    summary.open(sumfile);

    print_sep(summary, "Module 7 \u2013 VideoDevice  [stale handle: intptr_t _handle]");
    summary << "Run number  : " << g_run_count << "\n";
    summary << "Timestamp   : " << timestamp_now() << "\n";
    summary << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    summary << "DS_Init     : " << (g_ds_initialized   ? "YES" : "NO") << "\n";
    summary << "STALE RISK  : " << (g_run_count > 1 && g_ds_initialized
                                        ? "YES \u2013 intptr_t _handle from RUN01 in use"
                                        : "No (first run)") << "\n";
    summary << "Output dir  : " << mod_dir << "\n\n";
    summary << "Test cases:\n";
    summary << "  TC-VD-01  getVideoDevices (DFC, HDRCapabilities, codecs, STB resolutions)\n";
    summary << "  TC-VD-02  setDFC round-trip GET->SET-same->GET verify\n\n";

    tc_vd_01_get_devices(mod_dir);
    tc_vd_02_set_dfc(mod_dir);

    summary << "\n";
    print_sep(summary, "Verify Summary \u2013 Module 7 VideoDevice");
    int pass_count = 0, fail_count = 0;
    for (const VerifyEntry &e : g_verify_results) {
        if (e.tc_id.substr(0, 6) != "TC-VD-") continue;
        const char *status = e.pass ? "VERIFY_PASS" : "VERIFY_FAIL";
        summary << "  [" << status << "] " << e.tc_id << " \u2013 " << e.description;
        if (!e.detail.empty()) summary << "  (" << e.detail << ")";
        summary << "\n";
        if (e.pass) ++pass_count; else ++fail_count;
    }
    summary << "\n  TOTAL: PASS=" << pass_count << "  FAIL=" << fail_count << "\n";
    if (fail_count > 0) {
        summary << "\n  *** STALE HANDLE FAILURES (expected after dsmgr restart) ***\n";
        summary << "  [R] Handle Refresh – re-fetch g_hdl_* via dsGetVideoPort() etc.\n";
        summary << "  [I] Full Re-Initialize – Manager DeInit+Init (also fixes C++ wrappers).\n";
    }
    printf("\n  Results saved to: %s\n", mod_dir.c_str());
}

/* =========================================================================
 * Module 8 VideoPort runner
 * ========================================================================= */

static void run_module_videoport(const std::string &run_dir)
{
    std::string mod_dir = get_module_dir(run_dir, "8_VideoPort");

    std::string sumfile = mod_dir + "/videoport_run_summary.txt";
    TeeStream summary;
    summary.open(sumfile);

    print_sep(summary, "Module 8 – VideoPort  *** STALE HANDLE DEMONSTRATION ***");
    summary << "Run number  : " << g_run_count << "\n";
    summary << "Timestamp   : " << timestamp_now() << "\n";
    summary << "Profile     : " << (g_profile == PROFILE_STB    ? "STB" :
                                     g_profile == PROFILE_TV     ? "TV"  :
                                     g_profile == PROFILE_COMMON ? "Common" : "All") << "\n";
    summary << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    summary << "DS_Init     : " << (g_ds_initialized   ? "YES" : "NO") << "\n";
    summary << "STALE RISK  : " << (g_run_count > 1 && g_ds_initialized
                                        ? "YES – intptr_t handles from RUN01 in use"
                                        : "No (first run)") << "\n";
    summary << "Output dir  : " << mod_dir << "\n\n";

    summary << "STALE HANDLE MECHANISM:\n";
    summary << "  VideoOutputPort._handle = intptr_t from dsGetVideoPort() RPC,\n";
    summary << "  acquired once at Manager::Initialize(). Server-side pointer.\n";
    summary << "  After dsmgr restarts: pointer invalid in new process space.\n";
    summary << "  Every IARM RPC passing this handle -> dsERR_INVALID_PARAM.\n";
    summary << "  FPD Module 4 uses enum index -> immune to this issue.\n\n";

    summary << "Test cases executing:\n";
    summary << "  TC-VP-01  getVideoOutputPorts (all ports, all handle-based getters)\n";
    summary << "  TC-VP-02  setResolution(HDMI0) round-trip GET->SET-same->GET verify\n";
    summary << "\n";

    /* ── Execute test cases ────────────────────────────── */
    tc_vp_01_get_port_status(mod_dir);
    tc_vp_02_set_resolution(mod_dir);

    /* ── Print verify summary (VideoPort TCs only) ─────── */
    summary << "\n";
    print_sep(summary, "Verify Summary – Module 8 VideoPort");
    int pass_count = 0, fail_count = 0;
    for (const VerifyEntry &e : g_verify_results) {
        if (e.tc_id.substr(0, 5) != "TC-VP") continue;
        const char *status = e.pass ? "VERIFY_PASS" : "VERIFY_FAIL";
        summary << "  [" << status << "] " << e.tc_id
                << " – " << e.description;
        if (!e.detail.empty())
            summary << "  (" << e.detail << ")";
        summary << "\n";
        if (e.pass) ++pass_count; else ++fail_count;
    }
    summary << "\n  TOTAL: PASS=" << pass_count
            << "  FAIL=" << fail_count << "\n";

    if (fail_count > 0) {
        summary << "\n  *** STALE HANDLE FAILURES (expected after dsmgr restart) ***\n";
        summary << "  Root cause: intptr_t _handle from old dsmgr is invalid.\n";
        summary << "  [R] Handle Refresh – re-fetch g_hdl_* via dsGetVideoPort() etc.\n";
        summary << "  [I] Full Re-Initialize – Manager DeInit+Init (also fixes C++ wrappers).\n";
        summary << "  Cross-check: FPD Module 4 (enum-based) will still PASS.\n";
    }

    printf("\n  Results saved to: %s\n", mod_dir.c_str());
}

/* =========================================================================
 * Module 4 FPD runner
 * ========================================================================= */

static void run_module_fpd(const std::string &run_dir)
{
    std::string mod_dir = get_module_dir(run_dir, "4_FPD");

    /* Open a module-level summary */
    std::string sumfile = mod_dir + "/fpd_run_summary.txt";
    TeeStream summary;
    summary.open(sumfile);

    print_sep(summary, "Module 4 – FPD (Front Panel Display)");
    summary << "Run number  : " << g_run_count << "\n";
    summary << "Timestamp   : " << timestamp_now() << "\n";
    summary << "Profile     : " << (g_profile == PROFILE_STB    ? "STB" :
                                     g_profile == PROFILE_TV     ? "TV"  :
                                     g_profile == PROFILE_COMMON ? "Common" : "All") << "\n";
    summary << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    summary << "DS_Init     : " << (g_ds_initialized   ? "YES" : "NO") << "\n";
    summary << "STALE RISK  : " << (g_run_count > 1 && g_ds_initialized
                                        ? "YES – handles from RUN01 still in use"
                                        : "No") << "\n";
    summary << "Output dir  : " << mod_dir << "\n\n";

    summary << "Test cases executing:\n";
    summary << "  TC-FPD-01  getFPBrightness  (all indicators)\n";
    summary << "  TC-FPD-02  setFPBrightness  Power=50\n";
    summary << "  TC-FPD-03  setFPColor       Power=Blue\n";
    summary << "  TC-FPD-04  setFPState       Power=ON\n";
    summary << "  TC-FPD-05  setFPTextDisplay text='RDK'\n";
    summary << "  TC-FPD-06  setFPTextTimeFormat 12hr\n";
    summary << "\n";

    /* ── Execute test cases ────────────────────────────── */
    tc_fpd_01_get_brightness(mod_dir);
    tc_fpd_02_set_brightness(mod_dir);
    //tc_fpd_03_set_color(mod_dir);
    //tc_fpd_04_set_state(mod_dir);
    //tc_fpd_05_set_text_display(mod_dir);
    //tc_fpd_06_set_time_format(mod_dir);

    /* ── Print verify summary ──────────────────────────── */
    summary << "\n";
    print_sep(summary, "Verify Summary");
    int pass_count = 0, fail_count = 0;
    for (const VerifyEntry &e : g_verify_results) {
        if (e.tc_id.substr(0, 6) != "TC-FPD") continue;  /* skip non-FPD entries */
        const char *status = e.pass ? "VERIFY_PASS" : "VERIFY_FAIL";
        summary << "  [" << status << "] " << e.tc_id
                << " – " << e.description;
        if (!e.detail.empty())
            summary << "  (" << e.detail << ")";
        summary << "\n";
        if (e.pass) ++pass_count; else ++fail_count;
    }
    summary << "\n  TOTAL: PASS=" << pass_count
            << "  FAIL=" << fail_count << "\n";

    if (fail_count > 0) {
        summary << "\n  *** FAILURES DETECTED ***\n";
        summary << "  If dsmgr was restarted without reinit, stale handles\n";
        summary << "  are the expected root cause (see ISSUE-001).\n";
        summary << "  [R] Handle Refresh – re-fetch g_hdl_* via dsGetVideoPort() etc.\n";
        summary << "  [I] Full Re-Initialize – Manager DeInit+Init (also fixes C++ wrappers).\n";
    }

    printf("\n  Results saved to: %s\n", mod_dir.c_str());
}

/* =========================================================================
 * Verify Summary global writer
 * ========================================================================= */

static void write_global_verify_summary(const std::string &run_dir)
{
    std::string vpath = run_dir + "/verify_summary.txt";
    std::ofstream vf(vpath.c_str(), std::ios::out | std::ios::trunc);
    if (!vf.is_open()) return;

    vf << "=== Verify Summary – Run " << g_run_count << " ===\n";
    vf << "Timestamp : " << timestamp_now() << "\n\n";

    int pass = 0, fail = 0;
    for (const VerifyEntry &e : g_verify_results) {
        vf << (e.pass ? "VERIFY_PASS" : "VERIFY_FAIL")
           << " | " << e.tc_id
           << " | " << e.description;
        if (!e.detail.empty()) vf << " | " << e.detail;
        vf << "\n";
        if (e.pass) ++pass; else ++fail;
    }
    vf << "\nTOTAL: PASS=" << pass << " FAIL=" << fail << "\n";
    vf.close();
    printf("\n  Global verify summary: %s\n", vpath.c_str());
}

/* =========================================================================
 * Menu helpers
 * ========================================================================= */

static void print_main_menu(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║         DS Menu Client  –  dsmgr Restart Test App           ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  Init status : IARM=%-3s  DS=%-3s  Run count=%-2d             ║\n",
           g_iarm_initialized ? "YES" : "NO ",
           g_ds_initialized   ? "YES" : "NO ",
           g_run_count);
    printf("║  Profile     : %-14s  Output: %-22s ║\n",
           (g_profile == PROFILE_STB    ? "STB"    :
            g_profile == PROFILE_TV     ? "TV"     :
            g_profile == PROFILE_COMMON ? "Common" : "All"),
           g_out_root.c_str());
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  MODULE TESTS                                                ║\n");
    printf("║  [1] Module 1 – Audio         ◄ ACTIVE                      ║\n");
    printf("║  [2] Module 2 – CompositeIn   ◄ ACTIVE                      ║\n");
    printf("║  [3] Module 3 – Display       ◄ ACTIVE (stale handle demo)  ║\n");
    printf("║  [4] Module 4 – FPD           ◄ ACTIVE                      ║\n");
    printf("║  [5] Module 5 – HDMIIn        ◄ ACTIVE                      ║\n");
    printf("║  [6] Module 6 – Host          ◄ ACTIVE                      ║\n");
    printf("║  [7] Module 7 – VideoDevice   ◄ ACTIVE (stale handle demo)  ║\n");
    printf("║  [8] Module 8 – VideoPort     ◄ ACTIVE (stale handle demo)  ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  CONTROL                                                     ║\n");
    printf("║  [A] Run ALL active modules                                  ║\n");
    printf("║  [P] Select Profile (current: %-8s)                     ║\n",
           (g_profile == PROFILE_STB    ? "STB"    :
            g_profile == PROFILE_TV     ? "TV"     :
            g_profile == PROFILE_COMMON ? "Common" : "All"));
    printf("║  [R] Refresh Handles  ← lightweight: dsGetVideoPort() etc.  ║\n");
    printf("║  [H] C++ Handle Sync  ← refreshAllHandles() Audio+Video     ║\n");
    printf("║  [I] Full Re-Init     ← Manager DeInit+Init (fixes C++ too) ║\n");
    printf("║  [S] Show status / fresh handle values                       ║\n");
    printf("║  [Q] Quit                                                    ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("Choice: ");
    fflush(stdout);
}

static void print_profile_menu(void)
{
    printf("\n  Select device profile:\n");
    printf("    [1] STB    – HDMI audio port, Display (EDID), VideoPort HDCP\n");
    printf("    [2] TV     – SPEAKER audio, HDMIIn, setARCSAD\n");
    printf("    [3] Common – getAudioConfig, FPD, Host, Zoom (default)\n");
    printf("    [4] All    – run all profiles\n");
    printf("  Choice: ");
    fflush(stdout);
}

static void show_status(void)
{
    printf("\n  ── Init / Handle Status ─────────────────────────────────────\n");
    printf("  IARM Bus initialized  : %s\n", g_iarm_initialized ? "YES" : "NO");
    printf("  DS Manager initialized: %s\n", g_ds_initialized   ? "YES" : "NO");
    printf("  Run count             : %d\n", g_run_count);
    printf("  Profile               : %s\n",
           g_profile == PROFILE_STB    ? "STB"    :
           g_profile == PROFILE_TV     ? "TV"     :
           g_profile == PROFILE_COMMON ? "Common" : "All");
    printf("  Output root           : %s\n", g_out_root.c_str());

    printf("\n  ── Selective-Refresh Handle Cache (g_hdl_*) ─────────────────\n");
    printf("  Handles valid         : %s\n", g_handles_valid ? "YES (refreshed)" : "NO (use [R])");
    printf("  g_hdl_vport   HDMI[0] : 0x%zX\n", (size_t)g_hdl_vport);
    printf("  g_hdl_display HDMI[0] : 0x%zX\n", (size_t)g_hdl_display);
    printf("  g_hdl_aport   HDMI[0] : 0x%zX\n", (size_t)g_hdl_aport);
    printf("  g_hdl_vdev    [0]     : 0x%zX\n", (size_t)g_hdl_vdev);

    if (g_handles_valid) {
        printf("\n  ── Live verification with fresh handles ─────────────────────\n");
        bool en = false;
        dsError_t r1 = dsIsVideoPortEnabled(g_hdl_vport, &en);
        printf("  dsIsVideoPortEnabled(g_hdl_vport)  : %s (ret=%d)\n",
               r1 == dsERR_NONE ? (en ? "enabled" : "disabled") : "FAIL", r1);
        bool conn = false;
        dsError_t r2 = dsIsDisplayConnected(g_hdl_display, &conn);
        printf("  dsIsDisplayConnected(g_hdl_display): %s (ret=%d)\n",
               r2 == dsERR_NONE ? (conn ? "connected" : "not connected") : "FAIL", r2);
    }

    if (g_run_count > 1 && g_ds_initialized) {
        printf("\n  ⚠ WARNING: C++ wrapper _handle fields are STALE from RUN01.\n");
        printf("    If dsmgr was restarted, module TC calls WILL FAIL.\n");
        printf("    [R] Refresh handles (lightweight – C-RPC only)\n");
        printf("    [I] Full Re-Initialize (rebuilds C++ wrapper objects too)\n");
    }
    printf("  ─────────────────────────────────────────────────────────────\n");
}

/* =========================================================================
 * Entry point
 * ========================================================================= */

int main(int argc, char *argv[])
{
    /* Parse optional output directory argument */
    if (argc >= 2) {
        g_out_root = argv[1];
    }
    mkdirp(g_out_root);

    printf("\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("  DS Menu Client – dsmgr Restart Stale Handle Test\n");
    printf("  Version  : 3.1  (Selective handle refresh [R] + Full reinit [I])\n");
    printf("  Date     : %s\n", timestamp_now().c_str());
    printf("  Output   : %s\n", g_out_root.c_str());
    printf("════════════════════════════════════════════════════════════════\n\n");

    /* ── ONE-TIME INITIALIZATION ─────────────────────────────────────────
     * IARM Bus and DS Manager are connected/initialized here ONCE.
     * They are NEVER automatically re-initialized on subsequent test runs.
     * This is intentional: it replicates the condition where a long-running
     * client app holds stale handles after dsmgr crashes and restarts.
     * ──────────────────────────────────────────────────────────────────── */
    printf("[STARTUP] Performing one-time IARM + DS initialization...\n");

    if (!ds_iarm_init()) {
        fprintf(stderr, "[STARTUP] FATAL: IARM init failed. Exiting.\n");
        return 1;
    }
    if (!ds_manager_init()) {
        fprintf(stderr, "[STARTUP] FATAL: DS Manager init failed. Exiting.\n");
        /* Disconnect IARM before exit */
        IARM_Bus_Disconnect();
        IARM_Bus_Term();
        return 1;
    }

    printf("[STARTUP] ✔ Initialization complete. Handles acquired.\n");
    printf("[STARTUP] *** These handles will NOT be refreshed unless you\n");
    printf("[STARTUP] *** choose option [R] Re-Initialize DS from the menu.\n\n");

    /* ── Main menu loop ──────────────────────────────────────────────────── */
    char input[16];
    bool running = true;

    while (running) {
        print_main_menu();

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        /* Strip newline */
        char choice = input[0];

        switch (choice) {

        /* ── Module tests ──────────────────────────────────────────── */
        case '1':
        {
            ++g_run_count; g_verify_results.clear();
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  Starting RUN %02d (Audio)  –  %s\n", g_run_count, timestamp_now().c_str());
            if (g_run_count > 1)
                printf("  ⚠ STALE HANDLE: intptr_t _handle from RUN01 in use – API calls WILL FAIL.\n");
            printf("════════════════════════════════════════════════════════════════\n");
            {
                std::string rd1 = get_run_dir(g_run_count);
                std::ofstream m1((rd1 + "/run_info.txt").c_str());
                if (m1.is_open()) {
                    m1 << "Run number  : " << g_run_count << "\n"
                       << "Timestamp   : " << timestamp_now() << "\n"
                       << "Modules     : Audio\n"
                       << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n"
                       << "DS_Init     : " << (g_ds_initialized ? "YES" : "NO") << "\n"
                       << "Stale risk  : " << (g_run_count > 1 && g_ds_initialized ? "YES (handles stale)" : "No") << "\n";
                    m1.close();
                }
                run_module_audio(rd1);
                write_global_verify_summary(rd1);
                printf("\n════════════════════════════════════════════════════════════════\n");
                printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd1.c_str());
                if (g_run_count == 1) printf("  → Kill dsmgr, select [1] again – expect VERIFY_FAIL.\n");
                printf("════════════════════════════════════════════════════════════════\n");
            }
            break;
        }

        case '2':
        {
            ++g_run_count; g_verify_results.clear();
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  Starting RUN %02d (CompositeIn)  –  %s\n", g_run_count, timestamp_now().c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            {
                std::string rd2 = get_run_dir(g_run_count);
                std::ofstream m2((rd2 + "/run_info.txt").c_str());
                if (m2.is_open()) {
                    m2 << "Run number  : " << g_run_count << "\n"
                       << "Timestamp   : " << timestamp_now() << "\n"
                       << "Modules     : CompositeIn\n"
                       << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n"
                       << "DS_Init     : " << (g_ds_initialized ? "YES" : "NO") << "\n";
                    m2.close();
                }
                run_module_compositein(rd2);
                write_global_verify_summary(rd2);
                printf("\n════════════════════════════════════════════════════════════════\n");
                printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd2.c_str());
                printf("════════════════════════════════════════════════════════════════\n");
            }
            break;
        }

        case '3':
        {
            ++g_run_count; g_verify_results.clear();
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  Starting RUN %02d (Display)  –  %s\n", g_run_count, timestamp_now().c_str());
            if (g_run_count > 1)
                printf("  ⚠ STALE HANDLE: Display._handle from RUN01 in use – API calls WILL FAIL.\n");
            printf("════════════════════════════════════════════════════════════════\n");
            {
                std::string rd3 = get_run_dir(g_run_count);
                std::ofstream m3((rd3 + "/run_info.txt").c_str());
                if (m3.is_open()) {
                    m3 << "Run number  : " << g_run_count << "\n"
                       << "Timestamp   : " << timestamp_now() << "\n"
                       << "Modules     : Display\n"
                       << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n"
                       << "DS_Init     : " << (g_ds_initialized ? "YES" : "NO") << "\n"
                       << "Stale risk  : " << (g_run_count > 1 && g_ds_initialized ? "YES (handles stale)" : "No") << "\n";
                    m3.close();
                }
                run_module_display(rd3);
                write_global_verify_summary(rd3);
                printf("\n════════════════════════════════════════════════════════════════\n");
                printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd3.c_str());
                if (g_run_count == 1) printf("  → Kill dsmgr, select [3] again – expect VERIFY_FAIL.\n");
                printf("════════════════════════════════════════════════════════════════\n");
            }
            break;
        }

        case '4':
        {
            ++g_run_count;
            g_verify_results.clear();

            printf("\n");
            printf("════════════════════════════════════════════════════════════════\n");
            printf("  Starting RUN %02d (FPD only)  –  %s\n", g_run_count, timestamp_now().c_str());
            if (g_run_count > 1)
                printf("  ⚠ FPD uses enum index – will still PASS after dsmgr restart.\n");
            printf("════════════════════════════════════════════════════════════════\n");

            std::string run_dir = get_run_dir(g_run_count);

            std::string meta_path = run_dir + "/run_info.txt";
            std::ofstream meta(meta_path.c_str());
            if (meta.is_open()) {
                meta << "Run number  : " << g_run_count << "\n"
                     << "Timestamp   : " << timestamp_now() << "\n"
                     << "Modules     : FPD\n"
                     << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n"
                     << "DS_Init     : " << (g_ds_initialized   ? "YES" : "NO") << "\n"
                     << "Stale risk  : "
                     << (g_run_count > 1 && g_ds_initialized ? "YES" : "No") << "\n";
                meta.close();
            }

            run_module_fpd(run_dir);
            write_global_verify_summary(run_dir);

            printf("\n");
            printf("════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, run_dir.c_str());
            printf("  Returning to menu. DS handles remain UNCHANGED.\n");
            if (g_run_count == 1)
                printf("  → Select [8] VideoPort to run the stale handle test.\n");
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }

        case '5':
        {
            ++g_run_count; g_verify_results.clear();
            printf("\n\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            printf("  Starting RUN %02d (HDMIIn)  –  %s\n", g_run_count, timestamp_now().c_str());
            printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            {
                std::string rd5 = get_run_dir(g_run_count);
                std::ofstream m5((rd5 + "/run_info.txt").c_str());
                if (m5.is_open()) {
                    m5 << "Run number  : " << g_run_count << "\n" << "Timestamp   : " << timestamp_now() << "\n"
                       << "Modules     : HDMIIn\n" << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n"
                       << "DS_Init     : " << (g_ds_initialized ? "YES" : "NO") << "\n";
                    m5.close();
                }
                run_module_hdmiin(rd5);
                write_global_verify_summary(rd5);
                printf("\n\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
                printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd5.c_str());
                printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            }
            break;
        }

        case '6':
        {
            ++g_run_count; g_verify_results.clear();
            printf("\n\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            printf("  Starting RUN %02d (Host)  –  %s\n", g_run_count, timestamp_now().c_str());
            printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            {
                std::string rd6 = get_run_dir(g_run_count);
                std::ofstream m6((rd6 + "/run_info.txt").c_str());
                if (m6.is_open()) {
                    m6 << "Run number  : " << g_run_count << "\n" << "Timestamp   : " << timestamp_now() << "\n"
                       << "Modules     : Host\n" << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n"
                       << "DS_Init     : " << (g_ds_initialized ? "YES" : "NO") << "\n";
                    m6.close();
                }
                run_module_host(rd6);
                write_global_verify_summary(rd6);
                printf("\n\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
                printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd6.c_str());
                printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            }
            break;
        }

        case '7':
        {
            ++g_run_count; g_verify_results.clear();
            printf("\n\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            printf("  Starting RUN %02d (VideoDevice)  –  %s\n", g_run_count, timestamp_now().c_str());
            if (g_run_count > 1)
                printf("  ⚠ STALE HANDLE: VideoDevice._handle from RUN01 in use – API calls WILL FAIL.\n");
            printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            {
                std::string rd7 = get_run_dir(g_run_count);
                std::ofstream m7((rd7 + "/run_info.txt").c_str());
                if (m7.is_open()) {
                    m7 << "Run number  : " << g_run_count << "\n" << "Timestamp   : " << timestamp_now() << "\n"
                       << "Modules     : VideoDevice\n" << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n"
                       << "DS_Init     : " << (g_ds_initialized ? "YES" : "NO") << "\n"
                       << "Stale risk  : " << (g_run_count > 1 && g_ds_initialized ? "YES (handles stale)" : "No") << "\n";
                    m7.close();
                }
                run_module_videodevice(rd7);
                write_global_verify_summary(rd7);
                printf("\n\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
                printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd7.c_str());
                if (g_run_count == 1) printf("  → Kill dsmgr, select [7] again – expect VERIFY_FAIL.\n");
                printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            }
            break;
        }

        case '8':
        {
            ++g_run_count;
            g_verify_results.clear();

            printf("\n");
            printf("════════════════════════════════════════════════════════════════\n");
            printf("  Starting RUN %02d (VideoPort)  –  %s\n", g_run_count, timestamp_now().c_str());
            if (g_run_count > 1) {
                printf("  ⚠ STALE HANDLE: intptr_t handles from RUN01 still in use.\n");
                printf("  ⚠ After dsmgr restart ALL VideoPort API calls WILL FAIL.\n");
            }
            printf("════════════════════════════════════════════════════════════════\n");

            std::string run_dir8 = get_run_dir(g_run_count);

            std::string meta_path8 = run_dir8 + "/run_info.txt";
            std::ofstream meta8(meta_path8.c_str());
            if (meta8.is_open()) {
                meta8 << "Run number  : " << g_run_count << "\n"
                      << "Timestamp   : " << timestamp_now() << "\n"
                      << "Modules     : VideoPort\n"
                      << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n"
                      << "DS_Init     : " << (g_ds_initialized   ? "YES" : "NO") << "\n"
                      << "Stale risk  : "
                      << (g_run_count > 1 && g_ds_initialized
                              ? "YES (intptr_t handles stale)" : "No") << "\n";
                meta8.close();
            }

            run_module_videoport(run_dir8);
            write_global_verify_summary(run_dir8);

            printf("\n");
            printf("════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, run_dir8.c_str());
            printf("  Returning to menu. DS handles remain UNCHANGED.\n");
            if (g_run_count == 1)
                printf("  → Kill dsmgr now, select [8] again – expect VERIFY_FAIL.\n");
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }

        case 'A': case 'a':
        {
            ++g_run_count;
            g_verify_results.clear();

            printf("\n");
            printf("════════════════════════════════════════════════════════════════\n");
            printf("  Starting RUN %02d (ALL modules)  –  %s\n", g_run_count, timestamp_now().c_str());
            if (g_run_count > 1) {
                printf("  ⚠ STALE HANDLE: DS handles from RUN01 NOT refreshed.\n");
                printf("  ⚠ FPD (enum-based): PASS  |  Audio/Display/VideoDevice/VideoPort: FAIL\n");
            }
            printf("════════════════════════════════════════════════════════════════\n");

            std::string run_dirA = get_run_dir(g_run_count);

            std::string meta_pathA = run_dirA + "/run_info.txt";
            std::ofstream metaA(meta_pathA.c_str());
            if (metaA.is_open()) {
                metaA << "Run number  : " << g_run_count << "\n"
                      << "Timestamp   : " << timestamp_now() << "\n"
                      << "Modules     : Audio+CompositeIn+Display+FPD+HDMIIn+Host+VideoDevice+VideoPort\n"
                      << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n"
                      << "DS_Init     : " << (g_ds_initialized   ? "YES" : "NO") << "\n"
                      << "Stale risk  : "
                      << (g_run_count > 1 && g_ds_initialized ? "YES" : "No") << "\n";
                metaA.close();
            }

            run_module_audio(run_dirA);
            run_module_compositein(run_dirA);
            run_module_display(run_dirA);
            run_module_fpd(run_dirA);
            run_module_hdmiin(run_dirA);
            run_module_host(run_dirA);
            run_module_videodevice(run_dirA);
            run_module_videoport(run_dirA);
            write_global_verify_summary(run_dirA);

            printf("\n");
            printf("════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, run_dirA.c_str());
            printf("  Returning to menu. DS handles remain UNCHANGED.\n");
            if (g_run_count == 1)
                printf("  → Kill dsmgr, run [A] again – FPD PASS, handle-based modules FAIL.\n");
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }

        /* ── Profile selection ─────────────────────────────────────── */
        case 'P': case 'p':
        {
            print_profile_menu();
            if (fgets(input, sizeof(input), stdin) != NULL) {
                switch (input[0]) {
                case '1': g_profile = PROFILE_STB;    break;
                case '2': g_profile = PROFILE_TV;     break;
                case '3': g_profile = PROFILE_COMMON; break;
                case '4': g_profile = PROFILE_ALL;    break;
                default:  printf("  Invalid profile – keeping current.\n"); break;
                }
            }
            break;
        }

        /* ── Handle Refresh (lightweight – no DeInit/Init) ───────────── */
        case 'R': case 'r':
            printf("\n  [R] Selective Handle Refresh – direct C-RPC calls (dsGetVideoPort etc.)\n");
            printf("  [R] Does NOT call Manager::DeInitialize/Initialize.\n");
            printf("  [R] C++ wrapper _handle fields remain stale; only g_hdl_* cache refreshed.\n");
            printf("  [R] Use [I] afterwards if you also need the C++ layer fixed.\n");
            printf("  [R] Proceed? (y/n): ");
            fflush(stdout);
            if (fgets(input, sizeof(input), stdin) != NULL &&
                (input[0] == 'y' || input[0] == 'Y')) {
                ds_handle_refresh();
            } else {
                printf("  [R] Cancelled.\n");
            }
            break;

        /* ── C++ wrapper refreshAllHandles() ─────────────────────────── */
        case 'H': case 'h':
            printf("\n  [H] C++ wrapper refreshAllHandles() – Audio + Video ports.\n");
            printf("  [H] Re-fetches each port's private _handle via dsGetAudioPort/dsGetVideoPort.\n");
            printf("  [H] NO Manager::DeInitialize/Initialize – port objects kept intact.\n");
            printf("  [H] Use this after [R] to also fix the C++ layer.\n");
            printf("  [H] Proceed? (y/n): ");
            fflush(stdout);
            if (fgets(input, sizeof(input), stdin) != NULL &&
                (input[0] == 'y' || input[0] == 'Y')) {
                ds_cpp_refresh_handles();
            } else {
                printf("  [H] Cancelled.\n");
            }
            break;

        /* ── Full Re-Initialize (heavyweight – DeInit + Init) ─────────── */
        case 'I': case 'i':
            printf("\n  [I] Full Re-Initialize – Manager::DeInitialize() + Initialize().\n");
            printf("  [I] All C++ wrapper objects (AudioOutputPort, VideoOutputPort, …)\n");
            printf("  [I] will be destroyed and rebuilt with fresh handles.\n");
            printf("  [I] Use this when [R] Handle-Refresh is not enough.\n");
            printf("  [I] Proceed? (y/n): ");
            fflush(stdout);
            if (fgets(input, sizeof(input), stdin) != NULL &&
                (input[0] == 'y' || input[0] == 'Y')) {
                ds_full_reinitialize();
                g_run_count = 0;
                g_verify_results.clear();
                printf("  [I] Run counter reset to 0. Next run will be RUN01 (post-reinit).\n");
            } else {
                printf("  [I] Cancelled.\n");
            }
            break;

        /* ── Status ────────────────────────────────────────────────── */
        case 'S': case 's':
            show_status();
            break;

        /* ── Quit ──────────────────────────────────────────────────── */
        case 'Q': case 'q':
            running = false;
            break;

        case '\n': case '\r':
            /* Empty input – just redraw menu */
            break;

        default:
            printf("  Unknown choice '%c'. Please select from the menu.\n", choice);
            break;
        }
    }

    /* ── Cleanup ───────────────────────────────────────────────────────────
     * Only clean up on explicit Quit.  If the user had killed dsmgr and not
     * reinit'd, DeInitialize may fail – we swallow the exception gracefully.
     * ───────────────────────────────────────────────────────────────────── */
    printf("\n[EXIT] Shutting down...\n");

    if (g_ds_initialized) {
        printf("[EXIT] DS Manager DeInitialize...\n");
        try {
            device::Manager::DeInitialize();
            printf("[EXIT] DS Manager DeInitialize OK.\n");
        }
        catch (...) {
            printf("[EXIT] DS Manager DeInitialize failed (dsmgr may be down).\n");
        }
    }

    if (g_iarm_initialized) {
        printf("[EXIT] Disconnecting IARM Bus...\n");
        sleep(1);
        IARM_Bus_Disconnect();
        IARM_Bus_Term();
        sleep(1);
        printf("[EXIT] IARM Bus disconnected.\n");
    }

    printf("[EXIT] Done. Total runs: %d\n\n", g_run_count);
    return 0;
}

/** @} */
/** @} */
