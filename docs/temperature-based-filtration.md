# Temperature-Based Filtration Duration

## Overview

This document describes the temperature-based automatic filtration
duration calculation feature, which adapts pool filtration runtime to
water temperature for optimal water quality.

## Feature Description

The system can automatically calculate filtration duration based on
pool water temperature, replacing fixed start/stop times with dynamic
calculations that align with pool industry best practices and DIN
standards.

### Key Benefits

- **Optimal Water Quality**: Ensures adequate filtration based on
  temperature
- **Energy Efficiency**: Avoids over-filtration in cooler conditions
- **Temperature Protection**: Increases filtration when pool approaches
  maximum temperature
- **Configurable**: Users can choose between dynamic and fixed timer
  modes

## Configuration Parameters

### MQTT Homie Settings

All parameters are exposed as MQTT Homie settings and can be configured
at runtime:

- `pool-volume` (float, m³): Pool volume in cubic meters
  - Default: 30
  - Range: 0-1000
  - Example: 30 m³ pool

- `pump-capacity` (float, m³/h): Pump flow rate in cubic meters per
  hour
  - Default: 6
  - Range: 0-100
  - Example: 6 m³/h pump

- `use-temp-based-duration` (boolean): Enable/disable temperature-based
  calculation
  - Default: false (uses fixed timer)
  - true: Dynamic temperature-based duration
  - false: Traditional fixed start/stop times

### Timer Settings

When using fixed timer mode or as base for temperature-based mode:

- `timer-start-h` / `timer-start-min`: Filtration start time
- `timer-end-h` / `timer-end-min`: Filtration end time (fixed mode
  only)

## Calculation Methods

### Implemented: Option A - Enhanced Temperature Formula

**Selected for implementation** based on analysis of DIN standards and
pool industry best practices.

#### Formula

```text
turnoverFactor = baseTurnoverFactor × (temperature / 20.0)
filtrationHours = (poolVolume / pumpCapacity) × turnoverFactor
```

Where:

- `baseTurnoverFactor = 2.5` (industry minimum: 2-3 turnovers/day)
- Temperature scaling increases turnover rate with temperature
- Maximum capped at 4.0 turnovers at max pool temperature

#### Turnover Rates

| Temperature | Turnover Factor | Example (30m³, 6m³/h) |
|-------------|-----------------|------------------------|
| 20°C        | 2.5x            | 12.5 hours             |
| 24°C        | 3.0x            | 15.0 hours             |
| 28°C        | 3.5x            | 17.5 hours             |
| Max temp    | 4.0x            | 20.0 hours             |

#### Rationale

- **Aligns with DIN 19643**: German standard for pool water treatment
- **Industry Best Practice**: 2-3 turnovers minimum, 3-4 for warm pools
- **Temperature Responsive**: Accounts for increased bacterial growth
  at higher temperatures
- **Maximum Protection**: Prevents overheating when max temperature
  reached

### Alternative Approaches Considered

#### Option B: German Rule of Thumb Formula

Direct implementation of German pool industry formula:

```text
filtrationHours = (temperature / 2) + 2
```

**Pros:**

- Simple, well-known formula in German-speaking regions
- Direct temperature dependency
- No complex parameters

**Cons:**

- Does not account for pool volume or pump capacity
- Fixed hours independent of actual turnover capability
- May not suit all pool configurations

**Example:**

- 20°C: (20/2) + 2 = 12 hours
- 28°C: (28/2) + 2 = 16 hours

#### Option C: Fully Configurable Turnover Parameters

Add user-configurable turnover range settings:

```cpp
// Additional settings
min-turnovers-per-day: 2.5 (default)
max-turnovers-per-day: 4.0 (default)
temp-multiplier: 1.0 (fine-tuning factor)

// Calculation
baseTurnovers = min-turnovers-per-day
tempFactor = (temp / 20.0) * temp-multiplier
turnoverFactor = min(baseTurnovers * tempFactor, max-turnovers-per-day)
```

**Pros:**

- Maximum flexibility for different pool types
- Can accommodate special requirements (spas, commercial pools)
- Fine-tuning possible without code changes

**Cons:**

- More complex configuration
- Risk of misconfiguration
- May overwhelm typical users

**Use Cases:**

- Commercial pools requiring 3-4 turnovers minimum
- Spas needing rapid turnover (30-minute cycles)
- Experimental optimization for specific conditions

## Standards and References

### DIN 19643

German standard for public swimming pool water treatment. Recommends:

- Minimum 2-3 complete turnovers per day
- Higher rates for warm water or high bather loads
- Process-specific calculations based on pool usage

### Pool Industry Guidelines

International recommendations:

- Residential pools: 2-3 turnovers/day minimum
- Commercial pools: 3-4 turnovers/day
- Spas/hot tubs: 6-12 turnovers/day (30-120 minute cycles)

### German Rule of Thumb

Common formula in German-speaking pool industry:

