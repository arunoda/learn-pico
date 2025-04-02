#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "bitout.pio.h"

#define CLK_PIN 0
#define DATA_PIN 1

// This program send out 8bit data DATA_PIN
// each bit runs with the clock signal on CLK_PIN
// after the last bit there's a wait for about 2 cycles, but the clock output will be high
int main()
{
    stdio_init_all();

    // load pio & sm
    PIO pio;
    uint sm;
    uint offset;

    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&bitout_program, &pio, &sm, &offset, CLK_PIN, 2, true);
    hard_assert(success);

    // pin config
    pio_sm_config c = bitout_program_get_default_config(offset);
    // we use the DATA_PIN as out and set. So, that's why we defined it 
    sm_config_set_out_pins(&c, DATA_PIN, 1);
    sm_config_set_set_pins(&c, DATA_PIN, 1);
    // we do shift_right with autopull after 8 bits
    sm_config_set_out_shift(&c, true, true, 8);
    // set set the CLK_PIN as the side_set pin base
    sm_config_set_sideset_pins(&c, CLK_PIN);

    // initialize pins for GPIO & direction
    pio_gpio_init(pio, CLK_PIN);
    pio_gpio_init(pio, DATA_PIN);
    pio_sm_set_consecutive_pindirs(pio, sm, CLK_PIN, 1, true);
    pio_sm_set_consecutive_pindirs(pio, sm, DATA_PIN, 1, true);

    
    // clock setup
    float freq = 15000;
    float div = clock_get_hz(clk_sys) / (freq * bitout_CYCLES);
    sm_config_set_clkdiv(&c, div);

    // start the sm
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

    while (true) {
        // for now just sending out some data to the sm
        // We should not do it like this way
        pio_sm_put_blocking(pio, sm, 3);
    }
}
