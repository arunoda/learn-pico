#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "i2s.pio.h"

#define BCLK_PIN 0
#define SAMPLE_RATE 64

#define BUFFER_SIZE 3
#define BUTTON_PIN 12

const uint32_t buffer[BUFFER_SIZE] = {
    150000000 / (2 * 1) - 3,  // 1 Hz
    150000000 / (2 * 4) - 3,  // 4 Hz
    150000000 / (2 * 4) - 3,  // 4 Hz
};

static PIO pio;
static uint sm;
static uint dma_chan;

void resend_dma() {
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    
    // Initialize DMA channel with write address and config only
    dma_channel_configure(
        dma_chan,          // Channel to be configured
        &c,                // The configuration we just created
        &pio->txf[sm],     // Write address (PIO TX FIFO)
        buffer,              // Read address set below
        BUFFER_SIZE,                 // Transfer count set below
        true              // No auto start
    );
    
    printf("DMA transfer manually re-triggered\n");
}

int main() {
    stdio_init_all();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // PIO Blinking example
    pio = pio0;
    uint offset = pio_add_program(pio, &i2s_program);
    printf("Loaded program at %d\n", offset);

    // Initialize PIO program
    sm = 0;
    i2s_program_init(pio, sm, offset, BCLK_PIN);
    pio_sm_set_enabled(pio, sm, true);

    // Setup DMA with single channel
    dma_chan = dma_claim_unused_channel(true);
    
    // Initial trigger (optional)
    resend_dma();

    while (true) {
        if (!gpio_get(BUTTON_PIN)) {
            printf("Button pressed\n");
            sleep_ms(200);
            resend_dma();
        }
    }
}
