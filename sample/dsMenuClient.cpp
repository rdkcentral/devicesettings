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
 *     TC-AUD-03  setLevel            – set 50.0 + restore round-trip on SPEAKER0 (TV profile only)
 *     TC-AUD-04  setGain             – set -3.0 dB + restore round-trip on SPEAKER0 (TV profile only)
 *     TC-AUD-05  getLevelAndMute     – getLevel + isMuted read on all ports (stale-handle demo)
 *     TC-AUD-06  stereoMode          – getStereoMode/setStereoMode + getStereoAuto/setStereoAuto round-trip
 *     TC-AUD-07  audioDelay          – getAudioDelay/setAudioDelay round-trip
 *     TC-AUD-08  ms12Getters         – Dolby/IEQ/Bass/Surround/DRC/MISteering/GraphicEQ snapshot (read-only)
 *     TC-AUD-09  ms12Setters         – DolbyVolumeMode/BassEnhancer/DRCMode round-trip
 *     TC-AUD-10  capabilities        – isAudioMSDecode/isAudioMS12Decode/getAudioCaps/getMS12Caps snapshot
 *     TC-AUD-11  atmos               – getSinkDeviceAtmosCapability + setAudioAtmosOutputMode round-trip
 *     TC-AUD-12  arcLeConfig         – getSupportedARCTypes/getHdmiArcPortId/GetLEConfig snapshot
 *     TC-AUD-13  languageMixing      – AssocAudioMixing/FaderControl/PrimaryLanguage/SecondaryLanguage round-trip
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
#include <sys/wait.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <iomanip>

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
#define HDMI_PORT_NAME     "HDMI0"      /* VideoPort used by Module 8 TCs (STB/TV) */
#define INTERNAL_PORT_NAME "Internal0"  /* VideoPort dsVIDEOPORT_TYPE_INTERNAL (TV only) */

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
static pid_t    g_jctl_pid      = -1;    /* journalctl capture child PID, -1=stopped */

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
 * journalctl capture helpers
 *
 *   journalctl_start(run_dir)
 *     Forks a child that runs:
 *       journalctl --output=short-precise -f  > <run_dir>/journalctl.log 2>&1
 *     Returns the child PID on success, -1 on fork failure.
 *     Saves the log path to run_dir/journalctl.log.
 *
 *   journalctl_stop(pid, run_dir)
 *     Sends SIGTERM to the child, waits up to 2s for it to exit,
 *     then prints the path to the captured log.
 *     Safe to call with pid==-1 (no-op).
 * ========================================================================= */
