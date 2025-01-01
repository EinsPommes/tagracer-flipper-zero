"""
Datenbankmodelle für TagRacer
"""

from sqlalchemy import Column, Integer, String, DateTime, ForeignKey, Float, Enum
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relationship
import enum
from datetime import datetime

Base = declarative_base()

class GameStatus(enum.Enum):
    WAITING = "waiting"
    RUNNING = "running"
    FINISHED = "finished"

class Game(Base):
    __tablename__ = 'games'
    
    id = Column(Integer, primary_key=True)
    start_time = Column(DateTime, nullable=False, default=datetime.utcnow)
    end_time = Column(DateTime)
    status = Column(Enum(GameStatus), nullable=False, default=GameStatus.WAITING)
    
    scores = relationship("Score", back_populates="game")
    players = relationship("Player", secondary="game_players")

class Player(Base):
    __tablename__ = 'players'
    
    id = Column(Integer, primary_key=True)
    device_id = Column(String(50), unique=True, nullable=False)
    nickname = Column(String(50))
    created_at = Column(DateTime, default=datetime.utcnow)
    
    scores = relationship("Score", back_populates="player")
    games = relationship("Game", secondary="game_players")

class Score(Base):
    __tablename__ = 'scores'
    
    id = Column(Integer, primary_key=True)
    game_id = Column(Integer, ForeignKey('games.id'), nullable=False)
    player_id = Column(Integer, ForeignKey('players.id'), nullable=False)
    tag_id = Column(String(50), nullable=False)
    points = Column(Integer, nullable=False)
    timestamp = Column(DateTime, default=datetime.utcnow)
    latitude = Column(Float)
    longitude = Column(Float)
    
    game = relationship("Game", back_populates="scores")
    player = relationship("Player", back_populates="scores")

class GamePlayers(Base):
    __tablename__ = 'game_players'
    
    game_id = Column(Integer, ForeignKey('games.id'), primary_key=True)
    player_id = Column(Integer, ForeignKey('players.id'), primary_key=True)
    join_time = Column(DateTime, default=datetime.utcnow)

class TagRegistry(Base):
    __tablename__ = 'tag_registry'
    
    id = Column(Integer, primary_key=True)
    tag_id = Column(String(50), unique=True, nullable=False)
    points = Column(Integer, nullable=False, default=10)
    description = Column(String(200))
    is_active = Column(Integer, default=1)  # SQLite boolean
    created_at = Column(DateTime, default=datetime.utcnow)
    last_scanned = Column(DateTime)
