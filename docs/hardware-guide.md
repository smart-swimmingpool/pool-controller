---
title: Hardware Guide of Pool Controller
summary:
date: "2020-05-28"
lastmod: "2020-06-02"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "controller", "tutorial"]
menu:
  docs:
    parent: Pool Controller
    name: Hardware Guide
    weight: 20
---

This Hardware Guide will describe how to setup the hardware of the controller.

## Parts List (BOM)

- 1 * ESP32 DevKit Controller (recommended; full feature set) **or** ESP8266 NodeMCU
  ([Amazon ESP32](https://amzn.to/3Eg0LQV) / [Amazon ESP8266](https://amzn.to/2Ze9DSh))
- 2 * DS18B20 Temperature Sensors ([Amazon](https://amzn.to/2ZlfZ2c))
- 1 * Relais-Module 5V ([Amazon](https://amzn.to/31RBd5s))
- 1 * SSD1306 OLED 128x64 display (I2C) **or** Waveshare 2.9" e-paper (SPI)
  (optional, for local display without internet)
- 1–4 * Momentary push-buttons (for mode cycling and settings navigation)
- 1 * Breadboard and wires to connect (alternativly soldering of the circuit)

## Circuit

The circuit of the controller could be found on following image based on a breadboard wireing:

{{< figure library="true" src="../pool-controller_breadboard.png" title="Breadboard Circuit of Pool Controller" lightbox="true" >}}

The source [Fritzing](https://fritzing.org/) file could be found in GitHub project: [pool-controller.fzz](https://github.com/smart-swimmingpool/pool-controller/raw/main/docs/pool-controller.fzz)

### ESP8266 PIN Usage

The ESP8266 is connected using following PINs. You can find the constant values within the sources
of `main.cpp` (first column of table) which are associated to the pins.

| Constant in Source | PIN of ESP8266 | Description                                                      |
|--------------------|:--------------:|------------------------------------------------------------------|
| PIN_DS_SOLAR       | D5             | Pin of temperature sensor (DS18B20) for solar storage            |
| PIN_DS_POOL        | D6             | Pin of temperature sensor (DS18B20) for pool water               |
| PIN_RELAY_POOL     | D1             | Pin to connect relais for pool pump                              |
| PIN_RELAY_SOLAR    | D2             | Pin to connect relais for solar pump                             |
| PIN_BUTTON_MODE    | D7             | Mode-cycle push-button (connect to GND)                          |
| PIN_DISPLAY_SDA    | D3 (GPIO0)     | I2C SDA for OLED display ⚠ boot-sensitive, see note below        |
| PIN_DISPLAY_SCL    | D4 (GPIO2)     | I2C SCL for OLED display ⚠ boot-sensitive, see note below        |

> **⚠ Boot note (ESP8266):** D3 (GPIO0) and D4 (GPIO2) must be pulled HIGH at power-on for
> normal boot. The 4.7 kΩ pull-up resistors on the OLED module keep both I2C lines HIGH at
> idle, so the display can safely be connected to these pins. If you add any additional
> circuitry to D3 or D4 (e.g. a second I2C sensor without pull-up), ensure the lines are
> not pulled LOW when the device powers on.
>
> **ℹ ESP8266 GPIO limitation:** There are no remaining free GPIOs for UP/DOWN/SELECT buttons
> without a hardware redesign. Settings adjustment via buttons is therefore **ESP32 only**.

### ESP32 PIN Usage

| Constant in Source | GPIO of ESP32 | Description                                                      |
|--------------------|:-------------:|------------------------------------------------------------------|
| PIN_DS_SOLAR       | 15            | Pin of temperature sensor (DS18B20) for solar storage            |
| PIN_DS_POOL        | 16            | Pin of temperature sensor (DS18B20) for pool water               |
| PIN_RELAY_POOL     | 18            | Pin to connect relais for pool pump                              |
| PIN_RELAY_SOLAR    | 19            | Pin to connect relais for solar pump                             |
| PIN_BUTTON_MODE    | 23            | Mode-cycle push-button (connect to GND)                          |
| PIN_BUTTON_UP      | 34            | Settings UP button (connect to GND) – input-only GPIO            |
| PIN_BUTTON_DOWN    | 35            | Settings DOWN button (connect to GND) – input-only GPIO          |
| PIN_BUTTON_SELECT  | 32            | Settings SELECT button (connect to GND)                          |
| PIN_DISPLAY_SDA    | 21            | I2C SDA for OLED / e-paper display                               |
| PIN_DISPLAY_SCL    | 22            | I2C SCL for OLED / e-paper display                               |

{{% alert note %}}
TODO: improve PIN usage (see https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/)
{{% /alert %}}

## Local Display

A connected display shows the current controller status **without any internet or MQTT
connection**:

```
auto   [W]           ← mode + WiFi indicator ([W] = online, --- = offline)
Pool:  28.4C  ON     ← pool temperature + pump state
Solar: 62.1C  OFF    ← solar temperature + pump state
10:30 - 17:30        ← configured timer window
```

### Default display: SSD1306 OLED 128×64 (I2C)

Connect the display to the I2C pins defined above.  The 4.7 kΩ pull-up resistors
usually shipped on the module are sufficient.

### Alternative: e-paper (Waveshare 2.9", SPI)

Add `-D USE_EPAPER` to `build_flags` in `platformio.ini`.  Default SPI CS/DC/RST pins
are pre-defined in `LocalDisplayNode.cpp`; override them with
`-D DISPLAY_EPAPER_CS=x -D DISPLAY_EPAPER_DC=y -D DISPLAY_EPAPER_RST=z` if needed.
E-paper refreshes every 60 seconds (instead of 2 seconds for OLED) to avoid display wear.

## Direct Control (Offline Operation)

A single push-button connected to `PIN_BUTTON_MODE` allows cycling through all operation modes
**without an internet or MQTT connection**:

```
auto → boost → timer → manu → auto → …
```

Wire the button between `PIN_BUTTON_MODE` and **GND**. The internal pull-up resistor is enabled
automatically – no additional resistor is required.

| Mode  | Behaviour while offline                                         |
|-------|------------------------------------------------------------------|
| auto  | Pumps controlled automatically by temperature                   |
| boost | Both pumps forced on while below max. pool temperature          |
| timer | Pool pump follows the configured timer window                   |
| manu  | No automatic rule; pumps stay in their current on/off state     |

## Settings Adjustment Without Internet (ESP32)

Three additional buttons (UP / DOWN / SELECT, see pin table above) open a settings
menu on the display, allowing all key parameters to be changed **completely offline**:

| Setting        | Step  | Range       |
|----------------|------:|-------------|
| Pool Max Temp  | 0.5°C | 0 – 40°C    |
| Solar Min Temp | 0.5°C | 0 – 100°C   |
| Hysteresis     | 0.1°C | 0 – 10°C    |
| Start Hour     | 1 h   | 0 – 23      |
| Start Minute   | 5 min | 0 – 55      |
| End Hour       | 1 h   | 0 – 23      |
| End Minute     | 5 min | 0 – 55      |

**Button usage:**

| Button | Normal state       | Settings navigation | Value editing        |
|--------|-------------------|---------------------|----------------------|
| MODE   | Cycle mode        | Close menu          | Cancel (no save)     |
| SELECT | Open settings     | Edit current item   | Save value           |
| UP     | –                 | Previous item       | Increase value       |
| DOWN   | –                 | Next item           | Decrease value       |

The settings menu closes automatically after 30 seconds of inactivity.

## Power Supply

In my environment I use the USB to power the ESP8266 via small USB-Power-Adapter andan additional
230V power plug to be used as source for the power of the pumps which are switched via the relais.
