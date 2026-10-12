# Troubleshooting

| Problem | Checks |
| --- | --- |
| Sketch fails at `#include <BleMouse.h>` | Install a BLE mouse library with ESP32-S3 support and the `BleMouse.h` API. |
| MPU initialization says it is not responding | Check 3V3, common ground, SDA GPIO 8, SCL GPIO 9, wiring continuity, and the breakout address. The sketch uses `0x68`; if AD0 is high, a device may answer at `0x69`. |
| Pointer drifts at rest | Reboot on a stable surface and do not move it during startup calibration. Increase `GYRO_DEADZONE` slightly if necessary. |
| Pointer direction/axes are wrong | The MPU's orientation on perfboard determines the axes. Change the signs or swap the `mouseX` and `mouseY` mappings. |
| Buttons behave backward or do not click | Each button should connect GPIO 10 or 11 to GND when pressed. The sketch uses internal pull-ups and detects the press edge. |
| Bluetooth pairing fails | Check Serial Monitor at 115200 baud. The sketch starts advertising only after MPU initialization and gyro calibration complete. Forget an old pairing on the host and scan again. |
| Board resets or disconnects under motion | Measure the boost output under load, check the module current capability, shorten power leads, and inspect the ground path. |
| Battery/charger becomes warm | Disconnect USB and power. Check cell polarity, charger current configuration, cell health, and whether the board supports the way it is being charged. Do not continue with a damaged or hot cell. |

The photos show both faces of this particular perfboard build. They should not be used to infer pin locations on a different ESP32-S3 board revision.
