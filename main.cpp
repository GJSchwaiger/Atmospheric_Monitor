#include <iostream>        // For std::cout and std::cerr
#include <fcntl.h>         // For open()
#include <unistd.h>        // For read(), write(), close(), usleep()
#include <sys/ioctl.h>     // For ioctl()
#include <linux/i2c-dev.h> // For I2C definitions
#include <cstdint>         // For fixed-width integers like uint8_t

// Linux exposes the I2C bus as a device file.
// On modern Raspberry Pi models, I2C bus 1 is /dev/i2c-1.
#define I2C_BUS "/dev/i2c-1"

// The BMP280’s I2C address.
// Most boards use 0x76, some use 0x77.
// Confirm using: i2cdetect -y 1
#define BMP280_ADDR 0x76

int main() {

    // -------------------------------------------------
    // 1. Open the I2C bus (like opening a file)
    // -------------------------------------------------
    int file = open(I2C_BUS, O_RDWR);

    if (file < 0) {
        std::cerr << "Failed to open I2C bus\n";
        return 1;
    }

    // -------------------------------------------------
    // 2. Tell Linux which device address we want to talk to
    // -------------------------------------------------
    // I2C_SLAVE sets the target device address
    if (ioctl(file, I2C_SLAVE, BMP280_ADDR) < 0) {
        std::cerr << "Failed to connect to BMP280\n";
        close(file);
        return 1;
    }

    // -------------------------------------------------
    // 3. Configure the BMP280 control register (0xF4)
    // -------------------------------------------------
    // Register 0xF4 controls measurement mode.
    //
    // Value 0x27 means:
    //   - Temperature oversampling x1
    //   - Pressure oversampling x1
    //   - Normal mode (continuous measurement)
    //
    // We send two bytes:
    //   First byte  = register address (0xF4)
    //   Second byte = value to write (0x27)
    uint8_t config[2] = {0xF4, 0x27};

    if (write(file, config, 2) != 2) {
        std::cerr << "Failed to write control register\n";
        close(file);
        return 1;
    }

    // -------------------------------------------------
    // 4. Wait for measurement to complete
    // -------------------------------------------------
    // Give the sensor time to perform a reading.
    // 100 ms is more than enough for this configuration.
    usleep(100000);  // microseconds

    // -------------------------------------------------
    // 5. Tell the sensor which register we want to read
    // -------------------------------------------------
    // 0xF7 is the first pressure data register.
    // Pressure and temperature data span 6 bytes:
    //
    // 0xF7, 0xF8, 0xF9   → Pressure (20-bit)
    // 0xFA, 0xFB, 0xFC   → Temperature (20-bit)
    uint8_t reg = 0xF7;

    if (write(file, &reg, 1) != 1) {
        std::cerr << "Failed to set register pointer\n";
        close(file);
        return 1;
    }

    // -------------------------------------------------
    // 6. Read 6 bytes from the sensor
    // -------------------------------------------------
    uint8_t data[6];

    if (read(file, data, 6) != 6) {
        std::cerr << "Failed to read data\n";
        close(file);
        return 1;
    }

    // -------------------------------------------------
    // 7. Combine bytes into 20-bit raw values
    // -------------------------------------------------
    // The BMP280 stores data as:
    //
    // Pressure:
    //   data[0] = MSB
    //   data[1] = LSB
    //   data[2] = XLSB (upper 4 bits used)
    //
    // Temperature:
    //   data[3] = MSB
    //   data[4] = LSB
    //   data[5] = XLSB (upper 4 bits used)
    //
    // We shift and combine them into 32-bit integers.

    int32_t adc_P =
        ((int32_t)data[0] << 12) |   // shift MSB into top position
        ((int32_t)data[1] << 4)  |   // shift LSB
        ((int32_t)data[2] >> 4);     // take upper 4 bits only

    int32_t adc_T =
        ((int32_t)data[3] << 12) |
        ((int32_t)data[4] << 4)  |
        ((int32_t)data[5] >> 4);

    // -------------------------------------------------
    // 8. Print raw ADC values
    // -------------------------------------------------
    std::cout << "Raw Pressure ADC: " << adc_P << std::endl;
    std::cout << "Raw Temperature ADC: " << adc_T << std::endl;

    // -------------------------------------------------
    // 9. Close the I2C connection
    // -------------------------------------------------
    close(file);

    return 0;
}