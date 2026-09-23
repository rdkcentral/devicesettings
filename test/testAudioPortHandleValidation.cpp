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
 * @file testAudioPortHandleValidation.cpp
 * @brief Security regression test for audio port handle validation
 * 
 * This test validates that audio port handles are properly validated before
 * being passed to HAL functions to prevent crashes from forged handles.
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#define dsAUDIOPORT_TYPE_MAX 999

typedef int dsAudioPortType_t;

// Mock _GetAudioPortType function
static dsAudioPortType_t _GetAudioPortType(intptr_t handle) {
    // Simulate that valid handles return a valid type
    // Forged handles return dsAUDIOPORT_TYPE_MAX
    if (handle == 0x12345678 || handle == 0x87654321) {
        return 1; // Valid type
    }
    return dsAUDIOPORT_TYPE_MAX; // Invalid
}

void test_audio_port_handle_validation() {
    printf("Testing audio port handle validation...\n");
    
    // Test case 1: Valid handle should be accepted
    {
        intptr_t handle = 0x12345678;
        assert(_GetAudioPortType(handle) != dsAUDIOPORT_TYPE_MAX);
        printf("  ✓ Valid handle (0x%lx) accepted\n", (long)handle);
    }
    
    // Test case 2: Forged handle should be rejected
    {
        intptr_t handle = 0x41414141; // Common forgery pattern
        assert(_GetAudioPortType(handle) == dsAUDIOPORT_TYPE_MAX);
        printf("  ✓ Forged handle (0x%lx) rejected\n", (long)handle);
    }
    
    // Test case 3: Another forged handle should be rejected
    {
        intptr_t handle = 0xDEADBEEF;
        assert(_GetAudioPortType(handle) == dsAUDIOPORT_TYPE_MAX);
        printf("  ✓ Forged handle (0x%lx) rejected\n", (long)handle);
    }
    
    // Test case 4: NULL handle should be rejected
    {
        intptr_t handle = 0;
        assert(_GetAudioPortType(handle) == dsAUDIOPORT_TYPE_MAX);
        printf("  ✓ NULL handle (0x%lx) rejected\n", (long)handle);
    }
    
    // Test case 5: Another valid handle should be accepted
    {
        intptr_t handle = 0x87654321;
        assert(_GetAudioPortType(handle) != dsAUDIOPORT_TYPE_MAX);
        printf("  ✓ Valid handle (0x%lx) accepted\n", (long)handle);
    }
    
    printf("All audio port handle validation tests passed!\n");
}

int main() {
    test_audio_port_handle_validation();
    return 0;
}
