# Pool Controller 2.0 - MicroPython Version

This is the MicroPython port of the Smart Swimming Pool Controller.

## Hardware Requirements

- ESP32 development board
- 2x DS18B20 temperature sensors
- 2x Relay modules (5V)
- Pull-up resistors for OneWire bus
- Power supply

## Pin Configuration

Default pin assignments for ESP32:

- `PIN_DS_SOLAR = 15` - Solar temperature sensor
- `PIN_DS_POOL = 16` - Pool temperature sensor  
- `PIN_RELAY_POOL = 18` - Pool pump relay
- `PIN_RELAY_SOLAR = 19` - Solar pump relay

## Installation

1. Flash MicroPython firmware to your ESP32
2. Copy all files to the ESP32 filesystem
3. Edit `config.json` with your WiFi and MQTT settings
4. Reset the device

## Configuration

Edit the `config.json` file to configure:

- WiFi credentials
- MQTT broker settings
- Temperature thresholds
- Timer settings

## Operation Modes

### Auto Mode
- Pool pump runs based on timer
- Solar pump runs when conditions are optimal
- Automatic temperature-based control

### Manual Mode  
- No automatic control
- Pumps controlled via MQTT commands only

### Timer Mode
- Pool pump runs based on timer only
- Solar pump disabled

### Boost Mode
- Aggressive heating mode
- Solar pump runs whenever beneficial

## MQTT Interface

The controller uses Homie 4.0 convention for MQTT communication.

### Device Topics

Base topic: `homie/pool-controller`

### Control Topics

- `homie/pool-controller/pool-pump/switch/set` - Control pool pump (true/false)
- `homie/pool-controller/solar-pump/switch/set` - Control solar pump (true/false)
- `homie/pool-controller/operation-mode/mode/set` - Set operation mode (auto/manual/timer/boost)
- `homie/pool-controller/operation-mode/pool-max-temp/set` - Set max pool temperature
- `homie/pool-controller/operation-mode/solar-min-temp/set` - Set min solar temperature

### Status Topics

- `homie/pool-controller/pool-temp/temperature` - Pool temperature reading
- `homie/pool-controller/solar-temp/temperature` - Solar temperature reading
- `homie/pool-controller/pool-pump/switch` - Pool pump status
- `homie/pool-controller/solar-pump/switch` - Solar pump status

## File Structure

```
/
├── main.py              # Main application entry point
├── boot.py              # Boot configuration
├── config.json          # Configuration file
└── lib/
    ├── __init__.py      # Library package
    ├── config_manager.py # Configuration management
    ├── logger.py        # Logging utility
    ├── wifi_manager.py  # WiFi connection management
    ├── mqtt_client.py   # MQTT client wrapper
    ├── temperature_sensor.py # DS18B20 sensor handling
    ├── relay_module.py  # Relay control
    ├── operation_mode.py # Operation mode controller
    └── rules.py         # Control logic rules
```

## Dependencies

MicroPython libraries required:
- `umqtt.simple` - MQTT client
- `onewire` - OneWire protocol
- `ds18x20` - DS18B20 temperature sensor

## Troubleshooting

1. **WiFi Connection Issues**
   - Check SSID and password in config.json
   - Verify WiFi signal strength

2. **MQTT Connection Issues**  
   - Verify MQTT broker address and port
   - Check network connectivity

3. **Temperature Sensor Issues**
   - Verify OneWire connections
   - Check pull-up resistors (4.7kΩ recommended)
   - Ensure sensors are properly powered

4. **Relay Issues**
   - Check relay module power supply
   - Verify control signal connections
   - Test relay modules independently

## License

MIT License - see LICENSE file for details.