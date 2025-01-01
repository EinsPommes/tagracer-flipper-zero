"""
Hauptprogramm der TagRacer Bridge
"""

import asyncio
import logging
import signal
import json
from typing import Optional
from serial_handler import SerialHandler
from server_client import ServerClient
from config import LOG_LEVEL, LOG_FILE

class TagRacerBridge:
    def __init__(self):
        self.setup_logging()
        self.running = False
        self.serial_handler: Optional[SerialHandler] = None
        self.server_client: Optional[ServerClient] = None
        
    def setup_logging(self):
        """Logging-Konfiguration"""
        logging.basicConfig(
            level=getattr(logging, LOG_LEVEL),
            format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
            handlers=[
                logging.FileHandler(LOG_FILE),
                logging.StreamHandler()
            ]
        )
        
    async def handle_serial_message(self, message: dict):
        """Verarbeitet Nachrichten vom Flipper Zero"""
        logging.debug(f"Nachricht von Flipper Zero empfangen: {message}")
        if self.server_client:
            response = await self.server_client.send_tag_data(message)
            if response and not "error" in response:
                # Erfolgreiche Antwort zurück zum Flipper Zero senden
                self.serial_handler.send_message(response)
                
    async def handle_server_message(self, message: dict):
        """Verarbeitet Nachrichten vom Server"""
        logging.debug(f"Nachricht vom Server empfangen: {message}")
        if self.serial_handler:
            self.serial_handler.send_message(message)
            
    async def start(self):
        """Startet die Bridge"""
        self.running = True
        
        # Signal-Handler für sauberes Beenden
        loop = asyncio.get_event_loop()
        for sig in (signal.SIGINT, signal.SIGTERM):
            loop.add_signal_handler(sig, lambda: asyncio.create_task(self.shutdown()))
            
        # Komponenten initialisieren
        self.serial_handler = SerialHandler(
            message_callback=lambda msg: asyncio.create_task(self.handle_serial_message(msg))
        )
        self.server_client = ServerClient(
            message_callback=self.handle_server_message
        )
        
        # Verbindungen herstellen
        if not self.serial_handler.connect():
            logging.error("Konnte keine Verbindung zum Flipper Zero herstellen")
            await self.shutdown()
            return
            
        if not await self.server_client.connect():
            logging.error("Konnte keine Verbindung zum Server herstellen")
            await self.shutdown()
            return
            
        # WebSocket-Handler starten
        await self.server_client.start_websocket_handler()
        
    async def shutdown(self):
        """Beendet die Bridge sauber"""
        self.running = False
        logging.info("Beende TagRacer Bridge...")
        
        if self.serial_handler:
            self.serial_handler.disconnect()
            
        if self.server_client:
            await self.server_client.disconnect()
            
def main():
    """Haupteinstiegspunkt"""
    bridge = TagRacerBridge()
    
    try:
        asyncio.run(bridge.start())
    except Exception as e:
        logging.error(f"Unerwarteter Fehler: {e}")
        asyncio.run(bridge.shutdown())
        
if __name__ == "__main__":
    main()
