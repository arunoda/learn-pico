#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/audio_i2s.h"
#include "hardware/gpio.h"
#include <math.h>

// Define GPIO pins for I2S
#define I2S_DATA_PIN 0
#define I2S_BCLK_PIN 10
#define I2S_WS_PIN 11
#define BUTTON_PIN 12  // Button on GPIO12

// Audio sample rate and buffer
#define SAMPLE_RATE 64
#define SAMPLES_PER_BUFFER 64  // 1 second of audio at 64Hz sample rate

// Audio buffer
int16_t audio_buffer[SAMPLES_PER_BUFFER * 2]; // Left and right channels

// Function to generate sine wave
void generate_sine_wave(int16_t* buffer, int num_samples, float frequency) {
    const float amplitude = 32767.0f; // Max value for 16-bit signed integer
    const float angular_frequency = 2.0f * 3.14159f * frequency;
    
    for (int i = 0; i < num_samples; i++) {
        float time = (float)i / SAMPLE_RATE;
        float sample_value = amplitude * sinf(angular_frequency * time);
        
        // Interleave left and right channels with the same value
        buffer[i * 2] = (int16_t)sample_value;     // Left channel
        buffer[i * 2 + 1] = (int16_t)sample_value; // Right channel
    }
}

audio_buffer_pool_t *audio_buffer_pool = NULL;

// Function to initialize audio
audio_buffer_pool_t *init_audio() {
    static audio_format_t audio_format = {
        .sample_freq = SAMPLE_RATE,
        .format = AUDIO_BUFFER_FORMAT_PCM_S16,
        .channel_count = 2,
    };

    static audio_buffer_format_t producer_format = {
        .format = &audio_format,
        .sample_stride = 4
    };

    // Create buffer pool with 2 buffers of SAMPLES_PER_BUFFER samples each
    audio_buffer_pool_t *producer_pool = audio_new_producer_pool(&producer_format, 2, SAMPLES_PER_BUFFER);
    
    // Configure I2S
    audio_i2s_config_t config = {
        .data_pin = I2S_DATA_PIN,
        .clock_pin_base = I2S_BCLK_PIN, // BCLK will be clock_pin_base, WS will be clock_pin_base+1
        .dma_channel = 0,
        .pio_sm = 0,
    };

    // Initialize I2S with the configuration
    const audio_format_t *output_format = audio_i2s_setup(&audio_format, &config);
    if (!output_format) {
        printf("Audio format not supported\n");
        return NULL;
    }

    // Connect the producer pool to the I2S output
    audio_i2s_connect(producer_pool);
    
    // Enable I2S output
    audio_i2s_set_enabled(true);
    
    return producer_pool;
}

int main() {
    stdio_init_all();

    // Generate a 1Hz sine wave
    generate_sine_wave(audio_buffer, SAMPLES_PER_BUFFER, 1.0f);

    // Initialize audio
    audio_buffer_pool = init_audio();
    if (!audio_buffer_pool) {
        printf("Failed to initialize audio\n");
        return -1;
    }

    // Initialize button with pull-up
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    
    printf("Press the button on GPIO12 to play audio...\n");
    
    bool last_button_state = true; // Pull-up means button is high when not pressed
    
    while (true) {
        bool current_button_state = gpio_get(BUTTON_PIN);
        
        // Detect button press (high to low transition)
        if (last_button_state && !current_button_state) {
            printf("Button pressed! Triggering audio...\n");
            
            // Get a free buffer from the pool
            audio_buffer_t *buffer = take_audio_buffer(audio_buffer_pool, false);
            if (buffer) {
                // Copy our pre-generated audio data to the buffer
                memcpy(buffer->buffer->bytes, audio_buffer, SAMPLES_PER_BUFFER * 4); // 2 channels * 2 bytes per sample
                buffer->sample_count = SAMPLES_PER_BUFFER;
                
                // Give the buffer back to be played
                give_audio_buffer(audio_buffer_pool, buffer);
            }
            
            sleep_ms(300); // Simple debounce
        }
        
        last_button_state = current_button_state;
        sleep_ms(10); // Small delay for debouncing
    }
}