# Olimex ESP32-C6-EVB Variant

## Hardware

- Board: Olimex ESP32-C6-EVB
- Display: Adafruit GFX + Adafruit ILI9341/ST7789 on a configurable 320x240 SPI TFT, 2.8" preferred, 2.4" acceptable
- Display controller selection: set by `TFT_DRIVER_ILI9341` / `TFT_DRIVER_ST7789` in `src/Config.hpp`
- Input: KY-040 rotary encoder with push button
- Logic level: 3.3 V only

## Olimex Reference Documentation

Use the official Olimex files as the source of truth for board-level details:

- Product page: <https://www.olimex.com/Products/IoT/ESP32-C6/ESP32-C6-EVB/open-source-hardware>
- Repository: <https://github.com/OLIMEX/ESP32-C6-EVB>
- User manual: <https://github.com/OLIMEX/ESP32-C6-EVB/blob/main/DOCUMENTS/ESP32-C6-EVB-user-manual.pdf>
- Schematic Rev. A: <https://github.com/OLIMEX/ESP32-C6-EVB/blob/main/HARDWARE/Hardware-revision-A/ESP32-C6-EVB_Rev_A.pdf>
- Board dimensions Rev. A: <https://github.com/OLIMEX/ESP32-C6-EVB/blob/main/HARDWARE/Dimensions/ESP32-C6-EVB_rev_A_dimensions.pdf>
- Olimex Arduino full demo and pin reference: <https://github.com/OLIMEX/ESP32-C6-EVB/blob/main/SOFTWARE/ARDUINO/esp32-c6-evb-full-demo/esp32-c6-evb-full-demo.ino>

## Olimex Board Notes

- Native USB CDC is the expected serial console path. Keep `ARDUINO_USB_CDC_ON_BOOT=1` enabled for this environment.
- Olimex demo pins to avoid for the external local UI when possible: GPIO8 user LED, GPIO9 user button, GPIO10/11/22/23 relay outputs, and GPIO1/2/3/15 optocoupler inputs.
- The Olimex optocoupler inputs are intended for higher-voltage field signals; the SPI TFT and KY-040 wiring are not. Keep TFT and encoder signals at 3.3 V.
- Check the official schematic and user manual before changing any pin assignment or adding UEXT wiring.

## Current Pool Controller Pin Map

Configured in `src/Config.hpp` for `OLIMEX_ESP32_C6_EVB`:

| Function      | GPIO | Notes                                                                         |
| ------------- | ---: | ----------------------------------------------------------------------------- |
| TFT MOSI      |   18 | SPI write data                                                                |
| TFT SCLK      |   19 | SPI clock                                                                     |
| TFT MISO      |   -1 | Write-only TFT; leave SDO/MISO unconnected                                    |
| TFT CS        |   21 | TFT chip select                                                               |
| TFT DC        |    7 | TFT data/command                                                              |
| TFT RST       |   -1 | Display reset is tied high, not GPIO-controlled                               |
| TFT backlight |   -1 | Fixed 3.3 V backlight or external MOSFET/current limiting                     |
| KY-040 CLK    |    4 | Encoder A/CLK, internal pull-up                                               |
| KY-040 DT     |    5 | Encoder B/DT, internal pull-up                                                |
| KY-040 SW     |   16 | Push button, internal pull-up                                                 |
| DS18B20 solar |    6 | Temperature bus                                                               |
| DS18B20 pool  |   20 | Shared with the (unused) TFT MISO/readback pin if that pin is ever re-enabled |
| Pool relay    |   10 | Olimex relay output                                                           |
| Solar relay   |   11 | Olimex relay output                                                           |
| Status LED    |    8 | Olimex user LED                                                               |

## Safety Rules

- Do not feed 5 V into ESP32-C6 GPIOs.
- Do not drive the TFT backlight directly from an ESP32 GPIO.
- Power the TFT logic and KY-040 from 3.3 V.
- Verify display pinout, backlight polarity, and current limiting before wiring.

## Bring-Up Steps

1. Build `olimex_esp32_c6_evb`.
2. Power the board without TFT/encoder and verify serial boot over native USB CDC.
3. Connect TFT VCC/GND/SPI/DC; tie TFT RST high unless you update `PIN_TFT_RST` in `src/Config.hpp`.
4. Verify the TFT boot screen.
5. Connect KY-040 CLK/DT/SW to GPIO 4 / 5 / 16 from `src/Config.hpp`.
6. Verify rotate clockwise/counter-clockwise changes pages.
7. Verify short press opens menu.
8. Verify long press returns to overview.
9. Open QR page and scan `http://pool-controller.local`.
10. Only after UI validation, connect real relay loads and opto inputs.
