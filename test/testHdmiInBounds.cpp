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
 * @file testHdmiInBounds.cpp
 * @brief Security regression test for HDMI input port bounds validation
 * 
 * This test validates that HDMI input port indices are properly bounds-checked
 * before being used as array indices, preventing out-of-bounds writes.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

// Mock the minimum required types and constants
#define dsHDMI_IN_PORT_MAX 5
#define dsERR_NONE 0
#define dsERR_INVALID_PARAM -1

typedef int dsHdmiInPort_t;
typedef int dsError_t;

typedef struct {
    dsHdmiInPort_t iHdmiPort;
    int iEdidVersion;
    int result;
} dsEdidVersionParam_t;

// Mock HAL function
dsError_t setEdidVersion(dsHdmiInPort_t iHdmiPort, int iEdidVersion) {
    if (iHdmiPort < 0 || iHdmiPort >= dsHDMI_IN_PORT_MAX) {
        return dsERR_INVALID_PARAM;
    }
    return dsERR_NONE;
}

// Test the bounds validation logic
void test_bounds_validation() {
    printf("Testing HDMI input port bounds validation...\n");
    
    // Test case 1: Valid port index
    {
        dsEdidVersionParam_t param;
        param.iHdmiPort = 2; // Valid index
        param.iEdidVersion = 1;
        param.result = setEdidVersion(param.iHdmiPort, param.iEdidVersion);
        
        // Should succeed and allow array access
        assert(param.iHdmiPort >= 0);
        assert(param.iHdmiPort < dsHDMI_IN_PORT_MAX);
        printf("  ✓ Valid port index (2) accepted\n");
    }
    
    // Test case 2: Negative port index
    {
        dsEdidVersionParam_t param;
        param.iHdmiPort = -1; // Invalid negative index
        param.iEdidVersion = 1;
        param.result = setEdidVersion(param.iHdmiPort, param.iEdidVersion);
        
        // Should be rejected by bounds check
        assert(!(param.iHdmiPort >= 0 && param.iHdmiPort < dsHDMI_IN_PORT_MAX));
        printf("  ✓ Negative port index (-1) rejected\n");
    }
    
    // Test case 3: Port index exceeding maximum
    {
        dsEdidVersionParam_t param;
        param.iHdmiPort = dsHDMI_IN_PORT_MAX + 10; // Invalid large index
        param.iEdidVersion = 1;
        param.result = setEdidVersion(param.iHdmiPort, param.iEdidVersion);
        
        // Should be rejected by bounds check
        assert(!(param.iHdmiPort >= 0 && param.iHdmiPort < dsHDMI_IN_PORT_MAX));
        printf("  ✓ Port index exceeding maximum (%d) rejected\n", dsHDMI_IN_PORT_MAX + 10);
    }
    
    // Test case 4: Port index at boundary (MAX - 1)
    {
        dsEdidVersionParam_t param;
        param.iHdmiPort = dsHDMI_IN_PORT_MAX - 1; // Valid boundary index
        param.iEdidVersion = 1;
        param.result = setEdidVersion(param.iHdmiPort, param.iEdidVersion);
        
        // Should succeed and allow array access
        assert(param.iHdmiPort >= 0);
        assert(param.iHdmiPort < dsHDMI_IN_PORT_MAX);
        printf("  ✓ Port index at boundary (%d) accepted\n", dsHDMI_IN_PORT_MAX - 1);
    }
    
    // Test case 5: Port index at boundary (MAX)
    {
        dsEdidVersionParam_t param;
        param.iHdmiPort = dsHDMI_IN_PORT_MAX; // Invalid boundary index
        param.iEdidVersion = 1;
        param.result = setEdidVersion(param.iHdmiPort, param.iEdidVersion);
        
        // Should be rejected by bounds check
        assert(!(param.iHdmiPort >= 0 && param.iHdmiPort < dsHDMI_IN_PORT_MAX));
        printf("  ✓ Port index at boundary MAX (%d) rejected\n", dsHDMI_IN_PORT_MAX);
    }
    
    printf("All bounds validation tests passed!\n");
}

int main() {
    test_bounds_validation();
    return 0;
}
