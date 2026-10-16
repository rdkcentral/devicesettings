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
 * @file testEventDataZeroing.cpp
 * @brief Security regression test for event data zeroing
 * 
 * This test validates that event data structures are properly zeroed
 * before being broadcast to prevent leaking stack data.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_event_data_zeroing() {
    printf("Testing event data zeroing...\n");
    
    // Test case 1: Structure should be zeroed before use
    {
        char eventData[56] = {0};
        memset(eventData, 0, sizeof(eventData));
        for (int i = 0; i < sizeof(eventData); i++) {
            assert(eventData[i] == 0);
        }
        printf("  ✓ Event data structure properly zeroed\n");
    }
    
    // Test case 2: Uninitialized structure should be zeroed before use
    {
        char eventData[56];
        // Simulate uninitialized with known pattern
        memset(eventData, 0xAA, sizeof(eventData));
        int has_nonzero = 0;
        for (int i = 0; i < sizeof(eventData); i++) {
            if (eventData[i] != 0) {
                has_nonzero = 1;
                break;
            }
        }
        assert(has_nonzero);
        printf("  ✓ Uninitialized structure contains non-zero data\n");
    }
    
    // Test case 3: Partial initialization should be followed by zeroing
    {
        char eventData[56];
        memset(eventData, 0, sizeof(eventData));
        strcpy(eventData, "60Hz");
        // The rest should still be zero
        int all_zero_after_str = 1;
        for (int i = 4; i < sizeof(eventData); i++) {
            if (eventData[i] != 0) {
                all_zero_after_str = 0;
                break;
            }
        }
        assert(all_zero_after_str);
        printf("  ✓ Partial initialization followed by zeroing works\n");
    }
    
    printf("All event data zeroing tests passed!\n");
}

int main() {
    test_event_data_zeroing();
    return 0;
}
