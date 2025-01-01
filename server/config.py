import os
from dotenv import load_dotenv

# .env Datei laden
load_dotenv()

# Datenbank-Konfiguration
DATABASE_URL = os.getenv('DATABASE_URL', 'sqlite:///tagracer.db')

# Server-Konfiguration
HOST = os.getenv('HOST', '0.0.0.0')
PORT = int(os.getenv('PORT', 5000))
DEBUG = os.getenv('DEBUG', 'True').lower() == 'true'

# Security
SECRET_KEY = os.getenv('SECRET_KEY', 'your-secret-key-here')
CORS_ORIGINS = os.getenv('CORS_ORIGINS', '*').split(',')

# Spiel-Konfiguration
GAME_DURATION = int(os.getenv('GAME_DURATION', 300))  # 5 Minuten
MAX_PLAYERS = int(os.getenv('MAX_PLAYERS', 4))
TAG_COOLDOWN = int(os.getenv('TAG_COOLDOWN', 2))  # 2 Sekunden
BASE_POINTS = int(os.getenv('BASE_POINTS', 10))

# WebSocket-Konfiguration
SOCKET_PING_INTERVAL = int(os.getenv('SOCKET_PING_INTERVAL', 25))
SOCKET_PING_TIMEOUT = int(os.getenv('SOCKET_PING_TIMEOUT', 120))

# Achievement-System
ACHIEVEMENT_CHECK_INTERVAL = int(os.getenv('ACHIEVEMENT_CHECK_INTERVAL', 60))  # 1 Minute
