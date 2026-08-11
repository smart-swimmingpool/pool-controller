# Olimex ESP32-C6-EVB Variant

## Hardware

- Board: Olimex ESP32-C6-EVB
- Display: Adafruit GFX + Adafruit ILI9341/ST7789 on a configurable 320x240 SPI TFT, 2.8" preferred, 2.4" acceptable
- Display controller selection: set by `TFT_DRIVER_ILI9341` / `TFT_DRIVER_ST7789` in `src/Config.hpp`
- Input: KY-040 rotary encoder with push button
- Logic level: 3.3 V only

## Safety Rules

- Do not feed 5 V into ESP32-C6 GPIOs.
- Do not drive the TFT backlight directly from an ESP32 GPIO.
- Power the TFT logic and KY-040 from 3.3 V.
- Verify display pinout, backlight polarity, and current limiting before wiring.

## Bring-Up Steps

1. Build `olimex_esp32_c6_evb`.
2. Power the board without TFT/encoder and verify serial boot.
3. Connect TFT VCC/GND/SPI/DC; tie TFT RST high unless you update `PIN_TFT_RST` in `src/Config.hpp`.
4. Verify the TFT boot screen.
5. Connect KY-040 CLK/DT/SW to GPIO 4 / 5 / 0 from `src/Config.hpp`.
6. Verify rotate clockwise/counter-clockwise changes pages.
7. Verify short press opens menu.
8. Verify long press returns to overview.
9. Open QR page and scan `http://pool-controller.local`.
10. Only after UI validation, connect real relay loads and opto inputs.
