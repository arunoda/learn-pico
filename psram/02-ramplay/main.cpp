#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/structs/xip.h"
#include "DAC.h"
#include "samples/s01.h"

#define SAMPLE_RATE 44100

// Get PSRAM base address
volatile uint8_t* const PSRAM = (volatile uint8_t*)0x11000000;
volatile uint16_t* const PSRAM_AUDIO_BUFFER = (volatile uint16_t*)PSRAM;
const uint32_t buffer_len = s01_wav_len / 2;

DAC dac(pio0, 0, 1);

int main()
{
    stdio_init_all();

    // Configure PSRAM CS pin
    gpio_set_function(0, GPIO_FUNC_XIP_CS1); 
    // Make sure PSRAM range is writable
    xip_ctrl_hw->ctrl |= XIP_CTRL_WRITABLE_M1_BITS;

    sleep_ms(2000);
    dac.init(SAMPLE_RATE);

    // Write from flash to PSRAM
    // This sound useless, but it's just for testing that PSRAM is working
    for (int i = 0; i < s01_wav_len; i++) {
        PSRAM[i] = s01_wav[i];
    }

    while (true) {
        for (int i = 0; i < buffer_len; i++) {
            // Play the sample
            dac.writeMono(PSRAM_AUDIO_BUFFER[i], PSRAM_AUDIO_BUFFER[i]);
        }
    }
}
