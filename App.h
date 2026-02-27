#ifndef APP_H
#define APP_H

    #include <iostream>
    #include <fcntl.h>         // For open()
    #include <unistd.h>        // For read(), write(), close(), usleep()
    #include <sys/ioctl.h>     // For ioctl()
    #include <linux/i2c-dev.h> // For I2C definitions
    #include <cstdint>         // For fixed-width integers like uint8_t
    #include "driver_bmp280.h"

    using std::cerr;
    using std::cout;
    using std::endl;

    // Linux exposes the I2C bus as a device file.
    // On modern Raspberry Pi models, I2C bus 1 is /dev/i2c-1.
    #define I2C_BUS "/dev/i2c-1"

    // The BMP280’s I2C address.
    // Most boards use 0x76, some use 0x77.
    // Confirm using: i2cdetect -y 1
    #define BMP280_ADDR 0x76

    struct Registers{
        // Temperature registers
        int16_t dig_T1;
        int16_t  dig_T2;
        int16_t  dig_T3;

        // Pressure registers
        int16_t dig_P1;
        int16_t dig_P2;
        int16_t dig_P3;
        int16_t dig_P4;
        int16_t dig_P5;
        int16_t dig_P6;
        int16_t dig_P7;
        int16_t dig_P8;
        int16_t dig_P9;
    };

    struct BMP280RawData {
        int32_t adc_T;  // Raw temperature
        int32_t adc_P;  // Raw pressure
        Registers calib;
    };

    struct BMP280Compensated {
        double temperature; // in °C
        double pressure;    // in hPa
    };

    BMP280RawData readBMP280Data() {
        BMP280RawData data{};
        
        // 1. Open I2C bus
        int file = open(I2C_BUS, O_RDWR);
        if (file < 0) {
            throw std::runtime_error("Cannot open I2C bus");
        }

        // 2. Select BMP280 device address
        if (ioctl(file, I2C_SLAVE, BMP280_ADDR) < 0) {
            close(file);
            throw std::runtime_error("Cannot connect to BMP280");
        }

        // -------------------------
        // 3. Read 24 calibration bytes (0x88-0xA1)
        // -------------------------
        uint8_t reg = 0x88;           // start register
        write(file, &reg, 1);          // set pointer
        uint8_t calib[24];
        read(file, calib, 24);         // read all 24 bytes

        // Convert bytes to constants (little-endian)
        data.calib.dig_T1 = (calib[1] << 8) | calib[0];
        data.calib.dig_T2 = (calib[3] << 8) | calib[2];
        data.calib.dig_T3 = (calib[5] << 8) | calib[4];

        data.calib.dig_P1 = (calib[7] << 8) | calib[6];
        data.calib.dig_P2 = (calib[9] << 8) | calib[8];
        data.calib.dig_P3 = (calib[11] << 8) | calib[10];
        data.calib.dig_P4 = (calib[13] << 8) | calib[12];
        data.calib.dig_P5 = (calib[15] << 8) | calib[14];
        data.calib.dig_P6 = (calib[17] << 8) | calib[16];
        data.calib.dig_P7 = (calib[19] << 8) | calib[18];
        data.calib.dig_P8 = (calib[21] << 8) | calib[20];
        data.calib.dig_P9 = (calib[23] << 8) | calib[22];

        // -------------------------
        // 4. Configure sensor for measurement
        // -------------------------
        uint8_t ctrl[2] = {0xF4, 0x27}; // normal mode, oversampling x1
        write(file, ctrl, 2);

        usleep(100000); // wait 100ms for measurement

        // -------------------------
        // 5. Read raw pressure and temperature (6 bytes from 0xF7)
        // -------------------------
        reg = 0xF7;                  // start register for raw data
        write(file, &reg, 1);        // set pointer
        uint8_t raw[6];
        read(file, raw, 6);          // read 6 bytes

        // Convert bytes into 20-bit raw values
        data.adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | ((int32_t)raw[2] >> 4);
        data.adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | ((int32_t)raw[5] >> 4);

        close(file);
        return data;
}

BMP280Compensated compensateBMP280(const BMP280RawData &data) {
    BMP280Compensated result;

    // ------------------------
    // Temperature Compensation
    // ------------------------
    int32_t adc_T = data.adc_T;
    const auto &c = data.calib;

    // Use floating-point formula from datasheet
    double var1 = (adc_T / 16384.0 - c.dig_T1 / 1024.0) * c.dig_T2;
    double var2 = ((adc_T / 131072.0 - c.dig_T1 / 8192.0) * (adc_T / 131072.0 - c.dig_T1 / 8192.0)) * c.dig_T3;
    double t_fine = var1 + var2;

    result.temperature = t_fine / 5120.0;

    // ------------------------
    // Pressure Compensation
    // ------------------------
    int32_t adc_P = data.adc_P;

    double varP1 = t_fine / 2.0 - 64000.0;
    double varP2 = varP1 * varP1 * c.dig_P6 / 32768.0;
    varP2 = varP2 + varP1 * c.dig_P5 * 2.0;
    varP2 = varP2 / 4.0 + c.dig_P4 * 65536.0;
    double varP3 = (c.dig_P3 * varP1 * varP1 / 524288.0 + c.dig_P2 * varP1) / 524288.0;
    varP1 = (1.0 + varP3 / 32768.0) * c.dig_P1;

    double pressure = 1048576.0 - adc_P;
    pressure = (pressure - varP2 / 4096.0) * 6250.0 / varP1;

    varP1 = c.dig_P9 * pressure * pressure / 2147483648.0;
    varP2 = pressure * c.dig_P8 / 32768.0;
    pressure = pressure + (varP1 + varP2 + c.dig_P7) / 16.0;

    // Convert from Pa to hPa
    result.pressure = pressure / 100.0;

    return result;
}

#endif