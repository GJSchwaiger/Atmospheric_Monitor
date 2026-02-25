#include"App.h"

// Linux exposes the I2C bus as a device file.
// On modern Raspberry Pi models, I2C bus 1 is /dev/i2c-1.
#define I2C_BUS "/dev/i2c-1"

// The BMP280’s I2C address.
// Most boards use 0x76, some use 0x77.
// Confirm using: i2cdetect -y 1
#define BMP280_ADDR 0x76

int main() {

    try {
        // -------------------------------
        // Step 1: Read raw data from sensor
        // This includes ADC values and calibration constants
        // -------------------------------
        BMP280RawData rawData = readBMP280Data();

        // -------------------------------
        // Step 2: Apply compensation formulas
        // This converts raw ADC readings into °C and hPa
        // -------------------------------
        BMP280Compensated compensated = compensateBMP280(rawData);

        // -------------------------------
        // Step 3: Print the results
        // -------------------------------
        std::cout << "Temperature: " << compensated.temperature << " °C\n";
        std::cout << "Pressure: " << compensated.pressure << " hPa\n";

    } catch (const std::exception &e) {
        // -------------------------------
        // Step 4: Handle any errors
        // 'e.what()' returns a string describing the error
        // This happens if, for example, I2C cannot open or read fails
        // -------------------------------
        std::cerr << "Error: " << e.what() << "\n";
        return 1; // Exit the program with error code 1
    }

    return 0; // Exit normally
    
}