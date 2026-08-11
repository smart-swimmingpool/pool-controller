Status: DONE

Fix rounds completed:
- Added `#include <cstdint>` in `src/MqttPublisher.hpp` for Arduino 3.x / C6 `std::uint32_t` resolution.
- Kept pioarduino for `olimex_esp32_c6_evb`, because upstream `espressif32 @ 7.0.1` has no Arduino support for ESP32-C6.
- Pinned OneWire to upstream SHA `800f26f3ee6eb446a72e013785cac3700e54cc13` for ESP32-C6 support.
- Switched Olimex TFT dependencies from TFT_eSPI to Adafruit GFX + ILI9341/ST7789.
- Added IDF 5.5 WPS start compatibility in `src/WpsProvisioner.cpp`.
- Added IDF 5.x internal temperature sensor support via `driver/temperature_sensor` while retaining `temprature_sens_read()` for Arduino 2.x / IDF 4.x.
- Set `board_build.partitions = default_8MB.csv` for the 8MB ESP32-C6 target so the current firmware fits.

Commands run:
- `PLATFORMIO_CORE_DIR="/mnt/ssd/projects/.pio-core-pool-olimex" /mnt/ssd/projects/pool-controller/venv/bin/pio run -e esp32dev` — passed.
- `PLATFORMIO_CORE_DIR="/mnt/ssd/projects/.pio-core-pool-olimex" /mnt/ssd/projects/pool-controller/venv/bin/pio run -e norvi_ae01_r` — passed.
- `PLATFORMIO_CORE_DIR="/mnt/ssd/projects/.pio-core-pool-olimex-c6" /mnt/ssd/projects/pool-controller/venv/bin/pio run -e olimex_esp32_c6_evb` — passed.
- `clang-format -i src/ESP32TemperatureNode.cpp src/ESP32TemperatureNode.hpp src/WpsProvisioner.cpp` — completed.

Verification notes:
- `esp32dev` and `norvi_ae01_r` use upstream `espressif32 @ 7.0.1` / Arduino 2.x and were verified in one stable PlatformIO core cache.
- `olimex_esp32_c6_evb` uses pioarduino / Arduino 3.3.11 and was verified in a separate stable PlatformIO core cache.
- Mixing upstream `espressif32` and pioarduino in the same `PLATFORMIO_CORE_DIR` can replace the shared `framework-arduinoespressif32` package and cause framework path/toolchain failures; this is a local PlatformIO package-cache limitation, not a source compile failure.

Remaining concerns:
- Full Task 1 review still needed before marking task complete in SDD.
