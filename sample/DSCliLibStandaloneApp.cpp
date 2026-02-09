#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <curl/curl.h>
#include <iostream>
#include <cstdint>
#include <sys/time.h>
#include <errno.h>
#include <syslog.h>
#include <sys/resource.h>

// RDK specific includes - libds.so API
#include "manager.hpp"                  // Device Settings Manager from libds.so
#include "host.hpp"                     // Host class from libds.so
#include "frontPanelIndicator.hpp"      // FrontPanelIndicator class from libds.so
#include "frontPanelTextDisplay.hpp"    // FrontPanelTextDisplay class from libds.so
// #include "libIBus.h"                  // Uncomment if using IARM Bus
// #include "libIBusDaemon.h"            // Uncomment if using IARM Bus daemon

using namespace std;
using namespace device;  // For libds.so classes

// RDK/Yocto specific constants
#define MAX_DEBUG_LOG_BUFF_SIZE 8192
#define DEVICE_SETTINGS_ERROR_NONE 0
#define DEVICE_SETTINGS_ERROR_GENERAL 1
#define DEVICE_SETTINGS_ERROR_UNAVAILABLE 2
#define DEVICE_SETTINGS_ERROR_NOT_EXIST 43

// Define logging macros
#define LOGI(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOGD(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Global variables
static pthread_mutex_t gCurlInitMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gDSAppMutex = PTHREAD_MUTEX_INITIALIZER;
CURLSH* mCurlShared = nullptr;
char* gDebugPrintBuffer = nullptr;

// RDK specific globals
static bool gAppInitialized = false;
static bool gSignalHandlersSet = false;

// Function declarations (libds.so functions)
void libds_Initialize();
void libds_Terminate();
int setup_signals();
int breakpad_ExceptionHandler();
void cleanup_resources();

// Menu system functions
void showMainMenu();
void handleFPDModule();
void handleAudioPortsModule();
void handleVideoPortsModule();
void handleHDMIInModule();
void handleCompositeInModule();
int getUserChoice();
void clearScreen();


int main(int argc, char **argv)
{
    // Initialize syslog for RDK environment
    openlog("DSApp", LOG_PID | LOG_CONS, LOG_DAEMON);
    
    printf("\n=== DeviceSettings Interactive Test Application ===\n");
    printf("Build: %s %s\n", __DATE__, __TIME__);
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -h, --help    Show this help message\n");
            printf("  --version     Show version information\n");
            return 0;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("DSApp version 1.0.0 (RDK/Yocto build)\n");
            return 0;
        }
    }
    
    // Set up signal handlers early
    if (setup_signals() != 0) {
        LOGE("Failed to setup signal handlers");
        return -1;
    }

    // Allocate debug buffer with error checking
    gDebugPrintBuffer = new(std::nothrow) char[MAX_DEBUG_LOG_BUFF_SIZE];
    if (!gDebugPrintBuffer) {
        LOGE("Failed to allocate debug buffer of size %d", MAX_DEBUG_LOG_BUFF_SIZE);
        return -1;
    }
    memset(gDebugPrintBuffer, 0, MAX_DEBUG_LOG_BUFF_SIZE);
    
    // Initialize curl for network operations
    CURLcode curl_result = curl_global_init(CURL_GLOBAL_ALL);
    if (curl_result != CURLE_OK) {
        LOGE("Failed to initialize curl: %s", curl_easy_strerror(curl_result));
        cleanup_resources();
        return -1;
    }

    // Initialize breakpad for crash reporting
    if (breakpad_ExceptionHandler() != 0) {
        LOGE("Failed to initialize exception handler");
        // Continue anyway, not critical
    }

    // Initialize libds.so (Device Settings library)
    printf("\nInitializing libds.so (Device Settings library)...\n");
//    libds_Initialize();
    printf("libds.so initialized successfully!\n");

    // Set application as initialized
    pthread_mutex_lock(&gDSAppMutex);
    gAppInitialized = true;
    pthread_mutex_unlock(&gDSAppMutex);

    // Main interactive menu loop
    int result = 0;
    try {
        showMainMenu();
        printf("\nApplication completed successfully\n");
    } catch (const std::exception& e) {
        LOGE("Application exception: %s", e.what());
        result = -1;
    } catch (...) {
        LOGE("Unknown application exception");
        result = -1;
    }

    // Cleanup before exit
    cleanup_resources();
    
    printf("\nDevice Settings Application exiting with code %d\n", result);
    closelog();
    return result;
}

