#include <iostream>
#include <unistd.h>
#include "bmp280.h"
#include "driver_bmp280_interface.h"

int main()
{
    bmp280_handle_t handle;
    uint8_t res;

    // Initialize BMP280
    res = bmp280_init(&handle);
    if (res != 0)
    {
        std::cerr << "Failed to initialize BMP280! Error code: " << (int)res << std::endl;
        return -1;
    }
    std::cout << "BMP280 initialized successfully." << std::endl;

    while (true)
    {
        uint32_t temp_raw = 0;
        float temp_c = 0.0f;
        uint32_t pres_raw = 0;
        float pres_hpa = 0.0f;

        // Read temperature & pressure
        res = bmp280_read_temperature_pressure(&handle, &temp_raw, &temp_c, &pres_raw, &pres_hpa);
        if (res != 0)
        {
            std::cerr << "Failed to read BMP280! Error code: " << (int)res << std::endl;
        }
        else
        {
            std::cout << "Temperature: " << temp_c << " °C, "
                      << "Pressure: " << pres_hpa << " hPa" << std::endl;
        }

        sleep(1);
    }

    return 0;
}