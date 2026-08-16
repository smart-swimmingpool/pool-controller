# Design: Automatic NORVI Button Threshold Calibration

**Date:** 2026-08-16
**Status:** Draft for user review
**Scope:** Add a guided calibration wizard to the web UI that measures the NORVI AE01-R front-panel button ADC levels and derives the button thresholds automatically, replacing manual calibration.

## Motivation

The NORVI AE01-R front-panel buttons share a single analog input (GPIO32) and
produce distinct ADC levels via a resistor ladder. The thresholds
(`btn1Min`…`btnNoPress`) are currently calibrated manually: the values were
measured once on 2026-08-16 and hard-coded as defaults, then made configurable
through NVS and the web UI. Manual calibration requires reading live ADC values
and computing midpoints by hand — error-prone and hardware-dependent.

This design adds an automatic calibration flow: a guided wizard in the web UI
that samples the resting level and each button level, computes the thresholds
at the midpoints between adjacent levels, and persists them to NVS.

## Design Decisions (from brainstorming)

- **Trigger:** Guided wizard in the web UI (modal dialog), started via a
  "Start Calibration" button in the Button Thresholds section.
- **Sampling:** Press-and-hold; the device detects the level change itself and
  samples ~20 readings over 1 s, then averages them. No user timing required.
- **Resting level:** Measured first; `btn1Min` is set to the midpoint between
  the resting level and S1.
- **No-press sentinel:** `btnNoPress` stays at 4096 (no-op sentinel, S3 reads
  full scale). Calibration never changes it.
- **Error handling:** Per-step timeout (10 s) and minimum-gap check (100 ADC)
  cause the step to be retried; the wizard can be cancelled at any time; old
  thresholds remain untouched until a successful save.
- **Live feedback:** The wizard polls the current ADC value every ~500 ms so the
  user can see the level is stable and the button is detected.
- **Architecture:** Separate `CalibrationManager` module with its own state
  machine; `NorviButtonHandler` stays focused on detection.

## Architecture

### New module: `CalibrationManager`

`src/CalibrationManager.hpp/.cpp` — compiled only when `NORVI_AE01_R` is
defined.

Owns the calibration state machine and reads the ADC directly on GPIO32
(`PIN_BUTTON_ADC`) with its own simple averaging (20 samples over 1 s) and
stability detection. Independent of the filtering inside `NorviButtonHandler`.

**States:**

```text
IDLE → RESTING → BTN1 → BTN2 → BTN3 → COMPUTE → SAVE → DONE
   └─────────────── ERROR (retry current step or cancel → IDLE)
```

**Public API (static):**

- `begin()` — initialize.
- `loop()` — drive the state machine; called from `PoolController::loop()`.
- `start()` — begin calibration; only valid from `IDLE`.
- `cancel()` — abort and return to `IDLE`; old thresholds stay in NVS.
- `getStatus()` — current step, live ADC, measured levels so far, message.
- `isActive()` — true while a calibration is running.

**Status struct:**

```cpp
enum class Step { IDLE, RESTING, BTN1, BTN2, BTN3, DONE, ERROR };
struct CalibrationStatus {
  Step step;
  uint16_t liveAdc;        // current filtered ADC reading
  uint16_t restingLevel;   // measured so far (0 until measured)
  uint16_t s1, s2, s3;     // measured button levels (0 until measured)
  const char* message;     // instruction or error text
};
```

### Button callback suppression

`NorviButtonHandler::loop()` early-returns when `CalibrationManager::isActive()`
is true, so no button callback can fire during calibration (no accidental pump
toggle, mode cycle, or save-and-reboot). One-way dependency
`NorviButtonHandler → CalibrationManager`, no cycle.

### Threshold computation

Midpoints between adjacent levels:

```text
btn1Min = (resting + S1) / 2      btn1Max = (S1 + S2) / 2
btn2Min = (S1 + S2) / 2           btn2Max = (S2 + S3) / 2
btn3Min = (S2 + S3) / 2           btn3Max = 4095 (full scale stays)
btnNoPress = 4096 (sentinel, unchanged)
```

Sanity checks before saving:

- Levels must be strictly ascending: `resting < S1 < S2 < S3`.
- Minimum gap between adjacent levels: 100 ADC.
- `S3` must be ≤ 4095.

On success: `ConfigManager::save()` then `NorviButtonHandler::applySettings()`
so the new thresholds apply to the running handler immediately (no reboot).

## REST API (WebPortal.cpp, auth-protected)

- `POST /api/calibrate/start` — start calibration (from IDLE).
- `GET /api/calibrate/status` — JSON: `{ step, liveAdc, resting, s1, s2, s3, message }`.
- `POST /api/calibrate/cancel` — cancel calibration.

No "complete" endpoint: the state machine saves automatically when `COMPUTE`
succeeds.

## Web UI

- **Button:** "Start Calibration" in the Button Thresholds (NORVI) section.
- **Modal dialog** with:
  - Step instructions ("Please press and hold Button 1…").
  - Live ADC value (poll `GET /api/calibrate/status` every 500 ms).
  - Progress indicator (Resting → S1 → S2 → S3).
  - Cancel button.
  - On success: close modal, reload config so the fields show the new values.
  - On error: show message, offer retry of the current step.

## Error Handling

| Condition | Behavior |
|---|---|
| No stable level within 10 s | Retry current step, message "No stable level detected — please try again" |
| Level too close to previous (< 100 ADC) | Retry current step, message "Level too close to previous — please check" |
| Cancel (button or endpoint) | Return to IDLE, old thresholds untouched |
| Save failure | ERROR state, old thresholds untouched |

## Testing

Native tests (no hardware), with an injectable ADC read function (test hook
instead of `analogRead`):

- **State machine:** transitions IDLE→RESTING→…→DONE; cancel from every step;
  timeout behavior.
- **Threshold computation:** midpoint math, ascending-level sanity checks,
  minimum-gap enforcement, S3 = 4095 edge case.
- **Error cases:** unstable level, too-close level, timeout → correct state and
  message.
- **Web UI:** manual verification on the device (wizard flow, live ADC, cancel).

## Files

New:

- `src/CalibrationManager.hpp`
- `src/CalibrationManager.cpp`
- `test/native/tests/test_calibration_manager.cpp`

Modified:

- `src/NorviButtonHandler.cpp` — early-return in `loop()` when calibration active.
- `src/WebPortal.cpp` — three calibration endpoints.
- `data/web/index.html` — start button + modal markup.
- `data/web/app.js` — wizard logic, polling, modal handling.
- `test/native/CMakeLists.txt` — add test source.