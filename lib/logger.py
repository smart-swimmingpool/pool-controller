"""
Simple logging utility for Pool Controller
"""

import time


class Logger:
    DEBUG = 0
    INFO = 1
    WARNING = 2
    ERROR = 3
    CRITICAL = 4

    LEVEL_NAMES = {
        DEBUG: "DEBUG",
        INFO: "INFO",
        WARNING: "WARNING",
        ERROR: "ERROR",
        CRITICAL: "CRITICAL",
    }

    def __init__(self, level: int = INFO) -> None:
        self.level = level

    def _log(self, level: int, message: str) -> None:
        """Internal logging method"""
        if level >= self.level:
            timestamp = time.ticks_ms()
            level_name = self.LEVEL_NAMES.get(level, "UNKNOWN")
            print(f"[{timestamp}] {level_name}: {message}")

    def debug(self, message: str) -> None:
        """Log debug message"""
        self._log(self.DEBUG, message)

    def info(self, message: str) -> None:
        """Log info message"""
        self._log(self.INFO, message)

    def warning(self, message: str) -> None:
        """Log warning message"""
        self._log(self.WARNING, message)

    def error(self, message: str) -> None:
        """Log error message"""
        self._log(self.ERROR, message)

    def critical(self, message: str) -> None:
        """Log critical message"""
        self._log(self.CRITICAL, message)

    def set_level(self, level: int) -> None:
        """Set logging level"""
        self.level = level
