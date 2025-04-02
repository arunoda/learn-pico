#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hello.pio.h"

#define LED_PIN 13

static inline void hello_program_init(PIO pio, uint sm, uint offset, uint pin) {
    pio_sm_config c = hello_program_get_default_config(offset);

    // connect the pin to the state machine
    sm_config_set_out_pins(&c, pin, 1);

    // change the shift direction to the right (2nd argument)
    // then it will shift bits in osr from right to left
    // third argument is the autopull & we don't need it right now
    // 4th(32) is the number of bits it should shift to autopull
    sm_config_set_out_shift(&c, true, false, 32);

    // init the gpio of the pin
    pio_gpio_init(pio, pin);

    // set the direction
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);

    // init pio & sm
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

int main()
{
    stdio_init_all();

    PIO pio;
    uint sm;
    uint offset;

    // Initialize the PIO instance & sm & load the program
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&hello_program, &pio, &sm, &offset, LED_PIN, 1, true);
    hard_assert(success, "Failed to claim SM for GPIO range");

    // initialize the program
    hello_program_init(pio, sm, offset, LED_PIN);

    while (true) {
        // this wait until the FIFO has some space
        // in this case, it doesn't matter as FIFO can have 4 items by defult
        pio_sm_put_blocking(pio, sm, 1);
        sleep_ms(500);
        pio_sm_put_blocking(pio, sm, 0);
        sleep_ms(500);
    }
}
