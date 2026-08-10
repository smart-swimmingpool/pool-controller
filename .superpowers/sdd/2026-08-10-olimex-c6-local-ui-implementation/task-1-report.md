Status: DONE_WITH_CONCERNS

Fix round 4/5:
- Changed: `src/MqttPublisher.hpp`
- Why: add explicit `#include <cstdint>` so `std::uint32_t` resolves under Arduino 3.x / C6.

Commands run:
- `/mnt/ssd/projects/pool-controller/venv/bin/pio run -e esp32dev` — passed.
- `/mnt/ssd/projects/pool-controller/venv/bin/pio run -e norvi_ae01_r` — passed.
- `/mnt/ssd/projects/pool-controller/venv/bin/pio run -e olimex_esp32_c6_evb` — still fails before project compilation in PlatformIO/pioarduino package install with `FileNotFoundError: package-postinstall.py` while installing `esptoolpy-v5.3.0.zip`.

Outcome:
- Minimal project-code compatibility fix applied.
- ESP32 Dev and NORVI builds pass.
- Olimex C6 no longer shows the original `std::uint32_t` error, but full build is blocked by an upstream PlatformIO/pioarduino package-install failure.

Remaining concerns:
- `src/idf_component.yml` exists as an untracked file in the worktree; it was not modified for this fix.
- C6 verification is not complete until the upstream `package-postinstall.py` install issue is resolved.

Commands run:
- `git status --short` / `git diff -- src/Config.hpp platformio.ini` — confirmed only the requested files changed.
- `/mnt/ssd/projects/pool-controller/venv/bin/pio run -e esp32dev` — passed.
- `/mnt/ssd/projects/pool-controller/venv/bin/pio run -e norvi_ae01_r` — passed.
- `/mnt/ssd/projects/pool-controller/venv/bin/pio run -e olimex_esp32_c6_evb` — now gets past OneWire and TFT_eSPI removal/install; the remaining failure is a pioarduino/PlatformIO framework path `TypeError` in `builder/frameworks/arduino.py` while resolving `pioarduino-build.py`.

Acceptable failures:
- The Olimex C6 build no longer fails in OneWire/TFT_eSPI; the remaining framework path `TypeError` appears to be an upstream pioarduino/PlatformIO integration issue.

Unacceptable failures:
- None observed in the edited files.

Concerns:
- pioarduino is now required for the Olimex C6 Arduino environment.
- The Olimex build still does not complete due to the framework path error, so full compile validation remains pending.
