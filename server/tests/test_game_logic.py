import unittest
from datetime import datetime, timedelta
from server.models import Player, Game, GamePlayer, Tag, GameTag
from server.database import init_db, get_db
from server.app import app

class TestGameLogic(unittest.TestCase):
    def setUp(self):
        app.config['TESTING'] = True
        app.config['DATABASE_URL'] = 'sqlite:///:memory:'
        self.app = app.test_client()
        init_db()
        
        # Test-Spieler erstellen
        with get_db() as db:
            self.player = Player(username='test_player', device_id='test_device')
            db.add(self.player)
            db.flush()
            self.player_id = self.player.id
    
    def test_create_game(self):
        """Test: Spiel erstellen"""
        response = self.app.post('/api/games', json={
            'player_id': self.player_id
        })
        self.assertEqual(response.status_code, 200)
        data = response.get_json()
        self.assertEqual(data['status'], 'success')
        self.assertIn('game', data)
    
    def test_join_game(self):
        """Test: Spiel beitreten"""
        # Spiel erstellen
        response = self.app.post('/api/games', json={
            'player_id': self.player_id
        })
        game_id = response.get_json()['game']['id']
        
        # Zweiten Spieler erstellen
        with get_db() as db:
            player2 = Player(username='player2', device_id='device2')
            db.add(player2)
            db.flush()
            player2_id = player2.id
        
        # Spiel beitreten
        response = self.app.post(f'/api/games/{game_id}/join', json={
            'player_id': player2_id
        })
        self.assertEqual(response.status_code, 200)
    
    def test_scan_tag(self):
        """Test: Tag scannen"""
        # Spiel erstellen und starten
        response = self.app.post('/api/games', json={
            'player_id': self.player_id
        })
        game_id = response.get_json()['game']['id']
        
        # Spieler bereit melden
        self.app.post(f'/api/games/{game_id}/ready', json={
            'player_id': self.player_id
        })
        
        # Tag erstellen
        with get_db() as db:
            tag = Tag(uid='test_tag', points=10)
            db.add(tag)
            db.flush()
        
        # Tag scannen
        response = self.app.post(f'/api/games/{game_id}/tag', json={
            'player_id': self.player_id,
            'tag_uid': 'test_tag'
        })
        self.assertEqual(response.status_code, 200)
        data = response.get_json()
        self.assertEqual(data['points'], 10)
    
    def test_duplicate_tag(self):
        """Test: Doppeltes Scannen eines Tags"""
        # Spiel erstellen und starten
        response = self.app.post('/api/games', json={
            'player_id': self.player_id
        })
        game_id = response.get_json()['game']['id']
        
        # Spieler bereit melden
        self.app.post(f'/api/games/{game_id}/ready', json={
            'player_id': self.player_id
        })
        
        # Tag erstellen
        with get_db() as db:
            tag = Tag(uid='test_tag', points=10)
            db.add(tag)
            db.flush()
        
        # Tag zum ersten Mal scannen
        self.app.post(f'/api/games/{game_id}/tag', json={
            'player_id': self.player_id,
            'tag_uid': 'test_tag'
        })
        
        # Tag erneut scannen
        response = self.app.post(f'/api/games/{game_id}/tag', json={
            'player_id': self.player_id,
            'tag_uid': 'test_tag'
        })
        self.assertEqual(response.status_code, 400)
    
    def test_game_end(self):
        """Test: Spielende"""
        # Spiel erstellen
        response = self.app.post('/api/games', json={
            'player_id': self.player_id
        })
        game_id = response.get_json()['game']['id']
        
        # Spieler bereit melden
        self.app.post(f'/api/games/{game_id}/ready', json={
            'player_id': self.player_id
        })
        
        # Spielzeit manuell auf Ende setzen
        with get_db() as db:
            game = db.query(Game).get(game_id)
            game.start_time = datetime.utcnow() - timedelta(seconds=301)
            db.commit()
        
        # Versuchen einen Tag zu scannen
        response = self.app.post(f'/api/games/{game_id}/tag', json={
            'player_id': self.player_id,
            'tag_uid': 'test_tag'
        })
        self.assertEqual(response.status_code, 400)

if __name__ == '__main__':
    unittest.main()
