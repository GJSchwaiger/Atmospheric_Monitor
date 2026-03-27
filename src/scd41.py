import adafruit_scd4x
from config import SAMPLE_INTERVAL

def init_sensor(i2c):
    sensor = adafruit_scd4x.SCD4X(i2c)

    print(f"SCD41 serial: {[hex(i) for i in sensor.serial_number]}")

    sensor.start_periodic_measurement()
    print("SCD42 waiting for first measurement...")

    return sensor

def read(sensor):
    if sensor.data_ready:
        return {
            "co2_ppm": sensor.CO2,
            "temperature_c": round(sensor.temperature, 2),
            "temperature_f": round((sensor.temperature * 9/5)+32, 2),
            "humidity": round(sensor.relative_humidity, 2)
        }
    
    return None