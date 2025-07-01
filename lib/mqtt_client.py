"""
MQTT Client wrapper for Pool Controller
Provides simplified MQTT functionality.
"""

import time
from umqtt.simple import MQTTClient as UMQTTClient
from .logger import Logger

class MQTTClient:
    def __init__(self, client_id, server, port=1883, user=None, password=None):
        self.client_id = client_id
        self.server = server
        self.port = port
        self.user = user
        self.password = password
        self.client = None
        self.logger = Logger()
        self.connected = False
        self.callback = None
    
    def set_callback(self, callback):
        """Set message callback function"""
        self.callback = callback
    
    def connect(self):
        """Connect to MQTT broker"""
        try:
            self.client = UMQTTClient(
                self.client_id,
                self.server,
                port=self.port,
                user=self.user,
                password=self.password
            )
            
            if self.callback:
                self.client.set_callback(self.callback)
            
            self.client.connect()
            self.connected = True
            self.logger.info(f"Connected to MQTT broker: {self.server}:{self.port}")
            return True
            
        except Exception as e:
            self.logger.error(f"MQTT connection failed: {e}")
            self.connected = False
            return False
    
    def disconnect(self):
        """Disconnect from MQTT broker"""
        if self.client and self.connected:
            try:
                self.client.disconnect()
                self.connected = False
                self.logger.info("Disconnected from MQTT broker")
            except Exception as e:
                self.logger.error(f"Error disconnecting from MQTT: {e}")
    
    def publish(self, topic, message, retain=False):
        """Publish message to topic"""
        if not self.connected or not self.client:
            return False
        
        try:
            self.client.publish(topic, message, retain=retain)
            self.logger.debug(f"Published: {topic} = {message}")
            return True
        except Exception as e:
            self.logger.error(f"Error publishing to {topic}: {e}")
            return False
    
    def subscribe(self, topic):
        """Subscribe to topic"""
        if not self.connected or not self.client:
            return False
        
        try:
            self.client.subscribe(topic)
            self.logger.debug(f"Subscribed to: {topic}")
            return True
        except Exception as e:
            self.logger.error(f"Error subscribing to {topic}: {e}")
            return False
    
    def check_msg(self):
        """Check for incoming messages"""
        if self.connected and self.client:
            try:
                self.client.check_msg()
            except Exception as e:
                self.logger.error(f"Error checking messages: {e}")
                self.connected = False
    
    def is_connected(self):
        """Check if connected to MQTT broker"""
        return self.connected