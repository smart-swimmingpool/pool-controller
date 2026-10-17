# Tasks

- [ ] Extract pure timer/window calculations with rollover/midnight tests.
- [ ] Add `ControlInputs`, `ControlState`, `ControlDecision` and decision-reason types.
- [ ] Port Auto behavior to pure evaluation and compare against current relay-safety tests.
- [ ] Port Manual behavior.
- [ ] Port Boost behavior.
- [ ] Port Timer behavior including temperature-based runtime extension.
- [ ] Add Core-1 actuator application with final safe-mode/degradation clamps.
- [ ] Remove `RelayModuleNode*` dependencies from rule/domain classes.
- [ ] Add exhaustive native decision-table tests and edge cases.
- [ ] Verify relay safety suite and all PlatformIO board builds.

## Hardware gates

Measured on `esp32dev`, `norvi_ae01_r` and the single-core `olimex_esp32_c6_evb`; a gate fails on any board.

- [ ] Loop latency: worst-case `loop()` iteration ≤ 500 ms and p99 ≤ 100 ms over a 24 h run per board.
- [ ] Watchdog: 0 task-WDT resets during a 72 h soak per board.
- [ ] Pump fail-safe: after losing the solar probe, the solar relay is off within one measurement cycle; the
  filter pump follows the time fallback policy.
- [ ] Decision parity: relay states over a 24 h replay of recorded inputs match the pre-refactor firmware.
