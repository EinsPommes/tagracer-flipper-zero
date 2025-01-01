import unittest
from datetime import datetime, timedelta
from server.models import Player, Game, GamePlayer, Tag, GameTag, Achievement, PlayerAchievement
from server.database import init_db, get_db
from server.app import app, check_achievements

class TestAchievements(unittest.TestCase):
    def setUp(self):
        app.config['TESTING'] = True
        app.config['DATABASE_URL'] = 'sqlite:///:memory:'
        self.app = app.test_client()
        init_db()
        
        # Test-Spieler erstellen
        with get_db() as db:
            self.player = Player(username='test_player', device_id='test_device')
            db.add(self.player)
            
            # Test-Achievements erstellen
            self.achievements = [
                Achievement(
                    name='First Game',
                    description='Play your first game',
                    icon='🎮',
                    requirement='{"games_played": 1}',
                    points=10
                ),
                Achievement(
                    name='Tag Master',
                    description='Scan 50 tags',
                    icon='🏷️',
                    requirement='{"total_tags": 50}',
                    points=50
                ),
                Achievement(
                    name='Speed Demon',
                    description='Scan 5 tags in 30 seconds',
                    icon='⚡',
                    requirement='{"tags_30sec": 5}',
                    points=100
                )
            ]
            for achievement in self.achievements:
                db.add(achievement)
            
            db.flush()
            self.player_id = self.player.id
    
    def test_first_game_achievement(self):
        """Test: First Game Achievement"""
        # Spiel erstellen
        response = self.app.post('/api/games', json={
            'player_id': self.player_id
        })
        game_id = response.get_json()['game']['id']
        
        # Spieler bereit melden
        self.app.post(f'/api/games/{game_id}/ready', json={
            'player_id': self.player_id
        })
        
        # Achievement prüfen
        with get_db() as db:
            achievements = db.query(PlayerAchievement).filter_by(
                player_id=self.player_id
            ).all()
            self.assertEqual(len(achievements), 1)
            self.assertEqual(achievements[0].achievement.name, 'First Game')
    
    def test_tag_master_achievement(self):
        """Test: Tag Master Achievement"""
        # Spiel erstellen
        response = self.app.post('/api/games', json={
            'player_id': self.player_id
        })
        game_id = response.get_json()['game']['id']
        
        # Spieler bereit melden
        self.app.post(f'/api/games/{game_id}/ready', json={
            'player_id': self.player_id
        })
        
        # 50 Tags scannen
        with get_db() as db:
            game_player = db.query(GamePlayer).filter_by(
                game_id=game_id,
                player_id=self.player_id
            ).first()
            
            for i in range(50):
                tag = Tag(uid=f'tag_{i}', points=10)
                db.add(tag)
                db.flush()
                
                game_tag = GameTag(
                    game_id=game_id,
                    tag_id=tag.id,
                    player_id=game_player.id,
                    points_awarded=10
                )
                db.add(game_tag)
            
            db.commit()
            
            # Achievement prüfen
            check_achievements(self.player_id, game_id)
            
            achievements = db.query(PlayerAchievement).filter_by(
                player_id=self.player_id
            ).all()
            self.assertEqual(len(achievements), 2)  # First Game + Tag Master
            self.assertTrue(any(a.achievement.name == 'Tag Master' for a in achievements))
    
    def test_speed_demon_achievement(self):
        """Test: Speed Demon Achievement"""
        # Spiel erstellen
        response = self.app.post('/api/games', json={
            'player_id': self.player_id
        })
        game_id = response.get_json()['game']['id']
        
        # Spieler bereit melden
        self.app.post(f'/api/games/{game_id}/ready', json={
            'player_id': self.player_id
        })
        
        # 5 Tags schnell scannen
        with get_db() as db:
            game_player = db.query(GamePlayer).filter_by(
                game_id=game_id,
                player_id=self.player_id
            ).first()
            
            scan_time = datetime.utcnow()
            for i in range(5):
                tag = Tag(uid=f'speed_tag_{i}', points=10)
                db.add(tag)
                db.flush()
                
                game_tag = GameTag(
                    game_id=game_id,
                    tag_id=tag.id,
                    player_id=game_player.id,
                    points_awarded=10,
                    scan_time=scan_time
                )
                db.add(game_tag)
                scan_time += timedelta(seconds=5)
            
            db.commit()
            
            # Achievement prüfen
            check_achievements(self.player_id, game_id)
            
            achievements = db.query(PlayerAchievement).filter_by(
                player_id=self.player_id
            ).all()
            self.assertEqual(len(achievements), 2)  # First Game + Speed Demon
            self.assertTrue(any(a.achievement.name == 'Speed Demon' for a in achievements))

if __name__ == '__main__':
    unittest.main()
