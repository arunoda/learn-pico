#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
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

    // Generate sine wave samples for both channels
    const int SAMPLES = SAMPLE_RATE / 440; // 440Hz tone
    int16_t sine_wave[SAMPLES * 2]; // *2 for stereo
    
    for (int i = 0; i < SAMPLES; i++) {
        // Generate sine wave value between -32768 and 32767
        int16_t sample = (int16_t)(32767.0f * sin((2.0f * (float)PI * i) / SAMPLES));
        // Left channel
        sine_wave[i * 2] = sample;
        // Right channel (same data)
        sine_wave[i * 2 + 1] = sample;
    }

    int32_t *buffer = (int32_t *)sine_wave;

    while (true) {
        for (int i = 0; i < SAMPLES; i++) {
            pio_sm_put_blocking(pio, sm, buffer[i]);
        }
    }
}