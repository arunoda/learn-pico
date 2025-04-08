#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "i2s.pio.h"

#define SAMPLE_RATE 48000
#define ADC_A1 0

#define CAPTURE_DEPTH 4
uint16_t capture_buf[CAPTURE_DEPTH];

#define I2S_BCK_PIN 0
#define I2S_WS_PIN 1
#define I2S_DATA_PIN 2

PIO pio = pio0;
uint sm = 0;
uint offset;

uint32_t get_desired_clock_khz(uint32_t sample_rate) {
    if (sample_rate %  8000 == 0) {
        return 144000;
    }

    if (sample_rate % 11025 == 0) {
        return 135600;
    }

    return 150000;
}

void init_pio() {
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&i2s_program, &pio, &sm, &offset, I2S_BCK_PIN, 3, true);
    hard_assert(success);

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

int main()
{
    stdio_init_all();

    init_pio();

    adc_init();
    adc_gpio_init(26 + ADC_A1);
    adc_select_input(ADC_A1);

    adc_fifo_setup(
        true,
        true,
        1,
        false,
        false
    );

    // Calculate clock divider based on sample rate
    // ADC runs at 48MHz, and each conversion takes 96 cycles
    float clock_div = (48000000.0f / (SAMPLE_RATE * 96.0f)) - 1.0f;
    adc_set_clkdiv(clock_div);
    
    sleep_ms(3000);
    printf("ADC initialized\n");

    uint dma_channel = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_ADC);

    dma_channel_configure(dma_channel, &c,
        capture_buf,    // dst
        &adc_hw->fifo,  // src
        CAPTURE_DEPTH,  // transfer count
        true            // start immediately
    );

    printf("Starting capture\n");
    adc_run(true);
    
    while (true) {
        dma_channel_wait_for_finish_blocking(dma_channel);
        adc_run(false);
        adc_fifo_drain();

        for (int i = 0; i < CAPTURE_DEPTH; i++) {
            uint16_t val = capture_buf[i];
            uint16_t val_polarized = val - 2048;
        
            int32_t val_for_dac = val_polarized * 16;
            int32_t both_channels = val_for_dac | (val_for_dac << 16);
            pio_sm_put_blocking(pio, sm, both_channels);
        }

        dma_channel_set_read_addr(dma_channel, &adc_hw->fifo, false);
        dma_channel_set_write_addr(dma_channel, capture_buf, false);
        dma_channel_set_trans_count(dma_channel, CAPTURE_DEPTH, true);

        adc_run(true);
    }
}
