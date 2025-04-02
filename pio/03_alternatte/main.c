#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "blink.pio.h"

#define OUT_PIN 0

int main()
{
    stdio_init_all();

    // load pio
    PIO pio;
    uint sm;
    uint offset;

    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&blink_program, &pio, &sm, &offset, OUT_PIN, 2, true);
    hard_assert(success);

    // pin config
    pio_sm_config c = blink_program_get_default_config(offset);
    sm_config_set_set_pins(&c, OUT_PIN, 2);

    pio_gpio_init(pio, OUT_PIN);
    pio_gpio_init(pio, OUT_PIN + 1);
    pio_sm_set_consecutive_pindirs(pio, sm, OUT_PIN, 2, true);

    // freq setup
    float freq = 10000;
    float div = clock_get_hz(clk_sys) / (freq * blink_CYCLES);
    sm_config_set_clkdiv(&c, div);

    // pio enabling
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
