#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

void main2() {
    while(true) {
        printf("Hello from core 2\n");
        sleep_ms(2000);
    }
}

int main()
{
    stdio_init_all();

    multicore_launch_core1(main2);

    while (true) {
        printf("Hello from core 1\n");
        sleep_ms(1000);
    }
}
