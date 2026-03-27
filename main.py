import sys
import time
import queue
import threading

import board
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget,
    QVBoxLayout, QHBoxLayout, QLabel
)
from PyQt6.QtCore import QTimer, Qt
import pyqtgraph as pg

from config import SAMPLE_INTERVAL
from src.bmp280 import init_sensor as init_bmp, read as read_bmp
from src.scd41 import init_sensor as init_scd, read as read_scd


# ──────────────────────────────────────────────
# Sensor thread — runs in background
# ──────────────────────────────────────────────

def sensor_loop(q):
    i2c = board.I2C()
    bmp = init_bmp(i2c)
    scd = init_scd(i2c)

    print("Both sensors initialized")
    print("-" * 40)

    while True:
        bmp_data = read_bmp(bmp)
        scd_data = read_scd(scd)

        # Mirror your original console output
        print("[BMP280]")
        print(f"Temp:     {bmp_data['temperature_c']} °C  ({bmp_data['temperature_f']} °F)")
        print(f"Pressure: {bmp_data['pressure_hpa']} hPa")
        print(f"Altitude: {bmp_data['altitude_m']} m")

        if scd_data:
            print("[SCD41]")
            print(f"CO2:      {scd_data['co2_ppm']} ppm")
            print(f"Temp:     {scd_data['temperature_c']} °C  ({scd_data['temperature_f']} °F)")
            print(f"Humidity: {scd_data['humidity']} %")

            # Only queue a reading when SCD41 has fresh data
            q.put({
                "bmp_temp_c":    bmp_data["temperature_c"],
                "bmp_temp_f":    bmp_data["temperature_f"],
                "pressure_hpa":  bmp_data["pressure_hpa"],
                "altitude_m":    bmp_data["altitude_m"],
                "co2_ppm":       scd_data["co2_ppm"],
                "scd_temp_c":    scd_data["temperature_c"],
                "humidity":      scd_data["humidity"],
            })
        else:
            print("[SCD41] Waiting for data...")

        print("-" * 40)
        time.sleep(SAMPLE_INTERVAL)


# ──────────────────────────────────────────────
# GUI
# ──────────────────────────────────────────────

WINDOW_SAMPLES = 60   # how many readings to show on each graph at once


