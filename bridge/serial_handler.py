"""
Behandelt die serielle Kommunikation mit dem Flipper Zero
"""

import serial
import json
import logging
from queue import Queue
from threading import Thread, Event
from typing import Optional, Callable
from config import FLIPPER_SERIAL_PORT, FLIPPER_BAUD_RATE

class SerialHandler:
    def __init__(self, message_callback: Callable[[dict], None]):
        self.port = FLIPPER_SERIAL_PORT
        self.baud_rate = FLIPPER_BAUD_RATE
        self.serial: Optional[serial.Serial] = None
        self.running = False
        self.message_callback = message_callback
        self.read_thread: Optional[Thread] = None
        self.write_queue = Queue()
        self.write_thread: Optional[Thread] = None
        self.connected = Event()
        
    def connect(self) -> bool:
        """Verbindung zum Flipper Zero herstellen"""
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baud_rate,
                timeout=1
            )
            self.running = True
            self.connected.set()
            self._start_threads()
            logging.info(f"Verbunden mit Flipper Zero auf {self.port}")
            return True
        except Exception as e:
            logging.error(f"Verbindungsfehler: {e}")
            return False
            
    def disconnect(self):
        """Verbindung trennen und Threads beenden"""
        self.running = False
        self.connected.clear()
        if self.serial and self.serial.is_open:
            self.serial.close()
        if self.read_thread:
            self.read_thread.join()
        if self.write_thread:
            self.write_thread.join()
            
    def send_message(self, message: dict):
        """Nachricht in die Warteschlange einfügen"""
        if self.connected.is_set():
            self.write_queue.put(message)
            
    def _start_threads(self):
        """Lese- und Schreib-Threads starten"""
        self.read_thread = Thread(target=self._read_loop, daemon=True)
        self.write_thread = Thread(target=self._write_loop, daemon=True)
        self.read_thread.start()
        self.write_thread.start()
        
    def _read_loop(self):
        """Kontinuierliches Lesen von der seriellen Schnittstelle"""
        buffer = ""
        while self.running:
            if self.serial and self.serial.is_open:
                try:
                    if self.serial.in_waiting:
                        char = self.serial.read().decode('utf-8')
                        if char == '\n':
                            if buffer:
                                try:
                                    message = json.loads(buffer)
                                    self.message_callback(message)
                                except json.JSONDecodeError:
                                    logging.warning(f"Ungültige JSON-Nachricht: {buffer}")
                                buffer = ""
                        else:
                            buffer += char
                except Exception as e:
                    logging.error(f"Fehler beim Lesen: {e}")
                    self.connected.clear()
                    break
                    
    def _write_loop(self):
        """Kontinuierliches Schreiben zur seriellen Schnittstelle"""
        while self.running:
            try:
                message = self.write_queue.get(timeout=1)
                if self.serial and self.serial.is_open:
                    json_str = json.dumps(message) + '\n'
                    self.serial.write(json_str.encode('utf-8'))
                    logging.debug(f"Gesendet: {message}")
            except Queue.Empty:
                continue
            except Exception as e:
                logging.error(f"Fehler beim Schreiben: {e}")
                self.connected.clear()
                break
