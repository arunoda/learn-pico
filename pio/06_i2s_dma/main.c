#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "i2s.pio.h"

#define PI 3.14159265358979323846

// #define I2S_BCK_PIN 0
// #define I2S_WS_PIN 1
// #define I2S_DATA_PIN 2

#define I2S_BCK_PIN 5
#define I2S_WS_PIN 6
#define I2S_DATA_PIN 7

#define SAMPLE_RATE 48000
const int SAMPLES = SAMPLE_RATE / 110; // One period of 440Hz tone (A4 note)
volatile int16_t* buffer;

uint32_t get_desired_clock_khz(uint32_t sample_rate) {
    if (sample_rate %  8000 == 0) {
        return 144000;
    }

    if (sample_rate % 11025 == 0) {
        return 135600;
    }

    return 150000;
}

// PIO initialization
PIO pio = pio0;
uint sm = 0;
uint offset;

// DMA setup
uint dma_chan;

void init_pio() {
    offset = pio_add_program(pio, &i2s_program);
    
    pio_sm_config c = i2s_program_get_default_config(offset);

    //pin config
    sm_config_set_out_pins(&c, I2S_DATA_PIN, 1);
    // add support LSB with autopull after 32 bits (simply because our DAC is PT8211)
    // It's a 16bit stereo DAC with LSB format
    sm_config_set_out_shift(&c, false, true, 32);
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
}

void dma_handler() {
    dma_hw->ints0 = 1u << dma_chan;
    dma_channel_set_read_addr(dma_chan, buffer, false);
    dma_channel_set_trans_count(dma_chan, SAMPLES, true);
}

int main()
{
    stdio_init_all();
    init_pio();

    volatile int16_t sine_wave[SAMPLES * 2];
    buffer = (int16_t*) sine_wave;
    
    // Generate one complete sine wave cycle
    for (int i = 0; i < SAMPLES; i++) {
        // Calculate the phase angle for this sample (0 to 2π)
        float phase = (2.0f * PI * i) / (float)SAMPLES;
        
        // Generate sine wave sample scaled to full 16-bit range (-32768 to 32767)
        int16_t sample = (int16_t)(32767.0f * sin(phase));
        
        // Store the same sample in both left and right channels
        sine_wave[i * 2] = sample;     // Left channel
        sine_wave[i * 2 + 1] = 0;      // Right channel
    }

    dma_chan = dma_claim_unused_channel(true);

    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    dma_channel_configure(dma_chan, &c, &pio->txf[sm], buffer, SAMPLES, true);
    
    // enabled the dma_handler
    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    while (true) {
      
    }
}