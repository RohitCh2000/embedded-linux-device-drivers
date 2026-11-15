/*
 * hal.c - Simulated-register HAL implementation. See hal.h.
 *
 * The "hardware" is a set of in-memory register arrays:
 *   - GPIO: a 16-bit output latch + a read-back latch
 *   - I2C : a 256-byte register file for the simulated sensor at 0x48
 *   - SPI : a loopback shift register with one fixed response byte
 *           pattern (0xA5 header), like a simple SPI flash / ADC reply
 *   - UART: a transmit FIFO that echoes to the log
 */
#include "hal.h"

#include <stdio.h>
#include <string.h>

/* Simulated GPIO latches (bit N = pin N). */
static uint16_t gpio_out_latch;
static uint16_t gpio_in_latch;

/* Simulated I2C sensor register file. */
static uint8_t sensor_regs[256];

/* ------------------------------------------------------------------ */
int hal_init(void)
{
    gpio_out_latch = 0;
    gpio_in_latch = 0;
    memset(sensor_regs, 0, sizeof(sensor_regs));

    sensor_regs[HAL_SENSOR_REG_WHO_AM_I] = HAL_SENSOR_WHO_AM_I_VALUE;
    /* Default simulated temperature: 25.5 C.
     * 25.5 / 0.0625 = 408 = 0x198, left-justified 12-bit -> MSB 0x19, LSB 0x80 */
    sensor_regs[HAL_SENSOR_REG_TEMP_MSB] = 0x19;
    sensor_regs[HAL_SENSOR_REG_TEMP_LSB] = 0x80;

    printf("[HAL] init: GPIO/I2C/SPI/UART simulated peripherals ready\n");
    return HAL_OK;
}

void hal_deinit(void)
{
    printf("[HAL] deinit: peripherals shut down\n");
}

/* ---- GPIO --------------------------------------------------------- */
int hal_gpio_write(uint8_t pin, hal_gpio_level_t level)
{
    if (pin >= HAL_GPIO_PIN_COUNT) {
        return HAL_ERR_RANGE;
    }
    if (level == HAL_GPIO_HIGH) {
        gpio_out_latch |= (uint16_t)(1u << pin);
        gpio_in_latch  |= (uint16_t)(1u << pin); /* read-back follows output */
    } else {
        gpio_out_latch &= (uint16_t)~(1u << pin);
        gpio_in_latch  &= (uint16_t)~(1u << pin);
    }
    printf("[GPIO] write pin=%u level=%s (latch=0x%04X)\n",
           pin, level == HAL_GPIO_HIGH ? "HIGH" : "LOW", gpio_out_latch);
    return HAL_OK;
}

int hal_gpio_read(uint8_t pin, hal_gpio_level_t *level)
{
    if (level == NULL) {
        return HAL_ERR_ARG;
    }
    if (pin >= HAL_GPIO_PIN_COUNT) {
        return HAL_ERR_RANGE;
    }
    *level = (gpio_in_latch & (uint16_t)(1u << pin)) ? HAL_GPIO_HIGH : HAL_GPIO_LOW;
    printf("[GPIO] read  pin=%u level=%s\n",
           pin, *level == HAL_GPIO_HIGH ? "HIGH" : "LOW");
    return HAL_OK;
}

int hal_gpio_toggle(uint8_t pin)
{
    hal_gpio_level_t cur;
    int rc = hal_gpio_read(pin, &cur);
    if (rc != HAL_OK) {
        return rc;
    }
    return hal_gpio_write(pin, cur == HAL_GPIO_HIGH ? HAL_GPIO_LOW : HAL_GPIO_HIGH);
}

/* ---- I2C ---------------------------------------------------------- */
int hal_i2c_write(uint8_t dev_addr, uint8_t reg, const uint8_t *data, size_t len)
{
    if (data == NULL && len > 0) {
        return HAL_ERR_ARG;
    }
    if (dev_addr != HAL_I2C_SENSOR_ADDR) {
        printf("[I2C]  write addr=0x%02X reg=0x%02X -> NACK (no device)\n", dev_addr, reg);
        return HAL_ERR_NO_DEVICE;
    }
    printf("[I2C]  write addr=0x%02X reg=0x%02X len=%zu data=[", dev_addr, reg, len);
    for (size_t i = 0; i < len; i++) {
        sensor_regs[(uint8_t)(reg + i)] = data[i];
        printf("%s0x%02X", i == 0 ? "" : " ", data[i]);
    }
    printf("]\n");
    return HAL_OK;
}

int hal_i2c_read(uint8_t dev_addr, uint8_t reg, uint8_t *data, size_t len)
{
    if (data == NULL) {
        return HAL_ERR_ARG;
    }
    if (dev_addr != HAL_I2C_SENSOR_ADDR) {
        printf("[I2C]  read  addr=0x%02X reg=0x%02X -> NACK (no device)\n", dev_addr, reg);
        return HAL_ERR_NO_DEVICE;
    }
    printf("[I2C]  read  addr=0x%02X reg=0x%02X len=%zu data=[", dev_addr, reg, len);
    for (size_t i = 0; i < len; i++) {
        data[i] = sensor_regs[(uint8_t)(reg + i)];
        printf("%s0x%02X", i == 0 ? "" : " ", data[i]);
    }
    printf("]\n");
    return HAL_OK;
}

void hal_sensor_sim_set_raw(uint8_t msb, uint8_t lsb)
{
    sensor_regs[HAL_SENSOR_REG_TEMP_MSB] = msb;
    sensor_regs[HAL_SENSOR_REG_TEMP_LSB] = lsb;
    printf("[SIM]  sensor temperature registers set to MSB=0x%02X LSB=0x%02X\n", msb, lsb);
}

int hal_sensor_read_temperature_c(float *temp_c)
{
    if (temp_c == NULL) {
        return HAL_ERR_ARG;
    }
    uint8_t buf[2] = {0, 0};
    int rc = hal_i2c_read(HAL_I2C_SENSOR_ADDR, HAL_SENSOR_REG_TEMP_MSB, buf, 2);
    if (rc != HAL_OK) {
        return rc;
    }
    /* 12-bit signed, left-justified: combine, shift down, sign-extend. */
    int16_t raw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    raw >>= 4;
    if (raw & 0x0800) {
        raw |= (int16_t)0xF000;
    }
    *temp_c = (float)raw * 0.0625f;
    return HAL_OK;
}

/* ---- SPI ---------------------------------------------------------- */
int hal_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
    if (tx == NULL || rx == NULL) {
        return HAL_ERR_ARG;
    }
    printf("[SPI]  transfer len=%zu tx=[", len);
    for (size_t i = 0; i < len; i++) {
        printf("%s0x%02X", i == 0 ? "" : " ", tx[i]);
    }
    printf("] rx=[");
    for (size_t i = 0; i < len; i++) {
        /* Simulated device: replies 0xA5 to a read command (tx[0]==0x9F,
         * a common JEDEC-ID style opcode), otherwise echoes tx inverted. */
        rx[i] = (tx[0] == 0x9F && i == 0) ? 0xA5 : (uint8_t)~tx[i];
        printf("%s0x%02X", i == 0 ? "" : " ", rx[i]);
    }
    printf("]\n");
    return HAL_OK;
}

/* ---- UART --------------------------------------------------------- */
int hal_uart_write_string(const char *s)
{
    if (s == NULL) {
        return HAL_ERR_ARG;
    }
    printf("[UART] tx (%zu bytes): \"%s\"\n", strlen(s), s);
    return HAL_OK;
}
