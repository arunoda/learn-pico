#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"

#define LED_PIN 13

typedef struct {
    int32_t (*worker)(int32_t);
    int32_t arg;
} worker_payload;

queue_t job_queue;
queue_t rtn_queue;

int32_t job(int32_t arg) {
    sleep_ms(1000);
    return arg + 1;
}

void main1() {

    while (true) {
        worker_payload payload;
        queue_remove_blocking(&job_queue, &payload);

        int32_t rtn = payload.worker(payload.arg);
        queue_add_blocking(&rtn_queue, &rtn);
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    queue_init(&job_queue, sizeof(worker_payload), 1);
    queue_init(&rtn_queue, sizeof(int32_t), 1);
    
    multicore_launch_core1(main1);

    worker_payload payload = {job, 1};
    queue_add_blocking(&job_queue, &payload);

    while (true) {
        int32_t rtn_value;
        if (queue_try_remove(&rtn_queue, &rtn_value)) {
            printf("Got value: %d\n", rtn_value);

            payload.arg = rtn_value;
            queue_add_blocking(&job_queue, &payload);
        }

        gpio_put(LED_PIN, true);
        sleep_ms(100);
        gpio_put(LED_PIN, false);
        sleep_ms(100);
    }
}