```text
Filtration hours = (Water temperature °C / 2) + 2
```

Provides baseline that increases with temperature:

- 20°C → 12 hours
- 24°C → 14 hours
- 28°C → 16 hours
- 30°C → 17 hours

### Research Sources

- DIN 19643: Pool water treatment standards
- EPA WaterSense: Commercial pool best practices
- German pool industry (poolinfos.de, vivapool.de)
- Pool equipment manufacturers (ZODIAC, Fluidra)

## Logging and Monitoring

### Log Output

When temperature-based mode is enabled, the system logs:

```text
Using temperature-based filtration duration
Pool temp: 24.0°C
Max pool temp: 28.5°C
Filtration duration: 15.0h
Turnovers per day: 3.0x
```

### Warnings

The system validates configuration and warns about:

**Pump Capacity Too Low:**

```text
⚠ Warning: Pump capacity too low for pool volume
  Time for 1 turnover: 25.5h (>24h)
  Consider increasing pump capacity or reducing pool volume setting
```

**Invalid Configuration:**

```text
✖ Invalid filtration configuration (pool volume or pump capacity not
set)
  Pool volume: 0.0 m³
  Pump capacity: 0.0 m³/h
```

## Usage Examples

### Example 1: Standard Residential Pool

**Configuration:**

- Pool volume: 30 m³
- Pump capacity: 6 m³/h
- Temperature-based: enabled

**Results:**

| Water Temp | Turnovers | Duration | Notes             |
|------------|-----------|----------|-------------------|
| 18°C       | 2.5x      | 12.5h    | Spring/Fall       |
| 22°C       | 2.75x     | 13.75h   | Early summer      |
| 26°C       | 3.25x     | 16.25h   | Mid summer        |
| 28°C       | 3.5x      | 17.5h    | Hot summer        |

### Example 2: Large Pool with Powerful Pump

**Configuration:**

- Pool volume: 60 m³
- Pump capacity: 12 m³/h
- Temperature-based: enabled

**Results:**

| Water Temp | Turnovers | Duration | Notes             |
|------------|-----------|----------|-------------------|
| 20°C       | 2.5x      | 12.5h    | Base rate         |
| 28°C       | 3.5x      | 17.5h    | Warm weather      |

Same turnover rates, same duration (good pump sizing).

### Example 3: Undersized Pump

**Configuration:**

- Pool volume: 50 m³
- Pump capacity: 2 m³/h
- Temperature-based: enabled

**Warning:** Time for 1 turnover: 25.0h (>24h)

The system will warn that the pump cannot complete even one turnover in
24 hours. Consider upgrading pump or adjusting settings.

## Backward Compatibility

### Default Behavior

By default, `use-temp-based-duration` is **false**, preserving existing
fixed timer functionality.

Existing installations continue working without changes.

### Migration Path

To enable temperature-based filtration:

1. Configure pool volume and pump capacity settings
2. Set `use-temp-based-duration` to `true` via MQTT
3. Monitor logs to verify calculation
4. Adjust timer start time if needed (end time is auto-calculated)

## Future Enhancements

Potential improvements for future versions:

1. **Seasonal Adjustments**: Different formulas for summer/winter
2. **Usage-Based Scaling**: Increase turnovers after heavy use
3. **Solar Integration**: Optimize filtration during solar heating
4. **Water Quality Feedback**: Adjust based on sensor readings (ORP,
   pH)
5. **Machine Learning**: Optimize based on historical patterns
6. **Multi-Zone Pools**: Different calculations for pool vs. spa

## Troubleshooting

### Pump Runs Too Long

**Symptom:** Filtration duration exceeds expected hours

**Causes:**

- Pool volume setting too high
- Pump capacity setting too low
- Temperature very high
- Max temperature protection triggered

**Solutions:**

- Verify and correct pool volume measurement
- Verify and correct pump flow rate specification
- Check if pool temperature ≥ max temperature setting

### Pump Runs Too Short

**Symptom:** Filtration duration insufficient, water quality degrades

**Causes:**

- Temperature-based mode not enabled
- Pool volume setting too low
- Pump capacity setting too high

**Solutions:**

- Enable `use-temp-based-duration` setting
- Verify pool volume and pump capacity settings
- Consider switching to fixed timer with longer duration

### Configuration Not Applied

**Symptom:** Changes to settings don't affect behavior

**Causes:**

- Settings not persisted
- Controller not restarted
- Wrong MQTT topic

**Solutions:**

- Verify MQTT messages received and acknowledged
- Restart controller if needed
- Check Homie device topic structure

## Conclusion

The temperature-based filtration feature provides an intelligent,
standards-based approach to pool water management. Option A was selected
for implementation as it balances simplicity, effectiveness, and
alignment with industry best practices.

Users who prefer fixed timer schedules can continue using that mode,
while those wanting optimized, temperature-responsive filtration can
enable the new feature with minimal configuration.
