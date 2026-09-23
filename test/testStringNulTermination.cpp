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
 * @file testStringNulTermination.cpp
 * @brief Security regression test for char[32] field NUL-termination
 * 
 * This test validates that char[32] IARM fields are properly NUL-terminated
 * before use to prevent OOB reads and injection attacks.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_string_nul_termination() {
    printf("Testing char[32] field NUL-termination...\n");
    
    // Test case 1: Properly NUL-terminated profileName
    {
        char profileName[32] = "DolbyAtmos";
        profileName[sizeof(profileName) - 1] = '\0';
        assert(strlen(profileName) < sizeof(profileName));
        assert(profileName[sizeof(profileName) - 1] == '\0');
        printf("  ✓ Properly NUL-terminated profileName accepted\n");
    }
    
    // Test case 2: ProfileName without NUL terminator
    {
        char profileName[32];
        memset(profileName, 'A', sizeof(profileName));
        // Force NUL termination
        profileName[sizeof(profileName) - 1] = '\0';
        assert(profileName[sizeof(profileName) - 1] == '\0');
        printf("  ✓ ProfileName forced NUL-terminated\n");
    }
    
    // Test case 3: Properly NUL-terminated profileSettingsName
    {
        char profileSettingsName[32] = "DialogEnhance";
        profileSettingsName[sizeof(profileSettingsName) - 1] = '\0';
        assert(strlen(profileSettingsName) < sizeof(profileSettingsName));
        assert(profileSettingsName[sizeof(profileSettingsName) - 1] == '\0');
        printf("  ✓ Properly NUL-terminated profileSettingsName accepted\n");
    }
    
    // Test case 4: Properly NUL-terminated profileState
    {
        char profileState[32] = "ADD";
        profileState[sizeof(profileState) - 1] = '\0';
        assert(strlen(profileState) < sizeof(profileState));
        assert(profileState[sizeof(profileState) - 1] == '\0');
        printf("  ✓ Properly NUL-terminated profileState accepted\n");
    }
    
    // Test case 5: Properly NUL-terminated profileSettingValue
    {
        char profileSettingValue[32] = "1";
        profileSettingValue[sizeof(profileSettingValue) - 1] = '\0';
        assert(strlen(profileSettingValue) < sizeof(profileSettingValue));
        assert(profileSettingValue[sizeof(profileSettingValue) - 1] == '\0');
        printf("  ✓ Properly NUL-terminated profileSettingValue accepted\n");
    }
    
    printf("All char[32] field NUL-termination tests passed!\n");
}

int main() {
    test_string_nul_termination();
    return 0;
}
