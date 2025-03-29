#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "blink.pio.h"

#define LED_PIN 13
#define BUTTON_PIN 12

// This function continues to blink forever at the given frequency
void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz forever\n", pin, freq);

    // Calculate delay value for desired frequency
    uint32_t delay = (125000000 / (2 * freq)) - 3;
    
    // Send a large number for continuous blinking
    pio->txf[sm] = 0xFFFFFFFF;  // Practically infinite blinks
    pio->txf[sm] = delay;       // Send delay value
}

// This function blinks the LED N times with a given frequency
void blink_pin_n_times(PIO pio, uint sm, uint offset, uint pin, uint freq, uint count) {
    // Initialize if not already initialized
    static bool initialized = false;
    if (!initialized) {
        blink_program_init(pio, sm, offset, pin);
        pio_sm_set_enabled(pio, sm, true);
        initialized = true;
    }
    
    // Wait a moment to ensure the state machine has processed the stop command
    sleep_ms(10);

    printf("Blinking pin %d at %d Hz for %d times\n", pin, freq, count);

    // Calculate delay value for desired frequency
    uint32_t delay = (150000000 / (2 * freq)) - 3;
    
    // Send blink count and delay to PIO
    pio->txf[sm] = count;      // Number of times to blink
    pio->txf[sm] = delay;      // Delay value for frequency
}

int main()
{
    stdio_init_all();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    // PIO Blinking example
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &blink_program);
    printf("Loaded program at %d\n", offset);
    
    // Example: Blink 5 times at 1Hz
    blink_pin_n_times(pio, 0, offset, LED_PIN, 1, 3);
    
    while (true) {
        if (!gpio_get(BUTTON_PIN)) {
            blink_pin_n_times(pio, 0, offset, LED_PIN, 1, 3);
            sleep_ms(500);
        }
    }
}
