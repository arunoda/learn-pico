#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/structs/xip.h"
#include "pico/time.h"
#include "random_data.h"  // Include our generated 8MB random data

// PSRAM starts at this address on the Pico when configured properly
volatile uint8_t* const PSRAM_BASE = (volatile uint8_t*)0x11000000;

// Define a specific location in PSRAM for our string
#define PSRAM_STRING_ADDR 0x100000  // 1MB into the PSRAM
#define MAX_STRING_LENGTH 256

// Create a struct at a specific PSRAM location
typedef struct {
    char signature[8];         // To verify memory is working
    uint32_t length;           // Length of the string
    char data[MAX_STRING_LENGTH]; // The string data
} psram_string_t;

// Pointer to our string structure in PSRAM
volatile psram_string_t* const psram_string = (volatile psram_string_t*)(PSRAM_BASE + PSRAM_STRING_ADDR);

// Read and verify the string from PSRAM
bool read_psram_string(char* buffer, size_t buffer_size) {
    // First check signature
    if (strncmp((char*)psram_string->signature, "PSRAMSTR", 8) != 0) {
        printf("Error: PSRAM signature verification failed!\n");
        return false;
    }
    
    // Get length (with safety check)
    uint32_t length = psram_string->length;
    if (length >= buffer_size || length >= MAX_STRING_LENGTH) {
        length = buffer_size - 1;
    }
    
    // Copy data from PSRAM
    memcpy(buffer, (const void*)psram_string->data, length);
    buffer[length] = '\0';
    
    return true;
}

void test_psram_with_full_data() {
    absolute_time_t start_time = get_absolute_time();
    printf("\n=== PSRAM 8MB Random Data Test ===\n\n");
    
    // Copy 8MB of random data to PSRAM
    printf("Writing 8MB of random data to PSRAM...\n");
    
    // Write data in 1MB chunks and report progress
    for (uint32_t offset = 0; offset < RANDOM_DATA_SIZE; offset += (1024 * 1024)) {
        uint32_t end = offset + (1024 * 1024);
        if (end > RANDOM_DATA_SIZE) end = RANDOM_DATA_SIZE;
        
        for (uint32_t i = offset; i < end; i++) {
            PSRAM_BASE[i] = random_data[i];
        }
        
        printf("Written %d MB...\n", (offset + (1024 * 1024)) / (1024 * 1024));
    }
    
    absolute_time_t after_write = get_absolute_time();
    uint32_t write_time_ms = absolute_time_diff_us(start_time, after_write) / 1000;
    float write_speed_kbps = (float)RANDOM_DATA_SIZE / write_time_ms;
    printf("Write complete in %u ms (%.2f KB/s)\n", write_time_ms, write_speed_kbps);
    
    // Verify the random data
    printf("\nVerifying PSRAM data against source...\n");
    uint32_t error_count = 0;
    uint32_t first_error_addr = 0;
    uint8_t first_expected = 0;
    uint8_t first_actual = 0;

    // Verify data in 1MB chunks and report progress
    for (uint32_t offset = 0; offset < RANDOM_DATA_SIZE; offset += (1024 * 1024)) {
        uint32_t end = offset + (1024 * 1024);
        if (end > RANDOM_DATA_SIZE) end = RANDOM_DATA_SIZE;
        
        for (uint32_t i = offset; i < end; i++) {
            uint8_t expected = random_data[i];
            uint8_t value = PSRAM_BASE[i];
            
            if (value != expected) {
                if (error_count == 0) {
                    first_error_addr = i;
                    first_expected = expected;
                    first_actual = value;
                }
                error_count++;
                
                // Only print first few errors to avoid flooding console
                if (error_count <= 5) {
                    printf("Error at offset 0x%08X: got 0x%02X, expected 0x%02X\n", 
                          i, value, expected);
                }
            }
        }
        
        printf("Verified %d MB...\n", (offset + (1024 * 1024)) / (1024 * 1024));
    }
    
    absolute_time_t end_time = get_absolute_time();
    uint32_t total_time_ms = absolute_time_diff_us(start_time, end_time) / 1000;
    float read_time_ms = absolute_time_diff_us(after_write, end_time) / 1000;
    float read_speed_kbps = (float)RANDOM_DATA_SIZE / read_time_ms;
    
    if (error_count == 0) {
        printf("\nPSRAM TEST PASSED - All 8MB of random data verified successfully!\n");
    } else {
        printf("\nPSRAM TEST FAILED - %u errors detected\n", error_count);
        printf("First error at offset 0x%08X: got 0x%02X, expected 0x%02X\n",
               first_error_addr, first_actual, first_expected);
        
        // Additional diagnostics: check if nearby addresses show patterns
        if (first_error_addr > 0) {
            printf("\nNearby memory check:\n");
            for (int i = -4; i <= 4; i++) {
                if (i == 0) continue; // Skip the error address itself
                
                uint32_t addr = first_error_addr + i;
                if (addr < RANDOM_DATA_SIZE) {
                    printf("Offset %+d (0x%08X): got 0x%02X, expected 0x%02X\n", 
                          i, addr, PSRAM_BASE[addr], random_data[addr]);
                }
            }
        }
    }
    
    printf("Read speed: %.2f KB/s\n", read_speed_kbps);
    printf("Write speed: %.2f KB/s\n", write_speed_kbps);
    printf("Total test time: %u ms (%.2f seconds)\n", total_time_ms, total_time_ms / 1000.0f);
}

int main()
{
    stdio_init_all();

    // Configure PSRAM CS pin
    gpio_set_function(0, GPIO_FUNC_XIP_CS1); 
    // Make sure PSRAM range is writable
    xip_ctrl_hw->ctrl |= XIP_CTRL_WRITABLE_M1_BITS;

    sleep_ms(1000);
    printf("\nPSRAM 8MB test starting...\n");
    
    // Test PSRAM with 8MB random data
    test_psram_with_full_data();

    // Buffer to read string into
    char buffer[MAX_STRING_LENGTH];
    int counter = 0;

    PSRAM_BASE[0] = 100;
    
    while (true) {
        printf("PSRAM value %d\n", PSRAM_BASE[0]);
        sleep_ms(2000);
    }
}
