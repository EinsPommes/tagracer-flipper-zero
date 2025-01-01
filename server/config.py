"""
Server-Konfiguration
"""

import os
from datetime import timedelta

# Grundeinstellungen
DEBUG = os.getenv('DEBUG', 'False').lower() == 'true'
SECRET_KEY = os.getenv('SECRET_KEY', 'your-secret-key-here')

# Datenbank
DATABASE_URL = os.getenv('DATABASE_URL', 'sqlite:///tagracer.db')

# Spiel-Einstellungen
GAME_DURATION = timedelta(minutes=5)
DEFAULT_POINTS = 10
MAX_PLAYERS_PER_GAME = 50

# WebSocket
SOCKET_PING_INTERVAL = 25
SOCKET_PING_TIMEOUT = 120

# Cache
CACHE_TYPE = "SimpleCache"
CACHE_DEFAULT_TIMEOUT = 300

# API Rate Limiting
RATELIMIT_DEFAULT = "200 per minute"
RATELIMIT_STORAGE_URL = "memory://"

# CORS
CORS_ORIGINS = [
    "http://localhost:3000",
    "http://localhost:5000",
    "http://127.0.0.1:3000",
    "http://127.0.0.1:5000"
]

# Logging
LOG_LEVEL = os.getenv('LOG_LEVEL', 'INFO')
LOG_FORMAT = '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
LOG_FILE = 'tagracer.log'

# Security
SESSION_COOKIE_SECURE = True
SESSION_COOKIE_HTTPONLY = True
SESSION_COOKIE_SAMESITE = 'Lax'
PERMANENT_SESSION_LIFETIME = timedelta(days=1)
