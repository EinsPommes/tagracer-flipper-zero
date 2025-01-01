"""
Spiellogik und Rundenverwaltung
"""

from datetime import datetime, timedelta
from typing import List, Optional
from models import Game, Player, Score, GameStatus, TagRegistry
from database import get_session, cleanup_session
import logging

class GameManager:
    def __init__(self):
        self.current_game: Optional[Game] = None
        self.game_duration = timedelta(minutes=5)
        
    def start_new_game(self) -> Game:
        """Neue Spielrunde starten"""
        session = get_session()
        try:
            # Laufende Spiele beenden
            self._finish_running_games(session)
            
            # Neue Runde erstellen
            game = Game(
                start_time=datetime.utcnow(),
                status=GameStatus.RUNNING
            )
            session.add(game)
            session.commit()
            
            self.current_game = game
            logging.info(f"Neue Spielrunde {game.id} gestartet")
            return game
        finally:
            cleanup_session(session)
            
    def end_game(self, game_id: int) -> Game:
        """Spielrunde beenden"""
        session = get_session()
        try:
            game = session.query(Game).get(game_id)
            if game and game.status == GameStatus.RUNNING:
                game.status = GameStatus.FINISHED
                game.end_time = datetime.utcnow()
                session.commit()
                logging.info(f"Spielrunde {game.id} beendet")
            return game
        finally:
            cleanup_session(session)
            
    def join_game(self, game_id: int, player_id: int) -> bool:
        """Spieler einer Runde hinzufügen"""
        session = get_session()
        try:
            game = session.query(Game).get(game_id)
            player = session.query(Player).get(player_id)
            
            if game and player and game.status == GameStatus.RUNNING:
                if player not in game.players:
                    game.players.append(player)
                    session.commit()
                    logging.info(f"Spieler {player_id} ist Runde {game_id} beigetreten")
                return True
            return False
        finally:
            cleanup_session(session)
            
    def process_tag(self, game_id: int, player_id: int, tag_id: str,
                   latitude: float = None, longitude: float = None) -> Optional[Score]:
        """Tag verarbeiten und Punkte vergeben"""
        session = get_session()
        try:
            game = session.query(Game).get(game_id)
            if not game or game.status != GameStatus.RUNNING:
                return None
                
            # Prüfen ob Tag bereits in dieser Runde gescannt wurde
            existing_scan = session.query(Score).filter(
                Score.game_id == game_id,
                Score.player_id == player_id,
                Score.tag_id == tag_id
            ).first()
            
            if existing_scan:
                logging.warning(f"Tag {tag_id} wurde bereits in Runde {game_id} gescannt")
                return None
                
            # Punkte aus Registry holen
            tag_info = session.query(TagRegistry).filter_by(tag_id=tag_id).first()
            points = tag_info.points if tag_info else 10
            
            # Score erstellen
            score = Score(
                game_id=game_id,
                player_id=player_id,
                tag_id=tag_id,
                points=points,
                latitude=latitude,
                longitude=longitude
            )
            session.add(score)
            
            # Tag-Registry aktualisieren
            if tag_info:
                tag_info.last_scanned = datetime.utcnow()
            
            session.commit()
            logging.info(f"Tag {tag_id} gescannt: {points} Punkte für Spieler {player_id}")
            return score
        finally:
            cleanup_session(session)
            
    def get_game_scores(self, game_id: int) -> List[dict]:
        """Punktestand einer Runde abrufen"""
        session = get_session()
        try:
            scores = session.query(
                Player.nickname,
                Player.device_id,
                func.sum(Score.points).label('total_points')
            ).join(Score).filter(
                Score.game_id == game_id
            ).group_by(
                Player.id
            ).order_by(
                desc('total_points')
            ).all()
            
            return [
                {
                    'nickname': score.nickname or score.device_id,
                    'points': score.total_points or 0
                }
                for score in scores
            ]
        finally:
            cleanup_session(session)
            
    def _finish_running_games(self, session):
        """Alle laufenden Spiele beenden"""
        running_games = session.query(Game).filter_by(status=GameStatus.RUNNING).all()
        for game in running_games:
            game.status = GameStatus.FINISHED
            game.end_time = datetime.utcnow()
        session.commit()
