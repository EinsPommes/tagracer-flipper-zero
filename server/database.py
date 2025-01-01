"""
Datenbank-Konfiguration und Hilfsfunktionen
"""

from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker, scoped_session
from sqlalchemy.pool import QueuePool
from models import Base
import os
import logging

DATABASE_URL = os.getenv('DATABASE_URL', 'sqlite:///tagracer.db')

# Engine mit Connection-Pooling
engine = create_engine(
    DATABASE_URL,
    poolclass=QueuePool,
    pool_size=20,
    max_overflow=10,
    pool_timeout=30,
    pool_recycle=1800
)

# Session Factory
SessionFactory = sessionmaker(bind=engine)
Session = scoped_session(SessionFactory)

def init_db():
    """Datenbank initialisieren"""
    try:
        Base.metadata.create_all(engine)
        logging.info("Datenbank erfolgreich initialisiert")
    except Exception as e:
        logging.error(f"Fehler bei der Datenbankinitialisierung: {e}")
        raise

def get_session():
    """Neue Datenbankverbindung aus dem Pool holen"""
    return Session()

def cleanup_session(session):
    """Session aufräumen und in den Pool zurückgeben"""
    try:
        session.close()
    except Exception as e:
        logging.error(f"Fehler beim Schließen der Session: {e}")
    finally:
        Session.remove()
