#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define LED_PIN 13

int main()
{
    stdio_init_all();
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        gpio_put(LED_PIN, true);
        sleep_ms(1000);
        gpio_put(LED_PIN, false);
        sleep_ms(1000);

        printf("Hello, world from printf!\r\n");
    }
}
