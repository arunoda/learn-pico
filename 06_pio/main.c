#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

#include "blink.pio.h"

#define LED_PIN 13

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);

    // Here we send data to the PIO program, it's FIFO queue
    pio->txf[sm] = 0xDEADBEEF; // Dummy value for first pull

    // Second value - the actual delay value
    // PIO counter program takes 3 more cycles in total than we pass as
    pio->txf[sm] = (150000000 / (2 * freq)) - 3;
}



int main()
{
    stdio_init_all();

    // PIO Blinking example
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &blink_program);
    printf("Loaded program at %d\n", offset);
    
    blink_pin_forever(pio, 0, offset, LED_PIN, 1);

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
