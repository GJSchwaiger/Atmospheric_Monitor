#include <iostream>
#include <unistd.h>
#include "driver_bmp280.h"
#include "driver_bmp280_interface.h"

int main()
{
    bmp280_handle_t handle;
    uint8_t res;

    // Initialize handle structure
    DRIVER_BMP280_LINK_INIT(&handle, bmp280_handle_t);
    
    // Link interface functions
    DRIVER_BMP280_LINK_IIC_INIT(&handle, bmp280_interface_iic_init);
    DRIVER_BMP280_LINK_IIC_DEINIT(&handle, bmp280_interface_iic_deinit);
    DRIVER_BMP280_LINK_IIC_READ(&handle, bmp280_interface_iic_read);
    DRIVER_BMP280_LINK_IIC_WRITE(&handle, bmp280_interface_iic_write);
    DRIVER_BMP280_LINK_SPI_INIT(&handle, bmp280_interface_spi_init);
    DRIVER_BMP280_LINK_SPI_DEINIT(&handle, bmp280_interface_spi_deinit);
    DRIVER_BMP280_LINK_SPI_READ(&handle, bmp280_interface_spi_read);
    DRIVER_BMP280_LINK_SPI_WRITE(&handle, bmp280_interface_spi_write);
    DRIVER_BMP280_LINK_DELAY_MS(&handle, bmp280_interface_delay_ms);
    DRIVER_BMP280_LINK_DEBUG_PRINT(&handle, bmp280_interface_debug_print);

    // Try common BMP280 addresses 0x76 and 0x77
    uint8_t addresses[] = {0x76, 0x77};
    bool found = false;

    for (auto addr : addresses)
    {
        handle.iic_addr = addr;
        res = bmp280_init(&handle);
        if (res == 0)
        {
            std::cout << "BMP280 found at 0x" << std::hex << int(addr) << std::dec << std::endl;
            
            // Verify by reading chip ID
            uint8_t chip_id = 0;
            res = bmp280_get_reg(&handle, 0xD0, &chip_id);
            std::cout << "Chip ID: 0x" << std::hex << int(chip_id) << std::dec << std::endl;
            if (chip_id != 0x58)
            {
                std::cerr << "Warning: Expected chip ID 0x58, got 0x" << std::hex << int(chip_id) << std::dec << std::endl;
            }
            
            found = true;
            break;
        }
    }

    if (!found)
    {
        std::cerr << "Failed to detect BMP280 at 0x76 or 0x77" << std::endl;
        bmp280_interface_iic_deinit();
        return -1;
    }

    // Configure sensor oversampling
    res = bmp280_set_temperatue_oversampling(&handle, BMP280_OVERSAMPLING_x4);
    if (res != 0)
    {
        std::cerr << "Failed to set temperature oversampling! Error code: " << int(res) << std::endl;
        bmp280_interface_iic_deinit();
        return -1;
    }

    res = bmp280_set_pressure_oversampling(&handle, BMP280_OVERSAMPLING_x4);
    if (res != 0)
    {
        std::cerr << "Failed to set pressure oversampling! Error code: " << int(res) << std::endl;
        bmp280_interface_iic_deinit();
        return -1;
    }

    // Set sensor to FORCED mode - explicitly trigger each measurement
    res = bmp280_set_mode(&handle, BMP280_MODE_FORCED);
    if (res != 0)
    {
        std::cerr << "Failed to set BMP280 mode! Error code: " << int(res) << std::endl;
        bmp280_interface_iic_deinit();
        return -1;
    }

    // Main loop: read temperature and pressure every 500ms
    while (true)
    {
        uint8_t status = 0;
        uint32_t temp_raw = 0, pres_raw = 0;
        float temp_c = 0.0f, pres_hpa = 0.0f;

        // Trigger a measurement in FORCED mode
        res = bmp280_set_mode(&handle, BMP280_MODE_FORCED);
        if (res != 0)
        {
            std::cerr << "Failed to trigger measurement! Error code: " << int(res) << std::endl;
            bmp280_interface_delay_ms(500);
            continue;
        }

        // Wait longer for measurement to complete in FORCED mode
        bmp280_interface_delay_ms(200);
        bmp280_get_status(&handle, &status);
        int wait_count = 10;
        while ((status & BMP280_STATUS_MEASURING) && wait_count > 0)
        {
            bmp280_interface_delay_ms(20);
            bmp280_get_status(&handle, &status);
            wait_count--;
        }

        if (wait_count == 0)
        {
            std::cerr << "Timeout waiting for measurement to complete (status: " << int(status) << ")" << std::endl;
            continue;
        }

        res = bmp280_read_temperature_pressure(&handle, &temp_raw, &temp_c, &pres_raw, &pres_hpa);
        if (res != 0)
        {
            std::cerr << "Failed to read BMP280! Error code: " << int(res) << std::endl;
            bmp280_interface_delay_ms(500);
            continue;
        }

        std::cout << "Temp (raw): " << temp_raw << " => " << temp_c << " °C, "
                  << "Press (raw): " << pres_raw << " => " << pres_hpa << " hPa" << std::endl;

        bmp280_interface_delay_ms(500);
    }

    bmp280_deinit(&handle);
    bmp280_interface_iic_deinit();
    return 0;
}