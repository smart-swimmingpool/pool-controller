"""
Dallas Temperature Sensor Node for Pool Controller
Handles DS18B20 temperature sensors.
"""

import time
from machine import Pin
from onewire import OneWire
from ds18x20 import DS18X20
from .logger import Logger

class DallasTemperatureNode:
    def __init__(self, node_id, name, pin, measurement_interval=30):
        self.node_id = node_id
        self.name = name
        self.pin = pin
        self.measurement_interval = measurement_interval
        self.last_measurement = 0
        self.temperature = None
        self.logger = Logger()
        
        # Initialize OneWire and DS18X20
        self.ow_pin = Pin(pin)
        self.ow = OneWire(self.ow_pin)
        self.ds = DS18X20(self.ow)
        
        # Scan for devices
        self.devices = self.ds.scan()
        self.logger.info(f"Found {len(self.devices)} temperature sensor(s) on pin {pin}")
        
        if not self.devices:
            self.logger.warning(f"No temperature sensors found on pin {pin}")
    
    def update(self):
        """Update temperature reading"""
        current_time = time.ticks_ms()
        
        if time.ticks_diff(current_time, self.last_measurement) >= (self.measurement_interval * 1000):
            self.last_measurement = current_time
            
            if self.devices:
                try:
                    # Start temperature conversion
                    self.ds.convert_temp()
                    time.sleep_ms(750)  # Wait for conversion
                    
                    # Read temperature from first device
                    temp = self.ds.read_temp(self.devices[0])
                    
                    if temp is not None:
                        self.temperature = round(temp, 2)
                        self.logger.debug(f"{self.name}: {self.temperature}°C")
                    else:
                        self.logger.warning(f"Failed to read temperature from {self.name}")
                        
                except Exception as e:
                    self.logger.error(f"Error reading temperature from {self.name}: {e}")
    
    def get_temperature(self):
        """Get current temperature reading"""
        return self.temperature
    
    def get_node_id(self):
        """Get node ID"""
        return self.node_id
    
    def get_name(self):
        """Get node name"""
        return self.name