// Initialize libds.so Device Settings library
void libds_Initialize()
{
    try {
        LOGI("Initializing Device Settings Manager from libds.so...");
        
        // Initialize the Device Settings Manager
        // This will internally call dsFPInit() and other init functions from libdshalcli.so
        device::Manager::Initialize();
        
        LOGI("Device Settings Manager initialized successfully");
    } catch (const std::exception& e) {
        LOGE("Failed to initialize Device Settings Manager: %s", e.what());
        throw;
    }
}

// Terminate libds.so Device Settings library
void libds_Terminate()
{
    try {
        LOGI("Terminating Device Settings Manager from libds.so...");
        device::Manager::DeInitialize();
        LOGI("Device Settings Manager terminated successfully");
    } catch (const std::exception& e) {
        LOGE("Error during Device Settings Manager termination: %s", e.what());
    }
}

// Signal handler for graceful shutdown
static void signal_handler(int signum) {
    const char* signal_name = "UNKNOWN";
    switch (signum) {
        case SIGTERM: signal_name = "SIGTERM"; break;
        case SIGINT:  signal_name = "SIGINT"; break;
        case SIGHUP:  signal_name = "SIGHUP"; break;
        case SIGUSR1: signal_name = "SIGUSR1"; break;
        case SIGUSR2: signal_name = "SIGUSR2"; break;
    }
    
    syslog(LOG_INFO, "[DSApp] Received signal %s (%d), initiating graceful shutdown", signal_name, signum);
    
    // Mark application as not initialized to trigger cleanup
    pthread_mutex_lock(&gDSAppMutex);
    gAppInitialized = false;
    pthread_mutex_unlock(&gDSAppMutex);
    
    // Cleanup and exit
    cleanup_resources();
    closelog();
    exit(0);
}

int setup_signals() {
    LOGI("Setting up signal handlers for RDK environment...");
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    // Handle common termination signals
    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        LOGE("Failed to set SIGTERM handler: %s", strerror(errno));
        return -1;
    }
    
    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        LOGE("Failed to set SIGINT handler: %s", strerror(errno));
        return -1;
    }
    
    if (sigaction(SIGHUP, &sa, nullptr) == -1) {
        LOGE("Failed to set SIGHUP handler: %s", strerror(errno));
        return -1;
    }
    
    // Handle user-defined signals for debugging
    if (sigaction(SIGUSR1, &sa, nullptr) == -1) {
        LOGE("Failed to set SIGUSR1 handler: %s", strerror(errno));
        return -1;
    }
    
    if (sigaction(SIGUSR2, &sa, nullptr) == -1) {
        LOGE("Failed to set SIGUSR2 handler: %s", strerror(errno));
        return -1;
    }
    
    // Ignore SIGPIPE to handle broken pipe gracefully
    signal(SIGPIPE, SIG_IGN);
    
    gSignalHandlersSet = true;
    LOGI("Signal handlers configured successfully");
    return 0;
}

int breakpad_ExceptionHandler() {
    LOGI("Initializing crash reporting for RDK environment...");
    
    // Set core dump parameters for RDK debugging
    struct rlimit core_limit;
    core_limit.rlim_cur = RLIM_INFINITY;
    core_limit.rlim_max = RLIM_INFINITY;
    
    if (setrlimit(RLIMIT_CORE, &core_limit) != 0) {
        LOGE("Failed to set core dump limit: %s", strerror(errno));
    } else {
        LOGI("Core dump enabled for crash analysis");
    }
    
    // Set up crash dump directory (typical RDK location)
    const char* crash_dir = "/opt/logs/crashes";
    if (access(crash_dir, W_OK) == 0) {
        LOGI("Crash dumps will be written to: %s", crash_dir);
    } else {
        LOGD("Crash directory %s not accessible, using default location", crash_dir);
    }
    
    // TODO: Initialize actual breakpad/crashpad when available
    // For now, rely on system core dumps and signal handlers
    
    LOGI("Crash reporting initialization complete");
    return 0;
}

// DeviceSettings_Init() is now implemented in device_settings.cpp

// DeviceSettings_IsOperational() is now implemented in device_settings.cpp

// DeviceSettings_Connect() is now implemented in device_settings.cpp

