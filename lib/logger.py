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
        CRITICAL: "CRITICAL"
    }
    
    def __init__(self, level=INFO):
        self.level = level
    
    def _log(self, level, message):
        """Internal logging method"""
        if level >= self.level:
            timestamp = time.ticks_ms()
            level_name = self.LEVEL_NAMES.get(level, "UNKNOWN")
            print(f"[{timestamp}] {level_name}: {message}")
    
    def debug(self, message):
        """Log debug message"""
        self._log(self.DEBUG, message)
    
    def info(self, message):
        """Log info message"""
        self._log(self.INFO, message)
    
    def warning(self, message):
        """Log warning message"""
        self._log(self.WARNING, message)
    
    def error(self, message):
        """Log error message"""
        self._log(self.ERROR, message)
    
    def critical(self, message):
        """Log critical message"""
        self._log(self.CRITICAL, message)
    
    def set_level(self, level):
        """Set logging level"""
        self.level = level