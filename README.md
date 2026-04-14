# ESP32-C3 Wireless Microphone

A wireless microphone prototype using the ESP32-C3 Super Mini. It captures audio via an INMP441 I²S microphone, displays a live volume bar on an SSD1306 OLED screen, and streams audio over Wi-Fi UDP to a Python script running on your laptop.

## Features

- I²S audio capture using the INMP441 MEMS microphone
- Live VU volume bar on 0.96" SSD1306 OLED display
- Wi-Fi UDP audio streaming to PC (no audio cable needed)
- High-pass filter to reduce low-frequency hum and DC offset
- Noise gate to suppress background hiss during silence
- Python receiver script plays the audio through your PC speakers in real time

## Hardware

| Component | Details |
|---|---|
| Microcontroller | ESP32-C3 Super Mini |
| Microphone | INMP441 I²S MEMS module |
| Display | 0.96" SSD1306 I²C OLED (128x64) |

## Pin Mapping

### INMP441 Microphone

| INMP441 Pin | ESP32-C3 Pin |
|---|---|
| VDD | 3.3V |
| GND | GND |
| L/R | GND (selects left channel / mono) |
| SCK (BCLK) | GPIO2 |
| WS (LRCL) | GPIO3 |
| SD (DOUT) | GPIO4 |

### SSD1306 OLED (I²C)

| OLED Pin | ESP32-C3 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |

## Files

- `wireless_mic.ino` — Arduino sketch for ESP32-C3 Super Mini
- `udp_mic_rx.py` — Python receiver script (run on your laptop)

## Setup

### ESP32-C3 (Arduino IDE)

1. Install the **ESP32 board package** by Espressif Systems in Arduino IDE.
2. Install the **ESP8266 and ESP32 OLED driver for SSD1306** library by ThingPulse.
3. Open `wireless_mic.ino` and update:
   - `ssid` and `password` to your Wi-Fi credentials
   - `udpAddress` to your laptop’s IPv4 address
4. Select **ESP32C3 Dev Module** as the board and upload.

### Python (PC)

1. Install dependencies:
   ```
   pip install sounddevice numpy
   ```
2. Run the receiver:
   ```
   python udp_mic_rx.py
   ```
3. Talk into the mic — the OLED bar reacts immediately and audio plays on your PC with a short delay.

## How It Works

1. The ESP32-C3 reads 16-bit PCM samples from the INMP441 via I²S at 16 kHz.
2. A high-pass filter removes DC offset and low-frequency rumble.
3. A noise gate silences blocks where the RMS level is below a threshold.
4. Processed samples are packed into UDP packets and sent over Wi-Fi to the laptop.
5. The Python script receives the UDP packets and plays them through the system audio output using `sounddevice`.

## Notes

- Expected latency: ~100–200 ms (inherent to Wi-Fi + UDP buffering).
- The noise gate threshold (`rms < 800`) can be tuned based on your environment.
- ESP32-C3 only supports BLE, not Classic Bluetooth, so Wi-Fi UDP is used for audio streaming.
