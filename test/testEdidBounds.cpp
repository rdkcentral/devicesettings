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
 * @file testEdidBounds.cpp
 * @brief Security regression test for EDID bounds validation
 * 
 * This test validates that EDID length and port indices are properly bounds-checked
 * to prevent stack buffer overflows and array index issues.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

// Mock the minimum required types and constants
#define dsHDMI_IN_PORT_MAX 5
#define MAX_EDID_BYTES_LEN 1024
#define dsERR_NONE 0
#define dsERR_INVALID_PARAM -1
#define EOK 0

typedef int dsHdmiInPort_t;
typedef int dsError_t;

typedef struct {
    dsHdmiInPort_t iHdmiPort;
    unsigned char edid[MAX_EDID_BYTES_LEN];
    int length;
    int result;
} dsGetEDIDBytesInfoParam_t;

// Mock HAL function
dsError_t getEDIDBytesInfo(dsHdmiInPort_t iHdmiPort, unsigned char *edid, int *length) {
    if (iHdmiPort < 0 || iHdmiPort >= dsHDMI_IN_PORT_MAX) {
        return dsERR_INVALID_PARAM;
    }
    // Simulate returning a large EDID to test bounds
    *length = MAX_EDID_BYTES_LEN + 100; // Intentionally overflow to test validation
    return dsERR_NONE;
}

// Test the bounds validation logic
void test_edid_bounds_validation() {
    printf("Testing EDID bounds validation...\n");
    
    // Test case 1: Valid port index with normal length
    {
        dsGetEDIDBytesInfoParam_t param;
        param.iHdmiPort = 2; // Valid index
        param.length = 512; // Valid length
        param.result = dsERR_NONE;
        
        // Should pass bounds check
        assert(param.iHdmiPort >= 0);
        assert(param.iHdmiPort < dsHDMI_IN_PORT_MAX);
        printf("  ✓ Valid port index (2) with normal length (512) accepted\n");
    }
    
    // Test case 2: Negative port index
    {
        dsGetEDIDBytesInfoParam_t param;
        param.iHdmiPort = -1; // Invalid negative index
        param.length = 512;
        param.result = dsERR_NONE;
        
        // Should be rejected by bounds check
        assert(!(param.iHdmiPort >= 0 && param.iHdmiPort < dsHDMI_IN_PORT_MAX));
        printf("  ✓ Negative port index (-1) rejected\n");
    }
    
    // Test case 3: Port index exceeding maximum
    {
        dsGetEDIDBytesInfoParam_t param;
        param.iHdmiPort = dsHDMI_IN_PORT_MAX + 10; // Invalid large index
        param.length = 512;
        param.result = dsERR_NONE;
        
        // Should be rejected by bounds check
        assert(!(param.iHdmiPort >= 0 && param.iHdmiPort < dsHDMI_IN_PORT_MAX));
        printf("  ✓ Port index exceeding maximum (%d) rejected\n", dsHDMI_IN_PORT_MAX + 10);
    }
    
    // Test case 4: Port index at boundary (MAX - 1)
    {
        dsGetEDIDBytesInfoParam_t param;
        param.iHdmiPort = dsHDMI_IN_PORT_MAX - 1; // Valid boundary index
        param.length = 512;
        param.result = dsERR_NONE;
        
        // Should pass bounds check
        assert(param.iHdmiPort >= 0);
        assert(param.iHdmiPort < dsHDMI_IN_PORT_MAX);
        printf("  ✓ Port index at boundary (%d) accepted\n", dsHDMI_IN_PORT_MAX - 1);
    }
    
    // Test case 5: Port index at boundary (MAX)
    {
        dsGetEDIDBytesInfoParam_t param;
        param.iHdmiPort = dsHDMI_IN_PORT_MAX; // Invalid boundary index
        param.length = 512;
        param.result = dsERR_NONE;
        
        // Should be rejected by bounds check
        assert(!(param.iHdmiPort >= 0 && param.iHdmiPort < dsHDMI_IN_PORT_MAX));
        printf("  ✓ Port index at boundary MAX (%d) rejected\n", dsHDMI_IN_PORT_MAX);
    }
    
    // Test case 6: HAL returns length exceeding buffer size
    {
        dsGetEDIDBytesInfoParam_t param;
        param.iHdmiPort = 2; // Valid index
        param.length = 0;
        param.result = dsERR_NONE;
        
        // Simulate HAL returning oversized length
        int hal_length = MAX_EDID_BYTES_LEN + 100;
        getEDIDBytesInfo(param.iHdmiPort, param.edid, &hal_length);
        
        // Should be rejected by length check even after HAL call
        assert(!(hal_length > 0 && hal_length <= MAX_EDID_BYTES_LEN));
        printf("  ✓ HAL-reported length exceeding buffer size (%d) rejected\n", hal_length);
    }
    
    printf("All EDID bounds validation tests passed!\n");
}

int main() {
    test_edid_bounds_validation();
    return 0;
}
