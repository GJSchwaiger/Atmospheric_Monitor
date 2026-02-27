#include"App.h"

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
        cout << "Temperature: " << compensated.temperature << " °C\n";
        cout << "Pressure: " << compensated.pressure << " hPa\n";

    } catch (const std::exception &e) {
        // -------------------------------
        // Step 4: Handle any errors
        // 'e.what()' returns a string describing the error
        // This happens if, for example, I2C cannot open or read fails
        // -------------------------------
        cerr << "Error: " << e.what() << "\n";
        return 1; // Exit the program with error code 1
    }


    return 0; // Exit normally
    
}