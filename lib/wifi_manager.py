"""
WiFi Manager for Pool Controller
Handles WiFi connection and management.
"""

import time

import network

from .config_manager import ConfigManager
from .logger import Logger


class WiFiManager:
    def __init__(self):
        self.wlan = network.WLAN(network.STA_IF)
        self.logger = Logger()
        self.config_manager = ConfigManager()

    def connect(self, timeout=30):
        """Connect to WiFi network"""
        wifi_config = self.config_manager.get_wifi_config()
        ssid = wifi_config.get("ssid")
        password = wifi_config.get("password")

        if not ssid:
            self.logger.error("WiFi SSID not configured")
            return False

        self.logger.info(f"Connecting to WiFi: {ssid}")

        self.wlan.active(True)

        if self.wlan.isconnected():
            self.logger.info("Already connected to WiFi")
            return True

        self.wlan.connect(ssid, password)

        # Wait for connection
        start_time = time.time()
        while not self.wlan.isconnected():
            if time.time() - start_time > timeout:
                self.logger.error("WiFi connection timeout")
                return False
            time.sleep(1)

        ip_info = self.wlan.ifconfig()
        self.logger.info(f"Connected to WiFi. IP: {ip_info[0]}")
        return True

    def disconnect(self):
        """Disconnect from WiFi"""
        if self.wlan.isconnected():
            self.wlan.disconnect()
            self.logger.info("Disconnected from WiFi")

    def is_connected(self):
        """Check if connected to WiFi"""
        return self.wlan.isconnected()

    def get_ip(self):
        """Get current IP address"""
        if self.wlan.isconnected():
            return self.wlan.ifconfig()[0]
        return None

    def scan_networks(self):
        """Scan for available WiFi networks"""
        self.wlan.active(True)
        networks = self.wlan.scan()
        return [(net[0].decode("utf-8"), net[3]) for net in networks]
