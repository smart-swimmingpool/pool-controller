"""
Pytest configuration and fixtures for Pool Controller tests
"""

import pytest
from unittest.mock import Mock, MagicMock
import sys
import os

# Add lib directory to path for testing
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'lib'))

@pytest.fixture
def mock_pin():
    """Mock machine.Pin for testing"""
    pin_mock = Mock()
    pin_mock.value = Mock()
    return pin_mock

@pytest.fixture
def mock_onewire():
    """Mock OneWire for testing"""
    ow_mock = Mock()
    return ow_mock

@pytest.fixture
def mock_ds18x20():
    """Mock DS18X20 for testing"""
    ds_mock = Mock()
    ds_mock.scan.return_value = [b'\x28\x01\x02\x03\x04\x05\x06\x07']
    ds_mock.convert_temp = Mock()
    ds_mock.read_temp.return_value = 25.5
    return ds_mock

@pytest.fixture
def mock_mqtt_client():
    """Mock MQTT client for testing"""
    mqtt_mock = Mock()
    mqtt_mock.connect.return_value = True
    mqtt_mock.publish.return_value = True
    mqtt_mock.subscribe.return_value = True
    mqtt_mock.is_connected.return_value = True
    return mqtt_mock

@pytest.fixture
def sample_config():
    """Sample configuration for testing"""
    return {
        "device_id": "test-controller",
        "name": "Test Pool Controller",
        "wifi": {
            "ssid": "test-wifi",
            "password": "test-password"
        },
        "mqtt": {
            "host": "test-broker",
            "port": 1883,
            "user": "test-user",
            "password": "test-pass"
        },
        "settings": {
            "loop_interval": 30,
            "temperature_max_pool": 28.5,
            "temperature_min_solar": 55.0,
            "temperature_hysteresis": 1.0,
            "timer_start_hour": 10,
            "timer_start_minutes": 30,
            "timer_end_hour": 17,
            "timer_end_minutes": 30
        }
    }