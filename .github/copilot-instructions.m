# GitHub Copilot Instructions

## Project Overview
This repository contains PlatformIO-based IoT firmware for ESP8266 and ESP32 targets. It uses C++17, follows strict standards for architecture, memory management, maintainability, security, and 24/7 reliability.

## General Coding Standards
- Follow the coding conventions defined in `AGENTS.md`.
- Use clear, consistent naming and modular structure.
- Avoid blocking patterns, busy waits, and unbounded dynamic allocations.

## Commit Standards
- Apply Conventional Commits for all commits (`feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `chore`).
- Provide meaningful descriptions and break change identifiers when required.

## Architecture & Build
- Follow the layered project structure (`src/app`, `src/drivers`, `src/services`, `src/platform`, `include`, `test`).
- Respect environment and target configurations in `platformio.ini`.

## Resource Management
- Use static buffers when possible.
- Avoid `String` in loops; prefer JSON using `StaticJsonDocument`.
- Monitor heap and stack metrics.

## Concurrency & RTOS
- On ESP32, use FreeRTOS tasks with clear responsibilities.
- Use semaphores or queues for inter-task communication.
- Avoid unprotected globals.

## Security Requirements
- Enable Secure Boot and Flash Encryption for production builds.
- Disable debug interfaces in release.
- Use TLS for networking.

## OTA & Reliability
- Maintain safe OTA update patterns with checks, rollback, and validity.
- Ensure watchdogs are active.
- Provide reconnect logic with backoff and jitter.

## Logging & Telemetry
- Logs should be structured (`key=value`) and rate-limited.
- Include health metrics (heap, stack, uptime, reset reason, WiFi/MQTT status).

## Tests
- Include unit and component tests.
- Always test native builds in CI before merging.

## Do Not
- Use unbounded `delay()` or blocking calls in loop tasks.
- Introduce new warnings or formatting violations.

## References
Please consult the `AGENTS.md` file in this repository for detailed rules and examples.
