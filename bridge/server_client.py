"""
Behandelt die Kommunikation mit dem TagRacer Server
"""

import asyncio
import aiohttp
import json
import logging
from typing import Optional, Callable
from config import SERVER_URL, API_ENDPOINT, WEBSOCKET_ENDPOINT, MAX_RECONNECT_ATTEMPTS, RECONNECT_DELAY

class ServerClient:
    def __init__(self, message_callback: Callable[[dict], None]):
        self.session: Optional[aiohttp.ClientSession] = None
        self.ws: Optional[aiohttp.ClientWebSocketResponse] = None
        self.message_callback = message_callback
        self.running = False
        self.reconnect_task: Optional[asyncio.Task] = None
        
    async def connect(self) -> bool:
        """Verbindung zum Server herstellen"""
        try:
            self.session = aiohttp.ClientSession()
            self.ws = await self.session.ws_connect(WEBSOCKET_ENDPOINT)
            self.running = True
            logging.info("Verbunden mit TagRacer Server")
            return True
        except Exception as e:
            logging.error(f"Server-Verbindungsfehler: {e}")
            return False
            
    async def disconnect(self):
        """Verbindung zum Server trennen"""
        self.running = False
        if self.reconnect_task:
            self.reconnect_task.cancel()
        if self.ws:
            await self.ws.close()
        if self.session:
            await self.session.close()
            
    async def send_tag_data(self, tag_data: dict) -> dict:
        """Tag-Daten an den Server senden"""
        try:
            async with self.session.post(
                f"{SERVER_URL}{API_ENDPOINT}",
                json=tag_data
            ) as response:
                return await response.json()
        except Exception as e:
            logging.error(f"Fehler beim Senden der Tag-Daten: {e}")
            return {"error": str(e)}
            
    async def start_websocket_handler(self):
        """WebSocket-Verbindung verwalten und Nachrichten empfangen"""
        attempt = 0
        while self.running:
            try:
                async with self.ws as ws:
                    attempt = 0  # Reset bei erfolgreicher Verbindung
                    async for msg in ws:
                        if msg.type == aiohttp.WSMsgType.TEXT:
                            try:
                                data = json.loads(msg.data)
                                await self.message_callback(data)
                            except json.JSONDecodeError:
                                logging.warning(f"Ungültige WebSocket-Nachricht: {msg.data}")
                        elif msg.type == aiohttp.WSMsgType.ERROR:
                            logging.error(f"WebSocket-Fehler: {ws.exception()}")
                            break
            except Exception as e:
                logging.error(f"WebSocket-Verbindungsfehler: {e}")
                if attempt < MAX_RECONNECT_ATTEMPTS:
                    attempt += 1
                    await asyncio.sleep(RECONNECT_DELAY)
                    try:
                        self.ws = await self.session.ws_connect(WEBSOCKET_ENDPOINT)
                        logging.info("WebSocket-Verbindung wiederhergestellt")
                    except Exception as reconnect_error:
                        logging.error(f"Reconnect fehlgeschlagen: {reconnect_error}")
                else:
                    logging.error("Maximale Anzahl von Reconnect-Versuchen erreicht")
                    break
                    
    async def send_websocket_message(self, message: dict):
        """Nachricht über WebSocket senden"""
        if self.ws and not self.ws.closed:
            try:
                await self.ws.send_json(message)
            except Exception as e:
                logging.error(f"Fehler beim Senden der WebSocket-Nachricht: {e}")
                # Automatischer Reconnect wird durch start_websocket_handler behandelt