class Dashboard(QMainWindow):
    def __init__(self, data_queue):
        super().__init__()
        self.setWindowTitle("Sensor Dashboard")
        self.resize(1100, 800)
        self.data_queue = data_queue

        # Data lists — one per channel
        self.bmp_temp_data  = []
        self.pressure_data  = []
        self.altitude_data  = []
        self.co2_data       = []
        self.scd_temp_data  = []
        self.humidity_data  = []

        self._build_ui()
        self._start_timer()

    # ── UI construction ──────────────────────

    def _build_ui(self):
        central = QWidget()
        root = QVBoxLayout()
        root.setSpacing(10)
        root.setContentsMargins(12, 12, 12, 12)
        central.setLayout(root)
        self.setCentralWidget(central)

        # Readout row — BMP280
        bmp_row = QHBoxLayout()
        self.lbl_bmp_temp    = self._make_label("BMP Temp: --")
        self.lbl_pressure    = self._make_label("Pressure: --")
        self.lbl_altitude    = self._make_label("Altitude: --")
        for lbl in (self.lbl_bmp_temp, self.lbl_pressure, self.lbl_altitude):
            bmp_row.addWidget(lbl)
        root.addLayout(bmp_row)

        # Readout row — SCD41
        scd_row = QHBoxLayout()
        self.lbl_co2         = self._make_label("CO₂: --")
        self.lbl_scd_temp    = self._make_label("SCD Temp: --")
        self.lbl_humidity    = self._make_label("Humidity: --")
        for lbl in (self.lbl_co2, self.lbl_scd_temp, self.lbl_humidity):
            scd_row.addWidget(lbl)
        root.addLayout(scd_row)

        # Plots
        self.plot_bmp_temp  = self._make_plot("BMP280 Temperature", "°C",   "tomato")
        self.plot_pressure  = self._make_plot("Pressure",           "hPa",  "steelblue")
        self.plot_altitude  = self._make_plot("Altitude",           "m",    "mediumpurple")
        self.plot_co2       = self._make_plot("CO₂",                "ppm",  "seagreen")
        self.plot_scd_temp  = self._make_plot("SCD41 Temperature",  "°C",   "darkorange")
        self.plot_humidity  = self._make_plot("Humidity",           "%",    "cornflowerblue")

        for p in (
            self.plot_bmp_temp, self.plot_pressure, self.plot_altitude,
            self.plot_co2,      self.plot_scd_temp, self.plot_humidity,
        ):
            root.addWidget(p["widget"])

    def _make_label(self, text):
        lbl = QLabel(text)
        lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
        lbl.setStyleSheet(
            "font-size:16px; font-weight:bold; padding:6px 12px;"
            "background:#f0f0f0; border-radius:6px;"
        )
        return lbl

    def _make_plot(self, title, units, color):
        w = pg.PlotWidget()
        w.setTitle(title, color="k", size="11pt")
        w.setLabel("left", units)
        w.setBackground("w")
        w.showGrid(x=True, y=True, alpha=0.3)
        w.setMaximumHeight(160)
        curve = w.plot(pen=pg.mkPen(color=color, width=2))
        return {"widget": w, "curve": curve}

    # ── Timer ───────────────────────────────

    def _start_timer(self):
        self.timer = QTimer()
        self.timer.setInterval(500)          # check queue twice per second
        self.timer.timeout.connect(self._update)
        self.timer.start()

    # ── Update loop ─────────────────────────

    def _update(self):
        # Drain everything currently in the queue
        got_data = False
        while True:
            try:
                r = self.data_queue.get_nowait()
                self.bmp_temp_data.append(r["bmp_temp_c"])
                self.pressure_data.append(r["pressure_hpa"])
                self.altitude_data.append(r["altitude_m"])
                self.co2_data.append(r["co2_ppm"])
                self.scd_temp_data.append(r["scd_temp_c"])
                self.humidity_data.append(r["humidity"])
                got_data = True
            except queue.Empty:
                break

        if not got_data:
            return   # nothing new — skip redraw

        n = WINDOW_SAMPLES
        xs = list(range(len(self.bmp_temp_data)))[-n:]

        self.plot_bmp_temp["curve"].setData(xs, self.bmp_temp_data[-n:])
        self.plot_pressure["curve"].setData(xs, self.pressure_data[-n:])
        self.plot_altitude["curve"].setData(xs, self.altitude_data[-n:])
        self.plot_co2["curve"].setData(xs,      self.co2_data[-n:])
        self.plot_scd_temp["curve"].setData(xs, self.scd_temp_data[-n:])
        self.plot_humidity["curve"].setData(xs, self.humidity_data[-n:])

        # Update numeric labels with latest values
        self.lbl_bmp_temp.setText(f"BMP Temp: {self.bmp_temp_data[-1]:.2f} °C")
        self.lbl_pressure.setText(f"Pressure: {self.pressure_data[-1]:.2f} hPa")
        self.lbl_altitude.setText(f"Altitude: {self.altitude_data[-1]:.2f} m")
        self.lbl_co2.setText(f"CO₂: {self.co2_data[-1]:.0f} ppm")
        self.lbl_scd_temp.setText(f"SCD Temp: {self.scd_temp_data[-1]:.2f} °C")
        self.lbl_humidity.setText(f"Humidity: {self.humidity_data[-1]:.2f} %")


# ──────────────────────────────────────────────
# Entry point
# ──────────────────────────────────────────────

def main():
    data_queue = queue.Queue()

    # Start sensor thread
    t = threading.Thread(target=sensor_loop, args=(data_queue,), daemon=True)
    t.start()

    # Start GUI
    app = QApplication(sys.argv)
    window = Dashboard(data_queue)
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()