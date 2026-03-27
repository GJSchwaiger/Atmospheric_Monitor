import board
import adafruit_bmp280
from config import BMP280_ADDRESS

def init_sensor(i2c):
    sensor = adafruit_bmp280.Adafruit_BMP280_I2C(i2c, address=BMP280_ADDRESS)
    
    #set oversampling for better accuracy
    sensor.overscan_temperature = adafruit_bmp280.OVERSCAN_X16
    sensor.overscan_pressure = adafruit_bmp280.OVERSCAN_X16

    #IIR filter reduces noise from short term fluctuations
    sensor.iir_filter = adafruit_bmp280.IIR_FILTER_X16

    #set sea level pressure for accurate altitude
    sensor.sea_level_pressure = 1013.25

    return sensor

def read(sensor):
    return{
        "temperature_c": round(sensor.temperature, 2),
        "temperature_f": round((sensor.temperature * 9/5) + 32, 2),
        "pressure_hpa": round(sensor.pressure, 2),
        "altitude_m": round(sensor.altitude, 2)
    }