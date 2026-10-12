# Build and use

## Before soldering

Gather the components listed in the main [README](../README.md). Check that the MPU breakout is a 3.3 V I²C-compatible module and that the charging/boost board is intended for one 3.7 V nominal Li-ion/LiPo cell. Confirm the charger current against the battery maker's specification.

## Assembly sequence

1. Place the ESP32-S3 SuperMini, MPU-6050 breakout, and two buttons on perfboard.
2. Wire the sensor and buttons using the pin table in [wiring.md](wiring.md). Keep SDA/SCL short and connect all grounds.
3. Wire the battery and charger/boost module using its own printed pad labels. Keep the switch in the battery-positive path shown in the supplied build.
4. Inspect for solder bridges and verify battery polarity.
5. Before connecting the ESP32, measure the module's boosted output. Confirm the output is near 5 V and that it goes to the board's marked 5 V/VBUS input.
6. Connect the sensor to 3V3, never to the 5 V rail.

## Install and upload

1. Install the Arduino IDE and Espressif's Arduino-ESP32 board package.
2. Install an ESP32-S3-compatible `BleMouse.h` library. The supplied sketch uses the `BleMouse(name, manufacturer, batteryLevel)` constructor and methods `begin()`, `isConnected()`, `move()`, and `click()`.
3. Open `firmware/air_mouse.ino` and select the ESP32-S3 board entry matching the SuperMini.
4. Select the upload/USB mode that matches your board variant. Espressif documents USB CDC and DFU options in its [ESP32-S3 flashing guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/cdc_dfu_flash.html).
5. Upload. If needed, hold BOOT, tap RESET, release BOOT, then retry.
6. Open Serial Monitor at 115200 baud. Place the device still during startup calibration.

## Pair and operate

1. Search for Bluetooth devices on the host computer or tablet.
2. Pair with **Soorya Air Mouse**.
3. Tilt the device to move the pointer; use the GPIO 10 button for left click and GPIO 11 for right click.
4. If the cursor axes are swapped or reversed, edit the `mouseX`/`mouseY` assignments in `processGyro()` and flash again.

## Firmware behavior

- Startup waits briefly, initializes I²C at 100 kHz, wakes the MPU, then averages 1500 gyro samples for offsets. Keep the device still for approximately five seconds after reset.
- Motion uses gyro X/Y only. The accelerometer is configured but not used for pointer motion.
- A low-pass filter, 3 deg/s dead zone, sensitivity factor, and ±8 report limit are applied.
- The loop runs approximately every 5 ms, subject to I²C and BLE processing time.
- Button handling detects a HIGH-to-LOW transition and sends a click if BLE is connected. No software debounce is included.
- The BLE constructor reports battery level 100; no battery measurement is implemented.

## Charge-current caution

The cell capacity alone does not tell us the safe charging current. Verify the exact cell specification and the charge-current programming resistor on the module. Do not assume a common TP4056 module's factory setting is suitable for a 450 mAh cell. See the module documentation and the [TP4056 datasheet](https://www.toppwr.com/uploadfile/file/20240307/65e993d6379ea.pdf).
