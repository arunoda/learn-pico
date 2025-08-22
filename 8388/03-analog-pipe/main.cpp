#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// ——— CONFIG —————————————————————————————————
#define PIN_I2C_SDA 2
#define PIN_I2C_SCL 3
#define I2C_BAUD    400000      // 400 kHz fast-mode
static const uint8_t ES8388_ADDR = 0x10;  // AD0/CE tied low

// ——— HELPER: read one register (with repeated-start) —————
static inline void es8388_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(i2c1, ES8388_ADDR, buf, 2, false);
}

static bool es8388_read(uint8_t reg, uint8_t &out) {
    // write register index, no stop
    if (i2c_write_blocking(i2c1, ES8388_ADDR, &reg, 1, /*no_stop=*/true) < 0)
        return false;
    // repeated-start, read one byte
    return i2c_read_blocking(i2c1, ES8388_ADDR, &out, 1, /*no_stop=*/false) >= 0;
}

int main() {
    stdio_init_all();                  // for printf over USB-CDC/UART
    sleep_ms(2000);                     // let USB-CDC enumerate

    // ——— I²C SETUP ————————————————————————————————
    i2c_init(i2c1, I2C_BAUD);
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);
    sleep_ms(10);                      // give codec time to power up

    // Power up ES8388 codec
    es8388_write(0, 0b00000110); // R0: set to defaults: 0000 0110
    es8388_write(1, 0b01010000); // R1: ebable analog power: 0101 0000
    es8388_write(2, 0b00000000); // R2: enable chip power: 0000 0000
    es8388_write(3, 0b00001100); // R3: enable ADCs: 0000 1100
    es8388_write(4, 0b00111100); // R4: enable DACs: 0011 1100
    es8388_write(5, 0b00000000); // R5: no low power mode: 0000 0000
    es8388_write(6, 0b00000000); // R6: no low power mode: 0000
    es8388_write(7, 0b01111100); // R7: anaog voltage mgt (default): 0111 1100
    es8388_write(8, 0b10000000); // R8: master/slave mode: master mode selected: 1000 0000

    // ADC settings (we will do more later, no needed for this example)
    es8388_write(9, 0b00000000); // R9: mic gain to 0db: 0000 0000
    es8388_write(14, 0b00110000);
    es8388_write(15, 0b00100000);
    es8388_write(16, 0b00000000); // R16: ADCL volume, we need to set it to 0000 0000 of 0db
    es8388_write(17, 0b00000000); // R17: DACR volume, we need to set it to 0000 0000 of 0db
    
    // DAC and output settings
    es8388_write(26, 0b00000000); // R26: DACL volume, we need to set it to 0000 0000 of 0db
    es8388_write(27, 0b00000000); // R27: DACR volume, we need to set it to 0000 0000 of 0db
    es8388_write(28, 0b00000000); // R28: some phase inversion and few defaults. set to defaults: 0000 0000
    es8388_write(29, 0b00000000); // 
    es8388_write(38, 0b00000000); // R38: LIN select: 0000 0000 (LIN1 -> LEFT, RIN1 -> RIGHT)
    es8388_write(39, 0b01010000); // R39: need to enable LIN to Mixer and 0db volume 0101 0000
    es8388_write(42, 0b01010000); // R42: need to enable RIN to Mixer and 0db volume 0101 0000
    es8388_write(45, 0b00000000);
    es8388_write(46, 0b00011110); // R46: LOUT1 volume: need to change to 00011110 for 0db
    es8388_write(47, 0b00011110); // R47: ROUT1 volume: need to change to 00011110 for 0db
    es8388_write(48, 0b00000000); // R48: LOUT2 volume: moved to 0000 0000 for -45db
    es8388_write(49, 0b00000000); // R49: ROUT2 volume: moved to 0000 0000 for -45db

    // ——— READ LOOP ———————————————————————————————
    while (true) {
        // printf("Registry Values:\n");
        // printf("----------------------\n");
        // for (uint8_t lc=0; lc<53; lc++) {
        //     uint8_t v;
        //     if (es8388_read(lc, v)) {
        //         printf("ES8388 R%d=0x%02X (", lc, v);
        //         for (int i = 7; i >= 0; i--) {
        //             printf("%d", (v >> i) & 1);
        //             if (i == 4) printf(" "); // space after 4th bit
        //         }
        //         printf(")\n");
        //     } else {
        //         printf("** I2C read error: R%d **\n", lc);
        //     }
        // }

        // printf("----------------------\n");

        sleep_ms(3000);
        printf("ES8388 codec initialized and running.\n");
    }
}
