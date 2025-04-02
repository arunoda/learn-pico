#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "blink.pio.h"

#define LED_PIN 0

static inline void blink_program_init(PIO pio, uint sm, uint offset, uint pin) {
    pio_sm_config c = blink_program_get_default_config(offset);
    
    // we configure set pins
    // earlier it was out pins & shifting now we don't need to worry about those
    sm_config_set_set_pins(&c, pin, 1);

    // init gpio
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);

    // set the frequency of the run
    // we do that dividing the clock of the sm
    // but we also need to calculate how many cycles our program uses also
    // in this case, we send it via the global manually we defined called CYCLES in the program
    float freq = 10000;
    float div = clock_get_hz(clk_sys) / (freq * blink_CYCLES);
    sm_config_set_clkdiv(&c, div);

    // enable it
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

int main()
{
    stdio_init_all();

    PIO pio;
    uint sm;
    uint offset;

    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&blink_program, &pio, &sm, &offset, LED_PIN, 1, true);
    hard_assert(success, "PIO allocation failed");

    pio_sm_config* config;
    blink_program_init(pio, sm, offset, LED_PIN);
    

    while (true) {
        sleep_ms(100);
    }
}
