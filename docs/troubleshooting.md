# Troubleshooting

- Build fails: use ESP-IDF v5.2.8 and an initialized ESP-IDF terminal, targeting esp32s3.
- Flash fails: check the port, USB cable and BOOT/RESET download sequence.
- No console: select the console transport appropriate to your board; see README.
- MPU initialization/calibration fails: check 3V3, GND, SDA 8, SCL 9 and address 0x68.
- Pointer drifts: reset and keep still during calibration; tune dead zone if necessary.
- Pairing fails: forget the host's old pairing and pair again.
- Encoder does not scroll: check A=12, B=13 and common=GND. Swap A/B for reversed direction.
- Resets under load: check boost voltage and supply current capacity.
- Battery heats while charging: disconnect and check polarity, protection and charge-current compatibility.
