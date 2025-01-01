"""
TagRacer Bridge Konfiguration
"""

# Flipper Zero Verbindungseinstellungen
FLIPPER_SERIAL_PORT = "COM3"  # Ändern Sie dies entsprechend Ihrem System
FLIPPER_BAUD_RATE = 115200

# Server-Einstellungen
SERVER_URL = "http://localhost:5000"
API_ENDPOINT = "/api/tag"
WEBSOCKET_ENDPOINT = "ws://localhost:5000/ws"

# Logging-Einstellungen
LOG_LEVEL = "INFO"
LOG_FILE = "bridge.log"

# Reconnect-Einstellungen
MAX_RECONNECT_ATTEMPTS = 5
RECONNECT_DELAY = 5  # Sekunden

# Buffer-Einstellungen
MAX_QUEUE_SIZE = 100  # Maximale Anzahl von gepufferten Nachrichten
