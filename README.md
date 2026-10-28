# Pico W Telemetry

Work in progress: a Wi-Fi telemetry server on the Raspberry Pi Pico W, serving live temperature data to a browser dashboard.

## Progress
- [x] Blink on-board LED via CYW43 Wi-Fi chip
- [x] Heartbeat output over USB serial
- [x] Read the internal temperature sensor
- [x] Sample the temperature sensor from a timer interrupt with a moving average filter
- [x] Join Wi-Fi and get an IP address from the router
- [x] Serve telemetry over TCP so another machine can connect and read it
- [x] Read the ADC through its registers instead of the SDK helper
- [x] Serve the current temperature as JSON over HTTP
- [ ] Log readings on a computer with a Python script and SQLite
- [ ] Serve a web page at the Pico's IP that shows the live temperature

## Notes
- The temperature reading is the chip's own die temperature, not the room. The sensor is accurate to about ±2 °C per RP2040 datasheet. Raw samples are also noisy so averaging the readings will smooth them out.
- A 100 ms timer interrupt samples the ADC into a 16-slot ring buffer and the main loop reads the running average. Smooths the raw sensor noise, and the ISR keeps main() free.
- Pico W only supports 2.4Ghz Wi-Fi, 5GHz-only SSIDs will not appear in scans.
- Credentials live in **wifi_config.h** (git ignored).
- For an example template, check **wifi_config.h.example**.
- HTTP server on port 80 returns {"temp_c": ...} as JSON. lwIP is callback based rather than blocking; on_accept registers on_recv, which replies once the request arrives and then closes. Replying before reading the request caused a connection reset.
- The ADC is read through its CS and RESULT registers directly instead of the SDK's adc_read() to learn how the hardware works (RP2040 datasheet section 4.9).