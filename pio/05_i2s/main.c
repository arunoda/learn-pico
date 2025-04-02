#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "i2s.pio.h"

#define PI 3.14159265358979323846

#define LED_PIN 13
#define BUTTON_PIN 12

// #define I2S_BCK_PIN 0
// #define I2S_WS_PIN 1
// #define I2S_DATA_PIN 2

#define I2S_BCK_PIN 5
#define I2S_WS_PIN 6
#define I2S_DATA_PIN 7


#define SAMPLE_RATE 48000

uint32_t get_desired_clock_khz(uint32_t sample_rate) {
    if (sample_rate %  8000 == 0) {
        return 144000;
    }

    if (sample_rate % 11025 == 0) {
        return 135600;
    }

    return 150000;
}

int main()
{
    stdio_init_all();

    // PIO initialization
    PIO pio = pio0;
    uint sm = 0;
    uint offset;

    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&i2s_program, &pio, &sm, &offset, I2S_BCK_PIN, 3, true);
    hard_assert(success);

    pio_sm_config c = i2s_program_get_default_config(offset);

    //pin config
    sm_config_set_out_pins(&c, I2S_DATA_PIN, 1);
    sm_config_set_out_shift(&c, true, true, 32);
    sm_config_set_sideset_pins(&c, I2S_BCK_PIN);

    // Increase the FIFO size to 8 words
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    //gpio setup
    pio_gpio_init(pio, I2S_BCK_PIN);
    pio_gpio_init(pio, I2S_WS_PIN);
    pio_gpio_init(pio, I2S_DATA_PIN);
    pio_sm_set_consecutive_pindirs(pio, sm, I2S_BCK_PIN, 3, true);

    //clock setup
    set_sys_clock_khz(get_desired_clock_khz(SAMPLE_RATE), true);
    float div = clock_get_hz(clk_sys) / (SAMPLE_RATE * 32 * i2s_BCK_CYCLES);
    sm_config_set_clkdiv(&c, div);

    //start the sm
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

    // Generate sine wave samples for stereo output
    const int SAMPLES = SAMPLE_RATE / 440; // One period of 440Hz tone (A4 note)
    int16_t sine_wave[SAMPLES * 2]; // *2 for stereo (left and right channels)
    
    // Generate one complete sine wave cycle
    for (int i = 0; i < SAMPLES; i++) {
        // Calculate the phase angle for this sample (0 to 2π)
        float phase = (2.0f * PI * i) / SAMPLES;
        
        // Generate sine wave sample scaled to full 16-bit range (-32768 to 32767)
        int16_t sample = (int16_t)(32767.0f * sin(phase));
        
        // Store the same sample in both left and right channels
        sine_wave[i * 2] = sample;     // Left channel
        sine_wave[i * 2 + 1] = sample; // Right channel
    }

    int32_t *buffer = (int32_t *)sine_wave;

    // DMA setup
    uint dma_chan = dma_claim_unused_channel(true);
    dma_channel_config dma_config = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_config, true);
    channel_config_set_write_increment(&dma_config, false);
    channel_config_set_dreq(&dma_config, pio_get_dreq(pio, sm, true));
    channel_config_set_chain_to(&dma_config, dma_chan);
    channel_config_set_irq_quiet(&dma_config, false); // Need IRQs
    dma_channel_set_irq0_enabled(dma_chan, true);
    
    dma_channel_configure(
        dma_chan,          // Channel to be configured
        &dma_config,                // The configuration we just created
        &pio->txf[sm],     // Write address (PIO TX FIFO)
        sine_wave,              // Read address set below
        SAMPLES,                 // Transfer count set below
        true              // No auto start
    );

    // button setup
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        if (!gpio_get(BUTTON_PIN)) {
            gpio_put(LED_PIN, true);
            // It's very important to set the read address before the transfer count
            dma_channel_set_read_addr(dma_chan, sine_wave, false);
            dma_channel_set_trans_count(dma_chan, SAMPLES, true); // Start the transfer
            sleep_ms(10);
        } else {
            gpio_put(LED_PIN, false);
        }
    }
}