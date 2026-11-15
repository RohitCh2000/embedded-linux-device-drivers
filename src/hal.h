/*
 * hal.h - Simulated-register Hardware Abstraction Layer (HAL)
 *
 * A small, portable HAL in C11 that mirrors the shape of a real
 * embedded-Linux / bare-metal driver layer for GPIO, I2C, SPI and
 * UART on ARM platforms -- but backed by simulated registers in
 * memory, so it builds and runs on any host with gcc.
 *
 * Every bus transaction is logged to stdout, the way you would see
 * it with a logic analyser or with kernel debug prints enabled.
 *
 * Personal project: no employer code.
 */
#ifndef HAL_H
#define HAL_H

#include <stddef.h>
#include <stdint.h>

/* ---- Return codes ------------------------------------------------- */
#define HAL_OK              0
#define HAL_ERR_ARG        -1
#define HAL_ERR_RANGE      -2
#define HAL_ERR_NO_DEVICE  -3

/* ---- GPIO --------------------------------------------------------- */
#define HAL_GPIO_PIN_COUNT 16

typedef enum {
    HAL_GPIO_LOW  = 0,
    HAL_GPIO_HIGH = 1
} hal_gpio_level_t;

int hal_gpio_write(uint8_t pin, hal_gpio_level_t level);
int hal_gpio_read(uint8_t pin, hal_gpio_level_t *level);
int hal_gpio_toggle(uint8_t pin);

/* ---- I2C - simulated temperature sensor --------------------------- */
/*
 * Simulated sensor (register file lives in hal.c):
 *   7-bit address : 0x48
 *   WHO_AM_I      : reg 0x00, fixed value 0x48
 *   TEMP_MSB      : reg 0x01
 *   TEMP_LSB      : reg 0x02   (LSB: fractional bits, 1 LSB = 0.0625 C)
 * Temperature is a signed 12-bit value, left-justified in MSB:LSB,
 * like common I2C temperature sensors (TMP102-style encoding).
 */
#define HAL_I2C_SENSOR_ADDR   0x48u
#define HAL_SENSOR_REG_WHO_AM_I 0x00u
#define HAL_SENSOR_REG_TEMP_MSB 0x01u
#define HAL_SENSOR_REG_TEMP_LSB 0x02u
#define HAL_SENSOR_WHO_AM_I_VALUE 0x48u

int hal_i2c_write(uint8_t dev_addr, uint8_t reg, const uint8_t *data, size_t len);
int hal_i2c_read(uint8_t dev_addr, uint8_t reg, uint8_t *data, size_t len);

/* Convenience helper: read + decode the sensor temperature in deg C. */
int hal_sensor_read_temperature_c(float *temp_c);

/* Test/demo hook: preload the simulated sensor temperature registers. */
void hal_sensor_sim_set_raw(uint8_t msb, uint8_t lsb);

/* ---- SPI ---------------------------------------------------------- */
int hal_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len);

/* ---- UART --------------------------------------------------------- */
int hal_uart_write_string(const char *s);

/* ---- Lifecycle ---------------------------------------------------- */
int  hal_init(void);
void hal_deinit(void);

#endif /* HAL_H */
