#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

// Define the flash offset where we'll store our data
// IMPORTANT: This must be a multiple of FLASH_SECTOR_SIZE (4KB)
// Choose an offset that doesn't conflict with your program code
// Make sure PICO_FLASH_SIZE_BYTES is higher than FLASH_TARGET_OFFSET
// Currently we have 16MB of flash memory & we defined it using the my_board.h & use it as a custom board
#define FLASH_TARGET_OFFSET (1024 * 1024) * 15 // 15MB offset

// Define a simple data structure to store in flash
// The size of the struct must be multiples of pages size (256 bytes)
// otherwise the flash_range_program will crash
typedef struct {
    uint32_t magic;       // Magic number to identify valid data
    uint32_t counter;     // A simple counter
    char message[FLASH_PAGE_SIZE - sizeof(uint32_t) * 2];     // A message string
} flash_data_t;

// Magic number for data validation
#define DATA_MAGIC 0x12345678
#define LED_PIN 13

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // Wait for serial connection
    sleep_ms(2000);
    
    printf("Flash Memory Example\n");
    
    printf("Flash: %uKB sectors, %uB pages, %uMB total, offset: 0x%x\n",
           FLASH_SECTOR_SIZE / 1024,
           FLASH_PAGE_SIZE,
           PICO_FLASH_SIZE_BYTES / (1024 * 1024),
           FLASH_TARGET_OFFSET);
    
    // Read existing data from flash
    const flash_data_t* existing_data = (const flash_data_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
    
    // Check if we have valid data
    bool has_valid_data = (existing_data->magic == DATA_MAGIC);
    
    printf("Existing data %s valid\n", has_valid_data ? "is" : "is not");
    
    if (has_valid_data) {
        printf("  Counter: %u\n", existing_data->counter);
        printf("  Message: %s\n", existing_data->message);
    }
    
    // Prepare new data to write
    flash_data_t new_data;
    new_data.magic = DATA_MAGIC;
    new_data.counter = has_valid_data ? existing_data->counter + 1 : 1;
    snprintf(new_data.message, sizeof(new_data.message), "Hello from RP2350! Count: %u", new_data.counter);
    
    printf("Writing new data to flash...\n");
    
    // Disable interrupts during flash operations
    uint32_t ints = save_and_disable_interrupts();
    
    // Erase the sector
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    // Program the data
    flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t*)&new_data, sizeof(new_data));
    
    // Restore interrupts
    restore_interrupts(ints);
    
    printf("Data written successfully!\n");
    
    // Verify the data was written correctly
    const flash_data_t* updated_data = (const flash_data_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
    
    printf("Verification:\n");
    printf("  Magic: 0x%08x\n", updated_data->magic);
    printf("  Counter: %u\n", updated_data->counter);
    printf("  Message: %s\n", updated_data->message);
    
    printf("Flash example complete!\n");
    
    while (true) {
        gpio_put(LED_PIN, true);
        sleep_ms(250);
        gpio_put(LED_PIN, false);
        sleep_ms(250);
    }
    
    return 0;
}