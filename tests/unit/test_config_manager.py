"""
Unit tests for ConfigManager
"""

import pytest
import json
import os
from unittest.mock import patch, mock_open, Mock

from config_manager import ConfigManager

class TestConfigManager:
    
    def test_init_with_defaults(self):
        """Test ConfigManager initialization with default values"""
        config_manager = ConfigManager()
        assert config_manager.config_file == 'config.json'
        assert isinstance(config_manager.default_config, dict)
        assert 'device_id' in config_manager.default_config
    
    @patch('config_manager.os.listdir')
    @patch('builtins.open', new_callable=mock_open, read_data='{"test": "value"}')
    def test_load_config_existing_file(self, mock_file, mock_listdir):
        """Test loading configuration from existing file"""
        mock_listdir.return_value = ['config.json']
        
        config_manager = ConfigManager()
        config_manager.load_config()
        
        assert config_manager.config == {"test": "value"}
        mock_file.assert_called_once_with('config.json', 'r')
    
    @patch('config_manager.os.listdir')
    def test_load_config_no_file(self, mock_listdir):
        """Test loading configuration when no file exists"""
        mock_listdir.return_value = []
        
        with patch.object(ConfigManager, 'save_config') as mock_save:
            config_manager = ConfigManager()
            config_manager.load_config()
            
            assert config_manager.config == config_manager.default_config
            mock_save.assert_called_once()
    
    @patch('builtins.open', new_callable=mock_open)
    def test_save_config(self, mock_file):
        """Test saving configuration to file"""
        config_manager = ConfigManager()
        config_manager.config = {"test": "value"}
        config_manager.save_config()
        
        mock_file.assert_called_once_with('config.json', 'w')
        handle = mock_file()
        written_data = ''.join(call.args[0] for call in handle.write.call_args_list)
        assert '"test": "value"' in written_data
    
    def test_get_setting(self, sample_config):
        """Test getting specific setting values"""
        config_manager = ConfigManager()
        config_manager.config = sample_config
        
        assert config_manager.get_setting('loop_interval') == 30
        assert config_manager.get_setting('nonexistent', 'default') == 'default'
    
    def test_set_setting(self, sample_config):
        """Test setting specific setting values"""
        with patch.object(ConfigManager, 'save_config') as mock_save:
            config_manager = ConfigManager()
            config_manager.config = sample_config.copy()
            
            config_manager.set_setting('loop_interval', 60)
            
            assert config_manager.config['settings']['loop_interval'] == 60
            mock_save.assert_called_once()
    
    def test_get_wifi_config(self, sample_config):
        """Test getting WiFi configuration"""
        config_manager = ConfigManager()
        config_manager.config = sample_config
        
        wifi_config = config_manager.get_wifi_config()
        assert wifi_config['ssid'] == 'test-wifi'
        assert wifi_config['password'] == 'test-password'
    
    def test_get_mqtt_config(self, sample_config):
        """Test getting MQTT configuration"""
        config_manager = ConfigManager()
        config_manager.config = sample_config
        
        mqtt_config = config_manager.get_mqtt_config()
        assert mqtt_config['host'] == 'test-broker'
        assert mqtt_config['port'] == 1883