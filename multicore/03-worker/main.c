#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#define LED_PIN 13

typedef int32_t (*func_ptr_t)(int32_t);

int32_t worker1(int32_t arg) {
    sleep_ms(1000);
    return arg + 1;
}

void main1() {
    while (true) {
        func_ptr_t callback = (func_ptr_t) multicore_fifo_pop_blocking();
        int32_t arg = multicore_fifo_pop_blocking();
        multicore_fifo_push_blocking(callback(arg));
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    multicore_launch_core1(main1);

    sleep_ms(100);

    //call the initial work
    multicore_fifo_push_blocking((uintptr_t) &worker1);
    multicore_fifo_push_blocking(1);

    while (true) {
        if (multicore_fifo_rvalid()) {
            int32_t rtn_value = multicore_fifo_pop_blocking();
            printf("Got value: %d\n", rtn_value);

            multicore_fifo_push_blocking((uintptr_t) &worker1);
            multicore_fifo_push_blocking(rtn_value);
        }

        gpio_put(LED_PIN, true);
        sleep_ms(125);
        gpio_put(LED_PIN, false);
        sleep_ms(125);
    }
}
