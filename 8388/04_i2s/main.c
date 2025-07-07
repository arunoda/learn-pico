#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "i2s.pio.h"

#define PI 3.14159265358979323846

#define I2S_MCLK_PIN 3
#define I2S_BCK_PIN 4
#define I2S_WS_PIN 5
#define I2S_DATA_PIN 6

// #define I2S_BCK_PIN 5
// #define I2S_WS_PIN 6
// #define I2S_DATA_PIN 7


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

// PIO initialization
PIO pio = pio0;
uint sm = 0;
uint offset;

void init_pio_i2s() {
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&i2s_program, &pio, &sm, &offset, I2S_BCK_PIN, 3, true);
    hard_assert(success);

    pio_sm_config c = i2s_program_get_default_config(offset);

    //pin config
    sm_config_set_out_pins(&c, I2S_DATA_PIN, 1);
    // add support LSBJ with autopull after 32 bits (simply because our DAC is PT8211)
    // It's a 16bit stereo DAC with LSBJ format
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

void init_pwm_mclk() {
    // Find which PWM slice is connected to I2S_MCLK_PIN
    uint slice_num = pwm_gpio_to_slice_num(I2S_MCLK_PIN);
    
    // Set the pin to be controlled by PWM
    gpio_set_function(I2S_MCLK_PIN, GPIO_FUNC_PWM);
    
    // Get default PWM configuration
    pwm_config config = pwm_get_default_config();
    
    // Calculate the required frequency: SAMPLE_RATE * 256
    uint32_t target_freq = SAMPLE_RATE * 256;
    
    // Calculate clock divider and wrap value for the target frequency
    // PWM frequency = system_clock / (divider * wrap)
    // For 50% duty cycle, we want wrap to be even
    uint32_t system_clock = clock_get_hz(clk_sys);
    
    // Start with divider = 1 and find appropriate wrap value
    uint32_t divider = 1;
    uint32_t wrap;
    
    do {
        wrap = system_clock / (divider * target_freq);
        if (wrap <= 65535) break;  // PWM wrap must fit in 16 bits
        divider++;
    } while (divider <= 256);  // PWM divider is 8-bit integer + 4-bit fractional
    
    // Set the divider and wrap
    pwm_config_set_clkdiv_int(&config, divider);
    pwm_config_set_wrap(&config, wrap - 1);  // wrap is top value, so subtract 1
    
    // Apply configuration
    pwm_init(slice_num, &config, true);
    
    // Set duty cycle to 50% (half of wrap value)
    pwm_set_gpio_level(I2S_MCLK_PIN, wrap / 2);
    
    printf("PWM MCLK initialized: freq=%u Hz, divider=%u, wrap=%u\n", 
           target_freq, divider, wrap);
}

int main()
{
    stdio_init_all();

    init_pio_i2s();
    init_pwm_mclk();  // Use PWM instead of PIO for MCLK

    // Generate sine wave samples for stereo output
    const int SAMPLES = SAMPLE_RATE / 110; // One period of 440Hz tone (A4 note)
    int16_t sine_wave[SAMPLES * 2]; // *2 for stereo (left and right channels)
    
    // Generate one complete sine wave cycle
    for (int i = 0; i < SAMPLES; i++) {
        // Calculate the phase angle for this sample (0 to 2π)
        float phase = (2.0f * PI * i) / (float)SAMPLES;
        
        // Generate sine wave sample scaled to full 16-bit range (-32768 to 32767)
        int16_t sample = (int16_t)(32767.0f * sin(phase));
        
        // Store the same sample in both left and right channels
        sine_wave[i * 2] = sample;     // Left channel
        sine_wave[i * 2 + 1] = sample;      // Right channel
    }

    int32_t *buffer = (int32_t *)sine_wave;

    while (true) {
        for (int i = 0; i < SAMPLES; i++) {
            pio_sm_put_blocking(pio, sm, buffer[i]);
        }
    }
}