#include "pico/stdlib.h"
#include "blink.h"

static int led_pin = 0;

void blink_init(int pin) {
    led_pin = pin;
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
}

void blink_once(int on_ms, int off_ms) {
    gpio_put(led_pin, 1);
    sleep_ms(on_ms);
    gpio_put(led_pin, 0);
    sleep_ms(off_ms);
} 