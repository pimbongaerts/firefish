# Example: FireFish M4 dive logger / intervalometer (2025v2)

A typical use-case implementation for the FireFish board: a real-time underwater
data logger and camera intervalometer for scientific diving. Demonstrates the core
FireFish capabilities and is a candidate for the manuscript's **Operation** and
**Validation & characterization** sections.

## What it does
- Reads sensors over I²C/SPI: **MS5837 (Bar30) depth/pressure**, **DS-series RTC**,
  **ADXL345 accelerometer**, **ADS1115 ADC**, **Ping1D sonar altimeter**.
- Drives a **ST7789 TFT** display (Adafruit GFX) for live readouts.
- Logs to onboard **SPI flash** (Adafruit_SPIFlash) with a **FatFs** (`ff.c`/`ff.h`)
  filesystem, writing `data.csv` + a marker file.
- Triggers **camera focus/shutter** on an interval (intervalometer).

## Files
- `firefish_m4_2025v2.ino` — main sketch
- `ff.c`, `ff.h`, `ffconf.h`, `diskio.h` — ChaN **FatFs** library (used for flash formatting)

## Library dependencies (to document during clean-up)
Wire, SPI, Adafruit_GFX, Adafruit_ST7789, Adafruit_ADS1X15, Adafruit_SPIFlash,
Adafruit_I2CDevice, MS5837, RTClib, SparkFun_ADXL345, ping1d (Blue Robotics),
SdFat. Pin map is defined at the top of the `.ino`.

## ⚠️ Pending clean-up (author)
- Resolve `TODO` notes in the sketch (e.g., the `Adafruit_I2CDevice` include).
- Remove commented-out/unused code (e.g., u-blox GNSS include).
- Confirm the FatFs files' license/attribution (ChaN FatFs has its own permissive license) and keep it separate from the FireFish software license.
- Decide final naming (`2025v2` vs a versioned/renamed example).
