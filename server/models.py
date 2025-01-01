from sqlalchemy import Column, Integer, String, DateTime, Boolean, ForeignKey, Float, JSON
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func
from database import Base

class Player(Base):
    __tablename__ = 'players'
    
    id = Column(Integer, primary_key=True)
    username = Column(String(50), unique=True, nullable=False)
    device_id = Column(String(50), unique=True, nullable=False)
    total_score = Column(Integer, default=0)
    rank = Column(Integer)
    team_id = Column(Integer, nullable=True)
    created_at = Column(DateTime, server_default=func.now())
    last_active = Column(DateTime, onupdate=func.now())
    
    # Statistiken
    games_played = Column(Integer, default=0)
    total_tags = Column(Integer, default=0)
    fastest_tag = Column(Float, nullable=True)
    longest_combo = Column(Integer, default=0)
    territories_captured = Column(Integer, default=0)
    power_ups_collected = Column(Integer, default=0)
    
    # Beziehungen
    achievements = relationship('PlayerAchievement', back_populates='player')
    game_players = relationship('GamePlayer', back_populates='player')

class Game(Base):
    __tablename__ = 'games'
    
    id = Column(Integer, primary_key=True)
    mode = Column(String(20), nullable=False, default='classic')
    status = Column(String(20), default='waiting')
    max_players = Column(Integer, default=4)
    duration = Column(Integer, default=300)
    start_time = Column(DateTime, nullable=True)
    end_time = Column(DateTime, nullable=True)
    winning_team = Column(Integer, nullable=True)
    
    # Spieleinstellungen
    settings = Column(JSON, default={})
    map_data = Column(JSON, nullable=True)
    power_up_locations = Column(JSON, nullable=True)
    
    # Beziehungen
    players = relationship('GamePlayer', back_populates='game')
    tags = relationship('GameTag', back_populates='game')

class GamePlayer(Base):
    __tablename__ = 'game_players'
    
    id = Column(Integer, primary_key=True)
    game_id = Column(Integer, ForeignKey('games.id'))
    player_id = Column(Integer, ForeignKey('players.id'))
    team_id = Column(Integer, nullable=True)
    score = Column(Integer, default=0)
    ready = Column(Boolean, default=False)
    position = Column(JSON, nullable=True)
    combo_multiplier = Column(Integer, default=1)
    power_ups = Column(JSON, default=[])
    
    # Beziehungen
    game = relationship('Game', back_populates='players')
    player = relationship('Player', back_populates='game_players')

class Tag(Base):
    __tablename__ = 'tags'
    
    id = Column(Integer, primary_key=True)
    uid = Column(String(50), unique=True, nullable=False)
    points = Column(Integer, default=10)
    type = Column(String(20), default='standard')
    location = Column(JSON, nullable=True)
    owner_team = Column(Integer, nullable=True)
    
    # Beziehungen
    game_tags = relationship('GameTag', back_populates='tag')

class GameTag(Base):
    __tablename__ = 'game_tags'
    
    id = Column(Integer, primary_key=True)
    game_id = Column(Integer, ForeignKey('games.id'))
    tag_id = Column(Integer, ForeignKey('tags.id'))
    player_id = Column(Integer, ForeignKey('game_players.id'))
    points_awarded = Column(Integer)
    scan_time = Column(DateTime, server_default=func.now())
    combo_multiplier = Column(Integer, default=1)
    power_up_active = Column(Boolean, default=False)
    
    # Beziehungen
    game = relationship('Game', back_populates='tags')
    tag = relationship('Tag', back_populates='game_tags')

class Achievement(Base):
    __tablename__ = 'achievements'
    
    id = Column(Integer, primary_key=True)
    name = Column(String(50), unique=True, nullable=False)
    description = Column(String(200))
    icon = Column(String(20))
    requirement = Column(JSON, nullable=False)
    points = Column(Integer, default=10)
    secret = Column(Boolean, default=False)
    
    # Beziehungen
    players = relationship('PlayerAchievement', back_populates='achievement')

class PlayerAchievement(Base):
    __tablename__ = 'player_achievements'
    
    id = Column(Integer, primary_key=True)
    player_id = Column(Integer, ForeignKey('players.id'))
    achievement_id = Column(Integer, ForeignKey('achievements.id'))
    unlocked_at = Column(DateTime, server_default=func.now())
    
    # Beziehungen
    player = relationship('Player', back_populates='achievements')
    achievement = relationship('Achievement', back_populates='players')

class PowerUp(Base):
    __tablename__ = 'power_ups'
    
    id = Column(Integer, primary_key=True)
    type = Column(String(20), nullable=False)
    name = Column(String(50))
    description = Column(String(200))
    icon = Column(String(20))
    duration = Column(Integer, default=30)
    effects = Column(JSON, default={})
    spawn_chance = Column(Float, default=0.1)
    
    # Beziehungen
    game_power_ups = relationship('GamePowerUp', back_populates='power_up')

class GamePowerUp(Base):
    __tablename__ = 'game_power_ups'
    
    id = Column(Integer, primary_key=True)
    game_id = Column(Integer, ForeignKey('games.id'))
    power_up_id = Column(Integer, ForeignKey('power_ups.id'))
    player_id = Column(Integer, ForeignKey('game_players.id'), nullable=True)
    location = Column(JSON, nullable=True)
    spawn_time = Column(DateTime, server_default=func.now())
    collected_at = Column(DateTime, nullable=True)
    
    # Beziehungen
    power_up = relationship('PowerUp', back_populates='game_power_ups')
