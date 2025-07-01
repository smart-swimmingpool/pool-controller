"""
Smart Swimming Pool - Pool Controller
MicroPython Version

Main entry point for the pool controller.
"""

import gc
import time

from machine import reset

from lib.config_manager import ConfigManager
from lib.logger import Logger
from lib.mqtt_client import MQTTClient
from lib.operation_mode import OperationModeNode
from lib.relay_module import RelayModuleNode
from lib.temperature_sensor import DallasTemperatureNode
from lib.wifi_manager import WiFiManager

# Pin definitions for ESP32
PIN_DS_SOLAR = 15  # Temperature sensor for solar
PIN_DS_POOL = 16  # Temperature sensor for pool
PIN_RELAY_POOL = 18  # Pool pump relay
PIN_RELAY_SOLAR = 19  # Solar pump relay

# Global instances
logger = Logger()
config_manager = ConfigManager()
wifi_manager = WiFiManager()
mqtt_client = None

# Sensor and control nodes
solar_temp_node = None
pool_temp_node = None
pool_pump_node = None
solar_pump_node = None
operation_mode_node = None


def setup_hardware() -> None:
    """Initialize hardware components"""
    global solar_temp_node, pool_temp_node, pool_pump_node, solar_pump_node, operation_mode_node

    logger.info("Setting up hardware components...")

    # Initialize temperature sensors
    solar_temp_node = DallasTemperatureNode("solar-temp", "Solar Temperature", PIN_DS_SOLAR)
    pool_temp_node = DallasTemperatureNode("pool-temp", "Pool Temperature", PIN_DS_POOL)

    # Initialize relay modules
    pool_pump_node = RelayModuleNode("pool-pump", "Pool Pump", PIN_RELAY_POOL)
    solar_pump_node = RelayModuleNode("solar-pump", "Solar Pump", PIN_RELAY_SOLAR)

    # Initialize operation mode controller
    operation_mode_node = OperationModeNode("operation-mode", "Operation Mode")
    operation_mode_node.set_pool_temperature_node(pool_temp_node)
    operation_mode_node.set_solar_temperature_node(solar_temp_node)
    operation_mode_node.set_pool_pump_node(pool_pump_node)
    operation_mode_node.set_solar_pump_node(solar_pump_node)

    logger.info("Hardware setup complete")


def setup_mqtt() -> bool:
    """Initialize MQTT client and connect"""
    global mqtt_client

    config = config_manager.get_config()
    mqtt_config = config.get("mqtt", {})

    if not mqtt_config.get("host"):
        logger.error("MQTT host not configured")
        return False

    device_id = config.get("device_id", "pool-controller")

    mqtt_client = MQTTClient(
        client_id=device_id,
        server=mqtt_config["host"],
        port=mqtt_config.get("port", 1883),
        user=mqtt_config.get("user"),
        password=mqtt_config.get("password"),
    )

    # Set up MQTT callbacks
    mqtt_client.set_callback(on_mqtt_message)

    try:
        mqtt_client.connect()
        logger.info(f"Connected to MQTT broker: {mqtt_config['host']}")

        # Subscribe to control topics
        subscribe_topics()

        return True
    except Exception as e:
        logger.error(f"Failed to connect to MQTT: {e}")
        return False


def subscribe_topics() -> None:
    """Subscribe to MQTT control topics"""
    device_id = config_manager.get_config().get("device_id", "pool-controller")
    base_topic = f"homie/{device_id}"

    # Subscribe to settable properties
    topics = [
        f"{base_topic}/pool-pump/switch/set",
        f"{base_topic}/solar-pump/switch/set",
        f"{base_topic}/operation-mode/mode/set",
        f"{base_topic}/operation-mode/pool-max-temp/set",
        f"{base_topic}/operation-mode/solar-min-temp/set",
        f"{base_topic}/operation-mode/hysteresis/set",
    ]

    for topic in topics:
        mqtt_client.subscribe(topic)
        logger.debug(f"Subscribed to: {topic}")


def on_mqtt_message(topic: bytes, msg: bytes) -> None:
    """Handle incoming MQTT messages"""
    try:
        topic_str = topic.decode("utf-8")
        msg_str = msg.decode("utf-8")

        logger.debug(f"MQTT message: {topic_str} = {msg_str}")

        # Parse topic to determine target node and property
        parts = topic_str.split("/")
        if len(parts) >= 4 and parts[-1] == "set":
            # device_id = parts[1]
            node_id = parts[2]
            property_name = parts[3]

            # Route message to appropriate node
            if node_id == "pool-pump" and property_name == "switch":
                pool_pump_node.handle_mqtt_message(property_name, msg_str)
            elif node_id == "solar-pump" and property_name == "switch":
                solar_pump_node.handle_mqtt_message(property_name, msg_str)
            elif node_id == "operation-mode":
                operation_mode_node.handle_mqtt_message(property_name, msg_str)

    except Exception as e:
        logger.error(f"Error handling MQTT message: {e}")


