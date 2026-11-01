# Build and use

See the [README](../README.md#build-and-flash-with-esp-idf) for the supported ESP-IDF version, build, flash, console and pairing instructions. Firmware is built from the repository root.

Assemble the sensor and buttons according to [wiring.md](wiring.md). Add encoder A/CLK to GPIO 12, B/DT to GPIO 13 and common to GND. Powered encoder modules must use 3V3. Keep the device still through calibration. Rotate the encoder to scroll; swap A/B if direction is reversed.

Check solder bridges, battery polarity, charger current and boost voltage before powering the ESP32. Connect the MPU to 3V3 only.
