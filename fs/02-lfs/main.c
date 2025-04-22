#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pico_littlefs.h"

#define LED_PIN 13

int main()
{
    stdio_init_all();
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Wait for serial connection
    sleep_ms(2000);
    
    printf("LittleFS Example on RP2350\n");
    
    // Mount the filesystem
    if (!mount_littlefs()) {
        printf("Failed to mount filesystem\n");
        return 1;
    }

    // Create a test directory
    create_directory("/test");
    
    // Write a test file
    const char* test_data = "Hello from RP2350! This is a test file created with LittleFS.";
    write_file("/test/hello.txt", test_data, strlen(test_data));
    
    // List the root directory
    list_directory("/");
    
    // List the test directory
    list_directory("/test");
    
    // Read the test file
    char buffer[256];
    size_t bytes_read;
    if (read_file("/test/hello.txt", buffer, sizeof(buffer), &bytes_read)) {
        // Null-terminate the buffer
        buffer[bytes_read] = '\0';
        printf("File content: %s\n", buffer);
    }
    
    // Write another file
    const char* config_data = "sample_rate=48000\nvolume=75\nmode=stereo";
    write_file("/config.txt", config_data, strlen(config_data));
    
    // List the root directory again
    list_directory("/");
    
    // Delete a file
    delete_file("/test/hello.txt");
    
    // List the test directory again
    list_directory("/test");
    
    // Unmount the filesystem
    unmount_littlefs();
    
    printf("LittleFS example complete!\n");

    while (true) {
        gpio_put(LED_PIN, true);
        sleep_ms(1000);
        gpio_put(LED_PIN, false);
        sleep_ms(1000);

        printf("Hello, world from printf!\r\n");
    }
}
