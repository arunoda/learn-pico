#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

// Here we need to RC low pass filter with 10k & 10nf to get some smoothing
// This is not a good solution for audio, but it's good fo CVs since
// most of our setup is using Vactrols.
// So, it will ignore the high frequency noise.
#define PIN_LED 13
#define PIN_AUDIO 7
#define PWM_WRAP 4095

#define ADC_PIN 26     // GPIO 26 corresponds to ADC0 (A0 on Pico)
#define ADC_CHANNEL 0

struct PWM_DATA {
    uint pin;
    uint slice_num;
    uint channel;
};

typedef struct PWM_DATA PWM_DATA;

PWM_DATA setup_pwn(uint pin) {
    gpio_init(pin);
    gpio_set_function(pin, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint channel = pwm_gpio_to_channel(pin);

    // With these setting it runs about 36khz
    // If we double the wrap value, frequency halves
    pwm_set_wrap(slice_num, PWM_WRAP);
    pwm_set_clkdiv(slice_num, 1.0f);
    pwm_set_enabled(slice_num, true);

    return (PWM_DATA) {
        pin,
        slice_num,
        channel
    };
}

int main()
{
    stdio_init_all();

    PWM_DATA pwm_led = setup_pwn(PIN_LED);
    PWM_DATA pwm_audio = setup_pwn(PIN_AUDIO);

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_CHANNEL); 

    while (true) {
        uint16_t adc_value = adc_read();
        
        pwm_set_chan_level(pwm_led.slice_num, pwm_led.channel, adc_value);
        pwm_set_chan_level(pwm_audio.slice_num, pwm_audio.channel, adc_value);

        sleep_ms(10);
    }
}
