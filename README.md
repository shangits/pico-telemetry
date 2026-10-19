# Pico W Telemetry

Work in progress: a Wi-Fi telemetry server on the Raspberry Pi Pico W, serving live temperature and GPIO data to a browser dashboard.

## Progress
- [x] Blink on-board LED via CYW43 Wi-Fi chip
- [x] Heartbeat output over USB serial
- [x] Read the internal temperature sensor
- [ ] Sample the temperature sensor from a timer interrupt with a moving average filter

## Notes
- The temperature reading is the chip's own die temperature, not the room. The sensor is accurate to about ±2 °C per RP2040 datasheet. Raw samples are also noisy so averaging the readings will smooth them out.