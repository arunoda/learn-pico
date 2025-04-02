#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hello.pio.h"

#define LED_PIN 13

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
        pio_sm_put_blocking(pio, sm, 3);
        sleep_ms(500);
        pio_sm_put_blocking(pio, sm, 0);
        sleep_ms(500);
    }
}
