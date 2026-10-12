# Wiring and power

The pin assignments below come from the supplied sketch. The board photos show the assembled front and power side; they are reference photos, not a net-by-net view of every solder joint.

## Logic and controls

| Device pin | Connect to | Notes |
| --- | --- | --- |
| MPU VCC | ESP32-S3 `3V3` | Use the 3.3 V rail so I²C logic is at ESP32-safe levels. |
| MPU GND | Common GND | All circuit grounds must be connected. |
| MPU SDA | ESP32-S3 GPIO 8 | `SDA_PIN` in the sketch. |
| MPU SCL | ESP32-S3 GPIO 9 | `SCL_PIN` in the sketch. |
| MPU XDA, XCL, AD0, INT | Not connected | The sketch does not use these pins. Leave AD0 in the state that gives I²C address `0x68`. |
| Left-click button | GPIO 10 and GND | `INPUT_PULLUP`; pressing closes the switch to ground. |
| Right-click button | GPIO 11 and GND | `INPUT_PULLUP`; pressing closes the switch to ground. |

## Battery and 5 V supply

The user describes a 450 mAh, single-cell battery, an integrated TP4056 charging/boost module, and a switch in the battery-positive lead. The diagram follows that arrangement. Module layouts differ; identify `BAT+`, `BAT−`, boosted `OUT+`/`5V`, and `OUT−`/`GND` from the actual module markings before soldering.

1. Battery positive → power switch → module `BAT+`.
2. Battery negative → module `BAT−`.
3. Module boosted `OUT+`/`5V` → ESP32-S3 pin explicitly marked `5V` or `VBUS`.
4. Module `OUT−`/`GND` → ESP32-S3 GND; keep the sensor/button ground common.
5. MPU VCC remains on ESP32 3V3, not the 5 V boost output.
6. Charge through the module's marked USB input, following that module's documentation.

The module's exact power-path/load-sharing behavior is unknown from the photos. If the switch disconnects the cell from the charger, charging will require the switch to be on. If the module has separate `OUT`/`LOAD` pads, follow its documentation. Do not connect the boosted output to a charger input.

### Check before powering

- Verify battery polarity at the module pads with its silkscreen, not wire color alone.
- With the battery disconnected from the ESP32, measure the boost output with a multimeter. Confirm it is about 5 V before connecting it to the board's 5 V/VBUS input.
- Never apply the boost output to the ESP32's 3V3 pin or to GPIO pins.
- Check the cell maker's permitted charge current. The TP4056 charge current is set by the module's programming resistor; the common module appearance does not identify that value.
- Confirm the battery has suitable protection and the charger module is configured for that cell. Stop if the battery swells, heats, or is damaged.

See [`wiring.svg`](wiring.svg) for the overview. GPIO numbers refer to the ESP32-S3 pin labels, not perfboard hole positions.
