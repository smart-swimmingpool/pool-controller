---
title: Production Checklist
summary: Pre-deployment and annual maintenance checklist for the Pool Controller — hardware verification, firmware checks, safety inspections, and sign-off
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
tags: ["docs", "checklist", "commissioning", "production"]
menu:
  docs:
    parent: Pool Controller
    name: Production Checklist
    weight: 150
---

> ⚠️ **WARNING: This checklist involves 230V AC mains voltage verification.**
> Improper wiring poses risk of electric shock, fire, or equipment damage.
>
> - Only proceed if you have experience with mains-voltage installations
> - **Always disconnect power** before working on the circuit
> - This is a DIY project — not CE/UL certified for commercial use

## Overview

This checklist covers everything that should be verified before putting the
Pool Controller into production service. Use it during initial commissioning
and as an annual maintenance reference.

---

## Pre-Deployment Checklist

### Hardware

- [ ] ESP32 mounted securely inside enclosure
- [ ] Relay module mounted with adequate ventilation
- [ ] Power supply secured and rated for load
- [ ] All cable glands tightened and sealed
- [ ] Strain relief applied to all external cables
- [ ] Mains and low-voltage wiring physically separated
- [ ] No loose wires or stray strands
- [ ] All screw terminals tightened
- [ ] Fuse installed (correct rating for pump load)
- [ ] Enclosure IP rating matches location requirements

### Wiring

- [ ] DS18B20 sensors: VCC → 3.3V, GND → GND, DATA → GPIO (4.7 kΩ pull-up)
- [ ] Relay module: VCC → 5V, GND → GND, IN1 → GPIO18, IN2 → GPIO19
- [ ] Relay COM → mains Live
- [ ] Relay NO → pump Live
- [ ] Neutral → pump Neutral (direct, not through relay)
- [ ] Protective Earth connected to pump and enclosure (if metal)
- [ ] Ground continuity verified with multimeter

### Sensors

- [ ] Pool temperature reads plausible value (not -127 °C)
- [ ] Solar temperature reads plausible value (not -127 °C)
- [ ] Temperature values stable (not fluctuating wildly)
- [ ] Sensor cables undamaged and properly routed

### Firmware

- [ ] Latest firmware version flashed
- [ ] WiFi configured and connecting reliably
- [ ] mDNS name resolves (`pool-controller.local`)
- [ ] Web UI accessible at device IP
- [ ] Web UI password changed from default
- [ ] MQTT broker configured and connected
- [ ] Home Assistant Discovery active (controller visible in HA)
- [ ] All HA entities operational

### Relay Test (No-Load)

- [ ] Pool pump relay clicks when toggled
- [ ] Solar pump relay clicks when toggled
- [ ] Relay behavior correct (not inverted)
- [ ] Multimeter confirms COM–NO continuity when ON
- [ ] Multimeter confirms COM–NO open when OFF

### Relay Test (With Load)

- [ ] Pool pump starts and runs correctly
- [ ] Pool pump stops when toggled OFF
- [ ] Solar pump starts and runs correctly
- [ ] Solar pump stops when toggled OFF
- [ ] No unusual sounds from relays (buzzing, arcing)
- [ ] No excessive heat on relay terminals after 5 min

### Configuration

- [ ] Timezone configured correctly
- [ ] Timer settings match desired schedule
- [ ] Temperature thresholds configured
- [ ] Operation mode tested (Auto, Manual, Boost, Timer)
- [ ] `relay-invert` matches relay module type (active-low vs active-high)

---

## First 24 Hours Monitoring

- [ ] Controller stays online (no unexpected reboots)
- [ ] Temperatures update consistently
- [ ] MQTT connection stays active
- [ ] Heap stays above 20 KB
- [ ] WiFi signal stable (RSSI > -70 dBm recommended)
- [ ] No Safe Mode activation
- [ ] Relays operate on schedule (Auto mode)

---

## Annual Maintenance Checklist

- [ ] Verify all screw terminals are still tight
- [ ] Inspect cables for damage, wear, or corrosion
- [ ] Check enclosure seal and cable glands
- [ ] Test RCD/FI circuit breaker
- [ ] Clean enclosure ventilation openings
- [ ] Verify ground continuity
- [ ] Update firmware if newer version available
- [ ] Review configuration for any needed changes
- [ ] Check battery in RCD test unit (if applicable)
- [ ] Verify all sensor readings are still accurate

---

## Sign-Off

```text
Device ID / MAC: ______________________________
Installation date: ___________________________
Installed by: ________________________________
Firmware version: ____________________________
Last maintenance: ____________________________
Next maintenance due: ________________________
```

---

## Related Documents

- [Build from Zero](/docs/build-from-zero/) — Complete build guide
- [Electrical Safety](/docs/electrical-safety/) — Safety information
- [Safety Model](/docs/safety-model/) — System safety architecture
- [Security Checklist](/docs/security-checklist/) — Security hardening
- [Troubleshooting](/docs/troubleshooting/) — Common issues and fixes
