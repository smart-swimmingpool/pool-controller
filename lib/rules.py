"""
Operation Rules for Pool Controller
Implements different control logic for various operation modes.
"""

import time
from typing import Any, Optional, Tuple

from .logger import Logger


class BaseRule:
    """Base class for operation rules"""

    def __init__(self) -> None:
        self.logger = Logger()

    def apply(self, **kwargs: Any) -> None:
        """Apply the rule logic - to be implemented by subclasses"""
        pass

    def is_timer_active(self, start_time: Tuple[int, int], end_time: Tuple[int, int]) -> bool:
        """Check if current time is within timer range"""
        try:
            # Get current time (simplified - assumes RTC is set)
            current_time = time.localtime()
            current_hour = current_time[3]
            current_minute = current_time[4]

            start_hour, start_minute = start_time
            end_hour, end_minute = end_time

            # Convert to minutes for easier comparison
            current_minutes = current_hour * 60 + current_minute
            start_minutes = start_hour * 60 + start_minute
            end_minutes = end_hour * 60 + end_minute

            # Handle case where timer spans midnight
            if start_minutes <= end_minutes:
                return start_minutes <= current_minutes <= end_minutes
            else:
                return current_minutes >= start_minutes or current_minutes <= end_minutes

        except Exception as e:
            self.logger.error(f"Error checking timer: {e}")
            return False


class RuleAuto(BaseRule):
    """Automatic rule - smart control based on temperatures and timer"""

    def apply(
        self,
        pool_temp: Optional[float] = None,
        solar_temp: Optional[float] = None,
        pool_max_temp: float = 0.0,
        solar_min_temp: float = 0.0,
        hysteresis: float = 0.0,
        timer_start: Tuple[int, int] = (0, 0),
        timer_end: Tuple[int, int] = (0, 0),
        pool_pump: Any = None,
        solar_pump: Any = None,
        **kwargs: Any,
    ) -> None:

        self.logger.debug("Applying AUTO rule")

        # Check if we're in timer period
        timer_active = self.is_timer_active(timer_start, timer_end)

        # Pool pump control based on timer
        if pool_pump:
            pool_pump.set_state(timer_active)

        # Solar pump control based on temperatures and pool pump state
        if solar_pump and pool_pump and pool_pump.get_state():
            solar_on = solar_pump.get_state()

            if pool_temp is None or solar_temp is None:
                # If we can't read temperatures, turn off solar pump for safety
                if solar_on:
                    solar_pump.set_state(False)
                    self.logger.warning("Temperature sensors not available, turning off solar pump")
                return

            if solar_on:
                # Solar pump is currently on - check if we should turn it off
                if (
                    solar_temp < (solar_min_temp - hysteresis)
                    or pool_temp >= (pool_max_temp + hysteresis)
                    or pool_temp >= (solar_temp + hysteresis)
                ):
                    solar_pump.set_state(False)
                    self.logger.info("Turning off solar pump")
            else:
                # Solar pump is currently off - check if we should turn it on
                if (
                    pool_temp < pool_max_temp
                    and pool_temp < solar_temp
                    and solar_temp >= solar_min_temp
                ):
                    solar_pump.set_state(True)
                    self.logger.info("Turning on solar pump")
        elif solar_pump:
            # Pool pump is off, make sure solar pump is also off
            if solar_pump.get_state():
                solar_pump.set_state(False)
                self.logger.info("Pool pump off, turning off solar pump")


class RuleManual(BaseRule):
    """Manual rule - no automatic control"""

    def apply(self, **kwargs: Any) -> None:
        self.logger.debug("Applying MANUAL rule - no automatic control")
        # In manual mode, don't change any pump states
        pass


class RuleTimer(BaseRule):
    """Timer rule - pool pump based on timer, solar pump off"""

    def apply(
        self,
        timer_start: Tuple[int, int] = (0, 0),
        timer_end: Tuple[int, int] = (0, 0),
        pool_pump: Any = None,
        solar_pump: Any = None,
        **kwargs: Any,
    ) -> None:
        self.logger.debug("Applying TIMER rule")

        # Pool pump control based on timer
        timer_active = self.is_timer_active(timer_start, timer_end)

        if pool_pump:
            pool_pump.set_state(timer_active)

        # Solar pump always off in timer mode
        if solar_pump and solar_pump.get_state():
            solar_pump.set_state(False)
            self.logger.info("Timer mode: turning off solar pump")


class RuleBoost(BaseRule):
    """Boost rule - maximum heating when pool pump is on"""

    def apply(
        self,
        pool_temp: Optional[float] = None,
        solar_temp: Optional[float] = None,
        pool_max_temp: float = 0.0,
        solar_min_temp: float = 0.0,
        hysteresis: float = 0.0,
        pool_pump: Any = None,
        solar_pump: Any = None,
        **kwargs: Any,
    ) -> None:

        self.logger.debug("Applying BOOST rule")

        # In boost mode, pool pump should be manually controlled
        # Solar pump runs when pool pump is on and conditions are met

        if solar_pump and pool_pump and pool_pump.get_state():
            if pool_temp is None or solar_temp is None:
                # If we can't read temperatures, turn off solar pump for safety
                if solar_pump.get_state():
                    solar_pump.set_state(False)
                    self.logger.warning("Temperature sensors not available, turning off solar pump")
                return

            # More aggressive solar pump control in boost mode
            if pool_temp < (pool_max_temp + hysteresis) and solar_temp > pool_temp:
                if not solar_pump.get_state():
                    solar_pump.set_state(True)
                    self.logger.info("Boost mode: turning on solar pump")
            else:
                if solar_pump.get_state():
                    solar_pump.set_state(False)
                    self.logger.info("Boost mode: turning off solar pump")
        elif solar_pump:
            # Pool pump is off, make sure solar pump is also off
            if solar_pump.get_state():
                solar_pump.set_state(False)
                self.logger.info("Pool pump off, turning off solar pump")
