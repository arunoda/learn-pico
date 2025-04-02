#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "blink.pio.h"

#define LED_PIN 13
#define PATTERN_LENGTH_A 2 // Reduced pattern length for non-chaining example
#define PATTERN_LENGTH_B 3 // Reduced pattern length for non-chaining example
#define BUTTON_PIN 12

// Pattern of delays for LED blinking (in cycles)
const uint32_t led_pattern_a[PATTERN_LENGTH_A] = {
    150000000 / (2 * 1) - 3,  // 1 Hz
    150000000 / (2 * 2) - 3,  // 2 Hz
};

const uint32_t led_pattern_b[PATTERN_LENGTH_B] = {
    150000000 / (2 * 1) - 3,  // 1 Hz
    150000000 / (2 * 4) - 3,  // 4 Hz
    150000000 / (2 * 4) - 3,  // 4 Hz
};

int pattern_index = 0;

static PIO pio;
static uint sm;
static uint dma_chan;

void resend_dma() {
    static bool first_time = true;
    
    // Re-trigger DMA transfer
    const uint32_t* pattern = pattern_index == 0 ? led_pattern_a : led_pattern_b;
    uint count = pattern_index == 0 ? PATTERN_LENGTH_A : PATTERN_LENGTH_B;
    pattern_index = (pattern_index + 1) % 2;

    if (first_time) {
        // First time setup - configure DMA channel
        // We are doing here because we use different patterns with different lengths
        // If we setup earlier, even though we are changing it later things will go wrong
        // This approach works without no issues
        dma_channel_config c = dma_channel_get_default_config(dma_chan);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
        channel_config_set_read_increment(&c, true);
        channel_config_set_write_increment(&c, false);
        
        // Initialize DMA channel with write address and config only
        dma_channel_configure(
            dma_chan,          // Channel to be configured
            &c,                // The configuration we just created
            &pio->txf[sm],     // Write address (PIO TX FIFO)
            NULL,              // Read address set below
            0,                 // Transfer count set below
            false              // No auto start
        );
        
        first_time = false;
    }

    // It's very important to set the read address before the transfer count
    dma_channel_set_read_addr(dma_chan, pattern, false);
    dma_channel_set_trans_count(dma_chan, count, true); // Start the transfer
    
    printf("DMA transfer manually re-triggered\n");
}

int main() {
    stdio_init_all();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // PIO Blinking example
    pio = pio0;
    uint offset = pio_add_program(pio, &blink_program);
    printf("Loaded program at %d\n", offset);

    // Initialize PIO program
    sm = 0;
    blink_program_init(pio, sm, offset, LED_PIN);
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
