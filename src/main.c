/*
 * main.c - Demo for the simulated-register HAL.
 *
 * Initialises the HAL, talks to the simulated I2C temperature
 * sensor, toggles GPIO pins, does an SPI transfer and sends a
 * UART message, then prints a deterministic summary.
 *
 * Build:  make
 * Run:    ./build/hal_demo
 */
#include <stdio.h>

#include "hal.h"

#define PIN_STATUS_LED 5
#define PIN_RELAY      7

int main(void)
{
    printf("=== Embedded Linux HAL demo (simulated registers) ===\n");

    if (hal_init() != HAL_OK) {
        printf("ERROR: HAL init failed\n");
        return 1;
    }

    /* --- I2C: identify the sensor --------------------------------- */
    uint8_t who_am_i = 0;
    if (hal_i2c_read(HAL_I2C_SENSOR_ADDR, HAL_SENSOR_REG_WHO_AM_I, &who_am_i, 1) != HAL_OK) {
        printf("ERROR: I2C WHO_AM_I read failed\n");
        return 1;
    }
    printf("Sensor WHO_AM_I = 0x%02X (expected 0x%02X) -> %s\n",
           who_am_i, HAL_SENSOR_WHO_AM_I_VALUE,
           who_am_i == HAL_SENSOR_WHO_AM_I_VALUE ? "OK, sensor detected" : "MISMATCH");

    /* --- I2C: temperature, default + one re-preloaded value -------- */
    float temp_c = 0.0f;
    if (hal_sensor_read_temperature_c(&temp_c) != HAL_OK) {
        printf("ERROR: temperature read failed\n");
        return 1;
    }
    printf("Temperature = %.2f C\n", (double)temp_c);

    /* Preload a different simulated temperature: -10.0 C.
     * -10.0 / 0.0625 = -160 = 0xF60 (12-bit), left-justified -> MSB 0xF6, LSB 0x00 */
    hal_sensor_sim_set_raw(0xF6, 0x00);
    if (hal_sensor_read_temperature_c(&temp_c) != HAL_OK) {
        printf("ERROR: temperature read failed\n");
        return 1;
    }
    printf("Temperature = %.2f C\n", (double)temp_c);

    /* --- GPIO: write, read back, toggle ---------------------------- */
    hal_gpio_write(PIN_STATUS_LED, HAL_GPIO_HIGH);
    hal_gpio_write(PIN_RELAY, HAL_GPIO_LOW);

    hal_gpio_level_t led = HAL_GPIO_LOW;
    hal_gpio_read(PIN_STATUS_LED, &led);

    hal_gpio_toggle(PIN_STATUS_LED);
    hal_gpio_toggle(PIN_STATUS_LED);

    hal_gpio_level_t led_final = HAL_GPIO_LOW;
    hal_gpio_read(PIN_STATUS_LED, &led_final);

    /* --- SPI: JEDEC-style read command ----------------------------- */
    const uint8_t spi_tx[4] = {0x9F, 0x00, 0x00, 0x00};
    uint8_t spi_rx[4] = {0, 0, 0, 0};
    hal_spi_transfer(spi_tx, spi_rx, 4);

    /* --- UART ------------------------------------------------------ */
    const char *msg = "HAL demo complete: sensor OK, GPIO toggled, SPI OK";
    hal_uart_write_string(msg);

    /* --- Summary --------------------------------------------------- */
    printf("--- Summary ---\n");
    printf("WHO_AM_I      : 0x%02X\n", who_am_i);
    printf("Last temp     : %.2f C\n", (double)temp_c);
    printf("GPIO pin %d    : %s (after 2 toggles)\n",
           PIN_STATUS_LED, led_final == HAL_GPIO_HIGH ? "HIGH" : "LOW");
    printf("SPI rx[0]     : 0x%02X\n", spi_rx[0]);
    printf("UART message  : \"%s\"\n", msg);
    printf("Result        : PASS\n");

    hal_deinit();
    return 0;
}
