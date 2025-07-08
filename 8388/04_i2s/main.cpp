#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "i2s.pio.h"

#define PI 3.14159265358979323846

#define I2S_MCLK_PIN 3
#define I2S_BCK_PIN 4
#define I2S_WS_PIN 5
#define I2S_DATA_PIN 6
#define I2S_DATA_IN_PIN 7

// ——— I2C CONFIG —————————————————————————————————
#define PIN_I2C_SDA 0
#define PIN_I2C_SCL 1
#define I2C_BAUD    400000      // 400 kHz fast-mode
static const uint8_t ES8388_ADDR = 0x10;  // AD0/CE tied low

#define SAMPLE_RATE 44100

// PIO initialization
PIO pio = pio0;
uint sm = 0;
uint offset;


// ——— HELPER: read one register (with repeated-start) —————
static inline void es8388_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(i2c0, ES8388_ADDR, buf, 2, false);
}

static bool es8388_read(uint8_t reg, uint8_t &out) {
    // write register index, no stop
    if (i2c_write_blocking(i2c0, ES8388_ADDR, &reg, 1, /*no_stop=*/true) < 0)
        return false;
    // repeated-start, read one byte
    return i2c_read_blocking(i2c0, ES8388_ADDR, &out, 1, /*no_stop=*/false) >= 0;
}

uint32_t get_desired_clock_khz(uint32_t sample_rate) {
    if (sample_rate %  8000 == 0) {
        return 144000;
    }

    if (sample_rate % 11025 == 0) {
        return 135600;
    }

    return 150000;
}

void init_pio_i2s() {
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&i2s_slave_program, &pio, &sm, &offset, I2S_DATA_PIN, 1, true);
    hard_assert(success);

    pio_sm_config c = i2s_slave_program_get_default_config(offset);

    //pin out config
    sm_config_set_out_pins(&c, I2S_DATA_PIN, 1);
    // Enable autopull after 32 bits (16 bits left + 16 bits right) for 16-bit LSB format
    sm_config_set_out_shift(&c, false, true, 32);
    
    // No side-set pins needed - we're following external clocks
    // No in pins needed for this output-only example

    //gpio setup - set up all pins that PIO needs to access
    pio_gpio_init(pio, I2S_BCK_PIN);    // PIO needs to read BCK for wait instructions
    pio_gpio_init(pio, I2S_WS_PIN);     // PIO needs to read WS for wait instructions  
    pio_gpio_init(pio, I2S_DATA_PIN);   // PIO needs to output data
    
    // Set BCK and WS pins as inputs (they come from the codec)
    pio_sm_set_consecutive_pindirs(pio, sm, I2S_BCK_PIN, 1, false);  // BCK input
    pio_sm_set_consecutive_pindirs(pio, sm, I2S_WS_PIN, 1, false);   // WS input
    
    // Set data pin as output
    pio_sm_set_consecutive_pindirs(pio, sm, I2S_DATA_PIN, 1, true);  // DATA output

    //clock setup - no divider needed since we follow external clocks
    // The PIO will run at full speed and wait for external clock edges
    
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

void setup_8388() {
    // ——— I²C SETUP ————————————————————————————————
    i2c_init(i2c0, I2C_BAUD);
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);
    sleep_ms(10);                      // give codec time to power up

    // Power up ES8388 codec
    es8388_write(0, 0b00010110); // R0: set to defaults: 0000 0110
    es8388_write(1, 0b01010000); // R1: ebable analog power: 0101 0000
    es8388_write(2, 0b00000000); // R2: enable chip power: 0000 0000
    es8388_write(3, 0b00001000); // R3: enable ADCs: 0000 1100
    es8388_write(4, 0b00111100); // R4: enable DACs: 0011 1100
    es8388_write(5, 0b00000000); // R5: no low power mode: 0000 0000
    es8388_write(6, 0b00000000); // R6: no low power mode: 0000
    es8388_write(7, 0b01111100); // R7: anaog voltage mgt (default): 0111 1100
    es8388_write(8, 0b10000000); // R8: master/slave mode: master with MCLK/4

    // ADC settings (we will do more later, no needed for this example)
    es8388_write(9, 0b00000000); // R9: mic gain to 0db: 0000 0000
    es8388_write(10, 0b00000000); // Selet inputs
    es8388_write(11, 0b00000010); // Select ADC inputs
    es8388_write(12, 0b00001101); // R12: Set ADC to 16bit, LSBJ format, and enable ADC clock
    es8388_write(13, 0b00000010); // R13: ADC set clocks
    es8388_write(14, 0b00110000);
    es8388_write(15, 0b00100000);
    es8388_write(16, 0b00000000); // R16: ADCL volume, we need to set it to 0000 0000 of 0db
    es8388_write(17, 0b00000000); // R17: DACR volume, we need to set it to 0000 0000 of 0db
    es8388_write(18, 0b00111000);
    es8388_write(19, 0b10110000);
    es8388_write(20, 0b00110011);
    es8388_write(21, 0b00000110);

    // DAC and output settings
    es8388_write(23, 0b00011010); // R23: 16bit with LSB
    es8388_write(24, 0b00000010); // R24: May not need since we use slave mode: but set to 256 MCLK/Sample Rate
    es8388_write(26, 0b00000000); // R26: DACL volume, we need to set it to 0000 0000 of 0db
    es8388_write(27, 0b00000000); // R27: DACR volume, we need to set it to 0000 0000 of 0db
    es8388_write(28, 0b00000000); // R28: some phase inversion and few defaults. set to defaults: 0000 0000
    es8388_write(29, 0b00000000); // 
    es8388_write(38, 0b00000000); // R38: LIN select: 0000 0000 (LIN1 -> LEFT, RIN1 -> RIGHT)
    es8388_write(39, 0b10010000); // R39: need to enable DAC to Mixer and 0db volume 0101 0000
    es8388_write(42, 0b10010000); // R42: need to enable RIN to Mixer and 0db volume 0101 0000
    es8388_write(43, 0b11000000);
    es8388_write(45, 0b00000000);
    es8388_write(46, 0b00011110); // R46: LOUT1 volume: need to change to 00011110 for 0db
    es8388_write(47, 0b00000000); // R47: ROUT1 volume: moved to 0000 0000 for -45db
    es8388_write(48, 0b00000000); // R48: LOUT2 volume: moved to 0000 0000 for -45db
    es8388_write(49, 0b00000000); // R49: ROUT2 volume: moved to 0000 0000 for -45db
}


int main()
{
    stdio_init_all();
    setup_8388();

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
    int32_t read_sample = 0;

    init_pio_i2s();       // Initialize I2S slave PIO
    init_pwm_mclk();      // Use PWM for MCLK to codec

    while (true) {
        // setup_8388();
        for (int i = 0; i < SAMPLES; i++) {
            pio_sm_put_blocking(pio, sm, buffer[i]);
        }
    }
}