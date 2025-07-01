"""
Unit tests for RelayModuleNode
"""

import pytest
from unittest.mock import Mock, patch

from relay_module import RelayModuleNode

class TestRelayModuleNode:
    
    @patch('relay_module.Pin')
    def test_init(self, mock_pin):
        """Test RelayModuleNode initialization"""
        mock_pin_instance = Mock()
        mock_pin.return_value = mock_pin_instance
        
        relay = RelayModuleNode("test-relay", "Test Relay", 18)
        
        assert relay.node_id == "test-relay"
        assert relay.name == "Test Relay"
        assert relay.pin_num == 18
        mock_pin.assert_called_once_with(18, mock_pin.OUT)
        mock_pin_instance.value.assert_called_with(1)  # Should start OFF (active low)
    
    @patch('relay_module.Pin')
    def test_set_state_on(self, mock_pin):
        """Test setting relay state to ON"""
        mock_pin_instance = Mock()
        mock_pin.return_value = mock_pin_instance
        
        relay = RelayModuleNode("test-relay", "Test Relay", 18)
        relay.set_state(True)
        
        assert relay.get_state() is True
        # Should call value(0) for ON (active low)
        mock_pin_instance.value.assert_called_with(0)
    
    @patch('relay_module.Pin')
    def test_set_state_off(self, mock_pin):
        """Test setting relay state to OFF"""
        mock_pin_instance = Mock()
        mock_pin.return_value = mock_pin_instance
        
        relay = RelayModuleNode("test-relay", "Test Relay", 18)
        relay.set_state(False)
        
        assert relay.get_state() is False
        # Should call value(1) for OFF (active low)
        mock_pin_instance.value.assert_called_with(1)
    
    @patch('relay_module.Pin')
    def test_toggle(self, mock_pin):
        """Test toggling relay state"""
        mock_pin_instance = Mock()
        mock_pin.return_value = mock_pin_instance
        
        relay = RelayModuleNode("test-relay", "Test Relay", 18)
        initial_state = relay.get_state()
        relay.toggle()
        
        assert relay.get_state() != initial_state
    
    @patch('relay_module.Pin')
    def test_handle_mqtt_message_valid(self, mock_pin):
        """Test handling valid MQTT messages"""
        mock_pin_instance = Mock()
        mock_pin.return_value = mock_pin_instance
        
        relay = RelayModuleNode("test-relay", "Test Relay", 18)
        
        # Test valid ON values
        assert relay.handle_mqtt_message("switch", "true") is True
        assert relay.get_state() is True
        
        assert relay.handle_mqtt_message("switch", "on") is True
        assert relay.get_state() is True
        
        assert relay.handle_mqtt_message("switch", "1") is True
        assert relay.get_state() is True
        
        # Test valid OFF values
        assert relay.handle_mqtt_message("switch", "false") is True
        assert relay.get_state() is False
        
        assert relay.handle_mqtt_message("switch", "off") is True
        assert relay.get_state() is False
        
        assert relay.handle_mqtt_message("switch", "0") is True
        assert relay.get_state() is False
    
    @patch('relay_module.Pin')
    def test_handle_mqtt_message_invalid(self, mock_pin):
        """Test handling invalid MQTT messages"""
        mock_pin_instance = Mock()
        mock_pin.return_value = mock_pin_instance
        
        relay = RelayModuleNode("test-relay", "Test Relay", 18)
        
        # Test invalid property
        assert relay.handle_mqtt_message("invalid", "true") is False
        
        # Test invalid value
        assert relay.handle_mqtt_message("switch", "invalid") is False