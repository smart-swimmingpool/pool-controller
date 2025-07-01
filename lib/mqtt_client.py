"""
MQTT Client wrapper for Pool Controller
Provides simplified MQTT functionality.
"""

from typing import Any, Optional

from umqtt.simple import MQTTClient as UMQTTClient

from .logger import Logger


class MQTTClient:
    def __init__(
        self,
        client_id: str,
        server: str,
        port: int = 1883,
        user: Optional[str] = None,
        password: Optional[str] = None,
    ) -> None:
        self.client_id = client_id
        self.server = server
        self.port = port
        self.user = user
        self.password = password
        self.client = None
        self.logger = Logger()
        self.connected = False
        self.callback = None

    def set_callback(self, callback: Any) -> None:
        """Set message callback function"""
        self.callback = callback

    def connect(self) -> bool:
        """Connect to MQTT broker"""
        try:
            client = UMQTTClient(
                self.client_id,
                self.server,
                port=self.port,
                user=self.user,
                password=self.password,
            )
            if self.callback is not None:
                client.set_callback(self.callback)
            client.connect()
            self.client = client
            self.connected = True
            self.logger.info(
                f"Connected to MQTT broker: {self.server}:{self.port}"
            )
            return True
        except OSError as e:
            self.logger.error(f"MQTT connection failed: {e}")
            self.connected = False
            self.client = None
            return False

    def disconnect(self) -> None:
        """Disconnect from MQTT broker"""
        if self.client is not None and self.connected:
            try:
                self.client.disconnect()
                self.connected = False
                self.logger.info("Disconnected from MQTT broker")
            except OSError as e:
                self.logger.error(f"Error disconnecting from MQTT: {e}")

    def publish(self, topic: str, message: str, retain: bool = False) -> bool:
        """Publish message to topic"""
        if not self.connected or self.client is None:
            return False

        try:
            self.client.publish(topic, message, retain=retain)
            self.logger.debug(f"Published: {topic} = {message}")
            return True
        except OSError as e:
            self.logger.error(f"Error publishing to {topic}: {e}")
            return False

    def subscribe(self, topic: str) -> bool:
        """Subscribe to topic"""
        if not self.connected or self.client is None:
            return False

        try:
            self.client.subscribe(topic)
            self.logger.debug(f"Subscribed to: {topic}")
            return True
        except OSError as e:
            self.logger.error(f"Error subscribing to {topic}: {e}")
            return False

    def check_msg(self) -> None:
        """Check for incoming messages"""
        if self.connected and self.client is not None:
            try:
                self.client.check_msg()
            except OSError as e:
                self.logger.error(f"Error checking messages: {e}")
                self.connected = False

    def is_connected(self) -> bool:
        """Check if connected to MQTT broker"""
        return self.connected
