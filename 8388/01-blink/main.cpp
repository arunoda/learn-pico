#include <stdio.h>
#include "pico/stdlib.h"

#define LED_PIN 13
#define BUTTON_PIN 12

int main()
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN); // Enable pull-up resistor on button pin

    while (true) {
        bool button_pressed = gpio_get(BUTTON_PIN) == 0; // Check if button is pressed
        
        if (button_pressed) {
            gpio_put(LED_PIN, 0);
        } else {
            gpio_put(LED_PIN, 1); // Turn on the LED
            sleep_ms(200);        // Wait for 200 milliseconds
            gpio_put(LED_PIN, 0); // Turn off the LED
            sleep_ms(200);        // Wait for 200 milliseconds
        }

    }
}
