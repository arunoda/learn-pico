#include <stdio.h>
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pico/stdlib.h"
#include "led.pio.h"

#define LED_PIN 13

PIO pio = pio2;
uint sm = 1;
uint offset;
int dma_chan;
uint32_t pwm_data[32];

uint pwm_level = 0;

void pio_init() {
    offset = pio_add_program(pio, &led_program);

    pio_gpio_init(pio, LED_PIN);
    pio_sm_set_consecutive_pindirs(pio, sm, LED_PIN, 1, true);

    pio_sm_config c = led_program_get_default_config(offset);
    sm_config_set_out_pins(&c, LED_PIN, 1);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv(&c, 20);
    sm_config_set_out_shift(&c, true, true, 32);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

void dma_handler() {
    if (pwm_level == 0) {
        printf("Starting a new PWM level\n");
    }
    // clear up the interrupt
    // otherwise it won't trigger the next interrupt
    dma_hw->ints0 = 1u << dma_chan;
    dma_channel_set_read_addr(dma_chan, &pwm_data[pwm_level], true);

    pwm_level = (pwm_level + 1) % 32;
}

int main()
{
    stdio_init_all();

    //generate pwm_data
    for (int lc=0; lc<32; lc++) {
        pwm_data[lc] = ~(~0u << lc);
    }

    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));

    dma_channel_configure(dma_chan, &c, &pio->txf[sm], NULL, 10000, false);

    // configure the CPU interrupst for the DMA channel with the IRQ0
    dma_channel_set_irq0_enabled(dma_chan, true);
    // set up the handler to trigger
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    pio_init();

    dma_handler();

    while (true) {
        
    }
}
