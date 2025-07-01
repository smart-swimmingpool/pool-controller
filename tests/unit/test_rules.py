"""
Unit tests for operation rules
"""

import pytest
from unittest.mock import Mock, patch
import time

from rules import RuleAuto, RuleManual, RuleTimer, RuleBoost

class TestBaseRule:
    
    @patch('rules.time.localtime')
    def test_is_timer_active_within_range(self, mock_localtime):
        """Test timer active check within time range"""
        # Mock current time: 12:00
        mock_localtime.return_value = (2023, 1, 1, 12, 0, 0, 0, 0, 0)
        
        rule = RuleAuto()
        
        # Timer from 10:00 to 14:00
        assert rule.is_timer_active((10, 0), (14, 0)) is True
        
        # Timer from 13:00 to 16:00
        assert rule.is_timer_active((13, 0), (16, 0)) is False
    
    @patch('rules.time.localtime')
    def test_is_timer_active_across_midnight(self, mock_localtime):
        """Test timer active check across midnight"""
        # Mock current time: 23:00
        mock_localtime.return_value = (2023, 1, 1, 23, 0, 0, 0, 0, 0)
        
        rule = RuleAuto()
        
        # Timer from 22:00 to 02:00 (next day)
        assert rule.is_timer_active((22, 0), (2, 0)) is True

class TestRuleAuto:
    
    def test_apply_timer_active_pool_pump(self):
        """Test auto rule with timer active"""
        rule = RuleAuto()
        pool_pump = Mock()
        solar_pump = Mock()
        
        with patch.object(rule, 'is_timer_active', return_value=True):
            rule.apply(
                pool_temp=25.0,
                solar_temp=60.0,
                pool_max_temp=28.0,
                solar_min_temp=55.0,
                hysteresis=1.0,
                timer_start=(10, 0),
                timer_end=(16, 0),
                pool_pump=pool_pump,
                solar_pump=solar_pump
            )
        
        pool_pump.set_state.assert_called_with(True)
    
    def test_apply_timer_inactive_pool_pump(self):
        """Test auto rule with timer inactive"""
        rule = RuleAuto()
        pool_pump = Mock()
        solar_pump = Mock()
        
        with patch.object(rule, 'is_timer_active', return_value=False):
            rule.apply(
                pool_temp=25.0,
                solar_temp=60.0,
                pool_max_temp=28.0,
                solar_min_temp=55.0,
                hysteresis=1.0,
                timer_start=(10, 0),
                timer_end=(16, 0),
                pool_pump=pool_pump,
                solar_pump=solar_pump
            )
        
        pool_pump.set_state.assert_called_with(False)
    
    def test_apply_solar_pump_conditions_met(self):
        """Test auto rule solar pump activation when conditions are met"""
        rule = RuleAuto()
        pool_pump = Mock()
        pool_pump.get_state.return_value = True  # Pool pump is on
        solar_pump = Mock()
        solar_pump.get_state.return_value = False  # Solar pump is off
        
        with patch.object(rule, 'is_timer_active', return_value=True):
            rule.apply(
                pool_temp=25.0,  # Below max
                solar_temp=60.0,  # Above min and above pool temp
                pool_max_temp=28.0,
                solar_min_temp=55.0,
                hysteresis=1.0,
                timer_start=(10, 0),
                timer_end=(16, 0),
                pool_pump=pool_pump,
                solar_pump=solar_pump
            )
        
        solar_pump.set_state.assert_called_with(True)
    
    def test_apply_solar_pump_pool_too_hot(self):
        """Test auto rule solar pump deactivation when pool is too hot"""
        rule = RuleAuto()
        pool_pump = Mock()
        pool_pump.get_state.return_value = True  # Pool pump is on
        solar_pump = Mock()
        solar_pump.get_state.return_value = True  # Solar pump is on
        
        with patch.object(rule, 'is_timer_active', return_value=True):
            rule.apply(
                pool_temp=30.0,  # Above max + hysteresis
                solar_temp=60.0,
                pool_max_temp=28.0,
                solar_min_temp=55.0,
                hysteresis=1.0,
                timer_start=(10, 0),
                timer_end=(16, 0),
                pool_pump=pool_pump,
                solar_pump=solar_pump
            )
        
        solar_pump.set_state.assert_called_with(False)

class TestRuleManual:
    
    def test_apply_no_changes(self):
        """Test manual rule makes no automatic changes"""
        rule = RuleManual()
        pool_pump = Mock()
        solar_pump = Mock()
        
        rule.apply(
            pool_temp=25.0,
            solar_temp=60.0,
            pool_max_temp=28.0,
            solar_min_temp=55.0,
            hysteresis=1.0,
            timer_start=(10, 0),
            timer_end=(16, 0),
            pool_pump=pool_pump,
            solar_pump=solar_pump
        )
        
        # Should not call set_state on any pump
        pool_pump.set_state.assert_not_called()
        solar_pump.set_state.assert_not_called()

class TestRuleTimer:
    
    def test_apply_timer_mode(self):
        """Test timer rule behavior"""
        rule = RuleTimer()
        pool_pump = Mock()
        solar_pump = Mock()
        solar_pump.get_state.return_value = True  # Solar pump is on
        
        with patch.object(rule, 'is_timer_active', return_value=True):
            rule.apply(
                timer_start=(10, 0),
                timer_end=(16, 0),
                pool_pump=pool_pump,
                solar_pump=solar_pump
            )
        
        pool_pump.set_state.assert_called_with(True)
        solar_pump.set_state.assert_called_with(False)  # Solar should be turned off

class TestRuleBoost:
    
    def test_apply_boost_mode_activate_solar(self):
        """Test boost rule solar pump activation"""
        rule = RuleBoost()
        pool_pump = Mock()
        pool_pump.get_state.return_value = True  # Pool pump is on
        solar_pump = Mock()
        solar_pump.get_state.return_value = False  # Solar pump is off
        
        rule.apply(
            pool_temp=25.0,  # Below max
            solar_temp=30.0,  # Above pool temp
            pool_max_temp=28.0,
            solar_min_temp=55.0,
            hysteresis=1.0,
            pool_pump=pool_pump,
            solar_pump=solar_pump
        )
        
        solar_pump.set_state.assert_called_with(True)
    
    def test_apply_boost_mode_deactivate_solar(self):
        """Test boost rule solar pump deactivation"""
        rule = RuleBoost()
        pool_pump = Mock()
        pool_pump.get_state.return_value = True  # Pool pump is on
        solar_pump = Mock()
        solar_pump.get_state.return_value = True  # Solar pump is on
        
        rule.apply(
            pool_temp=30.0,  # Above max + hysteresis
            solar_temp=25.0,  # Below pool temp
            pool_max_temp=28.0,
            solar_min_temp=55.0,
            hysteresis=1.0,
            pool_pump=pool_pump,
            solar_pump=solar_pump
        )
        
        solar_pump.set_state.assert_called_with(False)