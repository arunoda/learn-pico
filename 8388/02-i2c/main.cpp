#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// ——— CONFIG —————————————————————————————————
#define PIN_I2C_SDA 0
#define PIN_I2C_SCL 1
#define I2C_BAUD    400000      // 400 kHz fast-mode
static const uint8_t ES8388_ADDR = 0x10;  // AD0/CE tied low

// ——— HELPER: read one register (with repeated-start) —————
static inline void es8388_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(i2c0, 0x10, buf, 2, false);
}

static bool es8388_read(uint8_t reg, uint8_t &out) {
    // write register index, no stop
    if (i2c_write_blocking(i2c0, ES8388_ADDR, &reg, 1, /*no_stop=*/true) < 0)
        return false;
    // repeated-start, read one byte
    return i2c_read_blocking(i2c0, ES8388_ADDR, &out, 1, /*no_stop=*/false) >= 0;
}

int main() {
    stdio_init_all();                  // for printf over USB-CDC/UART
    sleep_ms(2000);                     // let USB-CDC enumerate

    // ——— I²C SETUP ————————————————————————————————
    i2c_init(i2c0, I2C_BAUD);
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);
    sleep_ms(10);                      // give codec time to power up

    // ——— BUS SCAN —————————————————————————————————
    printf("Scanning I2C0 for devices...\n");
    bool found_any = false;
    for (uint8_t addr = 0x03; addr < 0x78; ++addr) {
        if (i2c_write_blocking(i2c0, addr, nullptr, 0, false) >= 0) {
            printf("  ↳ ACK @ 0x%02X\n", addr);
            found_any = true;
        }
    }
    if (!found_any) {
        printf("No I2C devices found! Check SDA/SCL wiring, pull-ups, RESETB, AD0/CE.\n");
        while (true) tight_loop_contents();
    }
    printf("Assuming ES8388 at 0x%02X\n\n", ES8388_ADDR);

    es8388_write(0x01, 0x50);   // CONTROL2 = Play & Record mode, turn on Vref buffer
    es8388_write(0x02, 0x00);

    // ——— READ LOOP ———————————————————————————————
    while (true) {
        uint8_t v0, v4, v8, v24;
        bool ok0  = es8388_read(0x00, v0);
        bool ok4  = es8388_read(0x04, v4);
        bool ok8  = es8388_read(0x08, v8);
        bool ok24 = es8388_read(0x18, v24);

        if (ok0 && ok4 && ok8 && ok24) {
            printf("ES8388  R0=0x%02X  R4=0x%02X  R8=0x%02X  R24=0x%02X\n",
                   v0, v4, v8, v24);
        } else {
            printf("** I2C read error:");
            if (!ok0)  printf(" R0");
            if (!ok4)  printf(" R4");
            if (!ok8)  printf(" R8");
            if (!ok24) printf(" R24");
            printf(" **\n");
        }

        sleep_ms(1000);
    }
}