void cleanup_resources() {
    LOGI("Cleaning up application resources...");
    
    // Mark application as shutting down
    pthread_mutex_lock(&gDSAppMutex);
    gAppInitialized = false;
    pthread_mutex_unlock(&gDSAppMutex);
    
    // Clean up curl resources
    if (mCurlShared) {
        curl_share_cleanup(mCurlShared);
        mCurlShared = nullptr;
        LOGD("CURL shared handle cleaned up");
    }
    
    curl_global_cleanup();
    LOGD("CURL global cleanup completed");
    
    // Free debug buffer
    if (gDebugPrintBuffer) {
        delete[] gDebugPrintBuffer;
        gDebugPrintBuffer = nullptr;
        LOGD("Debug buffer freed");
    }
    
    // Clean up RDK-specific resources
    LOGD("Terminating libds.so Device Settings...");
    libds_Terminate();  // Call libds.so cleanup
    
    // TODO: Add IARM Bus cleanup if used
    // IARM_Bus_Disconnect();
    // IARM_Bus_Term();
    
    // Destroy mutexes
    pthread_mutex_destroy(&gDSAppMutex);
    pthread_mutex_destroy(&gCurlInitMutex);
    LOGD("Mutexes destroyed");
    
    LOGI("Resource cleanup complete");
}

// Menu system implementation
void clearScreen() {
    printf("\033[2J\033[H"); // ANSI escape codes to clear screen
}

int getUserChoice() {
    int choice;
    printf("\nEnter your choice: ");
    if (scanf("%d", &choice) != 1) {
        // Clear input buffer on invalid input
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return -1;
    }
    return choice;
}

void showMainMenu() {
    int choice;
    
    do {
        clearScreen();
        printf("\n=== DeviceSettings Module Test Menu ===\n");
        printf("1. FPD (Front Panel Display)\n");
        printf("2. Audio Ports\n");
        printf("3. Video Ports\n");
        printf("4. HDMI Input\n");
        printf("5. Composite Input\n");
        printf("0. Exit\n");
        printf("========================================\n");
        
        choice = getUserChoice();
        
        switch (choice) {
            case 1:
                handleFPDModule();
                break;
            case 2:
                handleAudioPortsModule();
                break;
            case 3:
                handleVideoPortsModule();
                break;
            case 4:
                handleHDMIInModule();
                break;
            case 5:
                handleCompositeInModule();
                break;
            case 0:
                printf("\nExiting...\n");
                break;
            default:
                printf("\nInvalid choice! Please try again.\n");
                printf("Press Enter to continue...");
                getchar(); // consume newline
                getchar(); // wait for Enter
                break;
        }
    } while (choice != 0);
}

