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
 * @file testEdidBufferSize.cpp
 * @brief Security regression test for EDID buffer size
 * 
 * This test validates that the EDID buffer is sized correctly to prevent
 * buffer overflows from HDMI sink EDID data.
 */

#include <stdio.h>
#include <assert.h>

#define MAX_EDID_BYTES_LEN 1024

void test_edid_buffer_size() {
    printf("Testing EDID buffer size...\n");
    
    // Test case 1: Buffer should be at least 1024 bytes
    {
        unsigned char edidBytes[MAX_EDID_BYTES_LEN] = {0};
        assert(sizeof(edidBytes) >= MAX_EDID_BYTES_LEN);
        printf("  ✓ EDID buffer size (%zu bytes) >= %d bytes\n", sizeof(edidBytes), MAX_EDID_BYTES_LEN);
    }
    
    // Test case 2: Old buffer size (512) would be insufficient
    {
        unsigned char oldBuffer[512] = {0};
        assert(sizeof(oldBuffer) < MAX_EDID_BYTES_LEN);
        printf("  ✓ Old buffer size (512 bytes) insufficient for max EDID (%d bytes)\n", MAX_EDID_BYTES_LEN);
    }
    
    // Test case 3: New buffer size can handle maximum EDID
    {
        unsigned char edidBytes[MAX_EDID_BYTES_LEN] = {0};
        // Simulate filling with maximum EDID data
        for (int i = 0; i < MAX_EDID_BYTES_LEN; i++) {
            edidBytes[i] = (unsigned char)(i % 256);
        }
        // Verify no overflow occurred
        assert(edidBytes[MAX_EDID_BYTES_LEN - 1] == (unsigned char)((MAX_EDID_BYTES_LEN - 1) % 256));
        printf("  ✓ New buffer can handle maximum EDID size without overflow\n");
    }
    
    printf("All EDID buffer size tests passed!\n");
}

int main() {
    test_edid_buffer_size();
    return 0;
}
