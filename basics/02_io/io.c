#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/uart.h"  // Include UART hardware header

#define LED_DELAY_MS 250
#define LED_PIN 13
#define IO12_PIN 12  // Define IO12 pin number

// Define UART parameters
#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 4  // GPIO 4 for TX
#define UART_RX_PIN 5  // GPIO 5 for RX

// Perform initialisation
int init_led(void) {
    // Initialize LED pin as output
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // Initialize IO12 as input with pull-up resistor
    gpio_init(IO12_PIN);
    gpio_set_dir(IO12_PIN, GPIO_IN);
    gpio_pull_up(IO12_PIN);  // Enable pull-up resistor
    
    return PICO_OK;
}


void init_uart(void) {
    // Set up UART with specified baud rate
    uart_init(UART_ID, BAUD_RATE);
    
    // Configure UART parameters
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);  // 8 data bits, 1 stop bit, no parity
    uart_set_hw_flow(UART_ID, false, false);           // Disable hardware flow control
    uart_set_fifo_enabled(UART_ID, true);              // Enable FIFO
    
    // Set the GPIO pin functions to connect them to the UART
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // It's often a good idea to have a pullup on RX to avoid floating inputs
    gpio_pull_up(UART_RX_PIN);
}

int main() {
    // Initialize all stio, including the UART0 with 115200 (GP0/GP1)
    stdio_init_all();
    
    // Initialize the LED pins
    int rc = init_led();
    hard_assert(rc == PICO_OK);
    
    // Initialize custom UART on GPIO 4 & 5
    init_uart();
    
    // Send initial UART message
    printf("Pico2 Started\r\n");
    uart_puts(UART_ID, "UART1: Pico2 Started\r\n");

    // Variables for asynchronous LED control
    absolute_time_t next_led_change_time = get_absolute_time();
    bool led_state = true;

    while (true) {
        absolute_time_t current_time = get_absolute_time();

        // When the IO12 button is pressed, light the LED
        // If not blinking, light the LED without blocking the main loop
        bool io12_is_grounded = !gpio_get(IO12_PIN);
        if (io12_is_grounded) {
            gpio_put(LED_PIN, true);
        } else {
            gpio_put(LED_PIN, led_state);
        
            // Check if it's time to toggle the LED
            if (time_reached(next_led_change_time)) {
                // Time to toggle the LED
                led_state = !led_state;
                next_led_change_time = make_timeout_time_ms(LED_DELAY_MS);
            }
        }

        // Read and Write from UART1
        // This might be useful for something like MIDI handling in the future
        if (uart_is_readable(UART_ID)) {
            // Get a character from UART1
            uint8_t ch = uart_getc(UART_ID);
            
            // Echo it back if UART is writable
            if (uart_is_writable(UART_ID)) {
                uart_putc(UART_ID, ch);
            }

            // Toggle LED on each character received to show activity
            gpio_put(LED_PIN, true);
            sleep_ms(10);
            gpio_put(LED_PIN, false);
        }
    }
}
