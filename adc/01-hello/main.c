#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "i2s.pio.h"

#define PI 3.14159265358979323846

#define I2S_BCK_PIN 0
#define I2S_WS_PIN 1
#define I2S_DATA_PIN 2

#define A0 0
#define A1 1
#define SAMPLE_RATE 48000

// PIO initialization
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
    adc_gpio_init(26 + A0);
    adc_gpio_init(26 + A1);
    
    uint32_t delay_us = 1000000 / SAMPLE_RATE; // Microseconds between samples
    
    absolute_time_t next_sample_time = get_absolute_time();

    while (true) {
        // Calculate next sample time
        next_sample_time = delayed_by_us(next_sample_time, delay_us);

        // Read ADC A0
        adc_select_input(A0);
        uint16_t val0 = adc_read();
        
        // Read ADC A1
        adc_select_input(A1);
        uint16_t val1 = adc_read();
        
        // Add the values
        uint16_t val_combined = val0 + val1;
        // Scale appropriately to avoid overflow
        val_combined = val_combined / 2;
        
        float val_normalized = (val_combined - 2048) / 2048.0;
        
        int32_t val_for_dac = val_normalized * 32767;
        int32_t both_channels = val_for_dac | (val_for_dac << 16);
        
        pio_sm_put_blocking(pio, sm, both_channels);
        
        // Wait until it's time for the next sample
        sleep_until(next_sample_time);
    }
}
