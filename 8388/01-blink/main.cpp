#include <stdio.h>
#include "pico/stdlib.h"

#define LED_PIN 13

int main()
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        gpio_put(LED_PIN, 1); // Turn on the LED
        sleep_ms(200);        // Wait for 200 milliseconds
        gpio_put(LED_PIN, 0); // Turn off the LED
        sleep_ms(200);        // Wait for 200 milliseconds
    }
}