def publish_device_info() -> None:
    """Publish Homie device information"""
    device_id = config_manager.get_config().get("device_id", "pool-controller")
    base_topic = f"homie/{device_id}"

    # Device attributes
    mqtt_client.publish(f"{base_topic}/$homie", "4.0.0", retain=True)
    mqtt_client.publish(f"{base_topic}/$name", "Pool Controller", retain=True)
    mqtt_client.publish(f"{base_topic}/$state", "ready", retain=True)
    mqtt_client.publish(
        f"{base_topic}/$nodes",
        "pool-temp,solar-temp,pool-pump,solar-pump,operation-mode",
        retain=True,
    )

    # Node information
    nodes = [
        ("pool-temp", "Pool Temperature", "temperature"),
        ("solar-temp", "Solar Temperature", "temperature"),
        ("pool-pump", "Pool Pump", "switch"),
        ("solar-pump", "Solar Pump", "switch"),
        ("operation-mode", "Operation Mode", "controller"),
    ]

    for node_id, name, node_type in nodes:
        node_topic = f"{base_topic}/{node_id}"
        mqtt_client.publish(f"{node_topic}/$name", name, retain=True)
        mqtt_client.publish(f"{node_topic}/$type", node_type, retain=True)


def main_loop() -> None:
    """Main application loop"""
    last_update = 0
    update_interval = 30000  # 30 seconds

    logger.info("Starting main loop...")

    while True:
        try:
            current_time = time.ticks_ms()

            # Check MQTT connection
            if mqtt_client:
                mqtt_client.check_msg()

            # Update sensors and controllers periodically
            if time.ticks_diff(current_time, last_update) >= update_interval:
                # Update temperature sensors
                solar_temp_node.update()
                pool_temp_node.update()

                # Update operation mode (applies rules)
                operation_mode_node.update()

                # Publish status to MQTT
                if mqtt_client:
                    publish_status()

                last_update = current_time

                # Garbage collection
                gc.collect()

            time.sleep_ms(100)  # Small delay to prevent busy waiting

        except KeyboardInterrupt:
            logger.info("Shutting down...")
            break
        except Exception as e:
            logger.error(f"Error in main loop: {e}")
            time.sleep(5)  # Wait before retrying


def publish_status() -> None:
    """Publish current status to MQTT"""
    device_id = config_manager.get_config().get("device_id", "pool-controller")
    base_topic = f"homie/{device_id}"

    try:
        # Temperature readings
        solar_temp = solar_temp_node.get_temperature()
        pool_temp = pool_temp_node.get_temperature()

        if solar_temp is not None:
            mqtt_client.publish(f"{base_topic}/solar-temp/temperature", str(solar_temp))

        if pool_temp is not None:
            mqtt_client.publish(f"{base_topic}/pool-temp/temperature", str(pool_temp))

        # Relay states
        mqtt_client.publish(
            f"{base_topic}/pool-pump/switch", "true" if pool_pump_node.get_state() else "false"
        )
        mqtt_client.publish(
            f"{base_topic}/solar-pump/switch", "true" if solar_pump_node.get_state() else "false"
        )

        # Operation mode status
        operation_mode_node.publish_status(mqtt_client, base_topic)

    except Exception as e:
        logger.error(f"Error publishing status: {e}")


def main() -> None:
    """Main entry point"""
    logger.info("=== Pool Controller Starting ===")
    logger.info("MicroPython Version")

    try:
        # Load configuration
        config_manager.load_config()

        # Setup hardware
        setup_hardware()

        # Connect to WiFi
        if not wifi_manager.connect():
            logger.error("Failed to connect to WiFi")
            return

        # Setup MQTT
        if not setup_mqtt():
            logger.error("Failed to setup MQTT")
            return

        # Publish device information
        publish_device_info()

        # Start main loop
        main_loop()

    except Exception as e:
        logger.error(f"Fatal error: {e}")
        logger.info("Restarting in 10 seconds...")
        time.sleep(10)
        reset()


if __name__ == "__main__":
    main()
