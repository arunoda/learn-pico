#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

void main2() {
    while (true)
    {
        uint32_t val = multicore_fifo_pop_blocking();
        printf("Core2: %d\n", val);
        val += 1;
        sleep_ms(1000);
        multicore_fifo_push_blocking(val);
    }
} 

int main()
{
    stdio_init_all();

    multicore_launch_core1(main2);
    // push is blocking because the FIFO size is 4 words of 32bit values.
    multicore_fifo_push_blocking(1);

    while (true) {
        uint32_t val = multicore_fifo_pop_blocking();
        printf("Core1: %d\n", val);
        val += 1;
        sleep_ms(1000);
        multicore_fifo_push_blocking(val);
    }
}