void handleFPDModule() {
    int choice;

    do {
        clearScreen();
        printf("\n=== FPD (Front Panel Display) Module ===\n");
        printf("Using libds.so → libdshalcli.so → COM-RPC → DeviceSettings plugin\n");
        printf("1. GetFPDBrightness (Power LED)\n");
        printf("2. SetFPDBrightness (Power LED)\n");
        printf("3. GetFPDState (Power LED)\n");
        printf("4. SetFPDState (Power LED)\n");
        printf("5. GetFPDColor (Power LED)\n");
        printf("6. SetFPDColor (Power LED)\n");
        printf("7. Set Blink (Power LED)\n");
        printf("8. Test All Indicators\n");
        printf("0. Back to Main Menu\n");
        printf("=========================================\n");
        
        choice = getUserChoice();
        
        switch (choice) {
            case 1: {
                try {
                    printf("\nCalling libds.so FrontPanelIndicator::getBrightness()...\n");
                    
                    // Get the Power LED indicator from libds.so
                    FrontPanelIndicator& powerLED = FrontPanelIndicator::getInstance(FrontPanelIndicator::kPower);
                    
                    // Get brightness - this will call dsGetFPDBrightness from libdshalcli.so
                    int brightness = powerLED.getBrightness();
                    
                    printf("SUCCESS: Power LED Brightness: %d\n", brightness);
                } catch (const std::exception& e) {
                    printf("ERROR: %s\n", e.what());
                }
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 2: {
                int brightness;
                printf("\nEnter brightness level (0-100): ");
                if (scanf("%d", &brightness) == 1) {
                    try {
                        printf("Calling libds.so FrontPanelIndicator::setBrightness(%d)...\n", brightness);
                        
                        // Get the Power LED indicator from libds.so
                        FrontPanelIndicator& powerLED = FrontPanelIndicator::getInstance(FrontPanelIndicator::kPower);
                        
                        // Set brightness - this will call dsSetFPDBrightness from libdshalcli.so
                        powerLED.setBrightness(brightness, true);
                        
                        printf("SUCCESS: Power LED brightness set to %d\n", brightness);
                    } catch (const std::exception& e) {
                        printf("ERROR: %s\n", e.what());
                    }
                } else {
                    printf("Invalid input!\n");
                }
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 3: {
                try {
                    printf("\nCalling libds.so FrontPanelIndicator::getState()...\n");
                    
                    // Get the Power LED indicator from libds.so
                    FrontPanelIndicator& powerLED = FrontPanelIndicator::getInstance(FrontPanelIndicator::kPower);
                    
                    // Get state - this will call dsGetFPDState from libdshalcli.so
                    bool state = powerLED.getState();
                    
                    printf("SUCCESS: Power LED State: %s\n", state ? "ON" : "OFF");
                } catch (const std::exception& e) {
                    printf("ERROR: %s\n", e.what());
                }
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 4: {
                int state;
                printf("\nEnter FPD state (0=Off, 1=On): ");
                if (scanf("%d", &state) == 1) {
                    try {
                        printf("Calling libds.so FrontPanelIndicator::setState(%d)...\n", state);
                        
                        // Get the Power LED indicator from libds.so
                        FrontPanelIndicator& powerLED = FrontPanelIndicator::getInstance(FrontPanelIndicator::kPower);
                        
                        // Set state - this will call dsSetFPDState from libdshalcli.so
                        powerLED.setState(state != 0);
                        
                        printf("SUCCESS: Power LED state set to %s\n", state ? "ON" : "OFF");
                    } catch (const std::exception& e) {
                        printf("ERROR: %s\n", e.what());
                    }
                } else {
                    printf("Invalid input!\n");
                }
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 5: {
                try {
                    printf("\nCalling libds.so FrontPanelIndicator::getColor()...\n");
                    
                    // Get the Power LED indicator from libds.so
                    FrontPanelIndicator& powerLED = FrontPanelIndicator::getInstance(FrontPanelIndicator::kPower);
                    
                    // Get color - this will call dsGetFPColor from libdshalcli.so
                    uint32_t color = powerLED.getColor();
                    
                    printf("SUCCESS: Power LED Color: 0x%08X\n", color);
                } catch (const std::exception& e) {
                    printf("ERROR: %s\n", e.what());
                }
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 6: {
                unsigned int color;
                printf("\nEnter FPD color (hex, e.g., 0xFF0000 for red): 0x");
                if (scanf("%x", &color) == 1) {
                    try {
                        printf("Calling libds.so FrontPanelIndicator::setColor(0x%08X)...\n", color);
                        
                        // Get the Power LED indicator from libds.so
                        FrontPanelIndicator& powerLED = FrontPanelIndicator::getInstance(FrontPanelIndicator::kPower);
                        
                        // Set color - this will call dsSetFPDColor from libdshalcli.so
                        powerLED.setColor(color, true);
                        
                        printf("SUCCESS: Power LED color set to 0x%08X\n", color);
                    } catch (const std::exception& e) {
                        printf("ERROR: %s\n", e.what());
                    }
                } else {
                    printf("Invalid input!\n");
                }
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 7: {
                int interval, iterations;
                printf("\nEnter blink interval (ms): ");
                if (scanf("%d", &interval) != 1) {
                    printf("Invalid input!\n");
                    printf("Press Enter to continue...");
                    getchar(); getchar();
                    break;
                }
                printf("Enter blink iterations: ");
                if (scanf("%d", &iterations) == 1) {
                    try {
                        printf("Calling libds.so FrontPanelIndicator::setBlink(%d, %d)...\n", interval, iterations);
                        
                        // Get the Power LED indicator from libds.so
                        FrontPanelIndicator& powerLED = FrontPanelIndicator::getInstance(FrontPanelIndicator::kPower);
                        
                        // Set blink - this will call dsSetFPBlink from libdshalcli.so
                        FrontPanelIndicator::Blink blink(interval, iterations);
                        powerLED.setBlink(blink);
                        
                        printf("SUCCESS: Power LED blink set (interval=%d ms, iterations=%d)\n", interval, iterations);
                    } catch (const std::exception& e) {
                        printf("ERROR: %s\n", e.what());
                    }
                } else {
                    printf("Invalid input!\n");
                }
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 8: {
                printf("\nTesting all FPD indicators...\n");
                try {
                    const char* indicatorNames[] = {"Message", "Power", "Record", "Remote", "RFBypass"};
                    int indicatorIds[] = {
                        FrontPanelIndicator::kMessage,
                        FrontPanelIndicator::kPower,
                        FrontPanelIndicator::kRecord,
                        FrontPanelIndicator::kRemote,
                        FrontPanelIndicator::kRFBypass
                    };
                    
                    for (int i = 0; i < 5; i++) {
                        try {
                            FrontPanelIndicator& indicator = FrontPanelIndicator::getInstance(indicatorIds[i]);
                            int brightness = indicator.getBrightness();
                            bool state = indicator.getState();
                            printf("  %s LED: Brightness=%d, State=%s\n", 
                                   indicatorNames[i], brightness, state ? "ON" : "OFF");
                        } catch (const std::exception& e) {
                            printf("  %s LED: ERROR - %s\n", indicatorNames[i], e.what());
                        }
                    }
                    printf("Test completed!\n");
                } catch (const std::exception& e) {
                    printf("ERROR: %s\n", e.what());
                }
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 0:
                printf("\nReturning to main menu...\n");
                break;
            default:
                printf("\nInvalid choice! Please try again.\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
        }
    } while (choice != 0);
}

void handleAudioPortsModule() {
    int choice;
    
    do {
        clearScreen();
        printf("\n=== Audio Ports Module ===\n");
        printf("1. DeviceSettings_GetAudioPort\n");
        printf("2. DeviceSettings_SetAudioPort\n");
        printf("3. DeviceSettings_GetAudioFormat\n");
        printf("4. DeviceSettings_SetAudioFormat\n");
        printf("5. DeviceSettings_GetAudioCompression\n");
        printf("6. DeviceSettings_SetAudioCompression\n");
        printf("7. DeviceSettings_GetDialogEnhancement\n");
        printf("8. DeviceSettings_SetDialogEnhancement\n");
        printf("9. DeviceSettings_GetDolbyVolumeMode\n");
        printf("10. DeviceSettings_SetDolbyVolumeMode\n");
        printf("0. Back to Main Menu\n");
        printf("===============================\n");

        choice = getUserChoice();
        
        switch (choice) {
            case 1:
                printf("\nCalling DeviceSettings_GetAudioPort()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            case 2:
                printf("\nCalling DeviceSettings_SetAudioPort()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            // Add more cases as needed
            case 0:
                printf("\nReturning to main menu...\n");
                break;
            default:
                printf("\nInvalid choice! Please try again.\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
        }
    } while (choice != 0);
}

void handleVideoPortsModule() {
    int choice;
    
    do {
        clearScreen();
        printf("\n=== Video Ports Module ===\n");
        printf("1. DeviceSettings_GetVideoPort\n");
        printf("2. DeviceSettings_SetVideoPort\n");
        printf("3. DeviceSettings_GetVideoFormat\n");
        printf("4. DeviceSettings_SetVideoFormat\n");
        printf("5. DeviceSettings_GetVideoResolution\n");
        printf("6. DeviceSettings_SetVideoResolution\n");
        printf("7. DeviceSettings_GetVideoFrameRate\n");
        printf("8. DeviceSettings_SetVideoFrameRate\n");
        printf("0. Back to Main Menu\n");
        printf("===============================\n");
        
        choice = getUserChoice();
        
        switch (choice) {
            case 1:
                printf("\nCalling DeviceSettings_GetVideoPort()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            // Add more cases as needed
            case 0:
                printf("\nReturning to main menu...\n");
                break;
            default:
                printf("\nInvalid choice! Please try again.\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
        }
    } while (choice != 0);
}

void handleHDMIInModule() {
    int choice;
    
    do {
        clearScreen();
        printf("\n=== HDMI Input Module ===\n");
        printf("1. DeviceSettings_GetHDMIInput\n");
        printf("2. DeviceSettings_SetHDMIInput\n");
        printf("3. DeviceSettings_GetHDMIInputStatus\n");
        printf("4. DeviceSettings_GetHDMIInputSignalStatus\n");
        printf("0. Back to Main Menu\n");
        printf("==========================\n");

        choice = getUserChoice();
        
        switch (choice) {
            case 1:
                printf("\nCalling DeviceSettings_GetHDMIInput()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            // Add more cases as needed
            case 0:
                printf("\nReturning to main menu...\n");
                break;
            default:
                printf("\nInvalid choice! Please try again.\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
        }
    } while (choice != 0);
}

void handleCompositeInModule() {
    int choice;
    
    do {
        clearScreen();
        printf("\n=== Composite Input Module ===\n");
        printf("1. DeviceSettings_GetCompositeInput\n");
        printf("2. DeviceSettings_SetCompositeInput\n");
        printf("3. DeviceSettings_GetCompositeInputStatus\n");
        printf("0. Back to Main Menu\n");
        printf("===============================\n");
        
        choice = getUserChoice();
        
        switch (choice) {
            case 1:
                printf("\nCalling DeviceSettings_GetCompositeInput()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            // Add more cases as needed
            case 0:
                printf("\nReturning to main menu...\n");
                break;
            default:
                printf("\nInvalid choice! Please try again.\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
        }
    } while (choice != 0);
}
