"""
Unit tests for DallasTemperatureNode
"""

import pytest
from unittest.mock import Mock, patch, MagicMock
import time

from temperature_sensor import DallasTemperatureNode

class TestDallasTemperatureNode:
    
    @patch('temperature_sensor.Pin')
    @patch('temperature_sensor.OneWire')
    @patch('temperature_sensor.DS18X20')
    def test_init(self, mock_ds18x20, mock_onewire, mock_pin):
        """Test DallasTemperatureNode initialization"""
        mock_ds = Mock()
        mock_ds.scan.return_value = [b'\x28\x01\x02\x03\x04\x05\x06\x07']
        mock_ds18x20.return_value = mock_ds
        
        sensor = DallasTemperatureNode("test-sensor", "Test Sensor", 15)
        
        assert sensor.node_id == "test-sensor"
        assert sensor.name == "Test Sensor"
        assert sensor.pin == 15
        assert sensor.measurement_interval == 30
        mock_pin.assert_called_once_with(15)
        mock_ds.scan.assert_called_once()
    
    @patch('temperature_sensor.Pin')
    @patch('temperature_sensor.OneWire')
    @patch('temperature_sensor.DS18X20')
    @patch('temperature_sensor.time.ticks_ms')
    @patch('temperature_sensor.time.ticks_diff')
    @patch('temperature_sensor.time.sleep_ms')
    def test_update_temperature(self, mock_sleep, mock_ticks_diff, mock_ticks_ms, 
                               mock_ds18x20, mock_onewire, mock_pin):
        """Test temperature update functionality"""
        mock_ds = Mock()
        mock_ds.scan.return_value = [b'\x28\x01\x02\x03\x04\x05\x06\x07']
        mock_ds.convert_temp = Mock()
        mock_ds.read_temp.return_value = 25.5
        mock_ds18x20.return_value = mock_ds
        
        mock_ticks_ms.return_value = 30000
        mock_ticks_diff.return_value = 30000  # Simulate 30 seconds passed
        
        sensor = DallasTemperatureNode("test-sensor", "Test Sensor", 15)
        sensor.update()
        
        mock_ds.convert_temp.assert_called_once()
        mock_ds.read_temp.assert_called_once()
        assert sensor.get_temperature() == 25.5
    
    @patch('temperature_sensor.Pin')
    @patch('temperature_sensor.OneWire')
    @patch('temperature_sensor.DS18X20')
    def test_no_devices_found(self, mock_ds18x20, mock_onewire, mock_pin):
        """Test behavior when no temperature sensors are found"""
        mock_ds = Mock()
        mock_ds.scan.return_value = []
        mock_ds18x20.return_value = mock_ds
        
        sensor = DallasTemperatureNode("test-sensor", "Test Sensor", 15)
        
        assert len(sensor.devices) == 0
        assert sensor.get_temperature() is None
    
    @patch('temperature_sensor.Pin')
    @patch('temperature_sensor.OneWire')
    @patch('temperature_sensor.DS18X20')
    @patch('temperature_sensor.time.ticks_ms')
    @patch('temperature_sensor.time.ticks_diff')
    def test_update_not_due(self, mock_ticks_diff, mock_ticks_ms, 
                           mock_ds18x20, mock_onewire, mock_pin):
        """Test that update doesn't run when not due"""
        mock_ds = Mock()
        mock_ds.scan.return_value = [b'\x28\x01\x02\x03\x04\x05\x06\x07']
        mock_ds18x20.return_value = mock_ds
        
        mock_ticks_ms.return_value = 15000
        mock_ticks_diff.return_value = 15000  # Only 15 seconds passed
        
        sensor = DallasTemperatureNode("test-sensor", "Test Sensor", 15, measurement_interval=30)
        sensor.last_measurement = 1000  # Set a previous measurement time
        sensor.update()
        
        # Should not call convert_temp since interval hasn't passed
        mock_ds.convert_temp.assert_not_called()