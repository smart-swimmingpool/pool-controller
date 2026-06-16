---
title: Frequently Asked Questions (FAQ)
summary: Troubleshooting guide for common issues with the Smart Swimming Pool Controller. Structured by category for easy navigation.
type: docs
weight: 50
---

# ❓ Frequently Asked Questions (FAQ)

This FAQ covers **common issues** and their solutions for the **Smart Swimming Pool Controller**. 
If your problem isn't listed here, check the [Discussions](https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions) or open a new issue on GitHub.

---

## 🚨 Safety & Warnings

### ⚠️ Is this project safe for beginners?
**No.** This project involves **230V AC mains voltage**, which can be **deadly** if mishandled.
- **Only proceed if you have basic electronics knowledge** (e.g., understanding of voltage, current, and safety precautions).
- **Always use a Residual Current Device (RCD/FI circuit breaker)** for the pump circuit.
- **Disconnect power before working on the circuit.**
- **If in doubt, consult a qualified electrician.**
- **This project is NOT certified (no CE/UL mark). For personal use only!**

### ⚠️ What safety precautions should I take?
1. **Use an RCD (FI circuit breaker)** for the pump circuit.
2. **Keep low-voltage (sensor) wiring separate from mains wiring.**
3. **Use insulated tools and wear safety glasses.**
4. **Never work on live circuits.**
5. **Test connections with a multimeter before powering on.**

---

## 🔌 Hardware Issues

### 🔹 My DS18B20 sensor is not detected (shows -127°C or "Sensor error")

#### **Possible Causes & Solutions**
| **Cause** | **Solution** | **How to Test** |
|-----------|--------------|-----------------|
| Missing 4.7kΩ pull-up resistor | Add a **4.7kΩ resistor** between the DATA line and **3.3V**. | Measure resistance between DATA and 3.3V (should be ~4.7kΩ). |
| Wrong GPIO pin | Check `PIN_DS_SOLAR` (GPIO32) and `PIN_DS_POOL` (GPIO33) in `src/Config.hpp`. | Verify wiring matches the firmware configuration. |
| Sensor not connected to 3.3V | Ensure **VDD (red wire)** is connected to **3.3V** (not 5V!). | Measure voltage between VDD and GND (should be ~3.3V). |
| Sensor not connected to GND | Ensure **GND (black wire)** is connected to **GND**. | Measure continuity between sensor GND and ESP32 GND. |

#### **Debugging Steps**
1. **Check the serial monitor** (115200 baud) for sensor initialization messages.
2. **Test with a single sensor** (disconnect one and test the other).

---

### 🔹 My relay module doesn't work

#### **Possible Causes & Solutions**
| **Cause** | **Solution** | **How to Test** |
|-----------|--------------|-----------------|
| Relay module not powered | Ensure **VCC** is connected to **5V** and **GND** to **GND**. | Measure voltage between VCC and GND (should be ~5V). |
| Wrong logic level (active-low) | Use an **active-high relay module** or modify firmware. | Check if relay LED lights up when GPIO goes HIGH. |
| Incorrect GPIO pin | Verify `PIN_RELAY_POOL` (GPIO25) and `PIN_RELAY_SOLAR` (GPIO26). | Check wiring matches firmware configuration. |

---

## 💻 Software & Firmware Issues

### 🔹 How do I update the firmware?

#### **Option A: Web Interface (OTA)**
1. Open **Web Dashboard** (`http://<controller-ip>/`).
2. Go to **Security & Update** > **OTA Firmware Update**.
3. Select the `.bin` file and click **Update**.

#### **Option B: Arduino IDE/PlatformIO**
1. Open `pool-controller.ino` in Arduino IDE or PlatformIO.
2. Click **Upload** (→).

---

### 🔹 My controller doesn't connect to WiFi

#### **Debugging Steps**
1. **LED Status Codes**:
   - **Rapid blink (5 Hz)**: AP mode (no WiFi configured).
   - **Slow blink (1 Hz)**: WiFi connecting.
   - **Solid on**: Fully connected.

2. **Reconnect via AP Mode**:
   - Connect to `Pool-Controller-Setup` (open network).
   - Open `http://192.168.4.1` and reconfigure WiFi.

---

### 🔹 My controller doesn't connect to MQTT

#### **Debugging Steps**
1. Verify **MQTT Host/IP** and **Port** in Web Dashboard.
2. Test MQTT manually:
   ```bash
   mosquitto_sub -h <broker-ip> -t "#" -v
   ```

---

### 🔹 Home Assistant doesn't discover my controller

#### **Solution**
1. **Upgrade to v3.3.0+** (Homie support removed).
2. **Clear retained MQTT messages**:
   ```bash
   mosquitto_pub -h <broker> -t "homeassistant" -n -r
   ```
3. **Enable MQTT Discovery** in Home Assistant.

---

## 🌐 Integration Guides

### 🔹 How do I set up Mosquitto on Raspberry Pi?

```bash
sudo apt-get update && sudo apt-get upgrade
sudo apt-get install mosquitto mosquitto-clients
sudo systemctl start mosquitto
sudo systemctl enable mosquitto
```

---

### 🔹 How do I integrate with Node-RED?

1. Install Node-RED:
   ```bash
   npm install -g node-red
   node-red
   ```
2. Install MQTT nodes:
   - **Menu > Manage palette > Install > node-red-node-mqtt**

---

## 🔄 Operation & Usage

### 🔹 How do I change the operation mode?

#### **Via Web Dashboard**
1. Go to **Configuration** tab.
2. Select **Auto**, **Manual**, **Timer**, or **Boost**.

#### **Via MQTT**
```bash
mosquitto_pub -h <broker-ip> -t "homeassistant/select/pool-controller/mode/set" -m "auto"
```

---

## 📚 Additional Resources

- [Hardware Guide](hardware-guide.md)
- [MQTT Configuration](mqtt-configuration.md)
- [Home Assistant Integration](https://github.com/smart-swimmingpool/pool-controller/tree/main/docs/home-assistant)
- [openHAB Configuration](https://github.com/smart-swimmingpool/openhab-config)
- [Grafana Dashboard](https://github.com/smart-swimmingpool/grafana-dashboard)

---

## 🤝 Still Need Help?

1. **Search [Discussions](https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions)**.
2. **Open a [new issue](https://github.com/smart-swimmingpool/pool-controller/issues/new)**.

---

## 📄 License
This FAQ is part of the **Smart Swimming Pool** project, licensed under the **MIT License**.