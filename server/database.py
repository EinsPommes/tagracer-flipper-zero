from sqlalchemy import create_engine
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker, scoped_session
from contextlib import contextmanager
from config import DATABASE_URL

# Engine erstellen
engine = create_engine(
    DATABASE_URL,
    pool_size=20,
    max_overflow=0,
    pool_timeout=30,
    pool_recycle=1800
)

# Session-Factory erstellen
session_factory = sessionmaker(bind=engine)
Session = scoped_session(session_factory)

# Base-Klasse für Models
Base = declarative_base()

@contextmanager
def get_db():
    """Kontext-Manager für Datenbank-Sessions"""
    session = Session()
    try:
        yield session
        session.commit()
    except Exception as e:
        session.rollback()
        raise e
    finally:
        session.close()

def init_db():
    """Datenbank initialisieren"""
    from models import Player, Game, GamePlayer, Tag, GameTag, Achievement, PlayerAchievement
    Base.metadata.create_all(engine)
    
    # Standard-Achievements erstellen
    with get_db() as db:
        achievements = [
            {
                'name': 'First Steps',
                'description': 'Spiele dein erstes Spiel',
                'icon': '🎮',
                'requirement': '{"games_played": 1}',
                'points': 10
            },
            {
                'name': 'Tag Master',
                'description': 'Scanne 50 Tags',
                'icon': '🏷️',
                'requirement': '{"total_tags": 50}',
                'points': 50
            },
            {
                'name': 'Speed Demon',
                'description': 'Scanne 5 Tags in unter 30 Sekunden',
                'icon': '⚡',
                'requirement': '{"tags_30sec": 5}',
                'points': 100
            },
            {
                'name': 'High Score',
                'description': 'Erreiche 1000 Punkte in einem Spiel',
                'icon': '🏆',
                'requirement': '{"single_game_score": 1000}',
                'points': 200
            }
        ]
        
        for achievement_data in achievements:
            # Prüfen ob Achievement bereits existiert
            existing = db.query(Achievement).filter_by(name=achievement_data['name']).first()
            if not existing:
                achievement = Achievement(**achievement_data)
                db.add(achievement)

def get_session():
    """Session für manuelle Verwaltung zurückgeben"""
    return Session()

def cleanup_db():
    """Datenbank aufräumen"""
    Session.remove()
    engine.dispose()
