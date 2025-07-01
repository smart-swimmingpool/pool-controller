"""
Relay Module Node for Pool Controller
Controls relay switches for pumps.
"""

import time
from machine import Pin
from .logger import Logger

class RelayModuleNode:
    def __init__(self, node_id, name, pin, measurement_interval=30):
        self.node_id = node_id
        self.name = name
        self.pin_num = pin
        self.measurement_interval = measurement_interval
        self.last_measurement = 0
        self.logger = Logger()
        
        # Initialize relay pin (active low for most relay modules)
        self.relay_pin = Pin(pin, Pin.OUT)
        self.state = False
        self.set_state(False)  # Start with relay off
        
        self.logger.info(f"Relay module '{name}' initialized on pin {pin}")
    
    def set_state(self, state):
        """Set relay state"""
        self.state = bool(state)
        # Most relay modules are active low
        self.relay_pin.value(0 if self.state else 1)
        self.logger.info(f"{self.name}: {'ON' if self.state else 'OFF'}")
    
    def get_state(self):
        """Get current relay state"""
        return self.state
    
    def toggle(self):
        """Toggle relay state"""
        self.set_state(not self.state)
    
    def handle_mqtt_message(self, property_name, value):
        """Handle MQTT control message"""
        if property_name == "switch":
            if value.lower() in ["true", "on", "1"]:
                self.set_state(True)
                return True
            elif value.lower() in ["false", "off", "0"]:
                self.set_state(False)
                return True
            else:
                self.logger.warning(f"Invalid switch value: {value}")
                return False
        return False
    
    def get_node_id(self):
        """Get node ID"""
        return self.node_id
    
    def get_name(self):
        """Get node name"""
        return self.name