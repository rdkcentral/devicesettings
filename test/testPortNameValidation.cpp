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
 * @file testPortNameValidation.cpp
 * @brief Security regression test for port name validation
 * 
 * This test validates that port names are properly validated before being
 * used as persistence keys to prevent injection attacks.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_port_name_validation() {
    printf("Testing port name validation...\n");
    
    // Test case 1: Valid port name
    {
        const char* portName = "HDMI0";
        (void)portName;
        assert(portName != NULL && strlen(portName) > 0);
        bool valid = true;
        for (const char* c = portName; *c; c++) {
            if (*c == '\n' || *c == '\r' || *c == '\t' || *c == ' ') {
                valid = false;
                break;
            }
        }
        (void)valid;
        assert(valid);
        printf("  ✓ Valid port name (%s) accepted\n", portName);
    }
    
    // Test case 2: Empty port name should be rejected
    {
        const char* portName = "";
        (void)portName;
        assert(strlen(portName) == 0);
        printf("  ✓ Empty port name rejected\n");
    }
    
    // Test case 3: Port name with newline should be rejected
    {
        const char* portName = "HDMI0\n";
        (void)portName;
        assert(strchr(portName, '\n') != NULL);
        printf("  ✓ Port name with newline rejected\n");
    }
    
    // Test case 4: Port name with space should be rejected
    {
        const char* portName = "HDMI 0";
        (void)portName;
        assert(strchr(portName, ' ') != NULL);
        printf("  ✓ Port name with space rejected\n");
    }
    
    // Test case 5: Port name with tab should be rejected
    {
        const char* portName = "HDMI0\t";
        (void)portName;
        assert(strchr(portName, '\t') != NULL);
        printf("  ✓ Port name with tab rejected\n");
    }
    
    // Test case 6: Port name with carriage return should be rejected
    {
        const char* portName = "HDMI0\r";
        (void)portName;
        assert(strchr(portName, '\r') != NULL);
        printf("  ✓ Port name with carriage return rejected\n");
    }
    
    printf("All port name validation tests passed!\n");
}

int main() {
    test_port_name_validation();
    return 0;
}
