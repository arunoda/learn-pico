#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"

#define LED_PIN 13

const char src[] = "Hello World DMA!";
char dest[count_of(src)];

int main()
{
    stdio_init_all();

    int chan = dma_claim_unused_channel(true);

    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, true);

    dma_channel_configure(
        chan,
        &c,
        dest,
        src,
        count_of(src),
        true // start immidiately
    );

    while (true) {
        sleep_ms(1000);
        puts(dest);
    }
}