static pid_t journalctl_start(const std::string &run_dir)
{
    std::string logpath = run_dir + "/journalctl.log";

    pid_t pid = fork();
    if (pid < 0) {
        printf("  [JCTL] fork() failed: %s\n", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        /* ── Child ─────────────────────────────────────────────────── */
        /* Redirect stdout + stderr to the log file */
        FILE *f = fopen(logpath.c_str(), "w");
        if (f) {
            int fd = fileno(f);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            fclose(f);
        }
        /* exec journalctl – never returns on success */
        execlp("journalctl", "journalctl", "--output=short-precise", "-f", (char *)NULL);
        /* If exec fails, write error into log and exit */
        fprintf(stderr, "[JCTL] execlp journalctl failed: %s\n", strerror(errno));
        _exit(1);
    }

    /* ── Parent ─────────────────────────────────────────────────────── */
    printf("  [JCTL] journalctl capture started (pid=%d) → %s\n", (int)pid, logpath.c_str());
    /* Brief pause to let journalctl connect to the journal before tests run */
    usleep(200000);
    return pid;
}

static void journalctl_stop(pid_t pid, const std::string &run_dir)
{
    if (pid <= 0) return;

    std::string logpath = run_dir + "/journalctl.log";

    /* Send SIGTERM so journalctl flushes its buffer and exits cleanly */
    kill(pid, SIGTERM);

    /* Wait up to 2 seconds for the child to exit */
    int status = 0;
    for (int i = 0; i < 20; ++i) {
        pid_t r = waitpid(pid, &status, WNOHANG);
        if (r == pid) break;
        usleep(100000);   /* 100 ms */
    }
    /* Force-reap if still alive */
    waitpid(pid, &status, WNOHANG);

    printf("  [JCTL] journalctl capture stopped (pid=%d).\n", (int)pid);
    printf("  [JCTL] Journal log saved to: %s\n", logpath.c_str());
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
 * Audio port selection globals & helper
 *
 *  g_selected_aport_name  – name of the port chosen by the user before any
 *                            audio TC is run, e.g. "SPEAKER0", "HDMI0" …
 *  g_selected_aport_set   – true once the user has made a selection
 *
 *  select_audio_port()
 *    1. Enumerates all AudioOutputPorts returned by the DS Manager.
 *    2. Shows connection status (isConnected / isEnabled) for each port.
 *    3. Prompts the user to choose one by index number.
 *    4. Validates the choice and stores the port name in
 *       g_selected_aport_name.  Returns true on success.
 *
 *  Profile-based port availability:
 *    TV  profile → SPEAKER0, SPDIF0, HEADPHONE0, HDMI_ARC0
 *    STB profile → HDMI0
 *    The enumeration always shows whatever the platform reports; the profile
 *    is printed as context only (no hard filter).
 *
 *  get_selected_aport()
 *    Returns a reference to the currently selected AudioOutputPort.
 *    Throws device::Exception if the name is not found (stale handles or
 *    wrong name — same failure path as any other handle-based call).
 * ========================================================================= */
static std::string g_selected_aport_name;
static bool        g_selected_aport_set = false;

/*
 * Enumerate all audio ports, print their status, ask the user to pick one.
 * Returns true if a valid port was selected.
 */
static bool select_audio_port(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║   Audio Port Selection                                       ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  Profile : %-10s                                        ║\n",
           (g_profile == PROFILE_TV    ? "TV  (SPEAKER0/SPDIF0/HEADPHONE0/HDMI_ARC0)" :
            g_profile == PROFILE_STB   ? "STB (HDMI0)" :
            g_profile == PROFILE_COMMON? "Common" : "All"));
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    /* ── Enumerate ports ──────────────────────────────────────────────── */
    device::List<device::AudioOutputPort> aPorts;
    try {
        aPorts = device::Host::getInstance().getAudioOutputPorts();
    } catch (const device::Exception &e) {
        printf("  ERROR: getAudioOutputPorts() failed: %s\n", e.what());
        printf("  Is DS Manager initialized? Try [I] Full Re-Init first.\n");
        return false;
    }

    if (aPorts.size() == 0) {
        printf("  No audio output ports reported by the platform.\n");
        return false;
    }

    /* ── Print status table ───────────────────────────────────────────── */
    printf("  %-4s  %-16s  %-10s  %-10s  %-10s\n",
           "Idx", "Port Name", "Enabled", "Connected", "Muted");
    printf("  ────  ────────────────  ──────────  ──────────  ──────────\n");

    for (size_t i = 0; i < aPorts.size(); i++) {
        device::AudioOutputPort &p = aPorts.at(i);
        std::string en_str   = "?";
        std::string conn_str = "?";
        std::string mute_str = "?";

        /* isEnabled */
        try {
            en_str = p.isEnabled() ? "YES" : "NO";
        } catch (...) { en_str = "STALE"; }

        /* isConnected */
        try {
            conn_str = p.isConnected() ? "YES" : "NO";
        } catch (...) { conn_str = "STALE"; }

        /* isMuted */
        try {
            mute_str = p.isMuted() ? "MUTED" : "unmuted";
        } catch (...) { mute_str = "STALE"; }

        printf("  [%2zu]  %-16s  %-10s  %-10s  %-10s\n",
               i + 1, p.getName().c_str(),
               en_str.c_str(), conn_str.c_str(), mute_str.c_str());
    }

    printf("\n  Note: 'STALE' means dsmgr was restarted – handles are stale.\n");
    printf("        Use [I] Full Re-Init or [H] Handle Sync first.\n\n");

    /* ── User selection ───────────────────────────────────────────────── */
    if (g_selected_aport_set) {
        printf("  Currently selected port : %s\n", g_selected_aport_name.c_str());
        printf("  Press Enter to keep it, or enter a number to change: ");
    } else {
        printf("  Enter port number (1-%zu): ", aPorts.size());
    }
    fflush(stdout);

    char line[32];
    if (fgets(line, sizeof(line), stdin) == NULL)
        return g_selected_aport_set;   /* EOF – keep existing if set */

    /* If user just pressed Enter and we already have a selection, keep it */
    if ((line[0] == '\n' || line[0] == '\r') && g_selected_aport_set) {
        printf("  Keeping current selection: %s\n\n", g_selected_aport_name.c_str());
        return true;
    }

    int idx = atoi(line);
    if (idx < 1 || idx > (int)aPorts.size()) {
        printf("  Invalid selection '%s'. Port unchanged.\n\n", line);
        return g_selected_aport_set;
    }

    device::AudioOutputPort &chosen = aPorts.at((size_t)(idx - 1));
    g_selected_aport_name = chosen.getName();
    g_selected_aport_set  = true;

    /* ── Confirm connection status for chosen port ───────────────────── */
    printf("\n  ┌─ Selected Port: %s ─────────────────────────────────────\n",
           g_selected_aport_name.c_str());
    try {
        bool en   = chosen.isEnabled();
        bool conn = chosen.isConnected();
        bool mute = chosen.isMuted();
        float lv  = 0.0f;
        try { lv = chosen.getLevel(); } catch (...) {}

        printf("  │  Enabled   : %s\n",   en   ? "YES" : "NO");
        printf("  │  Connected : %s\n",   conn ? "YES" : "NO");
        printf("  │  Muted     : %s\n",   mute ? "YES (muted)" : "no");
        printf("  │  Level     : %.1f\n", lv);

        if (!conn) {
            printf("  │\n");
            printf("  │  ⚠ WARNING: Port reports NOT CONNECTED.\n");
            printf("  │    Audio tests will still run but may produce no output.\n");
            printf("  │    Ensure the physical connection is present.\n");
        }
        if (!en) {
            printf("  │\n");
            printf("  │  ⚠ WARNING: Port is DISABLED.\n");
            printf("  │    Some write tests may fail or have no audible effect.\n");
        }
    } catch (...) {
        printf("  │  Status   : STALE – handles invalid after dsmgr restart.\n");
        printf("  │    ⚠ TC execution will likely FAIL. Use [I] or [H] first.\n");
    }
    printf("  └──────────────────────────────────────────────────────────\n");

    printf("\n  ✔ Port '%s' selected. TCs will now run on this port.\n\n",
           g_selected_aport_name.c_str());
    return true;
}

/*
 * Convenience: look up the currently selected AudioOutputPort by name.
 * Throws device::Exception if not found (stale handle path).
 */
static device::AudioOutputPort & get_selected_aport(void)
{
    if (!g_selected_aport_set || g_selected_aport_name.empty()) {
        throw device::Exception(dsERR_INVALID_PARAM,
                                "No audio port selected – call select_audio_port() first");
    }
    return device::Host::getInstance().getAudioOutputPort(g_selected_aport_name);
}

/* =========================================================================
 * decode_tv_resolutions()
 *
 *   Decode a dsTVResolution_t bitmask (returned by getSupportedTvResolutions)
 *   into a human-readable hex string + list of named resolution names.
 *
 *   Bitmask values from dsAVDTypes.h  dsTVResolution_t enum:
 *     0x000001=480i   0x000002=480p   0x000004=576i   0x000008=576p
 *     0x000010=576p50 0x000020=720p   0x000040=720p50 0x000080=1080i
 *     0x000100=1080p  0x000200=1080p24 0x000400=1080i25 0x000800=1080p25
 *     0x001000=1080p30 0x002000=1080i50 0x004000=1080p50 0x008000=1080p60
 *     0x010000=2160p24 0x020000=2160p25 0x040000=2160p30
 *     0x080000=2160p50 0x100000=2160p60
 *
 *   Example output for 0x1319923:
 *     "0x1319923  [480i 480p 720p 1080p 1080p25 1080p30 1080p60
 *                  2160p24 2160p25 2160p60]  (10 resolutions)"
 * ========================================================================= */
static std::string decode_tv_resolutions(int res)
{
    static const struct { int bit; const char *name; } kTVRes[] = {
        { 0x000001, "480i"    }, { 0x000002, "480p"    },
        { 0x000004, "576i"    }, { 0x000008, "576p"    },
        { 0x000010, "576p50"  }, { 0x000020, "720p"    },
        { 0x000040, "720p50"  }, { 0x000080, "1080i"   },
        { 0x000100, "1080p"   }, { 0x000200, "1080p24" },
        { 0x000400, "1080i25" }, { 0x000800, "1080p25" },
        { 0x001000, "1080p30" }, { 0x002000, "1080i50" },
        { 0x004000, "1080p50" }, { 0x008000, "1080p60" },
        { 0x010000, "2160p24" }, { 0x020000, "2160p25" },
        { 0x040000, "2160p30" }, { 0x080000, "2160p50" },
        { 0x100000, "2160p60" },
    };
    std::ostringstream os;
    os << "0x" << std::hex << res << std::dec << "  [";
    int count = 0;
    for (size_t i = 0; i < sizeof(kTVRes)/sizeof(kTVRes[0]); ++i) {
        if (res & kTVRes[i].bit) {
            if (count > 0) os << " ";
            os << kTVRes[i].name;
            ++count;
        }
    }
    os << "]  (" << count << " resolutions)";
    return os.str();
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
 * TC-FPD-07: FrontPanelIndicator – ALL getter APIs snapshot (READ-ONLY)
 *
 *   Calls every read-only getter on each configured indicator.
 *   Getters covered per indicator:
 *     getBrightness, getBrightnessLevels, getColorMode, getBlink
 *     (getInterval + getIteration), getMaxCycleRate, getColor,
 *     getState, getSupportedColors
 *
 *   Output file: 4_FPD/FPD_all_getters.txt
 * ========================================================================= */
static void tc_fpd_07_all_getters(const std::string &mod_dir)
{
    printf("\n  [TC-FPD-07] FrontPanelIndicator – ALL getter APIs snapshot\n");

    std::string fname = mod_dir + "/FPD_all_getters.txt";
    TeeStream out;
    out.open(fname);

    out << "╔══════════════════════════════════════════════════════════════════╗\n";
    out << "║  TC-FPD-07 : FrontPanelIndicator – ALL Getter APIs Snapshot     ║\n";
    out << "║  Run BEFORE and AFTER dsmgr restart to compare output.          ║\n";
    out << "╚══════════════════════════════════════════════════════════════════╝\n";
    out << "Timestamp   : " << timestamp_now() << "\n";
    out << "Run number  : " << g_run_count << "\n";
    out << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init     : " << (g_ds_initialized   ? "YES (may be stale after restart)" : "NO") << "\n\n";

    bool any_fail = false;
    int  get_pass = 0, get_fail = 0;

    for (int i = 0; FPD_INDICATORS[i] != NULL; ++i) {
        const char *name = FPD_INDICATORS[i];
        out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        out << "  Indicator: " << name << "\n";
        out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

        try {
            device::FrontPanelIndicator &ind =
                device::FrontPanelIndicator::getInstance(name);

            /* getBrightness */
            try { int v = ind.getBrightness();
                  out << "  getBrightness()               : " << v << "\n"; ++get_pass; }
            catch (const device::Exception &e) { out << "  getBrightness()               : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }

            /* getBrightnessLevels */
            try { int levels=0, minL=0, maxL=0;
                  ind.getBrightnessLevels(levels, minL, maxL);
                  out << "  getBrightnessLevels()         : levels=" << levels
                      << "  min=" << minL << "  max=" << maxL << "\n"; ++get_pass; }
            catch (const device::Exception &e) { out << "  getBrightnessLevels()         : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }

            /* getColorMode */
            try { int v = ind.getColorMode();
                  out << "  getColorMode()                : " << v
                      << (v == 0 ? "  (single-color)" : "  (multi-color)") << "\n"; ++get_pass; }
            catch (const device::Exception &e) { out << "  getColorMode()                : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }

            /* getBlink (inline – cached, no RPC) */
            try { device::FrontPanelIndicator::Blink bl = ind.getBlink();
                  out << "  getBlink()                    : interval=" << bl.getInterval()
                      << "  iteration=" << bl.getIteration() << "\n"; ++get_pass; }
            catch (const device::Exception &e) { out << "  getBlink()                    : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }

            /* getMaxCycleRate (inline) */
            try { out << "  getMaxCycleRate()             : " << ind.getMaxCycleRate() << "\n"; ++get_pass; }
            catch (const device::Exception &e) { out << "  getMaxCycleRate()             : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }

            /* getColor */
            try { uint32_t v = ind.getColor();
                  char cbuf[12]; snprintf(cbuf, sizeof(cbuf), "0x%06X", v);
                  out << "  getColor()                    : " << cbuf << "\n"; ++get_pass; }
            catch (const device::Exception &e) { out << "  getColor()                    : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }

            /* getState */
            try { bool v = ind.getState();
                  out << "  getState()                    : " << (v ? "ON" : "OFF") << "\n"; ++get_pass; }
            catch (const device::Exception &e) { out << "  getState()                    : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }

            /* getSupportedColors */
            try { const device::List<device::FrontPanelIndicator::Color> colors = ind.getSupportedColors();
                  out << "  getSupportedColors()          : " << colors.size() << " colors";
                  for (size_t j = 0; j < colors.size(); ++j) out << "  [" << colors.at(j).getName() << "]";
                  out << "\n"; ++get_pass; }
            catch (const device::Exception &e) { out << "  getSupportedColors()          : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }

            out << "\n";
        }
        catch (const device::Exception &e) {
            out << "  getInstance(" << name << ") EXCEPTION: " << e.what() << "\n\n";
            any_fail = true;
        }
        catch (...) {
            out << "  getInstance(" << name << ") EXCEPTION (unknown)\n\n";
            any_fail = true;
        }
    }

    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    out << "  Getters called  : " << (get_pass + get_fail) << "\n";
    out << "  Getters OK      : " << get_pass << "\n";
    out << "  Getters FAILED  : " << get_fail << "\n\n";
    out << (any_fail ? "  RESULT: FAIL – " + std::to_string(get_fail) + " getter(s) threw exceptions\n"
                    : "  RESULT: PASS – all FPD indicator getters returned values OK\n");
    verify_record("TC-FPD-07",
                  "FrontPanelIndicator all-getters snapshot",
                  !any_fail,
                  "getters_ok=" + std::to_string(get_pass) +
                  " getters_fail=" + std::to_string(get_fail));
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * tc_aud_port_all_getters() – Generic ALL-getter snapshot for one audio port
 *
 *   Called by per-port TC wrappers:
 *     TC-AUD-01  SPEAKER0    TC-AUD-14  SPDIF0
 *     TC-AUD-15  HDMI_ARC0   TC-AUD-16  HEADPHONE0
 *
 *   port_name : port name string, e.g. "SPEAKER0"
 *   tc_id     : TC identifier,   e.g. "TC-AUD-01"
 *   Output file: 1_Audio/<port_name>_all_getters.txt
 * ========================================================================= */
static void tc_aud_port_all_getters(const std::string &mod_dir,
                                     const std::string &port_name,
                                     const std::string &tc_id)
{
    printf("\n  [%s] %s – ALL getter APIs snapshot (run before & after restart)\n",
           tc_id.c_str(), port_name.c_str());

    std::string fname = mod_dir + "/" + port_name + "_all_getters.txt";
    TeeStream out;
    out.open(fname);

    out << "╔══════════════════════════════════════════════════════════════════╗\n";
    out << "║  " << tc_id << " : " << port_name << " – ALL Getter APIs Snapshot\n";
    out << "║  Run BEFORE and AFTER dsmgr restart to compare output.          ║\n";
    out << "╚══════════════════════════════════════════════════════════════════╝\n";
    out << "Timestamp   : " << timestamp_now() << "\n";
    out << "Run number  : " << g_run_count << "\n";
    out << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init     : " << (g_ds_initialized   ? "YES (may be stale after restart)" : "NO") << "\n";
    out << "Stale risk  : " << (g_run_count > 1 && g_ds_initialized
                                ? "YES – intptr_t _handle from RUN-01 still in use"
                                : "No (first run or re-initialized)") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  AudioOutputPort._handle = intptr_t assigned at Manager::Initialize().\n";
    out << "  After dsmgr kill+restart, dsmgr allocates NEW server-side pointers.\n";
    out << "  The C++ wrapper still holds the OLD pointer  →  dsERR_INVALID_PARAM\n";
    out << "  →  device::Exception thrown on EVERY handle-based IARM RPC call.\n\n";

    bool any_fail = false;
    int  getter_pass = 0, getter_fail = 0;

    /* ── SECTION 1 : Port inventory ────────────────────────────────────────────── */
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    out << "  SECTION 1 – Port Inventory (all audio output ports)\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    try {
        device::List<device::AudioOutputPort> aPorts =
            device::Host::getInstance().getAudioOutputPorts();
        out << "Total ports : " << aPorts.size() << "\n\n";
        for (size_t i = 0; i < aPorts.size(); i++) {
            device::AudioOutputPort &p = aPorts.at(i);
            out << "  [" << i << "] " << p.getName();
            try {
                bool en = p.isEnabled();
                bool co = p.isConnected();
                out << "  enabled=" << (en ? "Y" : "N")
                    << "  connected=" << (co ? "Y" : "N");
                if (p.getName() == port_name) out << "  ◄ TARGET";
            } catch (...) {
                out << "  *** STALE HANDLE – isEnabled/isConnected threw exception ***";
                any_fail = true;
            }
            out << "\n";
        }
    }
    catch (const device::Exception &e) {
        out << "  EXCEPTION getting port list: " << e.what() << "\n";
        any_fail = true;
    }
    catch (...) {
        out << "  EXCEPTION (unknown) getting port list\n";
        any_fail = true;
    }

    /* ── SECTION 2 : Target port – every getter ───────────────────────────────────── */
    out << "\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    out << "  SECTION 2 – " << port_name << " : ALL Getter APIs (34 getters)\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    try {
        device::AudioOutputPort &sp =
            device::Host::getInstance().getAudioOutputPort(port_name);
        out << "Port handle : " << sp.getName() << "\n\n";

        /* ── Feature: Port Management ───────────────────────────────── */
        out << "── Feature: Port Management ──────────────────────────────────────\n";

        try { bool v = sp.isEnabled();
              out << "  isEnabled()                   : " << (v ? "Yes" : "No") << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  isEnabled()                   : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { bool v = sp.isConnected();
              out << "  isConnected()                 : " << (v ? "Yes" : "No") << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  isConnected()                 : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { bool v = sp.getEnablePersist();
              out << "  getEnablePersist()            : " << (v ? "true" : "false") << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getEnablePersist()            : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        /* ── Feature: Volume Control ────────────────────────────────── */
        out << "\n── Feature: Volume Control ─────────────────────────────────────────\n";

        try { bool v = sp.isMuted();
              out << "  isMuted()                     : " << (v ? "muted" : "unmuted") << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  isMuted()                     : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { float v = sp.getLevel();
              out << "  getLevel()                    : " << v << "  (0.0-100.0)\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getLevel()                    : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { float v = sp.getGain();
              out << "  getGain()                     : " << v << " dB\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getGain()                     : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { float v = sp.getDB();
              out << "  getDB()                       : " << v << " dB\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getDB()                       : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { float v = sp.getMaxDB();
              out << "  getMaxDB()                    : " << v << " dB\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getMaxDB()                    : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { float v = sp.getMinDB();
              out << "  getMinDB()                    : " << v << " dB\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getMinDB()                    : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { float v = sp.getOptimalLevel();
              out << "  getOptimalLevel()             : " << v << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getOptimalLevel()             : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { bool v = sp.isLoopThru();
              out << "  isLoopThru()                  : " << (v ? "yes" : "no") << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  isLoopThru()                  : EXCEPTION – " << e.what() << "\n";
              ++getter_fail; any_fail = true; }

        /* ── Feature: Audio Delay ───────────────────────────────────── */
        out << "\n── Feature: Audio Delay ────────────────────────────────────────────\n";

        try { uint32_t v = 0; sp.getAudioDelay(v);
              out << "  getAudioDelay()               : " << v << " ms\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getAudioDelay()               : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        /* ── Feature: Stereo / Encoding ─────────────────────────────── */
        out << "\n── Feature: Stereo / Encoding ─────────────────────────────────────\n";

        try { const device::AudioEncoding &v = sp.getEncoding();
              out << "  getEncoding()                 : " << v.getName() << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getEncoding()                 : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { const device::AudioStereoMode &v = sp.getStereoMode(false);
              out << "  getStereoMode()               : " << v.getName()
                  << " (id=" << v.getId() << ")\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getStereoMode()               : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { bool v = sp.getStereoAuto();
              out << "  getStereoAuto()               : " << (v ? "auto" : "manual") << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getStereoAuto()               : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        /* ── Feature: MS12 / Dolby Processing ─────────────────────────── */
        out << "\n── Feature: MS12 / Dolby Processing Getters ───────────────────────\n";

        try { bool v = sp.getDolbyVolumeMode();
              out << "  getDolbyVolumeMode()          : " << (v ? "on" : "off") << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getDolbyVolumeMode()          : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { int v = sp.getIntelligentEqualizerMode();
              out << "  getIntelligentEqualizerMode() : " << v << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getIntelligentEqualizerMode() : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { int v = sp.getBassEnhancer();
              out << "  getBassEnhancer()             : " << v << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getBassEnhancer()             : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { bool v = sp.isSurroundDecoderEnabled();
              out << "  isSurroundDecoderEnabled()    : " << (v ? "yes" : "no") << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  isSurroundDecoderEnabled()    : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { int v = sp.getDRCMode();
              out << "  getDRCMode()                  : " << v << "  (0=line, 1=RF)\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getDRCMode()                  : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { bool v = sp.getMISteering();
              out << "  getMISteering()               : " << (v ? "on" : "off") << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getMISteering()               : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { int v = sp.getGraphicEqualizerMode();
              out << "  getGraphicEqualizerMode()     : " << v << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getGraphicEqualizerMode()     : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        /* ── Feature: Capability Queries ────────────────────────────── */
        out << "\n── Feature: Capability Queries ───────────────────────────────────\n";

        try { bool v = sp.isAudioMSDecode();
              out << "  isAudioMSDecode()             : " << (v ? "yes" : "no") << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  isAudioMSDecode()             : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { bool v = sp.isAudioMS12Decode();
              out << "  isAudioMS12Decode()           : " << (v ? "yes" : "no") << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  isAudioMS12Decode()           : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { int caps = 0; sp.getAudioCapabilities(&caps);
              out << "  getAudioCapabilities()        : 0x" << std::hex << caps << std::dec
                  << "  (dsAudioCapabilities_t bitmask)\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getAudioCapabilities()        : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { int caps = 0; sp.getMS12Capabilities(&caps);
              out << "  getMS12Capabilities()         : 0x" << std::hex << caps << std::dec
                  << "  (dsMS12Capabilities_t bitmask)\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getMS12Capabilities()         : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        /* ── Feature: ATMOS ─────────────────────────────────────────────── */
        out << "\n── Feature: ATMOS ────────────────────────────────────────────────────\n";

        try { dsATMOSCapability_t v = dsAUDIO_ATMOS_NOTSUPPORTED;
              sp.getSinkDeviceAtmosCapability(v);
              out << "  getSinkDeviceAtmosCapability(): " << (int)v
                  << "  (0=none, 1=ddpAtmos, 2=trueHDAtmos)\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getSinkDeviceAtmosCapability(): EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        /* ── Feature: ARC / LE Config ───────────────────────────────── */
        out << "\n── Feature: ARC / LE Config ───────────────────────────────────────\n";

        try { int v = 0; sp.getSupportedARCTypes(&v);
              out << "  getSupportedARCTypes()        : 0x" << std::hex << v << std::dec
                  << "  (bit0=ARC, bit1=eARC)\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getSupportedARCTypes()        : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { int v = -1; sp.getHdmiArcPortId(&v);
              out << "  getHdmiArcPortId()            : " << v << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getHdmiArcPortId()            : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { bool v = sp.GetLEConfig();
              out << "  GetLEConfig()                 : " << (v ? "enabled" : "disabled") << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  GetLEConfig()                 : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        /* ── Feature: Language / Mixing ─────────────────────────────── */
        out << "\n── Feature: Language / Mixing ─────────────────────────────────────\n";

        try { bool v = false; sp.getAssociatedAudioMixing(&v);
              out << "  getAssociatedAudioMixing()    : " << (v ? "true" : "false") << "\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getAssociatedAudioMixing()    : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { int v = 0; sp.getFaderControl(&v);
              out << "  getFaderControl()             : " << v
                  << "  (-32=full-main ... 32=full-assoc)\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getFaderControl()             : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { std::string v; sp.getPrimaryLanguage(v);
              out << "  getPrimaryLanguage()          : \"" << v << "\"\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getPrimaryLanguage()          : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

        try { std::string v; sp.getSecondaryLanguage(v);
              out << "  getSecondaryLanguage()        : \"" << v << "\"\n";
              ++getter_pass; }
        catch (const device::Exception &e) {
              out << "  getSecondaryLanguage()        : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++getter_fail; any_fail = true; }

    }
    catch (const device::Exception &e) {
        out << "\nEXCEPTION getting " << port_name << " port object: " << e.what() << "\n";
        out << "  -> " << port_name << " not present on this platform, or DS not initialized.\n";
        any_fail = true;
    }
    catch (...) {
        out << "\nEXCEPTION (unknown) getting " << port_name << " port object\n";
        any_fail = true;
    }

    /* ── SECTION 3 : Result summary ────────────────────────────────────────────── */
    out << "\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    out << "  SECTION 3 – Result Summary\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    out << "  Getters called  : " << (getter_pass + getter_fail) << "\n";
    out << "  Getters OK      : " << getter_pass << "\n";
    out << "  Getters FAILED  : " << getter_fail << "\n\n";
    if (!any_fail) {
        out << "  RESULT: PASS – all " << port_name << " getters returned values OK\n";
        out << "  (This is the expected BEFORE-restart baseline)\n";
    } else {
        out << "  RESULT: FAIL – " << getter_fail << " getter(s) threw exceptions\n";
        out << "  (This is the expected AFTER-restart stale-handle result)\n";
        out << "  -> Use [H] C++ Handle Sync or [I] Full Re-Init to recover\n";
    }
    verify_record(tc_id,
                  port_name + " all-getters snapshot",
                  !any_fail,
                  "getters_ok=" + std::to_string(getter_pass) +
                  " getters_fail=" + std::to_string(getter_fail));
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-AUD-01: SPEAKER0 all getters snapshot
 * TC-AUD-14: SPDIF0   all getters snapshot
 * TC-AUD-15: HDMI_ARC0 all getters snapshot
 * TC-AUD-16: HEADPHONE0 all getters snapshot
 *   All delegate to tc_aud_port_all_getters() above.
 *   Output files: 1_Audio/<PORT>_all_getters.txt
 * ========================================================================= */
static void tc_aud_01_get_ports(const std::string &mod_dir)
{
    tc_aud_port_all_getters(mod_dir, "SPEAKER0",   "TC-AUD-01");
}

static void tc_aud_14_spdif0_all_getters(const std::string &mod_dir)
{
    tc_aud_port_all_getters(mod_dir, "SPDIF0",     "TC-AUD-14");
}

static void tc_aud_15_hdmi_arc0_all_getters(const std::string &mod_dir)
{
    tc_aud_port_all_getters(mod_dir, "HDMI_ARC0",  "TC-AUD-15");
}

static void tc_aud_16_headphone0_all_getters(const std::string &mod_dir)
{
    tc_aud_port_all_getters(mod_dir, "HEADPHONE0", "TC-AUD-16");
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
 * TC-AUD-03: setLevel – set+restore round-trip on SPEAKER0
 *
 *   GET level → SET to 50.0 → wait 2s → verify → restore → verify.
 *   setLevel() passes intptr_t _handle → will throw after dsmgr restart.
 *
 *   Output file: 1_Audio/setLevel_SPEAKER0_roundtrip.txt
 * ========================================================================= */
static void tc_aud_03_set_level(const std::string &mod_dir)
{
    std::string port_name = g_selected_aport_set ? g_selected_aport_name : "(none)";
    printf("\n  [TC-AUD-03] setLevel – set+restore round-trip on %s\n",
           port_name.c_str());

    std::string safe_name = port_name;
    for (char &c : safe_name) if (c == '/' || c == ' ') c = '_';
    std::string fname = mod_dir + "/setLevel_" + safe_name + "_roundtrip.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-AUD-03: setLevel round-trip (set+restore) ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "Port      : " << port_name << "\n";
    out << "Profile   : " << (g_profile == PROFILE_TV ? "TV" :
                              g_profile == PROFILE_STB ? "STB" :
                              g_profile == PROFILE_COMMON ? "Common" : "All") << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  setLevel() / getLevel() pass intptr_t _handle via IARM RPC.\n";
    out << "  Both calls will throw device::Exception after dsmgr restart.\n\n";

    if (!g_selected_aport_set) {
        out << "ERROR: No audio port selected. Run [P] port selection first.\n";
        out << "RESULT: SKIP\n";
        verify_record("TC-AUD-03", "setLevel round-trip", false, "no port selected");
        out << "Saved to: " << fname << "\n";
        return;
    }

    try {
        device::AudioOutputPort &aPort = get_selected_aport();
        out << "Port (confirmed) : " << aPort.getName() << "\n";
        out << "Test level       : 50.0  (valid range 0.0 – 100.0)\n\n";

        /* ── Connection check ──────────────────────────────────────── */
        bool connected = false;
        try { connected = aPort.isConnected(); } catch (...) {}
        out << "isConnected()    : " << (connected ? "YES" : "NO (not connected – test continues)") << "\n\n";

        /* Step 1: GET current level */
        float orig_level = aPort.getLevel();
        out << "Step 1 – GET current level       : " << orig_level << "\n";

        /* Step 2: SET to test level */
        float test_level = 50.0f;
        out << "Step 2 – SET level to            : " << test_level << "\n";
        aPort.setLevel(test_level);
        out << "         Waiting 2s...\n";
        sleep(2);

        /* Step 3: GET to verify */
        float after_set = aPort.getLevel();
        out << "Step 3 – GET after set           : " << after_set << "\n";
        float diff_set = after_set - test_level;
        bool pass_set = (diff_set >= -1.0f && diff_set <= 1.0f);
        out << "         Verify (tol ±1.0)       : " << (pass_set ? "PASS" : "FAIL (mismatch)") << "\n";

        /* Step 4: SET restore */
        out << "Step 4 – SET restore to          : " << orig_level << "\n";
        aPort.setLevel(orig_level);
        out << "         Waiting 2s...\n";
        sleep(2);

        /* Step 5: GET verify restore */
        float after_restore = aPort.getLevel();
        out << "Step 5 – GET after restore       : " << after_restore << "\n";
        float diff_restore = after_restore - orig_level;
        bool pass_restore = (diff_restore >= -1.0f && diff_restore <= 1.0f);
        out << "         Verify restore (tol ±1.0): " << (pass_restore ? "PASS" : "FAIL (mismatch)") << "\n";

        bool pass = pass_set && pass_restore;
        out << "\nRESULT: " << (pass ? "PASS" : "FAIL") << "\n";

        char detail[128];
        snprintf(detail, sizeof(detail),
                 "orig=%.1f after_set=%.1f after_restore=%.1f",
                 orig_level, after_set, after_restore);
        verify_record("TC-AUD-03", "setLevel(" + port_name + ",50.0) set+restore", pass, detail);
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL – stale handle or port not present\n";
        out << "  *** Expected after dsmgr restart (stale intptr_t handle) ***\n";
        verify_record("TC-AUD-03", "setLevel(" + port_name + ",50.0) set+restore", false,
                      std::string("STALE HANDLE: ") + e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\nRESULT: FAIL\n";
        verify_record("TC-AUD-03", "setLevel(" + port_name + ",50.0) set+restore", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-AUD-04: setGain – set+restore round-trip on SPEAKER0
 *
 *   GET gain → SET to -3.0 dB → wait 2s → verify → restore → verify.
 *   setGain() passes intptr_t _handle → will throw after dsmgr restart.
 *
 *   Output file: 1_Audio/setGain_SPEAKER0_roundtrip.txt
 * ========================================================================= */
static void tc_aud_04_set_gain(const std::string &mod_dir)
{
    std::string port_name = g_selected_aport_set ? g_selected_aport_name : "(none)";
    printf("\n  [TC-AUD-04] setGain – set+restore round-trip on %s\n",
           port_name.c_str());

    std::string safe_name = port_name;
    for (char &c : safe_name) if (c == '/' || c == ' ') c = '_';
    std::string fname = mod_dir + "/setGain_" + safe_name + "_roundtrip.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-AUD-04: setGain round-trip (set+restore) ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "Port      : " << port_name << "\n";
    out << "Profile   : " << (g_profile == PROFILE_TV ? "TV" :
                              g_profile == PROFILE_STB ? "STB" :
                              g_profile == PROFILE_COMMON ? "Common" : "All") << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  setGain() / getGain() pass intptr_t _handle via IARM RPC.\n";
    out << "  Both calls will throw device::Exception after dsmgr restart.\n\n";
    out << "NOTE: Gain (dB DAP offset) is only meaningfully supported on\n";
    out << "  SPEAKER and SPDIF ports. On HDMI/HDMI_ARC/HEADPHONE the HAL\n";
    out << "  may return dsERR_OPERATION_NOT_SUPPORTED or silently ignore.\n\n";

    if (!g_selected_aport_set) {
        out << "ERROR: No audio port selected. Run [P] port selection first.\n";
        out << "RESULT: SKIP\n";
        verify_record("TC-AUD-04", "setGain round-trip", false, "no port selected");
        out << "Saved to: " << fname << "\n";
        return;
    }

    try {
        device::AudioOutputPort &aPort = get_selected_aport();
        out << "Port (confirmed) : " << aPort.getName() << "\n";
        out << "Test gain        : -3.0 dB\n\n";

        /* ── Connection check ──────────────────────────────────────── */
        bool connected = false;
        try { connected = aPort.isConnected(); } catch (...) {}
        out << "isConnected()    : " << (connected ? "YES" : "NO (not connected – test continues)") << "\n\n";

        /* Step 1: GET current gain */
        float orig_gain = aPort.getGain();
        out << "Step 1 – GET current gain        : " << orig_gain << " dB\n";

        /* Step 2: SET to test gain */
        float test_gain = -3.0f;
        out << "Step 2 – SET gain to             : " << test_gain << " dB\n";
        aPort.setGain(test_gain);
        out << "         Waiting 2s...\n";
        sleep(2);

        /* Step 3: GET to verify */
        float after_set = aPort.getGain();
        out << "Step 3 – GET after set           : " << after_set << " dB\n";
        float diff_set = after_set - test_gain;
        bool pass_set = (diff_set >= -0.5f && diff_set <= 0.5f);
        out << "         Verify (tol ±0.5 dB)    : " << (pass_set ? "PASS" : "FAIL (mismatch)") << "\n";

        /* Step 4: SET restore */
        out << "Step 4 – SET restore to          : " << orig_gain << " dB\n";
        aPort.setGain(orig_gain);
        out << "         Waiting 2s...\n";
        sleep(2);

        /* Step 5: GET verify restore */
        float after_restore = aPort.getGain();
        out << "Step 5 – GET after restore       : " << after_restore << " dB\n";
        float diff_restore = after_restore - orig_gain;
        bool pass_restore = (diff_restore >= -0.5f && diff_restore <= 0.5f);
        out << "         Verify restore (tol ±0.5): " << (pass_restore ? "PASS" : "FAIL (mismatch)") << "\n";

        bool pass = pass_set && pass_restore;
        out << "\nRESULT: " << (pass ? "PASS" : "FAIL") << "\n";

        char detail[128];
        snprintf(detail, sizeof(detail),
                 "orig=%.2fdB after_set=%.2fdB after_restore=%.2fdB",
                 orig_gain, after_set, after_restore);
        verify_record("TC-AUD-04", "setGain(" + port_name + ",-3.0dB) set+restore", pass, detail);
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL – stale handle or port not present\n";
        out << "  *** Expected after dsmgr restart (stale intptr_t handle) ***\n";
        verify_record("TC-AUD-04", "setGain(" + port_name + ",-3.0dB) set+restore", false,
                      std::string("STALE HANDLE: ") + e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\nRESULT: FAIL\n";
        verify_record("TC-AUD-04", "setGain(" + port_name + ",-3.0dB) set+restore", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-AUD-05: getLevelAndMute – read level + mute state on all ports
 *
 *   Iterates all audio output ports and reads getLevel() + isMuted().
 *   Read-only test; demonstrates stale-handle failure after dsmgr restart.
 *
 *   Output file: 1_Audio/getLevelAndMute_allports.txt
 * ========================================================================= */
static void tc_aud_05_get_level_mute_all(const std::string &mod_dir)
{
    printf("\n  [TC-AUD-05] getLevelAndMute – read level+mute on all audio output ports\n");

    std::string fname = mod_dir + "/getLevelAndMute_allports.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-AUD-05: getLevel + isMuted – all audio output ports ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  getLevel() / isMuted() pass intptr_t _handle via IARM RPC.\n";
    out << "  Both calls will throw device::Exception after dsmgr restart.\n\n";

    bool any_fail = false;
    try {
        device::List<device::AudioOutputPort> aPorts =
            device::Host::getInstance().getAudioOutputPorts();
        out << "Port count : " << aPorts.size() << "\n\n";

        for (size_t i = 0; i < aPorts.size(); i++) {
            device::AudioOutputPort &aPort = aPorts.at(i);
            out << "── Port [" << aPort.getName() << "] ────────────────────────────────\n";

            /* getLevel – passes intptr_t _handle */
            try {
                float lv = aPort.getLevel();
                out << "  getLevel()  : " << lv << "\n";
            } catch (const device::Exception &e) {
                out << "  getLevel()  : EXCEPTION *** stale handle *** – " << e.what() << "\n";
                any_fail = true;
            } catch (...) {
                out << "  getLevel()  : EXCEPTION (unknown) *** stale handle ***\n";
                any_fail = true;
            }

            /* isMuted – passes intptr_t _handle */
            try {
                bool mu = aPort.isMuted();
                out << "  isMuted()   : " << (mu ? "muted" : "unmuted") << "\n";
            } catch (const device::Exception &e) {
                out << "  isMuted()   : EXCEPTION *** stale handle *** – " << e.what() << "\n";
                any_fail = true;
            } catch (...) {
                out << "  isMuted()   : EXCEPTION (unknown) *** stale handle ***\n";
                any_fail = true;
            }

            out << "\n";
        }

        bool pass = !any_fail;
        out << "RESULT: " << (pass ? "PASS" : "FAIL – handle-based calls threw exceptions") << "\n";
        verify_record("TC-AUD-05", "getLevel+isMuted all-port read", pass,
                      pass ? "all handle calls OK"
                           : "STALE HANDLE: getLevel/isMuted FAILED after dsmgr restart");
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-AUD-05", "getLevel+isMuted all-port read", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-AUD-05", "getLevel+isMuted all-port read", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-AUD-06: Stereo Mode – getStereoMode/setStereoMode + getStereoAuto/setStereoAuto
 *   Feature: Stereo / Output Mode
 *   round-trip: read → set alternate → verify → restore
 *   Run BEFORE dsmgr restart → PASS; AFTER restart without reinit → FAIL (stale handle)
 *   Output file: 1_Audio/stereoMode_roundtrip.txt
 * ========================================================================= */
static void tc_aud_06_stereo_mode(const std::string &mod_dir)
{
    printf("\n  [TC-AUD-06] Stereo Mode – getStereoMode/setStereoMode + getStereoAuto/setStereoAuto\n");

    std::string fname = mod_dir + "/stereoMode_roundtrip.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-AUD-06: Stereo Mode round-trip ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  getStereoMode/setStereoMode pass intptr_t _handle via IARM RPC.\n";
    out << "  Calls will throw device::Exception after dsmgr restart without reinit.\n\n";

    bool all_pass = true;
    try {
        device::List<device::AudioOutputPort> aPorts =
            device::Host::getInstance().getAudioOutputPorts();
        if (aPorts.size() == 0) {
            out << "No audio output ports.\nRESULT: SKIP\n";
            verify_record("TC-AUD-06", "stereoMode round-trip", false, "no ports");
            out << "Saved to: " << fname << "\n";
            return;
        }
        device::AudioOutputPort &aPort = aPorts.at(0);
        out << "Port: " << aPort.getName() << "\n\n";

        /* ── getStereoMode / setStereoMode round-trip ──────────────────── */
        const device::AudioStereoMode &smBefore = aPort.getStereoMode(false);
        int modeBefore = smBefore.getId();
        out << "getStereoMode() before : " << smBefore.getName()
            << " (id=" << modeBefore << ")\n";

        /* toggle between STEREO(2) and SURROUND(3) */
        int newMode = (modeBefore == dsAUDIO_STEREO_STEREO) ?
                       dsAUDIO_STEREO_SURROUND : dsAUDIO_STEREO_STEREO;
        out << "setStereoMode()  set   : id=" << newMode << "\n";
        aPort.setStereoMode(newMode, false);   /* false = do not persist */

        const device::AudioStereoMode &smAfter = aPort.getStereoMode(false);
        int modeAfter = smAfter.getId();
        out << "getStereoMode() after  : " << smAfter.getName()
            << " (id=" << modeAfter << ")\n";
        bool modeOk = (modeAfter == newMode);
        out << "  readback: " << (modeOk ? "PASS" : "FAIL") << "\n\n";
        if (!modeOk) all_pass = false;

        aPort.setStereoMode(modeBefore, false);
        const device::AudioStereoMode &smRest = aPort.getStereoMode(false);
        out << "setStereoMode() restore: id=" << modeBefore << "\n";
        out << "getStereoMode() final  : id=" << smRest.getId() << "\n";
        bool modeRestOk = (smRest.getId() == modeBefore);
        out << "  restore:  " << (modeRestOk ? "PASS" : "FAIL") << "\n\n";
        if (!modeRestOk) all_pass = false;

        /* ── getStereoAuto / setStereoAuto round-trip ──────────────────── */
        bool autoBefore = aPort.getStereoAuto();
        out << "getStereoAuto() before : " << (autoBefore ? "true" : "false") << "\n";
        aPort.setStereoAuto(!autoBefore, false);
        bool autoAfter = aPort.getStereoAuto();
        out << "setStereoAuto()  set   : " << (!autoBefore ? "true" : "false") << "\n";
        out << "getStereoAuto() after  : " << (autoAfter ? "true" : "false") << "\n";
        bool autoOk = (autoAfter == !autoBefore);
        out << "  readback: " << (autoOk ? "PASS" : "FAIL") << "\n";
        if (!autoOk) all_pass = false;
        aPort.setStereoAuto(autoBefore, false);
        out << "getStereoAuto() restore: " << (autoBefore ? "true" : "false") << "\n\n";

        out << "RESULT: " << (all_pass ? "PASS" : "FAIL (readback mismatch)") << "\n";
        verify_record("TC-AUD-06", "stereoMode+stereoAuto round-trip", all_pass,
                      "modeBefore=" + std::to_string(modeBefore) +
                      " modeAfter=" + std::to_string(modeAfter));
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL – stale handle or dsmgr not running\n";
        out << "  *** Expected after dsmgr restart (stale intptr_t handle) ***\n";
        verify_record("TC-AUD-06", "stereoMode+stereoAuto round-trip", false,
                      std::string("STALE HANDLE: ") + e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\nRESULT: FAIL\n";
        verify_record("TC-AUD-06", "stereoMode+stereoAuto round-trip", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-AUD-07: Audio Delay – getAudioDelay/setAudioDelay round-trip
 *   Feature: Audio Delay
 *   round-trip: read current delay → set 100ms → verify → restore
 *   Run BEFORE dsmgr restart → PASS; AFTER restart without reinit → FAIL (stale handle)
 *   Output file: 1_Audio/audioDelay_roundtrip.txt
 * ========================================================================= */
static void tc_aud_07_audio_delay(const std::string &mod_dir)
{
    printf("\n  [TC-AUD-07] Audio Delay – getAudioDelay/setAudioDelay round-trip\n");

    std::string fname = mod_dir + "/audioDelay_roundtrip.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-AUD-07: Audio Delay round-trip ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  getAudioDelay/setAudioDelay pass intptr_t _handle via IARM RPC.\n";
    out << "  Both throw device::Exception after dsmgr restart without reinit.\n\n";

    bool all_pass = true;
    try {
        device::List<device::AudioOutputPort> aPorts =
            device::Host::getInstance().getAudioOutputPorts();
        if (aPorts.size() == 0) {
            out << "No audio output ports.\nRESULT: SKIP\n";
            verify_record("TC-AUD-07", "audioDelay round-trip", false, "no ports");
            out << "Saved to: " << fname << "\n";
            return;
        }
        device::AudioOutputPort &aPort = aPorts.at(0);
        out << "Port: " << aPort.getName() << "\n\n";

        /* read current delay (returns bool: true=success) */
        uint32_t delayBefore = 0;
        bool readOk = aPort.getAudioDelay(delayBefore);
        out << "getAudioDelay() before : " << delayBefore
            << " ms  (ok=" << (readOk ? "T" : "F") << ")\n";

        /* set to test value */
        uint32_t testDelay = (delayBefore == 100) ? 150 : 100;
        out << "setAudioDelay()  set   : " << testDelay << " ms\n";
        aPort.setAudioDelay(testDelay);

        uint32_t delayAfter = 0;
        aPort.getAudioDelay(delayAfter);
        out << "getAudioDelay() after  : " << delayAfter << " ms\n";
        bool setOk = (delayAfter == testDelay);
        out << "  readback: " << (setOk ? "PASS" : "FAIL") << "\n\n";
        if (!setOk) all_pass = false;

        /* restore */
        aPort.setAudioDelay(delayBefore);
        uint32_t delayFinal = 0;
        aPort.getAudioDelay(delayFinal);
        out << "setAudioDelay() restore: " << delayBefore << " ms\n";
        out << "getAudioDelay() final  : " << delayFinal << " ms\n";
        bool restOk = (delayFinal == delayBefore);
        out << "  restore:  " << (restOk ? "PASS" : "FAIL") << "\n\n";
        if (!restOk) all_pass = false;

        out << "RESULT: " << (all_pass ? "PASS" : "FAIL (readback mismatch)") << "\n";
        verify_record("TC-AUD-07", "audioDelay round-trip", all_pass,
                      "delayBefore=" + std::to_string(delayBefore) +
                      " testDelay=" + std::to_string(testDelay) +
                      " delayAfter=" + std::to_string(delayAfter));
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL – stale handle or dsmgr not running\n";
        out << "  *** Expected after dsmgr restart (stale intptr_t handle) ***\n";
        verify_record("TC-AUD-07", "audioDelay round-trip", false,
                      std::string("STALE HANDLE: ") + e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\nRESULT: FAIL\n";
        verify_record("TC-AUD-07", "audioDelay round-trip", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-AUD-08: MS12 Dolby Processing – getters snapshot (read-only)
 *   Feature: MS12 / Dolby Processing
 *   Reads: getDolbyVolumeMode, getIntelligentEqualizerMode, getBassEnhancer,
 *          isSurroundDecoderEnabled, getDRCMode, getMISteering,
 *          getGraphicEqualizerMode — on every audio port
 *   Run BEFORE dsmgr restart → PASS; AFTER restart without reinit → FAIL (stale handle)
 *   Output file: 1_Audio/ms12_getters_snapshot.txt
 * ========================================================================= */
static void tc_aud_08_ms12_getters(const std::string &mod_dir)
{
    printf("\n  [TC-AUD-08] MS12 getters – Dolby/IEQ/Bass/Surround/DRC/MISteering/GraphicEQ snapshot\n");

    std::string fname = mod_dir + "/ms12_getters_snapshot.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-AUD-08: MS12 Dolby Processing – getters snapshot ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  All MS12 getters pass intptr_t _handle via IARM RPC.\n";
    out << "  All will throw device::Exception after dsmgr restart without reinit.\n\n";

    bool any_fail = false;
    try {
        device::List<device::AudioOutputPort> aPorts =
            device::Host::getInstance().getAudioOutputPorts();
        out << "Port count : " << aPorts.size() << "\n\n";

        for (size_t i = 0; i < aPorts.size(); i++) {
            device::AudioOutputPort &aPort = aPorts.at(i);
            out << "── Port [" << aPort.getName() << "] ──────────────────────────────\n";

            try {
                bool dvm = aPort.getDolbyVolumeMode();
                out << "  getDolbyVolumeMode()          : " << (dvm ? "on" : "off") << "\n";
            } catch (...) {
                out << "  getDolbyVolumeMode()          : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }
            try {
                int ieq = aPort.getIntelligentEqualizerMode();
                out << "  getIntelligentEqualizerMode() : " << ieq << "\n";
            } catch (...) {
                out << "  getIntelligentEqualizerMode() : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }
            try {
                int bass = aPort.getBassEnhancer();
                out << "  getBassEnhancer()             : " << bass << "\n";
            } catch (...) {
                out << "  getBassEnhancer()             : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }
            try {
                bool surr = aPort.isSurroundDecoderEnabled();
                out << "  isSurroundDecoderEnabled()    : " << (surr ? "yes" : "no") << "\n";
            } catch (...) {
                out << "  isSurroundDecoderEnabled()    : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }
            try {
                int drc = aPort.getDRCMode();
                out << "  getDRCMode()                  : " << drc << "\n";
            } catch (...) {
                out << "  getDRCMode()                  : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }
            try {
                bool mis = aPort.getMISteering();
                out << "  getMISteering()               : " << (mis ? "on" : "off") << "\n";
            } catch (...) {
                out << "  getMISteering()               : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }
            try {
                int geq = aPort.getGraphicEqualizerMode();
                out << "  getGraphicEqualizerMode()     : " << geq << "\n";
            } catch (...) {
                out << "  getGraphicEqualizerMode()     : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }
            out << "\n";
        }
        bool pass = !any_fail;
        out << "RESULT: " << (pass ? "PASS" : "FAIL – stale handle exceptions") << "\n";
        verify_record("TC-AUD-08", "MS12 getters snapshot", pass,
                      pass ? "all MS12 getters OK" : "STALE HANDLE: MS12 getters FAILED");
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-AUD-08", "MS12 getters snapshot", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\nRESULT: FAIL\n";
        verify_record("TC-AUD-08", "MS12 getters snapshot", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-AUD-09: MS12 setters – round-trip (DolbyVolumeMode, BassEnhancer, DRCMode)
 *   Feature: MS12 / Dolby Processing setters
 *   round-trip per setting: read → set alternate value → verify → restore
 *   Run BEFORE dsmgr restart → PASS; AFTER restart without reinit → FAIL (stale handle)
 *   Output file: 1_Audio/ms12_setters_roundtrip.txt
 * ========================================================================= */
static void tc_aud_09_ms12_setters(const std::string &mod_dir)
{
    printf("\n  [TC-AUD-09] MS12 setters – DolbyVolumeMode/BassEnhancer/DRCMode round-trip\n");

    std::string fname = mod_dir + "/ms12_setters_roundtrip.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-AUD-09: MS12 setter round-trip ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  MS12 get/set APIs pass intptr_t _handle – throw after dsmgr restart.\n\n";

    bool all_pass = true;
    try {
        device::List<device::AudioOutputPort> aPorts =
            device::Host::getInstance().getAudioOutputPorts();
        if (aPorts.size() == 0) {
            out << "No audio output ports.\nRESULT: SKIP\n";
            verify_record("TC-AUD-09", "MS12 setters round-trip", false, "no ports");
            out << "Saved to: " << fname << "\n";
            return;
        }
        device::AudioOutputPort &aPort = aPorts.at(0);
        out << "Port: " << aPort.getName() << "\n\n";

        /* ── DolbyVolumeMode toggle round-trip ─────────────────────────── */
        {
            bool dvmB4 = aPort.getDolbyVolumeMode();
            out << "getDolbyVolumeMode() before : " << (dvmB4 ? "on" : "off") << "\n";
            aPort.setDolbyVolumeMode(!dvmB4);
            bool dvmAfter = aPort.getDolbyVolumeMode();
            out << "setDolbyVolumeMode()  set   : " << (!dvmB4 ? "on" : "off") << "\n";
            out << "getDolbyVolumeMode() after  : " << (dvmAfter ? "on" : "off") << "\n";
            bool ok = (dvmAfter == !dvmB4);
            out << "  readback: " << (ok ? "PASS" : "FAIL") << "\n";
            if (!ok) all_pass = false;
            aPort.setDolbyVolumeMode(dvmB4);
            out << "getDolbyVolumeMode() restore: " << (dvmB4 ? "on" : "off") << "\n\n";
        }

        /* ── BassEnhancer round-trip (0–100) ───────────────────────────── */
        {
            int bassB4 = aPort.getBassEnhancer();
            out << "getBassEnhancer() before : " << bassB4 << "\n";
            int newBass = (bassB4 == 0) ? 5 : 0;
            aPort.setBassEnhancer(newBass);
            int bassAfter = aPort.getBassEnhancer();
            out << "setBassEnhancer()  set   : " << newBass << "\n";
            out << "getBassEnhancer() after  : " << bassAfter << "\n";
            bool ok = (bassAfter == newBass);
            out << "  readback: " << (ok ? "PASS" : "FAIL") << "\n";
            if (!ok) all_pass = false;
            aPort.setBassEnhancer(bassB4);
            out << "getBassEnhancer() restore: " << bassB4 << "\n\n";
        }

        /* ── DRCMode round-trip (0=line, 1=RF) ─────────────────────────── */
        {
            int drcB4 = aPort.getDRCMode();
            out << "getDRCMode() before : " << drcB4 << "\n";
            int newDrc = (drcB4 == 0) ? 1 : 0;
            aPort.setDRCMode(newDrc);
            int drcAfter = aPort.getDRCMode();
            out << "setDRCMode()  set   : " << newDrc << "\n";
            out << "getDRCMode() after  : " << drcAfter << "\n";
            bool ok = (drcAfter == newDrc);
            out << "  readback: " << (ok ? "PASS" : "FAIL") << "\n";
            if (!ok) all_pass = false;
            aPort.setDRCMode(drcB4);
            out << "getDRCMode() restore: " << drcB4 << "\n\n";
        }

        out << "RESULT: " << (all_pass ? "PASS" : "FAIL (readback mismatch)") << "\n";
        verify_record("TC-AUD-09", "MS12 setters round-trip", all_pass,
                      all_pass ? "DolbyVol+Bass+DRC all OK"
                               : "mismatch in MS12 setter readback");
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL – stale handle or dsmgr not running\n";
        out << "  *** Expected after dsmgr restart (stale intptr_t handle) ***\n";
        verify_record("TC-AUD-09", "MS12 setters round-trip", false,
                      std::string("STALE HANDLE: ") + e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\nRESULT: FAIL\n";
        verify_record("TC-AUD-09", "MS12 setters round-trip", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-AUD-10: Capabilities – isAudioMSDecode/isAudioMS12Decode/
 *                            getAudioCapabilities/getMS12Capabilities
 *   Feature: MS12 Capability Queries
 *   Read-only: bitmask + decode flags on all ports
 *   Run BEFORE dsmgr restart → PASS; AFTER restart without reinit → FAIL (stale handle)
 *   Output file: 1_Audio/audio_capabilities.txt
 * ========================================================================= */
static void tc_aud_10_capabilities(const std::string &mod_dir)
{
    printf("\n  [TC-AUD-10] Capabilities – isMSDecode/isMS12Decode/getAudioCaps/getMS12Caps\n");

    std::string fname = mod_dir + "/audio_capabilities.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-AUD-10: Audio Capabilities snapshot ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  All capability queries pass intptr_t _handle via IARM RPC.\n";
    out << "  All throw device::Exception after dsmgr restart without reinit.\n\n";

    bool any_fail = false;
    try {
        device::List<device::AudioOutputPort> aPorts =
            device::Host::getInstance().getAudioOutputPorts();
        out << "Port count : " << aPorts.size() << "\n\n";

        for (size_t i = 0; i < aPorts.size(); i++) {
            device::AudioOutputPort &aPort = aPorts.at(i);
            out << "── Port [" << aPort.getName() << "] ──────────────────────────────\n";

            try {
                bool msd = aPort.isAudioMSDecode();
                out << "  isAudioMSDecode()     : " << (msd ? "yes" : "no") << "\n";
            } catch (...) {
                out << "  isAudioMSDecode()     : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }
            try {
                bool ms12 = aPort.isAudioMS12Decode();
                out << "  isAudioMS12Decode()   : " << (ms12 ? "yes" : "no") << "\n";
            } catch (...) {
                out << "  isAudioMS12Decode()   : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }
            try {
                int caps = 0;
                aPort.getAudioCapabilities(&caps);
                out << "  getAudioCapabilities(): 0x" << std::hex << caps << std::dec
                    << "  (dsAudioCapabilities_t bitmask)\n";
            } catch (...) {
                out << "  getAudioCapabilities(): EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }
            try {
                int ms12caps = 0;
                aPort.getMS12Capabilities(&ms12caps);
                out << "  getMS12Capabilities() : 0x" << std::hex << ms12caps << std::dec
                    << "  (dsMS12Capabilities_t bitmask)\n";
            } catch (...) {
                out << "  getMS12Capabilities() : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }
            out << "\n";
        }
        bool pass = !any_fail;
        out << "RESULT: " << (pass ? "PASS" : "FAIL – stale handle exceptions") << "\n";
        verify_record("TC-AUD-10", "audio capabilities snapshot", pass,
                      pass ? "all capability getters OK"
                           : "STALE HANDLE: capability getters FAILED");
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-AUD-10", "audio capabilities snapshot", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\nRESULT: FAIL\n";
        verify_record("TC-AUD-10", "audio capabilities snapshot", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-AUD-11: ATMOS – getSinkDeviceAtmosCapability + setAudioAtmosOutputMode
 *   Feature: ATMOS
 *   round-trip: read atmos capability → toggle output mode → restore
 *   Run BEFORE dsmgr restart → PASS; AFTER restart without reinit → FAIL (stale handle)
 *   Output file: 1_Audio/atmos_roundtrip.txt
 * ========================================================================= */
static void tc_aud_11_atmos(const std::string &mod_dir)
{
    printf("\n  [TC-AUD-11] ATMOS – getSinkDeviceAtmosCapability + setAudioAtmosOutputMode\n");

    std::string fname = mod_dir + "/atmos_roundtrip.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-AUD-11: ATMOS capability + output mode ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  ATMOS APIs pass intptr_t _handle via IARM RPC.\n";
    out << "  Both throw device::Exception after dsmgr restart without reinit.\n\n";

    bool all_pass = true;
    try {
        device::List<device::AudioOutputPort> aPorts =
            device::Host::getInstance().getAudioOutputPorts();
        if (aPorts.size() == 0) {
            out << "No audio output ports.\nRESULT: SKIP\n";
            verify_record("TC-AUD-11", "ATMOS round-trip", false, "no ports");
            out << "Saved to: " << fname << "\n";
            return;
        }
        device::AudioOutputPort &aPort = aPorts.at(0);
        out << "Port: " << aPort.getName() << "\n\n";

        /* ── getSinkDeviceAtmosCapability (read-only) ──────────────────── */
        try {
            dsATMOSCapability_t atmosCap = dsAUDIO_ATMOS_NOTSUPPORTED;
            aPort.getSinkDeviceAtmosCapability(atmosCap);
            out << "getSinkDeviceAtmosCapability() : " << (int)atmosCap
                << "  (0=noSupport,1=ddpAtmos,2=trueHDAtmos)\n\n";
        } catch (const device::Exception &e) {
            out << "getSinkDeviceAtmosCapability() : EXCEPTION – " << e.what()
                << " *** stale handle ***\n\n";
            all_pass = false;
        }

        /* ── setAudioAtmosOutputMode toggle round-trip ─────────────────── */
        out << "setAudioAtmosOutputMode() round-trip:\n";
        try {
            aPort.setAudioAtmosOutputMode(true);
            out << "  setAudioAtmosOutputMode(true)  : OK\n";
            aPort.setAudioAtmosOutputMode(false);
            out << "  setAudioAtmosOutputMode(false) : OK (restored to false)\n";
            out << "  round-trip: PASS\n\n";
        } catch (const device::Exception &e) {
            out << "  setAudioAtmosOutputMode()      : EXCEPTION – " << e.what()
                << " *** stale handle ***\n\n";
            all_pass = false;
        }

        out << "RESULT: " << (all_pass ? "PASS" : "FAIL") << "\n";
        verify_record("TC-AUD-11", "ATMOS getSinkCap+setOutputMode", all_pass,
                      all_pass ? "getSinkAtmosCap+setAtmosMode OK"
                               : "STALE HANDLE: ATMOS FAILED");
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL – stale handle or dsmgr not running\n";
        out << "  *** Expected after dsmgr restart (stale intptr_t handle) ***\n";
        verify_record("TC-AUD-11", "ATMOS getSinkCap+setOutputMode", false,
                      std::string("STALE HANDLE: ") + e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\nRESULT: FAIL\n";
        verify_record("TC-AUD-11", "ATMOS getSinkCap+setOutputMode", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-AUD-12: ARC / LE Config – getSupportedARCTypes/getHdmiArcPortId/GetLEConfig
 *   Feature: ARC / eARC + LE Config
 *   Read-only snapshot on all audio ports
 *   Run BEFORE dsmgr restart → PASS; AFTER restart without reinit → FAIL (stale handle)
 *   Output file: 1_Audio/arc_le_info.txt
 * ========================================================================= */
static void tc_aud_12_arc_le(const std::string &mod_dir)
{
    printf("\n  [TC-AUD-12] ARC/LE – getSupportedARCTypes / getHdmiArcPortId / GetLEConfig\n");

    std::string fname = mod_dir + "/arc_le_info.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-AUD-12: ARC / LE Config info ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  ARC/LE APIs pass intptr_t _handle via IARM RPC.\n";
    out << "  All throw device::Exception after dsmgr restart without reinit.\n\n";

    bool any_fail = false;
    try {
        device::List<device::AudioOutputPort> aPorts =
            device::Host::getInstance().getAudioOutputPorts();
        out << "Port count : " << aPorts.size() << "\n\n";

        for (size_t i = 0; i < aPorts.size(); i++) {
            device::AudioOutputPort &aPort = aPorts.at(i);
            out << "── Port [" << aPort.getName() << "] ──────────────────────────────\n";

            try {
                int arcTypes = 0;
                aPort.getSupportedARCTypes(&arcTypes);
                out << "  getSupportedARCTypes() : 0x" << std::hex << arcTypes << std::dec
                    << "  (bit0=ARC, bit1=eARC)\n";
            } catch (...) {
                out << "  getSupportedARCTypes() : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }
            try {
                int arcPortId = -1;
                aPort.getHdmiArcPortId(&arcPortId);
                out << "  getHdmiArcPortId()     : " << arcPortId << "\n";
            } catch (...) {
                out << "  getHdmiArcPortId()     : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }
            try {
                bool leCfg = aPort.GetLEConfig();
                out << "  GetLEConfig()          : " << (leCfg ? "enabled" : "disabled") << "\n";
            } catch (...) {
                out << "  GetLEConfig()          : EXCEPTION *** stale handle ***\n";
                any_fail = true;
            }
            out << "\n";
        }
        bool pass = !any_fail;
        out << "RESULT: " << (pass ? "PASS" : "FAIL – stale handle exceptions") << "\n";
        verify_record("TC-AUD-12", "ARC/LE config info", pass,
                      pass ? "ARC/LE getters OK"
                           : "STALE HANDLE: ARC/LE getters FAILED");
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL\n";
        verify_record("TC-AUD-12", "ARC/LE config info", false, e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\nRESULT: FAIL\n";
        verify_record("TC-AUD-12", "ARC/LE config info", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-AUD-13: Language / Mixing – get+set round-trip
 *   Feature: Multi-Audio / Language
 *   round-trip: getAssociatedAudioMixing/set, getFaderControl/set,
 *               getPrimaryLanguage/set, getSecondaryLanguage/set
 *   Run BEFORE dsmgr restart → PASS; AFTER restart without reinit → FAIL (stale handle)
 *   Output file: 1_Audio/language_mixing_roundtrip.txt
 * ========================================================================= */
static void tc_aud_13_language_mixing(const std::string &mod_dir)
{
    printf("\n  [TC-AUD-13] Language/Mixing – AssocMixing/Fader/PrimaryLang/SecLang round-trip\n");

    std::string fname = mod_dir + "/language_mixing_roundtrip.txt";
    TeeStream out;
    out.open(fname);

    out << "=== TC-AUD-13: Language / Mixing round-trip ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "IARM_Init : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init   : " << (g_ds_initialized   ? "YES (may be stale)" : "NO") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  Language/Mixing APIs pass intptr_t _handle via IARM RPC.\n";
    out << "  All throw device::Exception after dsmgr restart without reinit.\n\n";

    bool all_pass = true;
    try {
        device::List<device::AudioOutputPort> aPorts =
            device::Host::getInstance().getAudioOutputPorts();
        if (aPorts.size() == 0) {
            out << "No audio output ports.\nRESULT: SKIP\n";
            verify_record("TC-AUD-13", "language+mixing round-trip", false, "no ports");
            out << "Saved to: " << fname << "\n";
            return;
        }
        device::AudioOutputPort &aPort = aPorts.at(0);
        out << "Port: " << aPort.getName() << "\n\n";

        /* ── AssociatedAudioMixing toggle round-trip ───────────────────── */
        {
            bool mixB4 = false;
            aPort.getAssociatedAudioMixing(&mixB4);
            out << "getAssociatedAudioMixing() before : " << (mixB4 ? "true" : "false") << "\n";
            aPort.setAssociatedAudioMixing(!mixB4);
            bool mixAfter = false;
            aPort.getAssociatedAudioMixing(&mixAfter);
            out << "setAssociatedAudioMixing()  set   : " << (!mixB4 ? "true" : "false") << "\n";
            out << "getAssociatedAudioMixing() after  : " << (mixAfter ? "true" : "false") << "\n";
            bool ok = (mixAfter == !mixB4);
            out << "  readback: " << (ok ? "PASS" : "FAIL") << "\n";
            if (!ok) all_pass = false;
            aPort.setAssociatedAudioMixing(mixB4);
            out << "getAssociatedAudioMixing() restore: " << (mixB4 ? "true" : "false") << "\n\n";
        }

        /* ── FaderControl round-trip (main↔assoc balance -32..32) ──────── */
        {
            int faderB4 = 0;
            aPort.getFaderControl(&faderB4);
            out << "getFaderControl() before : " << faderB4 << "\n";
            int newFader = (faderB4 == 0) ? 5 : 0;
            aPort.setFaderControl(newFader);
            int faderAfter = 0;
            aPort.getFaderControl(&faderAfter);
            out << "setFaderControl()  set   : " << newFader << "\n";
            out << "getFaderControl() after  : " << faderAfter << "\n";
            bool ok = (faderAfter == newFader);
            out << "  readback: " << (ok ? "PASS" : "FAIL") << "\n";
            if (!ok) all_pass = false;
            aPort.setFaderControl(faderB4);
            out << "getFaderControl() restore: " << faderB4 << "\n\n";
        }

        /* ── PrimaryLanguage round-trip (ISO 639-2 3-char code) ────────── */
        {
            std::string langB4;
            aPort.getPrimaryLanguage(langB4);
            out << "getPrimaryLanguage() before : \"" << langB4 << "\"\n";
            std::string newLang = (langB4 == "eng") ? "fra" : "eng";
            aPort.setPrimaryLanguage(newLang);
            std::string langAfter;
            aPort.getPrimaryLanguage(langAfter);
            out << "setPrimaryLanguage()  set   : \"" << newLang << "\"\n";
            out << "getPrimaryLanguage() after  : \"" << langAfter << "\"\n";
            bool ok = (langAfter == newLang);
            out << "  readback: " << (ok ? "PASS" : "FAIL") << "\n";
            if (!ok) all_pass = false;
            aPort.setPrimaryLanguage(langB4);
            out << "getPrimaryLanguage() restore: \"" << langB4 << "\"\n\n";
        }

        /* ── SecondaryLanguage round-trip ──────────────────────────────── */
        {
            std::string slangB4;
            aPort.getSecondaryLanguage(slangB4);
            out << "getSecondaryLanguage() before : \"" << slangB4 << "\"\n";
            std::string newSLang = (slangB4 == "spa") ? "deu" : "spa";
            aPort.setSecondaryLanguage(newSLang);
            std::string slangAfter;
            aPort.getSecondaryLanguage(slangAfter);
            out << "setSecondaryLanguage()  set   : \"" << newSLang << "\"\n";
            out << "getSecondaryLanguage() after  : \"" << slangAfter << "\"\n";
            bool ok = (slangAfter == newSLang);
            out << "  readback: " << (ok ? "PASS" : "FAIL") << "\n";
            if (!ok) all_pass = false;
            aPort.setSecondaryLanguage(slangB4);
            out << "getSecondaryLanguage() restore: \"" << slangB4 << "\"\n\n";
        }

        out << "RESULT: " << (all_pass ? "PASS" : "FAIL (readback mismatch)") << "\n";
        verify_record("TC-AUD-13", "language+mixing round-trip", all_pass,
                      all_pass ? "AssocMixing+Fader+PrimaryLang+SecLang all OK"
                               : "STALE HANDLE or readback mismatch");
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION (device::Exception): " << e.what() << "\n";
        out << "RESULT: FAIL – stale handle or dsmgr not running\n";
        out << "  *** Expected after dsmgr restart (stale intptr_t handle) ***\n";
        verify_record("TC-AUD-13", "language+mixing round-trip", false,
                      std::string("STALE HANDLE: ") + e.what());
    }
    catch (...) {
        out << "EXCEPTION (unknown)\nRESULT: FAIL\n";
        verify_record("TC-AUD-13", "language+mixing round-trip", false, "unknown exception");
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-CIN-01: CompositeIn – ALL getter APIs snapshot (global + per-port)
 *
 *   CompositeInput has a small API surface – all getters are covered here.
 *   Section 1 : Global getters   – getNumberOfInputs, isPresented, getActivePort
 *   Section 2 : Per-port getters – isPortConnected, isActivePort (all ports)
 *   Section 3 : Result summary
 *
 *   Output file: 2_CompositeIn/CompositeIn_all_getters.txt
 * ========================================================================= */
static void tc_cin_01_get_info(const std::string &mod_dir)
{
    printf("\n  [TC-CIN-01] CompositeIn – ALL getter APIs snapshot (global + per-port)\n");

    std::string fname = mod_dir + "/CompositeIn_all_getters.txt";
    TeeStream out;
    out.open(fname);

    out << "\u2554\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2557\n";
    out << "\u2551  TC-CIN-01 : CompositeIn \u2013 ALL Getter APIs Snapshot                 \u2551\n";
    out << "\u2551  Run BEFORE and AFTER dsmgr restart to compare output.            \u2551\n";
    out << "\u255a\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u255d\n";
    out << "Timestamp   : " << timestamp_now() << "\n";
    out << "Run number  : " << g_run_count << "\n";
    out << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init     : " << (g_ds_initialized   ? "YES (may be stale after restart)" : "NO") << "\n\n";

    bool any_fail = false;
    int  get_pass = 0, get_fail = 0;

    /* \u2500\u2500 SECTION 1 : Global getters \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
    out << "\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";
    out << "  SECTION 1 \u2013 Global Getters\n";
    out << "\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";

    uint8_t num = 0;
    try {
        device::CompositeInput &cin = device::CompositeInput::getInstance();

        try { num = cin.getNumberOfInputs();
              out << "  getNumberOfInputs()  : " << (int)num << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getNumberOfInputs()  : EXCEPTION \u2013 " << e.what() << "\n";
              ++get_fail; any_fail = true; }

        try { bool v = cin.isPresented();
              out << "  isPresented()        : " << (v ? "Yes" : "No") << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  isPresented()        : EXCEPTION \u2013 " << e.what() << "\n";
              ++get_fail; any_fail = true; }

        try { int8_t ap = cin.getActivePort();
              out << "  getActivePort()      : " << (int)ap
                  << (ap == COMPOSITE_IN_PORT_NONE ? "  (COMPOSITE_IN_PORT_NONE)" : "") << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getActivePort()      : EXCEPTION \u2013 " << e.what() << "\n";
              ++get_fail; any_fail = true; }

        /* \u2500\u2500 SECTION 2 : Per-port getters \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
        out << "\n\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";
        out << "  SECTION 2 \u2013 Per-Port Getters (all ports)\n";
        out << "\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";
        for (int8_t port = 0; port < (int8_t)num; ++port) {
            out << "  Port " << (int)port << ":";
            try { bool v = cin.isPortConnected(port);
                  out << "  isPortConnected=" << (v ? "Yes" : "No");
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  isPortConnected=EXCEPTION(" << e.what() << ")";
                  ++get_fail; any_fail = true; }
            try { bool v = cin.isActivePort(port);
                  out << "  isActivePort=" << (v ? "Yes" : "No");
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  isActivePort=EXCEPTION(" << e.what() << ")";
                  ++get_fail; any_fail = true; }
            out << "\n";
        }
    }
    catch (const device::Exception &e) {
        out << "\nEXCEPTION getting CompositeInput instance: " << e.what() << "\n";
        any_fail = true;
    }
    catch (...) {
        out << "\nEXCEPTION (unknown)\n";
        any_fail = true;
    }

    /* \u2500\u2500 SECTION 3 : Result summary \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
    out << "\n\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";
    out << "  SECTION 3 \u2013 Result Summary\n";
    out << "\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";
    out << "  Getters called  : " << (get_pass + get_fail) << "\n";
    out << "  Getters OK      : " << get_pass << "\n";
    out << "  Getters FAILED  : " << get_fail << "\n\n";
    out << (any_fail ? "  RESULT: FAIL \u2013 " + std::to_string(get_fail) + " getter(s) threw exceptions\n"
                    : "  RESULT: PASS \u2013 all CompositeIn getters returned values OK\n");
    verify_record("TC-CIN-01",
                  "CompositeIn all-getters snapshot",
                  !any_fail,
                  "getters_ok=" + std::to_string(get_pass) +
                  " getters_fail=" + std::to_string(get_fail));
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-CIN-02: (kept for backward compat – calls tc_cin_01 for full snapshot)
 *   Output file: 2_CompositeIn/CompositeIn_port_status.txt
 * ========================================================================= */
static void tc_cin_02_port_connected(const std::string &mod_dir)
{
    /* All CompositeIn getters are now in TC-CIN-01.  This TC runs the same
     * snapshot and writes a dedicated per-port status file for traceability. */
    printf("\n  [TC-CIN-02] CompositeIn \u2013 per-port status (redirects to TC-CIN-01 snapshot)\n");

    std::string fname = mod_dir + "/CompositeIn_port_status.txt";
    TeeStream out;
    out.open(fname);

    bool any_fail = false;
    int  get_pass = 0, get_fail = 0;

    try {
        device::CompositeInput &cin = device::CompositeInput::getInstance();
        uint8_t num = cin.getNumberOfInputs();
        out << "Number of composite inputs : " << (int)num << "\n\n";

        for (int8_t port = 0; port < (int8_t)num; ++port) {
            out << "  Port " << (int)port << ":";
            try { bool v = cin.isPortConnected(port);
                  out << "  connected=" << (v ? "Yes" : "No");
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  connected=EXCEPTION(" << e.what() << ")";
                  ++get_fail; any_fail = true; }
            try { bool v = cin.isActivePort(port);
                  out << "  active=" << (v ? "Yes" : "No");
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  active=EXCEPTION(" << e.what() << ")";
                  ++get_fail; any_fail = true; }
            out << "\n";
        }
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION: " << e.what() << "\n";
        any_fail = true;
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        any_fail = true;
    }
    out << (any_fail ? "\nRESULT: FAIL\n" : "\nRESULT: PASS\n");
    verify_record("TC-CIN-02",
                  "CompositeIn per-port status",
                  !any_fail,
                  "getters_ok=" + std::to_string(get_pass) +
                  " getters_fail=" + std::to_string(get_fail));
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-DSP-01: Display – ALL getter APIs snapshot (VideoOutputPort + Display sub-object)
 *
 *   Section 1 : VideoOutputPort getters  – getName, getId, getIndex, isEnabled,
 *               isActive, isDisplayConnected, isContentProtected,
 *               isDynamicResolutionSupported, getResolution, getDefaultResolution,
 *               getType, getVideoEOTF, getMatrixCoefficients, getColorSpace,
 *               getColorDepth, getQuantizationRange, getCurrentOutputSettings,
 *               getPreferredColorDepth, getColorDepthCapabilities, IsOutputHDR,
 *               getTVHDRCapabilities, getSupportedTvResolutions,
 *               getHDCPStatus/Protocol/CurrentProtocol, GetHdmiPreference
 *   Section 2 : Display sub-object       – hasSurround, getSurroundMode,
 *               getProductCode, getSerialNumber, getManufacturerYear,
 *               getManufacturerWeek, getConnectedDeviceType,
 *               isConnectedDeviceRepeater, getAspectRatio, getPhysicallAddress
 *   Section 3 : EDID bytes               – getEDIDBytes
 *   Section 4 : Result summary
 *
 *   Output file: 3_Display/Display_all_getters.txt
 * ========================================================================= */
static void tc_dsp_01_display_surround(const std::string &mod_dir)
{
    printf("\n  [TC-DSP-01] Display – ALL getter APIs snapshot (%s)\n", HDMI_PORT_NAME);

    std::string fname = mod_dir + "/Display_all_getters.txt";
    TeeStream out;
    out.open(fname);

    out << "╔══════════════════════════════════════════════════════════════════╗\n";
    out << "║  TC-DSP-01 : Display – ALL Getter APIs Snapshot                 ║\n";
    out << "║  Run BEFORE and AFTER dsmgr restart to compare output.          ║\n";
    out << "╚══════════════════════════════════════════════════════════════════╝\n";
    out << "Timestamp   : " << timestamp_now() << "\n";
    out << "Run number  : " << g_run_count << "\n";
    out << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init     : " << (g_ds_initialized ? "YES (may be stale after restart)" : "NO") << "\n\n";

    bool any_fail = false;
    int  get_pass = 0, get_fail = 0;

    try {
        device::VideoOutputPort &vPort =
            device::Host::getInstance().getVideoOutputPort(INTERNAL_PORT_NAME);

        /* ── SECTION 1 : VideoOutputPort getters ──────────────────────────── */
        out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        out << "  SECTION 1 – VideoOutputPort Getters\n";
        out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

        try { out << "  getName()                     : " << vPort.getName() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getName()                     : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  getId()                       : " << vPort.getId() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getId()                       : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  getIndex()                    : " << vPort.getIndex() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getIndex()                    : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { bool v = vPort.isEnabled(); out << "  isEnabled()                   : " << (v?"Yes":"No") << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  isEnabled()                   : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { bool v = vPort.isActive(); out << "  isActive()                    : " << (v?"Yes":"No") << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  isActive()                    : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }

        bool connected = false;
        try { connected = vPort.isDisplayConnected(); out << "  isDisplayConnected()          : " << (connected?"Yes":"No") << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  isDisplayConnected()          : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { bool v = vPort.isContentProtected(); out << "  isContentProtected()          : " << (v?"Yes":"No") << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  isContentProtected()          : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { bool v = vPort.isDynamicResolutionSupported(); out << "  isDynamicResolutionSupported(): " << (v?"Yes":"No") << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  isDynamicResolutionSupported(): EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  getResolution()               : " << vPort.getResolution().getName() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getResolution()               : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  getDefaultResolution()        : " << vPort.getDefaultResolution().getName() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getDefaultResolution()        : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  getType()                     : " << vPort.getType().getName() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getType()                     : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  getVideoEOTF()                : " << vPort.getVideoEOTF() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getVideoEOTF()                : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  getMatrixCoefficients()       : " << vPort.getMatrixCoefficients() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getMatrixCoefficients()       : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  getColorSpace()               : " << vPort.getColorSpace() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getColorSpace()               : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  getColorDepth()               : " << vPort.getColorDepth() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getColorDepth()               : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  getQuantizationRange()        : " << vPort.getQuantizationRange() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getQuantizationRange()        : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { int eo,mc,cs,cd,qr; vPort.getCurrentOutputSettings(eo,mc,cs,cd,qr);
              out << "  getCurrentOutputSettings()    : eotf=" << eo << " mc=" << mc << " cs=" << cs << " cd=" << cd << " qr=" << qr << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getCurrentOutputSettings()    : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  getPreferredColorDepth()      : " << vPort.getPreferredColorDepth() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getPreferredColorDepth()      : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { unsigned int cap=0; vPort.getColorDepthCapabilities(&cap);
              out << "  getColorDepthCapabilities()   : " << cap << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getColorDepthCapabilities()   : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  IsOutputHDR()                 : " << (vPort.IsOutputHDR()?"Yes":"No") << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  IsOutputHDR()                 : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { int cap=0; vPort.getTVHDRCapabilities(&cap);
              out << "  getTVHDRCapabilities()        : " << cap << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getTVHDRCapabilities()        : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { int res=0; vPort.getSupportedTvResolutions(&res);
              out << "  getSupportedTvResolutions()   : " << decode_tv_resolutions(res) << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getSupportedTvResolutions()   : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  getHDCPStatus()               : " << vPort.getHDCPStatus() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getHDCPStatus()               : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  getHDCPProtocol()             : " << vPort.getHDCPProtocol() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getHDCPProtocol()             : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  getHDCPCurrentProtocol()      : " << vPort.getHDCPCurrentProtocol() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getHDCPCurrentProtocol()      : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
        try { out << "  GetHdmiPreference()           : " << vPort.GetHdmiPreference() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  GetHdmiPreference()           : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }

        /* ── SECTION 2 : Display sub-object ───────────────────────────────── */
        out << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        out << "  SECTION 2 – Display Sub-Object Getters\n";
        out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

        if (!connected) {
            out << "  Display not connected – sub-object getters skipped.\n";
        } else {
            try {
                const device::VideoOutputPort::Display &disp = vPort.getDisplay();

                try { out << "  hasSurround()                 : " << (disp.hasSurround()?"Yes":"No") << "\n"; ++get_pass; }
                catch (const device::Exception &e) { out << "  hasSurround()                 : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
                try { out << "  getSurroundMode()             : " << disp.getSurroundMode() << "\n"; ++get_pass; }
                catch (const device::Exception &e) { out << "  getSurroundMode()             : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }

                /* Inline getters cached from EDID – no RPC */
                out << "  getProductCode()              : " << disp.getProductCode() << "\n"; ++get_pass;
                out << "  getSerialNumber()             : " << disp.getSerialNumber() << "\n"; ++get_pass;
                out << "  getManufacturerYear()         : " << disp.getManufacturerYear() << "\n"; ++get_pass;
                out << "  getManufacturerWeek()         : " << disp.getManufacturerWeek() << "\n"; ++get_pass;
                out << "  getConnectedDeviceType()      : " << disp.getConnectedDeviceType()
                    << (disp.getConnectedDeviceType() ? "  (HDMI)" : "  (DVI)") << "\n"; ++get_pass;
                out << "  isConnectedDeviceRepeater()   : " << (disp.isConnectedDeviceRepeater()?"Yes":"No") << "\n"; ++get_pass;
                out << "  getAspectRatio()              : " << disp.getAspectRatio().getName() << "\n"; ++get_pass;

                try { uint8_t a,b,c,d; disp.getPhysicallAddress(a,b,c,d);
                      out << "  getPhysicallAddress()         : " << (int)a << "." << (int)b << "." << (int)c << "." << (int)d << "\n"; ++get_pass; }
                catch (const device::Exception &e) { out << "  getPhysicallAddress()         : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }

                /* ── SECTION 3 : EDID bytes ────────────────────────────────── */
                out << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
                out << "  SECTION 3 – EDID Bytes\n";
                out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
                try { std::vector<uint8_t> edid; disp.getEDIDBytes(edid);
                      out << "  getEDIDBytes()                : " << edid.size() << " bytes";
                      if (!edid.empty()) {
                          out << "  first-8:";
                          for (size_t j=0; j<8 && j<edid.size(); ++j)
                              out << " 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)edid[j];
                          out << std::dec << std::setfill(' ');
                      }
                      out << "\n"; ++get_pass; }
                catch (const device::Exception &e) { out << "  getEDIDBytes()                : EXCEPTION – " << e.what() << "\n"; ++get_fail; any_fail=true; }
            }
            catch (const device::Exception &e) {
                out << "  getDisplay() EXCEPTION: " << e.what() << "\n"; any_fail=true;
            }
        }
    }
    catch (const device::Exception &e) {
        out << "\nEXCEPTION getting VideoOutputPort: " << e.what() << "\n"; any_fail=true;
    }
    catch (...) {
        out << "\nEXCEPTION (unknown)\n"; any_fail=true;
    }

    /* ── SECTION 4 : Result summary ──────────────────────────────────────── */
    out << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    out << "  Getters called  : " << (get_pass + get_fail) << "\n";
    out << "  Getters OK      : " << get_pass << "\n";
    out << "  Getters FAILED  : " << get_fail << "\n\n";
    out << (any_fail ? "  RESULT: FAIL – " + std::to_string(get_fail) + " getter(s) threw exceptions\n"
                    : "  RESULT: PASS – all Display getters returned values OK\n");
    verify_record("TC-DSP-01",
                  "Display all-getters snapshot",
                  !any_fail,
                  "getters_ok=" + std::to_string(get_pass) +
                  " getters_fail=" + std::to_string(get_fail));
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-DSP-02: Display – getEDIDBytes standalone raw file
 *
 *   EDID is also captured in TC-DSP-01 Section 3.
 *   This writes a standalone file for quick diff comparison across reboots.
 *
 *   Output file: 3_Display/Display_edid_raw.txt
 * ========================================================================= */
static void tc_dsp_02_display_edid(const std::string &mod_dir)
{
    printf("\n  [TC-DSP-02] Display – getEDIDBytes standalone (%s)\n", HDMI_PORT_NAME);

    std::string fname = mod_dir + "/Display_edid_raw.txt";
    TeeStream out;
    out.open(fname);

    bool any_fail = false;
    out << "=== TC-DSP-02: Display getEDIDBytes (standalone) ===\n";
    out << "Timestamp : " << timestamp_now() << "\n";
    out << "Run       : " << g_run_count << "\n\n";

    try {
        device::VideoOutputPort &vPort =
            device::Host::getInstance().getVideoOutputPort(HDMI_PORT_NAME);

        bool connected = vPort.isDisplayConnected();
        out << "isDisplayConnected() : " << (connected ? "Yes" : "No") << "\n";

        if (!connected) {
            out << "Display not connected – EDID skipped.\n";
            verify_record("TC-DSP-02", "Display getEDIDBytes", true, "display not connected – skip");
            out << "Saved to: " << fname << "\n";
            return;
        }

        const device::VideoOutputPort::Display &disp = vPort.getDisplay();

        try {
            std::vector<uint8_t> edid;
            disp.getEDIDBytes(edid);
            out << "EDID byte count      : " << edid.size() << "\n";
            out << "First 8 bytes (hex)  :";
            for (size_t b=0; b<edid.size() && b<8; ++b) {
                char h[8]; snprintf(h,sizeof(h)," %02X",edid[b]); out<<h;
            }
            out << "\n";
            bool ok = (edid.size() >= 128);
            out << (ok ? "RESULT: PASS\n" : "RESULT: FAIL (EDID too short, expected >=128)\n");
            verify_record("TC-DSP-02", "Display getEDIDBytes", ok,
                          "edidLen=" + std::to_string(edid.size()));
        }
        catch (const device::Exception &e) {
            out << "getEDIDBytes() EXCEPTION: " << e.what() << "\n";
            verify_record("TC-DSP-02", "Display getEDIDBytes", false, e.what());
            any_fail = true;
        }
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION: " << e.what() << "\n";
        verify_record("TC-DSP-02", "Display getEDIDBytes", false, e.what());
        any_fail = true;
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n";
        verify_record("TC-DSP-02", "Display getEDIDBytes", false, "unknown exception");
        any_fail = true;
    }
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-HIN-01: HdmiInput – ALL global getter APIs snapshot (before / after restart)
 *
 *   PURPOSE:
 *   Call every HdmiInput global getter (not per-port) and record the result.
 *   HdmiInput is a singleton – no intptr_t handle per port, so most calls
 *   route through IARM with a global dsmgr state.  After dsmgr restart the
 *   IARM bus reconnects, but dsmgr internal state is reset.
 *
 *   Section 1 : Global getters  – getNumberOfInputs, isPresented,
 *               getActivePort, getCurrentVideoMode, getCurrentVideoModeObj,
 *               getSupportedGameFeatures, getAVLatency
 *   Section 2 : Per-port status – isPortConnected, isActivePort (all ports)
 *   Section 3 : Result summary
 *
 *   Output file: 5_HDMIIn/HdmiIn_all_getters.txt
 * ========================================================================= */
static void tc_hin_01_get_info(const std::string &mod_dir)
{
    printf("\n  [TC-HIN-01] HdmiInput – ALL global getter APIs snapshot\n");

    std::string fname = mod_dir + "/HdmiIn_all_getters.txt";
    TeeStream out;
    out.open(fname);

    out << "\u2554\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2557\n";
    out << "\u2551  TC-HIN-01 : HdmiInput \u2013 ALL Getter APIs Snapshot                  \u2551\n";
    out << "\u2551  Run BEFORE and AFTER dsmgr restart to compare output.            \u2551\n";
    out << "\u255a\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u255d\n";
    out << "Timestamp   : " << timestamp_now() << "\n";
    out << "Run number  : " << g_run_count << "\n";
    out << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init     : " << (g_ds_initialized   ? "YES (may be stale after restart)" : "NO") << "\n";
    out << "Stale risk  : " << (g_run_count > 1 && g_ds_initialized
                                ? "YES \u2013 dsmgr state reset after restart"
                                : "No (first run or re-initialized)") << "\n\n";

    bool any_fail = false;
    int  get_pass = 0, get_fail = 0;

    /* \u2500\u2500 SECTION 1 : Global getters \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
    out << "\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";
    out << "  SECTION 1 \u2013 Global Getters\n";
    out << "\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";
    try {
        device::HdmiInput &hin = device::HdmiInput::getInstance();

        /* \u2500\u2500 Feature: Input Presence \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
        out << "\u2500\u2500 Feature: Input Presence \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";

        uint8_t num = 0;
        try { num = hin.getNumberOfInputs();
              out << "  getNumberOfInputs()           : " << (int)num << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getNumberOfInputs()           : EXCEPTION \u2013 " << e.what() << "\n";
              ++get_fail; any_fail = true; }

        try { bool v = hin.isPresented();
              out << "  isPresented()                 : " << (v ? "Yes" : "No") << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  isPresented()                 : EXCEPTION \u2013 " << e.what() << "\n";
              ++get_fail; any_fail = true; }

        /* \u2500\u2500 Feature: Active Port / Video Mode \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
        out << "\n\u2500\u2500 Feature: Active Port / Video Mode \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";

        int8_t activePort = -1;
        try { activePort = hin.getActivePort();
              out << "  getActivePort()               : " << (int)activePort
                  << (activePort == HDMI_IN_PORT_NONE ? "  (HDMI_IN_PORT_NONE)" : "") << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getActivePort()               : EXCEPTION \u2013 " << e.what() << "\n";
              ++get_fail; any_fail = true; }

        try { std::string v = hin.getCurrentVideoMode();
              out << "  getCurrentVideoMode()         : \"" << v << "\"\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getCurrentVideoMode()         : EXCEPTION \u2013 " << e.what() << "\n";
              ++get_fail; any_fail = true; }

        try { dsVideoPortResolution_t res = {};
              hin.getCurrentVideoModeObj(res);
              out << "  getCurrentVideoModeObj()      :\n";
              out << "    name          = \"" << res.name << "\"\n";
              out << "    pixelResolution = " << res.pixelResolution << "\n";
              out << "    frameRate       = " << res.frameRate << "\n";
              out << "    interlaced      = " << (res.interlaced ? "Yes" : "No") << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getCurrentVideoModeObj()      : EXCEPTION \u2013 " << e.what() << "\n";
              ++get_fail; any_fail = true; }

        /* \u2500\u2500 Feature: Game Features \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
        out << "\n\u2500\u2500 Feature: Game Features \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";

        try { std::vector<std::string> features;
              hin.getSupportedGameFeatures(features);
              out << "  getSupportedGameFeatures()    : " << features.size() << " features";
              for (const std::string &f : features) out << "  [" << f << "]";
              out << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getSupportedGameFeatures()    : EXCEPTION \u2013 " << e.what() << "\n";
              ++get_fail; any_fail = true; }

        /* \u2500\u2500 Feature: AV Latency \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
        out << "\n\u2500\u2500 Feature: AV Latency \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";

        try { int audio_delay = 0, video_latency = 0;
              hin.getAVLatency(&audio_delay, &video_latency);
              out << "  getAVLatency()                :\n";
              out << "    audio_output_delay = " << audio_delay << " ms\n";
              out << "    video_latency      = " << video_latency << " ms\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getAVLatency()                : EXCEPTION \u2013 " << e.what() << "\n";
              ++get_fail; any_fail = true; }

        /* \u2500\u2500 SECTION 2 : Per-port status \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
        out << "\n\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";
        out << "  SECTION 2 \u2013 Per-Port Status (isConnected / isActive)\n";
        out << "\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";
        for (int8_t port = 0; port < (int8_t)num; ++port) {
            out << "  Port " << (int)port << ":";
            try { bool co = hin.isPortConnected(port);
                  out << "  connected=" << (co ? "Yes" : "No");
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  connected=EXCEPTION(" << e.what() << ")";
                  ++get_fail; any_fail = true; }
            try { bool ac = hin.isActivePort(port);
                  out << "  active=" << (ac ? "Yes" : "No");
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  active=EXCEPTION(" << e.what() << ")";
                  ++get_fail; any_fail = true; }
            out << "\n";
        }
    }
    catch (const device::Exception &e) {
        out << "\nEXCEPTION getting HdmiInput instance: " << e.what() << "\n";
        any_fail = true;
    }
    catch (...) {
        out << "\nEXCEPTION (unknown) getting HdmiInput instance\n";
        any_fail = true;
    }

    /* \u2500\u2500 SECTION 3 : Result summary \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
    out << "\n\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";
    out << "  SECTION 3 \u2013 Result Summary\n";
    out << "\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";
    out << "  Getters called  : " << (get_pass + get_fail) << "\n";
    out << "  Getters OK      : " << get_pass << "\n";
    out << "  Getters FAILED  : " << get_fail << "\n\n";
    if (!any_fail) {
        out << "  RESULT: PASS \u2013 all HdmiInput global getters returned values OK\n";
    } else {
        out << "  RESULT: FAIL \u2013 " << get_fail << " getter(s) threw exceptions\n";
    }
    verify_record("TC-HIN-01",
                  "HdmiInput all-getters snapshot",
                  !any_fail,
                  "getters_ok=" + std::to_string(get_pass) +
                  " getters_fail=" + std::to_string(get_fail));
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-HIN-02: HdmiInput – per-port detail getters (EDID / SPD / EDID-ver /
 *            VRR / ALLM / HdmiVersion)
 *
 *   Iterates every available HDMI input port and calls all per-port
 *   getter APIs.  Results are grouped by port then by feature.
 *
 *   Output file: 5_HDMIIn/HdmiIn_perport_getters.txt
 * ========================================================================= */
static void tc_hin_02_port_connected(const std::string &mod_dir)
{
    printf("\n  [TC-HIN-02] HdmiInput \u2013 per-port detail getters (EDID/SPD/VRR/ALLM/HdmiVer)\n");

    std::string fname = mod_dir + "/HdmiIn_perport_getters.txt";
    TeeStream out;
    out.open(fname);

    out << "\u2554\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2557\n";
    out << "\u2551  TC-HIN-02 : HdmiInput \u2013 Per-Port Detail Getters (all ports)       \u2551\n";
    out << "\u2551  Run BEFORE and AFTER dsmgr restart to compare output.            \u2551\n";
    out << "\u255a\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u255d\n";
    out << "Timestamp   : " << timestamp_now() << "\n";
    out << "Run number  : " << g_run_count << "\n";
    out << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init     : " << (g_ds_initialized ? "YES (may be stale after restart)" : "NO") << "\n\n";

    bool any_fail = false;
    int  get_pass = 0, get_fail = 0;

    try {
        device::HdmiInput &hin = device::HdmiInput::getInstance();
        uint8_t num = hin.getNumberOfInputs();
        out << "Number of HDMI inputs : " << (int)num << "\n\n";

        for (int8_t port = 0; port < (int8_t)num; ++port) {
            out << "\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";
            out << "  Port " << (int)port << "\n";
            out << "\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";

            /* ── Step 1: Check isActivePort() first ─────────────────────────── */
            out << "  \u2500\u2500 Connection \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";
            bool is_active = false;
            try { is_active = hin.isActivePort(port);
                  out << "  isActivePort()                : " << (is_active ? "Yes (ACTIVE)" : "No (inactive)") << "\n";
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  isActivePort()                : EXCEPTION \u2013 " << e.what() << "\n";
                  ++get_fail; any_fail = true; }

            try { bool v = hin.isPortConnected(port);
                  out << "  isPortConnected()             : " << (v ? "Yes" : "No") << "\n";
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  isPortConnected()             : EXCEPTION \u2013 " << e.what() << "\n";
                  ++get_fail; any_fail = true; }

            if (!is_active) {
                /* Port is inactive – skip all detail getters */
                out << "\n  >> Port " << (int)port
                    << " is NOT active \u2013 skipping EDID / SPD / VRR / ALLM / HdmiVersion getters.\n\n";
            } else {
                /* ── Step 2: Port is active – call all remaining getter APIs ── */

                /* EDID */
                out << "  \u2500\u2500 EDID \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";
                try { std::vector<uint8_t> edid;
                      hin.getEDIDBytesInfo((int)port, edid);
                      out << "  getEDIDBytesInfo()            : " << edid.size() << " bytes";
                      if (!edid.empty()) {
                          out << "  first-8:";
                          for (size_t j = 0; j < 8 && j < edid.size(); ++j)
                              out << " 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)edid[j];
                          out << std::dec << std::setfill(' ');
                      }
                      out << "\n";
                      ++get_pass; }
                catch (const device::Exception &e) {
                      out << "  getEDIDBytesInfo()            : EXCEPTION \u2013 " << e.what() << "\n";
                      ++get_fail; any_fail = true; }

                try { int ver = 0;
                      hin.getEdidVersion((int)port, &ver);
                      out << "  getEdidVersion()              : " << ver
                          << "  (1=EDID 1.4, 2=EDID 2.0)\n";
                      ++get_pass; }
                catch (const device::Exception &e) {
                      out << "  getEdidVersion()              : EXCEPTION \u2013 " << e.what() << "\n";
                      ++get_fail; any_fail = true; }

                try { bool v = false;
                      hin.getEdid2AllmSupport((int)port, &v);
                      out << "  getEdid2AllmSupport()         : " << (v ? "Yes" : "No") << "\n";
                      ++get_pass; }
                catch (const device::Exception &e) {
                      out << "  getEdid2AllmSupport()         : EXCEPTION \u2013 " << e.what() << "\n";
                      ++get_fail; any_fail = true; }

                /* SPD Info */
                out << "  \u2500\u2500 SPD Info \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";
                try { std::vector<uint8_t> spd;
                      hin.getHDMISPDInfo((int)port, spd);
                      out << "  getHDMISPDInfo()              : " << spd.size() << " bytes\n";
                      ++get_pass; }
                catch (const device::Exception &e) {
                      out << "  getHDMISPDInfo()              : EXCEPTION \u2013 " << e.what() << "\n";
                      ++get_fail; any_fail = true; }

                /* VRR / ALLM */
                out << "  \u2500\u2500 VRR / ALLM \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";
                try { bool v = false;
                      hin.getVRRSupport((int)port, &v);
                      out << "  getVRRSupport()               : " << (v ? "Yes" : "No") << "\n";
                      ++get_pass; }
                catch (const device::Exception &e) {
                      out << "  getVRRSupport()               : EXCEPTION \u2013 " << e.what() << "\n";
                      ++get_fail; any_fail = true; }

                try { dsHdmiInVrrStatus_t vs = {};
                      hin.getVRRStatus((int)port, &vs);
                      out << "  getVRRStatus()                : vrrType=" << (int)vs.vrrType
                          << "  freesync_Hz=" << vs.vrrAmdfreesyncFramerate_Hz << "\n";
                      ++get_pass; }
                catch (const device::Exception &e) {
                      out << "  getVRRStatus()                : EXCEPTION \u2013 " << e.what() << "\n";
                      ++get_fail; any_fail = true; }

                try { bool v = false;
                      hin.getHdmiALLMStatus((int)port, &v);
                      out << "  getHdmiALLMStatus()           : " << (v ? "enabled" : "disabled") << "\n";
                      ++get_pass; }
                catch (const device::Exception &e) {
                      out << "  getHdmiALLMStatus()           : EXCEPTION \u2013 " << e.what() << "\n";
                      ++get_fail; any_fail = true; }

                /* HDMI Version */
                out << "  \u2500\u2500 HDMI Version \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";
                try { dsHdmiMaxCapabilityVersion_t capver = {};
                      hin.getHdmiVersion((int)port, &capver);
                      out << "  getHdmiVersion()              : capabilityVersion=" << (int)capver << "\n";
                      ++get_pass; }
                catch (const device::Exception &e) {
                      out << "  getHdmiVersion()              : EXCEPTION \u2013 " << e.what() << "\n";
                      ++get_fail; any_fail = true; }

                out << "\n";
            } /* end if (is_active) */
        }
    }
    catch (const device::Exception &e) {
        out << "\nEXCEPTION getting HdmiInput instance: " << e.what() << "\n";
        any_fail = true;
    }
    catch (...) {
        out << "\nEXCEPTION (unknown) getting HdmiInput instance\n";
        any_fail = true;
    }

    out << "\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";
    out << "  Getters called  : " << (get_pass + get_fail) << "\n";
    out << "  Getters OK      : " << get_pass << "\n";
    out << "  Getters FAILED  : " << get_fail << "\n\n";
    if (!any_fail) {
        out << "  RESULT: PASS \u2013 all per-port getters returned values OK\n";
    } else {
        out << "  RESULT: FAIL \u2013 " << get_fail << " getter(s) threw exceptions\n";
    }
    verify_record("TC-HIN-02",
                  "HdmiInput per-port detail getters",
                  !any_fail,
                  "getters_ok=" + std::to_string(get_pass) +
                  " getters_fail=" + std::to_string(get_fail));
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
    printf("\n  [TC-HST-01] Host \u2013 ALL getter APIs snapshot\n");

    std::string fname = mod_dir + "/Host_all_getters.txt";
    TeeStream out;
    out.open(fname);

    out << "\u2554\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2557\n";
    out << "\u2551  TC-HST-01 : Host \u2013 ALL Getter APIs Snapshot                      \u2551\n";
    out << "\u2551  Run BEFORE and AFTER dsmgr restart to compare output.            \u2551\n";
    out << "\u255a\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u255d\n";
    out << "Timestamp   : " << timestamp_now() << "\n";
    out << "Run number  : " << g_run_count << "\n";
    out << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init     : " << (g_ds_initialized   ? "YES (may be stale after restart)" : "NO") << "\n\n";

    bool any_fail = false;
    int  get_pass = 0, get_fail = 0;

    try {
        device::Host &h = device::Host::getInstance();

        /* \u2500\u2500 Feature: System Info \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
        out << "\u2500\u2500 Feature: System Info \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";

        try { float v = h.getCPUTemperature();
              out << "  getCPUTemperature()           : " << v << " \u00b0C\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getCPUTemperature()           : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { uint32_t v = h.getVersion();
              char vbuf[16]; snprintf(vbuf, sizeof(vbuf), "0x%08X", v);
              out << "  getVersion()                  : " << vbuf << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getVersion()                  : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { std::string v = h.getSocIDFromSDK();
              out << "  getSocIDFromSDK()             : \"" << v << "\"\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getSocIDFromSDK()             : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        /* \u2500\u2500 Feature: Power \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
        out << "\n\u2500\u2500 Feature: Power / Sleep \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";

        try { int v = h.getPowerMode();
              out << "  getPowerMode()                : " << v << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getPowerMode()                : EXCEPTION \u2013 " << "It is not support on libds" << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { device::SleepMode v = h.getPreferredSleepMode();
              out << "  getPreferredSleepMode()       : " << v.getName() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getPreferredSleepMode()       : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { device::List<device::SleepMode> modes = h.getAvailableSleepModes();
              out << "  getAvailableSleepModes()      : " << modes.size() << " modes:";
              for (size_t i = 0; i < modes.size(); ++i) out << "  [" << modes.at(i).getName() << "]";
              out << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getAvailableSleepModes()      : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        /* \u2500\u2500 Feature: Port Discovery \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
        out << "\n\u2500\u2500 Feature: Port Discovery \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";

        try { bool v = h.isHDMIOutPortPresent();
              out << "  isHDMIOutPortPresent()        : " << (v ? "Yes" : "No") << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  isHDMIOutPortPresent()        : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { out << "  getDefaultVideoPortName()     : " << h.getDefaultVideoPortName() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getDefaultVideoPortName()     : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { out << "  getDefaultAudioPortName()     : " << h.getDefaultAudioPortName() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getDefaultAudioPortName()     : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { out << "  getAudioOutputPorts().size()  : " << h.getAudioOutputPorts().size() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getAudioOutputPorts().size()  : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { out << "  getVideoOutputPorts().size()  : " << h.getVideoOutputPorts().size() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getVideoOutputPorts().size()  : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { out << "  getVideoDevices().size()      : " << h.getVideoDevices().size() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getVideoDevices().size()      : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        /* \u2500\u2500 Feature: Host EDID \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
        out << "\n\u2500\u2500 Feature: Host EDID \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";

        try { std::vector<uint8_t> edid;
              h.getHostEDID(edid);
              out << "  getHostEDID()                 : " << edid.size() << " bytes";
              if (!edid.empty()) {
                  out << "  first-8:";
                  for (size_t j = 0; j < 8 && j < edid.size(); ++j)
                      out << " 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)edid[j];
                  out << std::dec << std::setfill(' ');
              }
              out << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getHostEDID()                 : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        /* \u2500\u2500 Feature: Audio Settings \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500 */
        out << "\n\u2500\u2500 Feature: Audio Settings \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";

        try { dsATMOSCapability_t v = dsAUDIO_ATMOS_NOTSUPPORTED;
              h.getSinkDeviceAtmosCapability(v);
              out << "  getSinkDeviceAtmosCapability(): " << (int)v << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getSinkDeviceAtmosCapability(): EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { bool v = false;
              h.getAssociatedAudioMixing(&v);
              out << "  getAssociatedAudioMixing()    : " << (v ? "enabled" : "disabled") << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getAssociatedAudioMixing()    : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { int v = 0;
              h.getFaderControl(&v);
              out << "  getFaderControl()             : " << v << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getFaderControl()             : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { std::string v;
              h.getPrimaryLanguage(v);
              out << "  getPrimaryLanguage()          : \"" << v << "\"\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getPrimaryLanguage()          : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { std::string v;
              h.getSecondaryLanguage(v);
              out << "  getSecondaryLanguage()        : \"" << v << "\"\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getSecondaryLanguage()        : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { dsAudioFormat_t fmt = dsAUDIO_FORMAT_NONE;
              h.getCurrentAudioFormat(fmt);
              out << "  getCurrentAudioFormat()       : " << (int)fmt << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getCurrentAudioFormat()       : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { std::string v;
              h.getMS12ConfigDetails(v);
              out << "  getMS12ConfigDetails()        : \"" << v << "\"\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "  getMS12ConfigDetails()        : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }
    }
    catch (const device::Exception &e) {
        out << "\nEXCEPTION getting Host instance: " << e.what() << "\n";
        any_fail = true;
    }
    catch (...) {
        out << "\nEXCEPTION (unknown)\n";
        any_fail = true;
    }

    out << "\n\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\n";
    out << "  Getters called  : " << (get_pass + get_fail) << "\n";
    out << "  Getters OK      : " << get_pass << "\n";
    out << "  Getters FAILED  : " << get_fail << "\n\n";
    out << (any_fail ? "  RESULT: FAIL \u2013 " + std::to_string(get_fail) + " getter(s) threw exceptions\n"
                    : "  RESULT: PASS \u2013 all Host getters returned values OK\n");
    verify_record("TC-HST-01",
                  "Host all-getters snapshot",
                  !any_fail,
                  "getters_ok=" + std::to_string(get_pass) +
                  " getters_fail=" + std::to_string(get_fail));
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-HST-02: Host – sleep modes + port counts (kept for backward compat)
 *
 *   Output file: 6_Host/Host_sleep_modes.txt
 * ========================================================================= */
static void tc_hst_02_sleep_modes(const std::string &mod_dir)
{
    printf("\n  [TC-HST-02] Host \u2013 sleep modes + port counts\n");

    std::string fname = mod_dir + "/Host_sleep_modes.txt";
    TeeStream out;
    out.open(fname);

    bool any_fail = false;
    int  get_pass = 0, get_fail = 0;
    try {
        device::Host &h = device::Host::getInstance();

        try { device::SleepMode v = h.getPreferredSleepMode();
              out << "getPreferredSleepMode()      : " << v.getName() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "getPreferredSleepMode()      : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { device::List<device::SleepMode> modes = h.getAvailableSleepModes();
              out << "getAvailableSleepModes()     : " << modes.size() << " modes\n";
              for (size_t i = 0; i < modes.size(); ++i) out << "  [" << i << "] " << modes.at(i).getName() << "\n";
              ++get_pass; }
        catch (const device::Exception &e) { out << "getAvailableSleepModes()     : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { out << "getAudioOutputPorts().size() : " << h.getAudioOutputPorts().size() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "getAudioOutputPorts().size() : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { out << "getVideoOutputPorts().size() : " << h.getVideoOutputPorts().size() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "getVideoOutputPorts().size() : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }

        try { out << "getVideoDevices().size()     : " << h.getVideoDevices().size() << "\n"; ++get_pass; }
        catch (const device::Exception &e) { out << "getVideoDevices().size()     : EXCEPTION \u2013 " << e.what() << "\n"; ++get_fail; any_fail=true; }
    }
    catch (const device::Exception &e) {
        out << "EXCEPTION: " << e.what() << "\n"; any_fail = true;
    }
    catch (...) {
        out << "EXCEPTION (unknown)\n"; any_fail = true;
    }
    out << (any_fail ? "\nRESULT: FAIL\n" : "\nRESULT: PASS\n");
    verify_record("TC-HST-02",
                  "Host sleep modes + port counts",
                  !any_fail,
                  "getters_ok=" + std::to_string(get_pass) +
                  " getters_fail=" + std::to_string(get_fail));
    out << "Saved to: " << fname << "\n";
}

/* =========================================================================
 * TC-VD-01: VideoDevice – ALL getter APIs snapshot (before / after restart)
 *
 *   PURPOSE:
 *   Call every VideoDevice getter and record the result.
 *
 *   RUN BEFORE dsmgr restart  →  all handle-based getters print values  →  RESULT=PASS
 *   RUN AFTER  dsmgr restart  →  every handle-based call throws device::Exception
 *                                because intptr_t _handle is stale  →  RESULT=FAIL
 *
 *   Section 1 : Device inventory       – device count
 *   Section 2 : Device[0] all getters  – all 8 getter APIs grouped by feature
 *   Section 3 : Result summary
 *
 *   Getter APIs covered (8 total):
 *     Handle-based (fail after restart): getDFC, getHDRCapabilities,
 *       getSupportedVideoCodingFormats, getVideoCodecInfo, getFRFMode,
 *       getCurrentDisframerate
 *     Static / no-handle (always pass): getSupportedDFCs,
 *       getSettopSupportedResolutions
 *
 *   Output file: 7_VideoDevice/VideoDevice_all_getters.txt
 * ========================================================================= */
static void tc_vd_01_get_devices(const std::string &mod_dir)
{
    printf("\n  [TC-VD-01] VideoDevice – ALL getter APIs snapshot (run before & after restart)\n");

    std::string fname = mod_dir + "/VideoDevice_all_getters.txt";
    TeeStream out;
    out.open(fname);

    out << "╔══════════════════════════════════════════════════════════════════╗\n";
    out << "║  TC-VD-01 : VideoDevice – ALL Getter APIs Snapshot              ║\n";
    out << "║  Run BEFORE and AFTER dsmgr restart to compare output.          ║\n";
    out << "╚══════════════════════════════════════════════════════════════════╝\n";
    out << "Timestamp   : " << timestamp_now() << "\n";
    out << "Run number  : " << g_run_count << "\n";
    out << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init     : " << (g_ds_initialized   ? "YES (may be stale after restart)" : "NO") << "\n";
    out << "Stale risk  : " << (g_run_count > 1 && g_ds_initialized
                                ? "YES – intptr_t _handle from RUN-01 still in use"
                                : "No (first run or re-initialized)") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  VideoDevice._handle = intptr_t assigned at Manager::Initialize().\n";
    out << "  After dsmgr kill+restart, dsmgr allocates NEW server-side pointers.\n";
    out << "  The C++ wrapper still holds the OLD pointer  →  dsERR_INVALID_PARAM\n";
    out << "  →  device::Exception on EVERY handle-based IARM RPC call.\n";
    out << "  getSupportedDFCs / getSettopSupportedResolutions are static (no handle).\n\n";

    bool any_fail  = false;
    int  get_pass  = 0, get_fail = 0;

    /* ── SECTION 1 : Device inventory ────────────────────────────────────── */
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    out << "  SECTION 1 – Device Inventory\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    size_t dev_count = 0;
    try {
        device::List<device::VideoDevice> vDevices =
            device::Host::getInstance().getVideoDevices();
        dev_count = vDevices.size();
        out << "Total devices : " << dev_count << "\n\n";
        for (size_t i = 0; i < dev_count; i++)
            out << "  [" << i << "] VideoDevice id=" << i << "\n";
    }
    catch (const device::Exception &e) {
        out << "  EXCEPTION getting device list: " << e.what() << "\n";
        any_fail = true;
    }
    catch (...) {
        out << "  EXCEPTION (unknown) getting device list\n";
        any_fail = true;
    }

    /* ── SECTION 2 : Device[0] – every getter ────────────────────────────── */
    out << "\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    out << "  SECTION 2 – VideoDevice[0] : ALL Getter APIs\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    try {
        device::List<device::VideoDevice> vDevices =
            device::Host::getInstance().getVideoDevices();
        if (vDevices.size() == 0) {
            out << "  No VideoDevice instances on this platform – skipping Section 2.\n";
        } else {
            device::VideoDevice &dev = vDevices.at(0);
            out << "Device index  : 0\n\n";

            /* ── Feature: DFC (Decoder Format Conversion / Zoom) ────────── */
            out << "── Feature: DFC / Zoom ───────────────────────────────────────\n";

            try { const device::VideoDFC &v = dev.getDFC();
                  out << "  getDFC()                       : " << v.getName()
                      << "  (id=" << v.getId() << ")\n";
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  getDFC()                       : EXCEPTION – " << e.what() << " *** STALE ***\n";
                  ++get_fail; any_fail = true; }

            /* getSupportedDFCs – reads static config, no IARM handle */
            try { const device::List<device::VideoDFC> dfcs = dev.getSupportedDFCs();
                  out << "  getSupportedDFCs()             : " << dfcs.size() << " modes";
                  for (size_t d = 0; d < dfcs.size(); ++d)
                      out << "  [" << dfcs.at(d).getName() << "]";
                  out << "\n  (static config – survives restart)\n";
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  getSupportedDFCs()             : EXCEPTION – " << e.what() << "\n";
                  ++get_fail; any_fail = true; }

            /* ── Feature: HDR Capabilities ──────────────────────────────── */
            out << "\n── Feature: HDR Capabilities ─────────────────────────────────\n";

            try { int caps = 0; dev.getHDRCapabilities(&caps);
                  out << "  getHDRCapabilities()           : 0x" << std::hex << caps << std::dec
                      << "  (dsHDRStandard_t bitmask)\n";
                  if (caps & 0x01) out << "    bit0 = HDR10\n";
                  if (caps & 0x02) out << "    bit1 = HLG\n";
                  if (caps & 0x04) out << "    bit2 = DolbyVision\n";
                  if (caps & 0x08) out << "    bit3 = TechnicolorPrime\n";
                  if (caps & 0x10) out << "    bit4 = HDR10PLUS\n";
                  if (caps & 0x20) out << "    bit5 = SDR\n";
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  getHDRCapabilities()           : EXCEPTION – " << e.what() << " *** STALE ***\n";
                  ++get_fail; any_fail = true; }

            /* ── Feature: Video Coding Formats ──────────────────────────── */
            out << "\n── Feature: Video Coding Formats ─────────────────────────────\n";

            unsigned int fmt_mask = 0;
            try { fmt_mask = dev.getSupportedVideoCodingFormats();
                  out << "  getSupportedVideoCodingFormats : 0x" << std::hex << fmt_mask << std::dec
                      << "  (dsVideoCodingFormat_t bitmask)\n";
                  if (fmt_mask & 0x01) out << "    bit0 = MPEG2\n";
                  if (fmt_mask & 0x02) out << "    bit1 = MPEG4\n";
                  if (fmt_mask & 0x04) out << "    bit2 = MPEG4_P2\n";
                  if (fmt_mask & 0x08) out << "    bit3 = H264\n";
                  if (fmt_mask & 0x10) out << "    bit4 = H265 (HEVC)\n";
                  if (fmt_mask & 0x20) out << "    bit5 = VP8\n";
                  if (fmt_mask & 0x40) out << "    bit6 = VP9\n";
                  if (fmt_mask & 0x80) out << "    bit7 = AV1\n";
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  getSupportedVideoCodingFormats : EXCEPTION – " << e.what() << " *** STALE ***\n";
                  ++get_fail; any_fail = true; }

            /* getVideoCodecInfo – per supported format */
            if (fmt_mask != 0) {
                static const struct { unsigned int bit; dsVideoCodingFormat_t fmt; const char *name; } kFmts[] = {
                    { 0x01, dsVIDEO_CODEC_MPEGHPART2,  "HEVC/H265" },
                    { 0x02, dsVIDEO_CODEC_MPEG4PART10, "H264/AVC"  },
                    { 0x04, dsVIDEO_CODEC_MPEG2,       "MPEG2"     },
                };
                for (size_t k = 0; k < sizeof(kFmts)/sizeof(kFmts[0]); ++k) {
                    if (!(fmt_mask & kFmts[k].bit)) continue;
                    try { dsVideoCodecInfo_t info = dev.getVideoCodecInfo(kFmts[k].fmt);
                          out << "  getVideoCodecInfo(" << kFmts[k].name << ")"
                              << std::string(10 - strlen(kFmts[k].name), ' ')
                              << ": num_entries=" << info.num_entries;
                          for (int e = 0; e < info.num_entries && e < 4; ++e)
                              out << "  profile[" << e << "]=" << info.entries[e].profile;
                          out << "\n";
                          ++get_pass; }
                    catch (const device::Exception &ex) {
                          out << "  getVideoCodecInfo(" << kFmts[k].name << ")"
                              << std::string(10 - strlen(kFmts[k].name), ' ')
                              << ": EXCEPTION – " << ex.what() << " *** STALE ***\n";
                          ++get_fail; any_fail = true; }
                }
            }

            /* ── Feature: Supported STB Resolutions (static) ────────────── */
            out << "\n── Feature: STB Supported Resolutions (static config) ────────\n";

            try { std::list<std::string> stbRes;
                  dev.getSettopSupportedResolutions(stbRes);
                  out << "  getSettopSupportedResolutions  : " << stbRes.size() << " entries";
                  int n = 0;
                  for (const std::string &r : stbRes) {
                      if (n % 4 == 0) out << "\n    ";
                      out << "[" << r << "] ";
                      ++n;
                  }
                  out << "\n  (static config – survives restart)\n";
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  getSettopSupportedResolutions  : EXCEPTION – " << e.what() << "\n";
                  ++get_fail; any_fail = true; }

            /* ── Feature: Frame Rate / Display Framerate ─────────────────── */
            out << "\n── Feature: Frame Rate / Display Framerate ───────────────────\n";

            try { int frfmode = 0;
                  dev.getFRFMode(&frfmode);
                  out << "  getFRFMode()                   : " << frfmode
                      << "  (0=disabled, 1=enabled)\n";
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  getFRFMode()                   : EXCEPTION – " << e.what() << " *** STALE ***\n";
                  ++get_fail; any_fail = true; }

            try { char framerate[20] = {0};
                  dev.getCurrentDisframerate(framerate);
                  out << "  getCurrentDisframerate()       : \"" << framerate << "\"\n";
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "  getCurrentDisframerate()       : EXCEPTION – " << e.what() << " *** STALE ***\n";
                  ++get_fail; any_fail = true; }
        }
    }
    catch (const device::Exception &e) {
        out << "\nEXCEPTION accessing VideoDevice[0]: " << e.what() << "\n";
        any_fail = true;
    }
    catch (...) {
        out << "\nEXCEPTION (unknown) accessing VideoDevice[0]\n";
        any_fail = true;
    }

    /* ── SECTION 3 : Result summary ──────────────────────────────────────── */
    out << "\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    out << "  SECTION 3 – Result Summary\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    out << "  Getters called  : " << (get_pass + get_fail) << "\n";
    out << "  Getters OK      : " << get_pass << "\n";
    out << "  Getters FAILED  : " << get_fail << "\n\n";
    if (!any_fail) {
        out << "  RESULT: PASS – all VideoDevice getters returned values OK\n";
        out << "  (This is the expected BEFORE-restart baseline)\n";
    } else {
        out << "  RESULT: FAIL – " << get_fail << " getter(s) threw exceptions\n";
        out << "  (This is the expected AFTER-restart stale-handle result)\n";
        out << "  -> Use [H] C++ Handle Sync or [I] Full Re-Init to recover\n";
    }
    verify_record("TC-VD-01",
                  "VideoDevice all-getters snapshot",
                  !any_fail,
                  "getters_ok=" + std::to_string(get_pass) +
                  " getters_fail=" + std::to_string(get_fail));
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
 * TC-VP-03: INTERNAL0 (TV) – ALL VideoPort + Display getter APIs snapshot
 *
 *   PURPOSE:
 *   Call every VideoOutputPort getter on the INTERNAL0 port (TV panel,
 *   dsVIDEOPORT_TYPE_INTERNAL) and record the result.
 *
 *   RUN BEFORE dsmgr restart  →  all handle-based getters print values  →  RESULT=PASS
 *   RUN AFTER  dsmgr restart  →  every handle-based call throws device::Exception
 *                                because intptr_t _handle is stale  →  RESULT=FAIL
 *
 *   NOTE: HDCP and EDID getters are called but will throw on INTERNAL0
 *   (no HDCP on panel connection) – logged as N/A, not counted as fail.
 *
 *   Section 1 : Port inventory         – all video ports + connection status
 *   Section 2 : INTERNAL0 getters      – all VideoPort + Display APIs
 *   Section 3 : Result summary
 *
 *   Output file: 8_VideoPort/INTERNAL0_all_getters.txt
 * ========================================================================= */
static void tc_vp_03_internal_all_getters(const std::string &mod_dir)
{
    printf("\n  [TC-VP-03] INTERNAL0 – ALL VideoPort+Display getter APIs snapshot (TV only)\n");

    /* TV-only port: skip on STB profile */
    if (g_profile == PROFILE_STB) {
        printf("  SKIPPED – INTERNAL0 is a TV panel port, not present on STB profile.\n");
        verify_record("TC-VP-03", "INTERNAL0 all-getters snapshot", true,
                      "SKIPPED (STB profile – no INTERNAL port)");
        return;
    }

    std::string fname = mod_dir + "/INTERNAL0_all_getters.txt";
    TeeStream out;
    out.open(fname);

    out << "╔══════════════════════════════════════════════════════════════════╗\n";
    out << "║  TC-VP-03 : INTERNAL0 (TV) – ALL VideoPort+Display Getter APIs   ║\n";
    out << "║  Run BEFORE and AFTER dsmgr restart to compare output.          ║\n";
    out << "╚══════════════════════════════════════════════════════════════════╝\n";
    out << "Timestamp   : " << timestamp_now() << "\n";
    out << "Run number  : " << g_run_count << "\n";
    out << "Profile     : " << (g_profile == PROFILE_TV ? "TV" : "All") << "\n";
    out << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n";
    out << "DS_Init     : " << (g_ds_initialized   ? "YES (may be stale after restart)" : "NO") << "\n";
    out << "Stale risk  : " << (g_run_count > 1 && g_ds_initialized
                                ? "YES – intptr_t _handle from RUN-01 still in use"
                                : "No (first run or re-initialized)") << "\n\n";
    out << "STALE HANDLE NOTE:\n";
    out << "  VideoOutputPort._handle = intptr_t assigned at Manager::Initialize().\n";
    out << "  After dsmgr kill+restart, dsmgr allocates NEW server-side pointers.\n";
    out << "  The C++ wrapper still holds the OLD pointer  →  dsERR_INVALID_PARAM\n";
    out << "  →  device::Exception thrown on EVERY handle-based IARM RPC call.\n";
    out << "  NOTE: HDCP/EDID APIs on INTERNAL0 may throw even on first run (no panel HDCP).\n\n";

    bool any_fail  = false;
    int  get_pass  = 0, get_fail = 0;

    /* ── SECTION 1 : Port inventory ──────────────────────────────────────── */
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    out << "  SECTION 1 – Port Inventory (all video output ports)\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    try {
        device::List<device::VideoOutputPort> vPorts =
            device::Host::getInstance().getVideoOutputPorts();
        out << "Total ports : " << vPorts.size() << "\n\n";
        for (size_t i = 0; i < vPorts.size(); i++) {
            device::VideoOutputPort &p = vPorts.at(i);
            out << "  [" << i << "] " << p.getName();
            try {
                bool en = p.isEnabled();
                bool co = p.isDisplayConnected();
                out << "  enabled=" << (en ? "Y" : "N")
                    << "  display_connected=" << (co ? "Y" : "N");
            } catch (...) {
                out << "  *** STALE HANDLE – isEnabled/isDisplayConnected threw exception ***";
                any_fail = true;
            }
            out << "\n";
        }
    }
    catch (const device::Exception &e) {
        out << "  EXCEPTION getting port list: " << e.what() << "\n";
        any_fail = true;
    }
    catch (...) {
        out << "  EXCEPTION (unknown) getting port list\n";
        any_fail = true;
    }

    /* ── SECTION 2 : INTERNAL0 – every getter ─────────────────────────────── */
    out << "\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    out << "  SECTION 2 – INTERNAL0 : ALL Getter APIs\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    try {
        device::VideoOutputPort &vp =
            device::Host::getInstance().getVideoOutputPort(INTERNAL_PORT_NAME);
        out << "Port handle : " << vp.getName() << "\n\n";

        /* ── Feature: Port Management ─────────────────────────────────── */
        out << "── Feature: Port Management ──────────────────────────────────\n";

        try { bool v = vp.isEnabled();
              out << "  isEnabled()                   : " << (v ? "Yes" : "No") << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  isEnabled()                   : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        try { bool v = vp.isDisplayConnected();
              out << "  isDisplayConnected()          : " << (v ? "Yes" : "No") << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  isDisplayConnected()          : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        try { bool v = vp.isActive();
              out << "  isActive()                    : " << (v ? "Yes" : "No") << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  isActive()                    : EXCEPTION – " << e.what() << "\n";
              ++get_fail; any_fail = true; }

        try { bool v = vp.isDynamicResolutionSupported();
              out << "  isDynamicResolutionSupported(): " << (v ? "Yes" : "No") << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  isDynamicResolutionSupported(): EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        try { bool v = vp.isContentProtected();
              out << "  isContentProtected()          : " << (v ? "Yes" : "No") << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  isContentProtected()          : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        /* ── Feature: Type Info (static – no handle needed) ──────────── */
        out << "\n── Feature: Type Info (static config, no handle) ─────────────\n";

        try { const device::VideoOutputPortType &t = vp.getType();
              out << "  getType().getName()           : " << t.getName() << "\n";
              out << "  getType().isHDCPSupported()   : " << (t.isHDCPSupported() ? "Yes" : "No") << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getType()                     : EXCEPTION – " << e.what() << "\n";
              ++get_fail; any_fail = true; }

        /* ── Feature: Resolution ──────────────────────────────────────── */
        out << "\n── Feature: Resolution ───────────────────────────────────────\n";

        try { const device::VideoResolution &r = vp.getResolution();
              out << "  getResolution().getName()     : " << r.getName() << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getResolution()               : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        try { const device::VideoResolution &r = vp.getDefaultResolution();
              out << "  getDefaultResolution().name   : " << r.getName() << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getDefaultResolution()        : EXCEPTION – " << e.what() << "\n";
              ++get_fail; any_fail = true; }

        /* ── Feature: Output Color/HDR Settings ───────────────────────── */
        out << "\n── Feature: Output Color / HDR Settings ──────────────────────\n";

        try { int v = vp.getVideoEOTF();
              out << "  getVideoEOTF()                : " << v
                  << "  (dsHDRStandard_t)\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getVideoEOTF()                : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        try { int v = vp.getMatrixCoefficients();
              out << "  getMatrixCoefficients()       : " << v << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getMatrixCoefficients()       : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        try { int v = vp.getColorDepth();
              out << "  getColorDepth()               : " << v
                  << "  (bits per component)\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getColorDepth()               : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        try { int v = vp.getColorSpace();
              out << "  getColorSpace()               : " << v
                  << "  (dsDisplayColorSpace_t)\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getColorSpace()               : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        try { int v = vp.getQuantizationRange();
              out << "  getQuantizationRange()        : " << v
                  << "  (dsDisplayQuantizationRange_t)\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getQuantizationRange()        : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        try { int eotf=0, mc=0, cs=0, cd=0, qr=0;
              vp.getCurrentOutputSettings(eotf, mc, cs, cd, qr);
              out << "  getCurrentOutputSettings()    :\n";
              out << "    EOTF=" << eotf << "  MatrixCoeff=" << mc
                  << "  ColorSpace=" << cs
                  << "  ColorDepth=" << cd
                  << "  QuantRange=" << qr << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getCurrentOutputSettings()    : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        try { unsigned int v = vp.getPreferredColorDepth();
              out << "  getPreferredColorDepth()      : " << v << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getPreferredColorDepth()      : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        try { unsigned int caps = 0; vp.getColorDepthCapabilities(&caps);
              out << "  getColorDepthCapabilities()   : 0x" << std::hex << caps << std::dec
                  << "  (bitmask)\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getColorDepthCapabilities()   : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        try { bool v = vp.IsOutputHDR();
              out << "  IsOutputHDR()                 : " << (v ? "Yes (HDR active)" : "No (SDR)") << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  IsOutputHDR()                 : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        try { int caps = 0; vp.getTVHDRCapabilities(&caps);
              out << "  getTVHDRCapabilities()        : 0x" << std::hex << caps << std::dec
                  << "  (dsHDRStandard_t bitmask)\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getTVHDRCapabilities()        : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        try { int res = 0; vp.getSupportedTvResolutions(&res);
              out << "  getSupportedTvResolutions()   : " << decode_tv_resolutions(res) << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getSupportedTvResolutions()   : EXCEPTION – " << e.what() << " *** STALE ***\n";
              ++get_fail; any_fail = true; }

        /* ── Feature: HDCP (N/A on INTERNAL panel – logged informational) ── */
        out << "\n── Feature: HDCP (N/A for INTERNAL panel port – informational) ──\n";

        try { int v = vp.getHDCPStatus();
              out << "  getHDCPStatus()               : " << v
                  << "  (dsHdcpStatus_t)\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getHDCPStatus()               : EXCEPTION – " << e.what()
                  << " (expected N/A on INTERNAL port)\n"; }

        try { int v = vp.getHDCPProtocol();
              out << "  getHDCPProtocol()             : " << v
                  << "  (dsHdcpProtocolVersion_t)\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getHDCPProtocol()             : EXCEPTION – " << e.what()
                  << " (expected N/A on INTERNAL port)\n"; }

        try { int v = vp.getHDCPReceiverProtocol();
              out << "  getHDCPReceiverProtocol()     : " << v << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getHDCPReceiverProtocol()     : EXCEPTION – " << e.what()
                  << " (expected N/A on INTERNAL port)\n"; }

        try { int v = vp.getHDCPCurrentProtocol();
              out << "  getHDCPCurrentProtocol()      : " << v << "\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  getHDCPCurrentProtocol()      : EXCEPTION – " << e.what()
                  << " (expected N/A on INTERNAL port)\n"; }

        try { int v = vp.GetHdmiPreference();
              out << "  GetHdmiPreference()           : " << v
                  << "  (dsHdcpProtocolVersion_t)\n";
              ++get_pass; }
        catch (const device::Exception &e) {
              out << "  GetHdmiPreference()           : EXCEPTION – " << e.what()
                  << " (expected N/A on INTERNAL port)\n"; }

        /* ── Feature: Display (nested object via getDisplay()) ────────── */
        out << "\n── Feature: Display (nested VideoOutputPort::Display object) ──\n";

        try {
            const device::VideoOutputPort::Display &disp = vp.getDisplay();
            out << "  getDisplay() – ok, querying Display sub-object:\n";
            ++get_pass;

            try { bool v = disp.hasSurround();
                  out << "    hasSurround()             : " << (v ? "Yes" : "No") << "\n";
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "    hasSurround()             : EXCEPTION – " << e.what() << " *** STALE ***\n";
                  ++get_fail; any_fail = true; }

            try { int v = disp.getSurroundMode();
                  out << "    getSurroundMode()         : " << v
                      << "  (dsSURROUND_t bitmask: bit0=DD, bit1=DD+)\n";
                  ++get_pass; }
            catch (const device::Exception &e) {
                  out << "    getSurroundMode()         : EXCEPTION – " << e.what() << " *** STALE ***\n";
                  ++get_fail; any_fail = true; }

            try {
                std::vector<uint8_t> edid;
                disp.getEDIDBytes(edid);
                out << "    getEDIDBytes()            : " << edid.size() << " bytes retrieved\n";
                if (!edid.empty()) {
                    out << "      EDID[0..7]            :";
                    for (size_t j = 0; j < 8 && j < edid.size(); ++j)
                        out << " 0x" << std::hex
                            << std::setw(2) << std::setfill('0') << (int)edid[j];
                    out << std::dec << std::setfill(' ') << "\n";
                }
                ++get_pass;
            }
            catch (const device::Exception &e) {
                out << "    getEDIDBytes()            : EXCEPTION – " << e.what()
                    << " (expected N/A on INTERNAL panel – no external EDID)\n";
            }
        }
        catch (const device::Exception &e) {
            out << "  getDisplay()                  : EXCEPTION – " << e.what() << " *** STALE ***\n";
            ++get_fail; any_fail = true;
        }
    }
    catch (const device::Exception &e) {
        out << "\nEXCEPTION getting INTERNAL0 port object: " << e.what() << "\n";
        out << "  -> INTERNAL0 not present on this platform, or DS not initialized.\n";
        any_fail = true;
    }
    catch (...) {
        out << "\nEXCEPTION (unknown) getting INTERNAL0 port object\n";
        any_fail = true;
    }

    /* ── SECTION 3 : Result summary ──────────────────────────────────────── */
    out << "\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    out << "  SECTION 3 – Result Summary\n";
    out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    out << "  Getters called  : " << (get_pass + get_fail) << "\n";
    out << "  Getters OK      : " << get_pass << "\n";
    out << "  Getters FAILED  : " << get_fail << "\n";
    out << "  (HDCP/EDID N/A exceptions on INTERNAL port are not counted as failures)\n\n";
    if (!any_fail) {
        out << "  RESULT: PASS – all INTERNAL0 getters returned values OK\n";
        out << "  (This is the expected BEFORE-restart baseline)\n";
    } else {
        out << "  RESULT: FAIL – " << get_fail << " getter(s) threw exceptions\n";
        out << "  (This is the expected AFTER-restart stale-handle result)\n";
        out << "  -> Use [H] C++ Handle Sync or [I] Full Re-Init to recover\n";
    }
    verify_record("TC-VP-03",
                  "INTERNAL0 all-getters snapshot (TV)",
                  !any_fail,
                  "getters_ok=" + std::to_string(get_pass) +
                  " getters_fail=" + std::to_string(get_fail));
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
    summary << "  TC-AUD-02  setMuted round-trip (toggle+restore) on first audio port\n";
    summary << "  TC-AUD-03  setLevel round-trip (set 50.0 + restore) on SPEAKER0\n";
    summary << "  TC-AUD-04  setGain round-trip (set -3.0 dB + restore) on SPEAKER0\n";
    summary << "  TC-AUD-05  getLevelAndMute all-port read (getLevel + isMuted)\n";
    summary << "  TC-AUD-06  stereoMode+stereoAuto round-trip\n";
    summary << "  TC-AUD-07  audioDelay round-trip (getAudioDelay/setAudioDelay)\n";
    summary << "  TC-AUD-08  MS12 Dolby getters snapshot (read-only)\n";
    summary << "  TC-AUD-09  MS12 setters round-trip (DolbyVol/Bass/DRC)\n";
    summary << "  TC-AUD-10  audio capabilities snapshot (MSD/MS12/bitmask)\n";
    summary << "  TC-AUD-11  ATMOS getSinkCap + setAudioAtmosOutputMode\n";
    summary << "  TC-AUD-12  ARC/LE Config info (getSupportedARCTypes/getHdmiArcPortId/GetLEConfig)\n";
    summary << "  TC-AUD-13  Language/Mixing round-trip (AssocMix/Fader/PrimaryLang/SecLang)\n";
    summary << "  TC-AUD-14  SPDIF0     all-getters snapshot\n";
    summary << "  TC-AUD-15  HDMI_ARC0  all-getters snapshot\n";
    summary << "  TC-AUD-16  HEADPHONE0 all-getters snapshot\n\n";

    tc_aud_01_get_ports(mod_dir);
    tc_aud_02_set_muted(mod_dir);
    tc_aud_03_set_level(mod_dir);
    tc_aud_04_set_gain(mod_dir);
    tc_aud_05_get_level_mute_all(mod_dir);
    tc_aud_06_stereo_mode(mod_dir);
    tc_aud_07_audio_delay(mod_dir);
    tc_aud_08_ms12_getters(mod_dir);
    tc_aud_09_ms12_setters(mod_dir);
    tc_aud_10_capabilities(mod_dir);
    tc_aud_11_atmos(mod_dir);
    tc_aud_12_arc_le(mod_dir);
    tc_aud_13_language_mixing(mod_dir);
    tc_aud_14_spdif0_all_getters(mod_dir);
    tc_aud_15_hdmi_arc0_all_getters(mod_dir);
    tc_aud_16_headphone0_all_getters(mod_dir);

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
    summary << "  TC-DSP-01  Display all-getter snapshot (VOP: name/id/isEnabled/isActive/isConnected/isContentProtected/\n";
    summary << "             isDynamicResolution/getResolution/getDefaultResolution/getType/getVideoEOTF/\n";
    summary << "             getMatrixCoefficients/getColorSpace/getColorDepth/getQuantizationRange/\n";
    summary << "             getCurrentOutputSettings/getPreferredColorDepth/getColorDepthCapabilities/\n";
    summary << "             IsOutputHDR/getTVHDRCapabilities/getSupportedTvResolutions/HDCP group/GetHdmiPreference\n";
    summary << "             + Display: hasSurround/getSurroundMode/getProductCode/getSerialNumber/\n";
    summary << "             getManufacturerYear/getManufacturerWeek/getConnectedDeviceType/\n";
    summary << "             isConnectedDeviceRepeater/getAspectRatio/getPhysicallAddress/getEDIDBytes)\n";
    summary << "  TC-DSP-02  getEDIDBytes standalone raw file\n\n";

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
    summary << "  TC-HIN-01  HdmiInput all-getters snapshot (global: numInputs/isPresented/activePort/videoMode/gameFeatures/AVLatency + per-port status)\n";
    summary << "  TC-HIN-02  HdmiInput per-port detail getters (EDID/SPD/EdidVer/VRR/ALLM/HdmiVersion)\n\n";

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
    summary << "  TC-HST-01  Host all-getter snapshot (getCPUTemperature/getVersion/getSocIDFromSDK/\n";
    summary << "             getPowerMode/getPreferredSleepMode/getAvailableSleepModes/\n";
    summary << "             isHDMIOutPortPresent/getDefaultVideoPortName/getDefaultAudioPortName/\n";
    summary << "             getAudioOutputPorts/getVideoOutputPorts/getVideoDevices/getHostEDID/\n";
    summary << "             getSinkDeviceAtmosCapability/getAssociatedAudioMixing/getFaderControl/\n";
    summary << "             getPrimaryLanguage/getSecondaryLanguage/getCurrentAudioFormat/getMS12ConfigDetails)\n";
    summary << "  TC-HST-02  sleep modes + port counts (backward compat)\n\n";

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
    summary << "  TC-VD-01  VideoDevice all-getters snapshot (DFC/HDRCaps/codecs/FRF/framerate)\n";
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
    summary << "  TC-VP-03  INTERNAL0 (TV) all-getters snapshot (VideoPort+Display)\n";
    summary << "\n";

    /* ── Execute test cases ────────────────────────────── */
    tc_vp_01_get_port_status(mod_dir);
    tc_vp_02_set_resolution(mod_dir);
    tc_vp_03_internal_all_getters(mod_dir);

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
 * TC submenu – Audio module: individual test case selection
 *
 *   print_audio_tc_menu()      – render the Audio TC list
 *   handle_audio_tc_submenu()  – input loop; each choice runs ONE TC
 *                                 (journalctl + run-dir created per run)
 * ========================================================================= */

/* -------------------------------------------------------------------------
 * start_tc_run() – shared helper used by ALL per-module TC sub-menus.
 *
 *   Increments g_run_count, clears g_verify_results, creates the run
 *   directory via get_run_dir(), writes run_info.txt, and prints the
 *   run-start banner.  Returns the new run-directory path.
 * ------------------------------------------------------------------------- */
static std::string start_tc_run(const char *tc_label)
{
    ++g_run_count;
    g_verify_results.clear();
    printf("\n\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
    printf("  Starting RUN %02d (%s)  \u2013  %s\n",
           g_run_count, tc_label, timestamp_now().c_str());
    if (g_run_count > 1 && g_ds_initialized)
        printf("  \u26a0 STALE HANDLE: intptr_t _handle from RUN01 in use \u2013 API calls WILL FAIL.\n");
    printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
    std::string rd = get_run_dir(g_run_count);
    std::ofstream m((rd + "/run_info.txt").c_str());
    if (m.is_open()) {
        m << "Run number  : " << g_run_count << "\n"
          << "Timestamp   : " << timestamp_now() << "\n"
          << "TC          : " << tc_label << "\n"
          << "IARM_Init   : " << (g_iarm_initialized ? "YES" : "NO") << "\n"
          << "DS_Init     : " << (g_ds_initialized   ? "YES" : "NO") << "\n"
          << "Stale risk  : "
          << (g_run_count > 1 && g_ds_initialized ? "YES (handles stale)" : "No") << "\n";
        m.close();
    }
    return rd;
}

static void print_audio_tc_menu(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║       Module 1 – Audio  ▸  Test Case Selection              ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  READ (getter snapshot – safe, no audio side-effects)       ║\n");
    printf("║  [1] TC-AUD-01  SPEAKER0 all getters                        ║\n");
    printf("║  [5] TC-AUD-05  getLevelAndMute     (all ports)             ║\n");
    printf("║  [8] TC-AUD-08  MS12 Dolby getters snapshot                 ║\n");
    printf("║  [C] TC-AUD-10  Audio capabilities (MSD/MS12/bitmask)       ║\n");
    printf("║  [E] TC-AUD-12  ARC/LE Config info                          ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  Per-Port ALL-getter snapshots (same 34 getters per port)   ║\n");
    printf("║  [1] TC-AUD-01  SPEAKER0   all getters                      ║\n");
    printf("║  [G] TC-AUD-14  SPDIF0     all getters                      ║\n");
    printf("║  [H] TC-AUD-15  HDMI_ARC0  all getters                      ║\n");
    printf("║  [J] TC-AUD-16  HEADPHONE0 all getters                      ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  READ+WRITE (round-trip: set → verify → restore)            ║\n");
    printf("║  [2] TC-AUD-02  setMuted         toggle+restore (SPEAKER0)  ║\n");
    printf("║  [3] TC-AUD-03  setLevel         50.0+restore   (SPEAKER0)  ║\n");
    printf("║  [4] TC-AUD-04  setGain          -3.0dB+restore (SPEAKER0)  ║\n");
    printf("║  [6] TC-AUD-06  Stereo Mode+Auto round-trip                 ║\n");
    printf("║  [7] TC-AUD-07  Audio Delay      round-trip                 ║\n");
    printf("║  [9] TC-AUD-09  MS12 setters     DolbyVol/Bass/DRC          ║\n");
    printf("║  [D] TC-AUD-11  ATMOS            getSinkCap+setOutputMode   ║\n");
    printf("║  [F] TC-AUD-13  Language/Mixing  AssocMix/Fader/Lang        ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  [A] Run ALL audio TCs (TC-AUD-01..13 + AUD-14..16)         ║\n");
    printf("║  [B] Back to TC module menu                                 ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("Choice: ");
    fflush(stdout);
}

static void handle_audio_tc_submenu(void)
{
    char input[16];
    bool in_menu = true;

    while (in_menu) {
        print_audio_tc_menu();

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        char ch = input[0];

        switch (ch) {
        case '1':
        {
            std::string rd      = start_tc_run("TC-AUD-01");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_01_get_ports(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case '2':
        {
            std::string rd      = start_tc_run("TC-AUD-02");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_02_set_muted(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case '3':
        {
            std::string rd      = start_tc_run("TC-AUD-03");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_03_set_level(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case '4':
        {
            std::string rd      = start_tc_run("TC-AUD-04");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_04_set_gain(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case '5':
        {
            std::string rd      = start_tc_run("TC-AUD-05");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_05_get_level_mute_all(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case '6':
        {
            std::string rd      = start_tc_run("TC-AUD-06");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_06_stereo_mode(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case '7':
        {
            std::string rd      = start_tc_run("TC-AUD-07");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_07_audio_delay(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case '8':
        {
            std::string rd      = start_tc_run("TC-AUD-08");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_08_ms12_getters(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case '9':
        {
            std::string rd      = start_tc_run("TC-AUD-09");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_09_ms12_setters(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case 'C': case 'c':
        {
            std::string rd      = start_tc_run("TC-AUD-10");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_10_capabilities(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case 'D': case 'd':
        {
            std::string rd      = start_tc_run("TC-AUD-11");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_11_atmos(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case 'E': case 'e':
        {
            std::string rd      = start_tc_run("TC-AUD-12");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_12_arc_le(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case 'F': case 'f':
        {
            std::string rd      = start_tc_run("TC-AUD-13");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_13_language_mixing(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case 'G': case 'g':
        {
            std::string rd      = start_tc_run("TC-AUD-14");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_14_spdif0_all_getters(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case 'H': case 'h':
        {
            std::string rd      = start_tc_run("TC-AUD-15");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_15_hdmi_arc0_all_getters(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case 'J': case 'j':
        {
            std::string rd      = start_tc_run("TC-AUD-16");
            std::string mod_dir = get_module_dir(rd, "1_Audio");
            g_jctl_pid = journalctl_start(rd);
            tc_aud_16_headphone0_all_getters(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case 'A': case 'a':
        {
            std::string rd = start_tc_run("TC-AUD-01..16 (ALL)");
            g_jctl_pid = journalctl_start(rd);
            run_module_audio(rd);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }
        case 'B': case 'b':
            in_menu = false;
            break;
        case '\n': case '\r':
            break;
        default:
            printf("  Unknown choice '%c'. Please select from the menu.\n", ch);
            break;
        }
    }
}

/* =========================================================================
 * TC menu – top-level module selector for per-TC runs
 *
 *   Presents the 8 DS modules; selecting [1] (Audio) drops into
 *   handle_audio_tc_submenu().  Other modules print a placeholder until
 *   their TC submenus are implemented.
 * ========================================================================= */
static void print_tc_module_menu(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║       TC Menu  –  Select Module                             ║\n");
    printf("║       Run individual test cases without a full module run   ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  [1] Audio         TC-AUD-01 … TC-AUD-13  (13 TCs) ◄ ACTIVE║\n");
    printf("║  [2] CompositeIn   TC-CIN-01 … TC-CIN-02  (2 TCs) ◄ ACTIVE ║\n");
    printf("║  [3] Display       TC-DSP-01 … TC-DSP-02  (2 TCs) ◄ ACTIVE ║\n");
    printf("║  [4] FPD           TC-FPD-01 … TC-FPD-07  (7 TCs) ◄ ACTIVE ║\n");
    printf("║  [5] HDMIIn        TC-HIN-01 … TC-HIN-02  (2 TCs) ◄ ACTIVE ║\n");
    printf("║  [6] Host          TC-HST-01 … TC-HST-02  (2 TCs) ◄ ACTIVE ║\n");
    printf("║  [7] VideoDevice   TC-VD-01  … TC-VD-02   (2 TCs) ◄ ACTIVE  ║\n");
    printf("║  [8] VideoPort     TC-VP-01  … TC-VP-03  (3 TCs) ◄ ACTIVE  ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  [B] Back to main menu                                      ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("Choice: ");
    fflush(stdout);
}

static void print_hin_tc_menu(void)
{
    printf("\n");
    printf("\u2554\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2557\n");
    printf("\u2551       Module 5 \u2013 HDMIIn  \u25b8  Test Case Selection                  \u2551\n");
    printf("\u2560\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2563\n");
    printf("\u2551  READ (getter snapshot \u2013 all reads, no side-effects)              \u2551\n");
    printf("\u2551  [1] TC-HIN-01  Global getters snapshot                          \u2551\n");
    printf("\u2551       getNumberOfInputs / isPresented / getActivePort             \u2551\n");
    printf("\u2551       getCurrentVideoMode / getSupportedGameFeatures / getAVLatency\u2551\n");
    printf("\u2551       + per-port: isPortConnected / isActivePort (all ports)      \u2551\n");
    printf("\u2560\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2563\n");
    printf("\u2551  [2] TC-HIN-02  Per-port detail getters (all ports)               \u2551\n");
    printf("\u2551       getEDIDBytesInfo / getEdidVersion / getEdid2AllmSupport      \u2551\n");
    printf("\u2551       getHDMISPDInfo / getVRRSupport / getVRRStatus                \u2551\n");
    printf("\u2551       getHdmiALLMStatus / getHdmiVersion                          \u2551\n");
    printf("\u2560\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2563\n");
    printf("\u2551  [A] Run ALL HDMIIn TCs (TC-HIN-01 \u2026 TC-HIN-02)                  \u2551\n");
    printf("\u2551  [B] Back to TC module menu                                      \u2551\n");
    printf("\u255a\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u255d\n");
    printf("Choice: ");
    fflush(stdout);
}

static void handle_hin_tc_submenu(void)
{
    char input[16];
    bool in_menu = true;

    while (in_menu) {
        print_hin_tc_menu();

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        char ch = input[0];

        switch (ch) {
        case '1':
        {
            std::string rd      = start_tc_run("TC-HIN-01");
            std::string mod_dir = get_module_dir(rd, "5_HDMIIn");
            g_jctl_pid = journalctl_start(rd);
            tc_hin_01_get_info(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            break;
        }
        case '2':
        {
            std::string rd      = start_tc_run("TC-HIN-02");
            std::string mod_dir = get_module_dir(rd, "5_HDMIIn");
            g_jctl_pid = journalctl_start(rd);
            tc_hin_02_port_connected(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            break;
        }
        case 'A': case 'a':
        {
            std::string rd = start_tc_run("TC-HIN-01..02 (ALL)");
            std::string mod_dir = get_module_dir(rd, "5_HDMIIn");
            g_jctl_pid = journalctl_start(rd);
            tc_hin_01_get_info(mod_dir);
            tc_hin_02_port_connected(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            break;
        }
        case 'B': case 'b':
            in_menu = false;
            break;
        case '\n': case '\r':
            break;
        default:
            printf("  Unknown choice '%c'. Please select from the menu.\n", ch);
            break;
        }
    }
}

static void print_vp_tc_menu(void)
{
    printf("\n");
    printf("\u2554\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2557\n");
    printf("\u2551       Module 8 \u2013 VideoPort  \u25b8  Test Case Selection              \u2551\n");
    printf("\u2560\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2563\n");
    printf("\u2551  READ (getter snapshot \u2013 no side-effects)                      \u2551\n");
    printf("\u2551  [1] TC-VP-01  getVideoOutputPorts (all ports snapshot)        \u2551\n");
    printf("\u2551  [3] TC-VP-03  INTERNAL0 all-getters snapshot  (TV only)       \u2551\n");
    printf("\u2560\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2563\n");
    printf("\u2551  READ+WRITE (round-trip: GET \u2192 SET-same \u2192 GET verify)         \u2551\n");
    printf("\u2551  [2] TC-VP-02  setResolution  HDMI0 round-trip (STB/TV)        \u2551\n");
    printf("\u2560\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2563\n");
    printf("\u2551  [A] Run ALL VideoPort TCs (TC-VP-01 \u2026 TC-VP-03)               \u2551\n");
    printf("\u2551  [B] Back to TC module menu                                    \u2551\n");
    printf("\u255a\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u255d\n");
    printf("Choice: ");
    fflush(stdout);
}

static void handle_vp_tc_submenu(void)
{
    char input[16];
    bool in_menu = true;

    while (in_menu) {
        print_vp_tc_menu();

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        char ch = input[0];

        switch (ch) {
        case '1':
        {
            std::string rd      = start_tc_run("TC-VP-01");
            std::string mod_dir = get_module_dir(rd, "8_VideoPort");
            g_jctl_pid = journalctl_start(rd);
            tc_vp_01_get_port_status(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            break;
        }
        case '2':
        {
            std::string rd      = start_tc_run("TC-VP-02");
            std::string mod_dir = get_module_dir(rd, "8_VideoPort");
            g_jctl_pid = journalctl_start(rd);
            tc_vp_02_set_resolution(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            break;
        }
        case '3':
        {
            std::string rd      = start_tc_run("TC-VP-03");
            std::string mod_dir = get_module_dir(rd, "8_VideoPort");
            g_jctl_pid = journalctl_start(rd);
            tc_vp_03_internal_all_getters(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            if (g_run_count == 1)
                printf("  \u2192 Kill dsmgr, select [3] again \u2013 expect VERIFY_FAIL on all handle getters.\n");
            printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            break;
        }
        case 'A': case 'a':
        {
            std::string rd = start_tc_run("TC-VP-01..03 (ALL)");
            std::string mod_dir = get_module_dir(rd, "8_VideoPort");
            g_jctl_pid = journalctl_start(rd);
            tc_vp_01_get_port_status(mod_dir);
            tc_vp_02_set_resolution(mod_dir);
            tc_vp_03_internal_all_getters(mod_dir);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
            break;
        }
        case 'B': case 'b':
            in_menu = false;
            break;
        case '\n': case '\r':
            break;
        default:
            printf("  Unknown choice '%c'. Please select from the menu.\n", ch);
            break;
        }
    }
}

/* ── Module 2 : CompositeIn ──────────────────────────────────────────────── */
static void print_cin_tc_menu(void)
{
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║       Module 2 – CompositeIn  ▸  Test Case Selection             ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  READ (getter snapshot – no side-effects)                      ║\n");
    printf("║  [1] TC-CIN-01  ALL getter snapshot                            ║\n");
    printf("║       getNumberOfInputs / isPresented / getActivePort           ║\n");
    printf("║       + per-port: isPortConnected / isActivePort                ║\n");
    printf("║  [2] TC-CIN-02  Per-port connected status                       ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  [A] Run ALL CompositeIn TCs (TC-CIN-01 … TC-CIN-02)            ║\n");
    printf("║  [B] Back to TC module menu                                    ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("Choice: ");
    fflush(stdout);
}

static void handle_cin_tc_submenu(void)
{
    char input[16];
    bool in_menu = true;
    while (in_menu) {
        print_cin_tc_menu();
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        char ch = input[0];
        switch (ch) {
        case '1': {
            std::string rd = start_tc_run("TC-CIN-01");
            std::string md = get_module_dir(rd, "2_CompositeIn");
            g_jctl_pid = journalctl_start(rd);
            tc_cin_01_get_info(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case '2': {
            std::string rd = start_tc_run("TC-CIN-02");
            std::string md = get_module_dir(rd, "2_CompositeIn");
            g_jctl_pid = journalctl_start(rd);
            tc_cin_02_port_connected(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case 'A': case 'a': {
            std::string rd = start_tc_run("TC-CIN-01..02 (ALL)");
            std::string md = get_module_dir(rd, "2_CompositeIn");
            g_jctl_pid = journalctl_start(rd);
            tc_cin_01_get_info(md);
            tc_cin_02_port_connected(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case 'B': case 'b': in_menu = false; break;
        case '\n': case '\r': break;
        default: printf("  Unknown choice '%c'.\n", ch); break;
        }
    }
}

/* ── Module 3 : Display ──────────────────────────────────────────────────── */
static void print_dsp_tc_menu(void)
{
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║       Module 3 – Display  ▸  Test Case Selection                 ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  READ (getter snapshot – no side-effects)                      ║\n");
    printf("║  [1] TC-DSP-01  ALL getter snapshot (VOP + Display sub-object) ║\n");
    printf("║       26 VOP getters + 10 Display getters + EDID bytes          ║\n");
    printf("║  [2] TC-DSP-02  getEDIDBytes standalone raw file               ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  [A] Run ALL Display TCs (TC-DSP-01 … TC-DSP-02)               ║\n");
    printf("║  [B] Back to TC module menu                                    ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("Choice: ");
    fflush(stdout);
}

static void handle_dsp_tc_submenu(void)
{
    char input[16];
    bool in_menu = true;
    while (in_menu) {
        print_dsp_tc_menu();
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        char ch = input[0];
        switch (ch) {
        case '1': {
            std::string rd = start_tc_run("TC-DSP-01");
            std::string md = get_module_dir(rd, "3_Display");
            g_jctl_pid = journalctl_start(rd);
            tc_dsp_01_display_surround(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case '2': {
            std::string rd = start_tc_run("TC-DSP-02");
            std::string md = get_module_dir(rd, "3_Display");
            g_jctl_pid = journalctl_start(rd);
            tc_dsp_02_display_edid(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case 'A': case 'a': {
            std::string rd = start_tc_run("TC-DSP-01..02 (ALL)");
            std::string md = get_module_dir(rd, "3_Display");
            g_jctl_pid = journalctl_start(rd);
            tc_dsp_01_display_surround(md);
            tc_dsp_02_display_edid(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case 'B': case 'b': in_menu = false; break;
        case '\n': case '\r': break;
        default: printf("  Unknown choice '%c'.\n", ch); break;
        }
    }
}

/* ── Module 4 : FPD ──────────────────────────────────────────────────────── */
static void print_fpd_tc_menu(void)
{
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║       Module 4 – FrontPanel (FPD)  ▸  Test Case Selection        ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  READ (getter snapshot – no side-effects)                      ║\n");
    printf("║  [1] TC-FPD-01  getBrightness (all indicators)                 ║\n");
    printf("║  [7] TC-FPD-07  ALL getter snapshot (8 getters per indicator)  ║\n");
    printf("║       getBrightness/Levels/ColorMode/Blink/MaxCycleRate         ║\n");
    printf("║       getColor/State/SupportedColors                           ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  READ+WRITE (round-trip: set → verify → restore)               ║\n");
    printf("║  [2] TC-FPD-02  setBrightness (50, restore)                    ║\n");
    printf("║  [3] TC-FPD-03  setColor (Power → Blue, restore)               ║\n");
    printf("║  [4] TC-FPD-04  setState (Power → ON, restore)                 ║\n");
    printf("║  [5] TC-FPD-05  setTextDisplay (\"RDK\")                         ║\n");
    printf("║  [6] TC-FPD-06  setTextTimeFormat (12-hr)                      ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  [A] Run ALL FPD TCs (TC-FPD-01 … TC-FPD-07)                   ║\n");
    printf("║  [B] Back to TC module menu                                    ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("Choice: ");
    fflush(stdout);
}

static void handle_fpd_tc_submenu(void)
{
    char input[16];
    bool in_menu = true;
    while (in_menu) {
        print_fpd_tc_menu();
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        char ch = input[0];
        switch (ch) {
        case '1': {
            std::string rd = start_tc_run("TC-FPD-01");
            std::string md = get_module_dir(rd, "4_FPD");
            g_jctl_pid = journalctl_start(rd);
            tc_fpd_01_get_brightness(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case '2': {
            std::string rd = start_tc_run("TC-FPD-02");
            std::string md = get_module_dir(rd, "4_FPD");
            g_jctl_pid = journalctl_start(rd);
            tc_fpd_02_set_brightness(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case '3': {
            std::string rd = start_tc_run("TC-FPD-03");
            std::string md = get_module_dir(rd, "4_FPD");
            g_jctl_pid = journalctl_start(rd);
            tc_fpd_03_set_color(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case '4': {
            std::string rd = start_tc_run("TC-FPD-04");
            std::string md = get_module_dir(rd, "4_FPD");
            g_jctl_pid = journalctl_start(rd);
            tc_fpd_04_set_state(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case '5': {
            std::string rd = start_tc_run("TC-FPD-05");
            std::string md = get_module_dir(rd, "4_FPD");
            g_jctl_pid = journalctl_start(rd);
            tc_fpd_05_set_text_display(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case '6': {
            std::string rd = start_tc_run("TC-FPD-06");
            std::string md = get_module_dir(rd, "4_FPD");
            g_jctl_pid = journalctl_start(rd);
            tc_fpd_06_set_time_format(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case '7': {
            std::string rd = start_tc_run("TC-FPD-07");
            std::string md = get_module_dir(rd, "4_FPD");
            g_jctl_pid = journalctl_start(rd);
            tc_fpd_07_all_getters(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case 'A': case 'a': {
            std::string rd = start_tc_run("TC-FPD-01..07 (ALL)");
            std::string md = get_module_dir(rd, "4_FPD");
            g_jctl_pid = journalctl_start(rd);
            tc_fpd_07_all_getters(md);
            tc_fpd_01_get_brightness(md);
            tc_fpd_02_set_brightness(md);
            tc_fpd_03_set_color(md);
            tc_fpd_04_set_state(md);
            tc_fpd_05_set_text_display(md);
            tc_fpd_06_set_time_format(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case 'B': case 'b': in_menu = false; break;
        case '\n': case '\r': break;
        default: printf("  Unknown choice '%c'.\n", ch); break;
        }
    }
}

/* ── Module 6 : Host ─────────────────────────────────────────────────────── */
static void print_hst_tc_menu(void)
{
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║       Module 6 – Host  ▸  Test Case Selection                     ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  READ (getter snapshot – no side-effects)                      ║\n");
    printf("║  [1] TC-HST-01  ALL getter snapshot (20 getters)               ║\n");
    printf("║       CPU temp / version / power mode / SocID / EDID            ║\n");
    printf("║       ATMOS / AudioMixing / Fader / Language / AudioFormat      ║\n");
    printf("║  [2] TC-HST-02  Sleep modes + port counts                      ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  [A] Run ALL Host TCs (TC-HST-01 … TC-HST-02)                   ║\n");
    printf("║  [B] Back to TC module menu                                    ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("Choice: ");
    fflush(stdout);
}

static void handle_hst_tc_submenu(void)
{
    char input[16];
    bool in_menu = true;
    while (in_menu) {
        print_hst_tc_menu();
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        char ch = input[0];
        switch (ch) {
        case '1': {
            std::string rd = start_tc_run("TC-HST-01");
            std::string md = get_module_dir(rd, "6_Host");
            g_jctl_pid = journalctl_start(rd);
            tc_hst_01_get_info(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case '2': {
            std::string rd = start_tc_run("TC-HST-02");
            std::string md = get_module_dir(rd, "6_Host");
            g_jctl_pid = journalctl_start(rd);
            tc_hst_02_sleep_modes(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case 'A': case 'a': {
            std::string rd = start_tc_run("TC-HST-01..02 (ALL)");
            std::string md = get_module_dir(rd, "6_Host");
            g_jctl_pid = journalctl_start(rd);
            tc_hst_01_get_info(md);
            tc_hst_02_sleep_modes(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case 'B': case 'b': in_menu = false; break;
        case '\n': case '\r': break;
        default: printf("  Unknown choice '%c'.\n", ch); break;
        }
    }
}

/* ── Module 7 : VideoDevice ──────────────────────────────────────────────── */
static void print_vd_tc_menu(void)
{
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║       Module 7 – VideoDevice  ▸  Test Case Selection             ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  READ (getter snapshot – no side-effects)                      ║\n");
    printf("║  [1] TC-VD-01  ALL getter snapshot                             ║\n");
    printf("║       getDFC / getSupportedDFCs / getHDRCapabilities            ║\n");
    printf("║       getSupportedVideoCodingFormats / getVideoCodecInfo        ║\n");
    printf("║       getSettopSupportedResolutions / getCurrentDisframerate    ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  READ+WRITE (round-trip: GET → SET-same → GET verify)          ║\n");
    printf("║  [2] TC-VD-02  setDFC round-trip                               ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  [A] Run ALL VideoDevice TCs (TC-VD-01 … TC-VD-02)              ║\n");
    printf("║  [B] Back to TC module menu                                    ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("Choice: ");
    fflush(stdout);
}

static void handle_vd_tc_submenu(void)
{
    char input[16];
    bool in_menu = true;
    while (in_menu) {
        print_vd_tc_menu();
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        char ch = input[0];
        switch (ch) {
        case '1': {
            std::string rd = start_tc_run("TC-VD-01");
            std::string md = get_module_dir(rd, "7_VideoDevice");
            g_jctl_pid = journalctl_start(rd);
            tc_vd_01_get_devices(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case '2': {
            std::string rd = start_tc_run("TC-VD-02");
            std::string md = get_module_dir(rd, "7_VideoDevice");
            g_jctl_pid = journalctl_start(rd);
            tc_vd_02_set_dfc(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case 'A': case 'a': {
            std::string rd = start_tc_run("TC-VD-01..02 (ALL)");
            std::string md = get_module_dir(rd, "7_VideoDevice");
            g_jctl_pid = journalctl_start(rd);
            tc_vd_01_get_devices(md);
            tc_vd_02_set_dfc(md);
            write_global_verify_summary(rd);
            journalctl_stop(g_jctl_pid, rd); g_jctl_pid = -1;
            printf("\n════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, rd.c_str());
            printf("════════════════════════════════════════════════════════════════\n");
            break; }
        case 'B': case 'b': in_menu = false; break;
        case '\n': case '\r': break;
        default: printf("  Unknown choice '%c'.\n", ch); break;
        }
    }
}

static void handle_tc_module_submenu(void)
{
    char input[16];
    bool in_menu = true;

    while (in_menu) {
        print_tc_module_menu();

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        char ch = input[0];

        switch (ch) {
        case '1':
            handle_audio_tc_submenu();
            break;
        case '2':
            handle_cin_tc_submenu();
            break;
        case '3':
            handle_dsp_tc_submenu();
            break;
        case '4':
            handle_fpd_tc_submenu();
            break;
        case '5':
            handle_hin_tc_submenu();
            break;
        case '6':
            handle_hst_tc_submenu();
            break;
        case '7':
            handle_vd_tc_submenu();
            break;
        case '8':
            handle_vp_tc_submenu();
            break;
        case 'B': case 'b':
            in_menu = false;
            break;
        case '\n': case '\r':
            break;
        default:
            printf("  Unknown choice '%c'. Please select from the menu.\n", ch);
            break;
        }
    }
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
    printf("║  [T] TC Menu – run individual test case per module           ║\n");
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
                g_jctl_pid = journalctl_start(rd1);
                run_module_audio(rd1);
                write_global_verify_summary(rd1);
                journalctl_stop(g_jctl_pid, rd1); g_jctl_pid = -1;
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
                g_jctl_pid = journalctl_start(rd2);
                run_module_compositein(rd2);
                write_global_verify_summary(rd2);
                journalctl_stop(g_jctl_pid, rd2); g_jctl_pid = -1;
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
                g_jctl_pid = journalctl_start(rd3);
                run_module_display(rd3);
                write_global_verify_summary(rd3);
                journalctl_stop(g_jctl_pid, rd3); g_jctl_pid = -1;
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

            g_jctl_pid = journalctl_start(run_dir);
            run_module_fpd(run_dir);
            write_global_verify_summary(run_dir);
            journalctl_stop(g_jctl_pid, run_dir); g_jctl_pid = -1;

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
                g_jctl_pid = journalctl_start(rd5);
                run_module_hdmiin(rd5);
                write_global_verify_summary(rd5);
                journalctl_stop(g_jctl_pid, rd5); g_jctl_pid = -1;
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
                g_jctl_pid = journalctl_start(rd6);
                run_module_host(rd6);
                write_global_verify_summary(rd6);
                journalctl_stop(g_jctl_pid, rd6); g_jctl_pid = -1;
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
                g_jctl_pid = journalctl_start(rd7);
                run_module_videodevice(rd7);
                write_global_verify_summary(rd7);
                journalctl_stop(g_jctl_pid, rd7); g_jctl_pid = -1;
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

            g_jctl_pid = journalctl_start(run_dir8);
            run_module_videoport(run_dir8);
            write_global_verify_summary(run_dir8);
            journalctl_stop(g_jctl_pid, run_dir8); g_jctl_pid = -1;

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

            g_jctl_pid = journalctl_start(run_dirA);
            run_module_audio(run_dirA);
            run_module_compositein(run_dirA);
            run_module_display(run_dirA);
            run_module_fpd(run_dirA);
            run_module_hdmiin(run_dirA);
            run_module_host(run_dirA);
            run_module_videodevice(run_dirA);
            run_module_videoport(run_dirA);
            write_global_verify_summary(run_dirA);
            journalctl_stop(g_jctl_pid, run_dirA); g_jctl_pid = -1;

            printf("\n");
            printf("════════════════════════════════════════════════════════════════\n");
            printf("  RUN %02d complete.  Results: %s\n", g_run_count, run_dirA.c_str());
            printf("  Returning to menu. DS handles remain UNCHANGED.\n");
            if (g_run_count == 1)
                printf("  → Kill dsmgr, run [A] again – FPD PASS, handle-based modules FAIL.\n");
            printf("════════════════════════════════════════════════════════════════\n");
            break;
        }

        /* ── TC submenu – individual test case selection ─────────────── */
        case 'T': case 't':
            handle_tc_module_submenu();
            break;

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
