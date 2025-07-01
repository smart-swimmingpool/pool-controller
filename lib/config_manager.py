"""
Configuration Manager for Pool Controller
Handles loading and saving of configuration settings.
"""

import json
import os

from .logger import Logger


class ConfigManager:
    def __init__(self, config_file="config.json"):
        self.config_file = config_file
        self.config = {}
        self.logger = Logger()

        # Default configuration
        self.default_config = {
            "device_id": "pool-controller",
            "name": "Pool Controller",
            "wifi": {"ssid": "", "password": ""},
            "mqtt": {"host": "", "port": 1883, "user": "", "password": ""},
            "settings": {
                "loop_interval": 30,
                "temperature_max_pool": 28.5,
                "temperature_min_solar": 55.0,
                "temperature_hysteresis": 1.0,
                "timer_start_hour": 10,
                "timer_start_minutes": 30,
                "timer_end_hour": 17,
                "timer_end_minutes": 30,
            },
        }

    def load_config(self):
        """Load configuration from file"""
        try:
            if self.config_file in os.listdir():
                with open(self.config_file, "r") as f:
                    self.config = json.load(f)
                self.logger.info(f"Configuration loaded from {self.config_file}")
            else:
                self.logger.info("No config file found, using defaults")
                self.config = self.default_config.copy()
                self.save_config()
        except Exception as e:
            self.logger.error(f"Error loading config: {e}")
            self.config = self.default_config.copy()

    def save_config(self):
        """Save configuration to file"""
        try:
            with open(self.config_file, "w") as f:
                json.dump(self.config, f, indent=2)
            self.logger.info(f"Configuration saved to {self.config_file}")
        except Exception as e:
            self.logger.error(f"Error saving config: {e}")

    def get_config(self):
        """Get the current configuration"""
        return self.config

    def get_setting(self, key, default=None):
        """Get a specific setting value"""
        return self.config.get("settings", {}).get(key, default)

    def set_setting(self, key, value):
        """Set a specific setting value"""
        if "settings" not in self.config:
            self.config["settings"] = {}
        self.config["settings"][key] = value
        self.save_config()

    def get_wifi_config(self):
        """Get WiFi configuration"""
        return self.config.get("wifi", {})

    def get_mqtt_config(self):
        """Get MQTT configuration"""
        return self.config.get("mqtt", {})
