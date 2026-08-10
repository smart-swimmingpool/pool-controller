Status: DONE_WITH_CONCERNS

Commits:
- 8faceba `build: add olimex esp32-c6 board variant`
- 91054d6 `build: use pioarduino for olimex c6 arduino`
- <pending> `build: pin onewire for esp32-c6 compatibility`

Commands run:
- `git status --short` / `git diff -- src/Config.hpp platformio.ini` — confirmed only the requested files changed.
- `/mnt/ssd/projects/pool-controller/venv/bin/pio run -e esp32dev` — passed.
- `/mnt/ssd/projects/pool-controller/venv/bin/pio run -e norvi_ae01_r` — passed.
- `/mnt/ssd/projects/pool-controller/venv/bin/pio run -e olimex_esp32_c6_evb` — now gets past OneWire install; the remaining failure is a pioarduino/PlatformIO framework path `TypeError` in `builder/frameworks/arduino.py` while resolving `pioarduino-build.py`.

Acceptable failures:
- The Olimex C6 build no longer fails in OneWire; the remaining framework path `TypeError` appears to be an upstream pioarduino/PlatformIO integration issue.

Unacceptable failures:
- None observed in the edited files.

Concerns:
- pioarduino is now required for the Olimex C6 Arduino environment.
- The Olimex build still does not complete due to the framework path error, so full compile validation remains pending.
