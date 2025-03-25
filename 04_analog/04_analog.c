#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#define PIN_PWM 13
#define PWM_WRAP 65535  // Using 12-bit resolution for better precision (ADC is 12-bit)

// ADC Configuration
#define ADC_PIN 26     // GPIO 26 corresponds to ADC0 (A0 on Pico)
#define ADC_CHANNEL 0  // Channel 0 for ADC0

int main()
{
    stdio_init_all();
    printf("ADC to PWM Demo\n");

    // Initialize ADC
    adc_init();
    adc_gpio_init(ADC_PIN);  // Set up the GPIO for ADC input
    adc_select_input(ADC_CHANNEL);  // Select ADC channel

    // Initialize PWM for the LED pin
    gpio_set_function(PIN_PWM, GPIO_FUNC_PWM);
    
    // Get the PWM slice and channel for the given pin
    uint slice_num = pwm_gpio_to_slice_num(PIN_PWM);
    uint channel = pwm_gpio_to_channel(PIN_PWM);
    
    // Configure PWM with appropriate wrap value
    pwm_set_wrap(slice_num, PWM_WRAP);
    pwm_set_clkdiv(slice_num, 4.0f);  // Slow down PWM clock for smoother control
    pwm_set_enabled(slice_num, true);

    while (true) {
        // Read ADC value
        uint16_t adc_value = adc_read();
        
        // Scale ADC value to PWM range
        // ADC gives 12-bit value (0-4095) & PWM is 16bit so we need to scale it
        uint16_t pwm_level = adc_value * 16;
        
        // Set PWM duty cycle based on scaled ADC value
        pwm_set_chan_level(slice_num, channel, pwm_level);
        
        // Small delay to avoid excessive readings
        sleep_ms(10);
    }
}
