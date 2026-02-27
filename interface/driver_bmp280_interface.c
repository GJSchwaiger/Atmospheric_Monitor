#include "driver_bmp280_interface.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static int i2c_fd = -1;

/**
 * @brief  interface iic bus init
 */
uint8_t bmp280_interface_iic_init(void)
{
    i2c_fd = open("/dev/i2c-1", O_RDWR);
    if (i2c_fd < 0)
    {
        perror("Failed to open /dev/i2c-1");
        return 1;
    }
    printf("I2C bus opened, fd=%d\n", i2c_fd);
    return 0;
}

/**
 * @brief  interface iic bus deinit
 */
uint8_t bmp280_interface_iic_deinit(void)
{
    if (i2c_fd >= 0)
        close(i2c_fd);
    i2c_fd = -1;
    return 0;
}

/**
 * @brief  interface iic read
 */
uint8_t bmp280_interface_iic_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (i2c_fd < 0)
        return 1;
    if (ioctl(i2c_fd, I2C_SLAVE, addr) < 0)
    {
        perror("Failed to set I2C slave address");
        return 1;
    }
    if (write(i2c_fd, &reg, 1) != 1)
    {
        perror("Failed to write register address");
        return 1;
    }
    if (read(i2c_fd, buf, len) != len)
    {
        perror("Failed to read from device");
        return 1;
    }
    return 0;
}

/**
 * @brief  interface iic write
 */
uint8_t bmp280_interface_iic_write(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (i2c_fd < 0)
        return 1;
    if (ioctl(i2c_fd, I2C_SLAVE, addr) < 0)
    {
        perror("Failed to set I2C slave address");
        return 1;
    }
    uint8_t tmp[len + 1];
    tmp[0] = reg;
    memcpy(tmp + 1, buf, len);
    if (write(i2c_fd, tmp, len + 1) != len + 1)
    {
        perror("Failed to write to device");
        return 1;
    }
    return 0;
}

/* SPI not used on Linux */
uint8_t bmp280_interface_spi_init(void) { return 0; }
uint8_t bmp280_interface_spi_deinit(void) { return 0; }
uint8_t bmp280_interface_spi_read(uint8_t reg, uint8_t *buf, uint16_t len) { return 0; }
uint8_t bmp280_interface_spi_write(uint8_t reg, uint8_t *buf, uint16_t len) { return 0; }

/**
 * @brief  delay in ms
 */
void bmp280_interface_delay_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

/**
 * @brief  debug print
 */
void bmp280_interface_debug_print(const char *const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}