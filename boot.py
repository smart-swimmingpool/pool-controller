"""
Boot script for Pool Controller
Sets up basic system configuration.
"""

import gc
import esp
import network

# Disable debug output
esp.osdebug(None)

# Enable garbage collection
gc.enable()

# Configure WiFi to station mode
wlan = network.WLAN(network.STA_IF)
wlan.active(True)

print("Pool Controller - Boot complete")