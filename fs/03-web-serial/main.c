#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

// Buffer for incoming data
char buffer[256];
int buffer_pos = 0;

// Process incoming commands
void process_command(const char* cmd) {
    // Log the received command
    
    // Process specific commands
    if (strcmp(cmd, "ping") == 0) {
        printf("pong\n");
    } 
    else if (strncmp(cmd, "echo ", 5) == 0) {
        printf("%s\n", cmd + 5);
    } 
    else if (strcmp(cmd, "info") == 0) {
        printf("Raspberry Pi Pico 2 Web Serial Interface\n");
        printf("SDK Version: %s\n", PICO_SDK_VERSION_STRING);
    } 
    else if (strcmp(cmd, "help") == 0) {
        printf("Available commands:\n");
        printf("  ping - Check connection\n");
        printf("  echo <text> - Echo back text\n");
        printf("  info - Show device information\n");
        printf("  help - Show this help message\n");
    } 
    else {
        printf("Unknown command: %s\n", cmd);
        printf("Type 'help' for available commands\n");
    }
}

int main() {
    // Initialize stdio over USB
    stdio_init_all();
    
    // Wait for USB connection
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    
    printf("Raspberry Pi Pico 2 Web Serial Interface\n");
    printf("Ready to receive commands...\n");
    
    // Main loop
    while (1) {
        // Check if there's any incoming data
        int c = getchar_timeout_us(0);
        
        if (c != PICO_ERROR_TIMEOUT) {
            // If we received a newline or carriage return, process the command
            if (c == '\n' || c == '\r') {
                if (buffer_pos > 0) {
                    // Null terminate the buffer
                    buffer[buffer_pos] = '\0';
                    
                    // Process the command
                    process_command(buffer);
                    
                    // Reset buffer position
                    buffer_pos = 0;
                }
            } else {
                // Add character to buffer if there's space
                if (buffer_pos < sizeof(buffer) - 1) {
                    buffer[buffer_pos++] = (char)c;
                }
            }
        }
        
        // Small delay to prevent tight loop
        sleep_ms(10);
    }
    
    return 0;
}