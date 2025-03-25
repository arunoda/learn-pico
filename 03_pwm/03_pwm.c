#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"

#define PIN_PWM 13
#define PWM_WRAP 20000  // Maximum is 65535 but we can tune it based on the LED brightness

// Sine wave parameters
#define SINE_STEPS 100  // Number of steps in one sine wave cycle
#define SINE_SPEED 20    // Speed of the sine wave (ms per step)

int main()
{
    stdio_init_all();
    printf("PWM Sine Wave Demo\n");

    // Initialize PWM for the LED pin
    gpio_set_function(PIN_PWM, GPIO_FUNC_PWM);
    
    // Slice is the PWM generator and each channel has two different channels
    // They share same clock but can have different duty cycles
    // Here we will find out the slice and the channel for the given pin
    uint slice_num = pwm_gpio_to_slice_num(PIN_PWM);
    uint channel = pwm_gpio_to_channel(PIN_PWM);
    
    // Configure PWM with maximum wrap value for smooth brightness
    pwm_set_wrap(slice_num, PWM_WRAP);
    pwm_set_clkdiv(slice_num, 4.0f);  // Slow down PWM clock for smoother control
    pwm_set_enabled(slice_num, true);

    // Precompute sine wave values for better performance
    uint16_t sine_values[SINE_STEPS];
    for (int i = 0; i < SINE_STEPS; i++) {
        // Calculate sine value between 0 and 1
        // sin() returns values between -1 and 1, so we add 1 and divide by 2
        float angle = (float)i * 2.0f * 3.14159f / SINE_STEPS;
        float sine_val = (sinf(angle) + 1.0f) / 2.0f;
        // Scale to PWM range
        sine_values[i] = (uint16_t)(sine_val * PWM_WRAP);
    }
    
    int sine_index = 0;

    while (true) {
        // Set PWM duty cycle based on sine wave
        pwm_set_chan_level(slice_num, channel, sine_values[sine_index]);
        
        // Move to next step in sine wave
        sine_index = (sine_index + 1) % SINE_STEPS;
        
        // Control the speed of the sine wave
        sleep_ms(SINE_SPEED);
    }
}
