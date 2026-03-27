import time
import board
from config import SAMPLE_INTERVAL
from src.bmp280 import init_sensor as init_bmp, read as read_bmp
from src.scd41 import init_sensor as init_scd, read as read_scd

def main():
    i2c = board.I2C()
    bmp = init_bmp(i2c)
    scd = init_scd(i2c)

    print("Both sensors initialized")
    print("-" * 40)

    while True:
        bmp_data = read_bmp(bmp)
        print(f"[BMP280]")
        print(f"Temp:   {bmp_data['temperature_c']} degrees C  ({bmp_data['temperature_f']} degrees F)")
        print(f"Pressure: {bmp_data['pressure_hpa']} hPa")
        print(f"Altitude: {bmp_data['altitude_m']} m")

        scd_data = read_scd(scd)
        if scd_data:
            print(f"[SCD41]")
            print(f"CO2: {scd_data['co2_ppm']} ppm")
            print(f"Temp: {scd_data['temperature_c']}C ({scd_data['temperature_f']}F)")
            print(f"Humidity: {scd_data['humidity']}%")

        else:
            print("[SCD41] Waiting for data...")

        print("-" * 40)
        time.sleep(SAMPLE_INTERVAL)

if __name__ == "__main__":
    main()