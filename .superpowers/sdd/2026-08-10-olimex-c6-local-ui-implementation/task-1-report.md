Status: DONE_WITH_CONCERNS

Commits:
- 8faceba `build: add olimex esp32-c6 board variant`
- <pending> `build: use pioarduino for olimex c6 arduino`

Commands run:
- `git status --short` / `git diff -- src/Config.hpp platformio.ini` — confirmed only the requested files changed.
- `/mnt/ssd/projects/pool-controller/venv/bin/pio run -e esp32dev` — passed.
- `/mnt/ssd/projects/pool-controller/venv/bin/pio run -e norvi_ae01_r` — passed.
- `/mnt/ssd/projects/pool-controller/venv/bin/pio run -e olimex_esp32_c6_evb` — failed on OneWire ESP32-C6 register incompatibilities after the pioarduino platform and Arduino 3.3.11 toolchain were installed successfully.

Acceptable failures:
- The Olimex C6 build currently fails in `OneWire` on ESP32-C6-specific register access (`GPIO.in`, `GPIO.out_w1ts`, etc.). This is unrelated to the platform swap and indicates a later code/library compatibility fix is needed.

Unacceptable failures:
- None observed in the edited files.

Concerns:
- pioarduino is now required for the Olimex C6 Arduino environment.
- The Olimex build still does not complete because the project’s OneWire dependency is not ESP32-C6 compatible yet.
