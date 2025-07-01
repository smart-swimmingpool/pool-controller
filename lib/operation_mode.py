"""
Operation Mode Controller for Pool Controller
Implements different operational rules and modes.
"""

import time

from .logger import Logger
from .rules import RuleAuto, RuleBoost, RuleManual, RuleTimer


class OperationModeNode:
    def __init__(self, node_id, name, measurement_interval=30):
        self.node_id = node_id
        self.name = name
        self.measurement_interval = measurement_interval
        self.last_measurement = 0
        self.logger = Logger()

        # Operation modes
        self.mode = "auto"
        self.available_modes = ["auto", "manual", "timer", "boost"]

        # Temperature settings
        self.pool_max_temp = 28.5
        self.solar_min_temp = 55.0
        self.temperature_hysteresis = 1.0

        # Timer settings
        self.timer_start_hour = 10
        self.timer_start_minutes = 30
        self.timer_end_hour = 17
        self.timer_end_minutes = 30

        # Node references
        self.pool_temp_node = None
        self.solar_temp_node = None
        self.pool_pump_node = None
        self.solar_pump_node = None

        # Rules
        self.rules = {
            "auto": RuleAuto(),
            "manual": RuleManual(),
            "timer": RuleTimer(),
            "boost": RuleBoost(),
        }

        self.logger.info(f"Operation mode controller initialized: {name}")

    def set_pool_temperature_node(self, node):
        """Set pool temperature sensor node"""
        self.pool_temp_node = node

    def set_solar_temperature_node(self, node):
        """Set solar temperature sensor node"""
        self.solar_temp_node = node

    def set_pool_pump_node(self, node):
        """Set pool pump relay node"""
        self.pool_pump_node = node

    def set_solar_pump_node(self, node):
        """Set solar pump relay node"""
        self.solar_pump_node = node

    def set_mode(self, mode):
        """Set operation mode"""
        if mode in self.available_modes:
            self.mode = mode
            self.logger.info(f"Operation mode set to: {mode}")
            return True
        else:
            self.logger.warning(f"Invalid operation mode: {mode}")
            return False

    def get_mode(self):
        """Get current operation mode"""
        return self.mode

    def update(self):
        """Update operation mode logic"""
        current_time = time.ticks_ms()

        if time.ticks_diff(current_time, self.last_measurement) >= (
            self.measurement_interval * 1000
        ):
            self.last_measurement = current_time

            # Get current temperatures
            pool_temp = self.pool_temp_node.get_temperature() if self.pool_temp_node else None
            solar_temp = self.solar_temp_node.get_temperature() if self.solar_temp_node else None

            # Apply current rule
            if self.mode in self.rules:
                rule = self.rules[self.mode]
                rule.apply(
                    pool_temp=pool_temp,
                    solar_temp=solar_temp,
                    pool_max_temp=self.pool_max_temp,
                    solar_min_temp=self.solar_min_temp,
                    hysteresis=self.temperature_hysteresis,
                    timer_start=(self.timer_start_hour, self.timer_start_minutes),
                    timer_end=(self.timer_end_hour, self.timer_end_minutes),
                    pool_pump=self.pool_pump_node,
                    solar_pump=self.solar_pump_node,
                )

    def handle_mqtt_message(self, property_name, value):
        """Handle MQTT control message"""
        try:
            if property_name == "mode":
                return self.set_mode(value)
            elif property_name == "pool-max-temp":
                self.pool_max_temp = float(value)
                self.logger.info(f"Pool max temperature set to: {self.pool_max_temp}°C")
                return True
            elif property_name == "solar-min-temp":
                self.solar_min_temp = float(value)
                self.logger.info(f"Solar min temperature set to: {self.solar_min_temp}°C")
                return True
            elif property_name == "hysteresis":
                self.temperature_hysteresis = float(value)
                self.logger.info(f"Temperature hysteresis set to: {self.temperature_hysteresis}K")
                return True
            elif property_name == "timer-start-hour":
                self.timer_start_hour = int(value)
                self.logger.info(f"Timer start hour set to: {self.timer_start_hour}")
                return True
            elif property_name == "timer-start-minutes":
                self.timer_start_minutes = int(value)
                self.logger.info(f"Timer start minutes set to: {self.timer_start_minutes}")
                return True
            elif property_name == "timer-end-hour":
                self.timer_end_hour = int(value)
                self.logger.info(f"Timer end hour set to: {self.timer_end_hour}")
                return True
            elif property_name == "timer-end-minutes":
                self.timer_end_minutes = int(value)
                self.logger.info(f"Timer end minutes set to: {self.timer_end_minutes}")
                return True
        except ValueError as e:
            self.logger.error(f"Invalid value for {property_name}: {value}")
            return False

        return False

    def publish_status(self, mqtt_client, base_topic):
        """Publish current status to MQTT"""
        node_topic = f"{base_topic}/{self.node_id}"

        mqtt_client.publish(f"{node_topic}/mode", self.mode)
        mqtt_client.publish(f"{node_topic}/pool-max-temp", str(self.pool_max_temp))
        mqtt_client.publish(f"{node_topic}/solar-min-temp", str(self.solar_min_temp))
        mqtt_client.publish(f"{node_topic}/hysteresis", str(self.temperature_hysteresis))
        mqtt_client.publish(f"{node_topic}/timer-start-hour", str(self.timer_start_hour))
        mqtt_client.publish(f"{node_topic}/timer-start-minutes", str(self.timer_start_minutes))
        mqtt_client.publish(f"{node_topic}/timer-end-hour", str(self.timer_end_hour))
        mqtt_client.publish(f"{node_topic}/timer-end-minutes", str(self.timer_end_minutes))

    def get_node_id(self):
        """Get node ID"""
        return self.node_id

    def get_name(self):
        """Get node name"""
        return self.name
