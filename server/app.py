from flask import Flask, request, jsonify
from flask_socketio import SocketIO, emit, join_room, leave_room
from flask_cors import CORS
from datetime import datetime, timedelta
import json

from database import init_db, get_db
from models import Player, Game, GamePlayer, Tag, GameTag, Achievement, PlayerAchievement
from config import (
    HOST, PORT, DEBUG, SECRET_KEY, CORS_ORIGINS,
    GAME_DURATION, MAX_PLAYERS, TAG_COOLDOWN, BASE_POINTS
)

# Flask und SocketIO Setup
app = Flask(__name__)
app.config['SECRET_KEY'] = SECRET_KEY
CORS(app, origins=CORS_ORIGINS)
socketio = SocketIO(app, cors_allowed_origins=CORS_ORIGINS, ping_interval=25, ping_timeout=120)

# Aktive Spiele im Speicher halten
active_games = {}

@app.before_first_request
def setup():
    init_db()

# Hilfsfunktionen
def check_achievements(player_id, game_id=None):
    """Überprüft und vergibt Achievements für einen Spieler"""
    with get_db() as db:
        player = db.query(Player).get(player_id)
        if not player:
            return
        
        # Alle nicht freigeschalteten Achievements laden
        unlocked_ids = [a.achievement_id for a in player.achievements]
        achievements = db.query(Achievement).filter(~Achievement.id.in_(unlocked_ids)).all()
        
        for achievement in achievements:
            requirements = json.loads(achievement.requirement)
            
            # Prüfe verschiedene Achievement-Typen
            if 'games_played' in requirements and player.total_games >= requirements['games_played']:
                unlock = True
            elif 'total_tags' in requirements:
                total_tags = db.query(GameTag).join(GamePlayer).filter(GamePlayer.player_id == player_id).count()
                unlock = total_tags >= requirements['total_tags']
            elif 'tags_30sec' in requirements and game_id:
                # Prüfe Tags in den letzten 30 Sekunden
                thirty_sec_ago = datetime.utcnow() - timedelta(seconds=30)
                recent_tags = db.query(GameTag).join(GamePlayer).filter(
                    GamePlayer.player_id == player_id,
                    GamePlayer.game_id == game_id,
                    GameTag.scan_time >= thirty_sec_ago
                ).count()
                unlock = recent_tags >= requirements['tags_30sec']
            elif 'single_game_score' in requirements and game_id:
                game_score = db.query(GamePlayer).filter(
                    GamePlayer.player_id == player_id,
                    GamePlayer.game_id == game_id
                ).first().score
                unlock = game_score >= requirements['single_game_score']
            else:
                unlock = False
            
            if unlock:
                new_achievement = PlayerAchievement(
                    player_id=player_id,
                    achievement_id=achievement.id
                )
                db.add(new_achievement)
                player.total_score += achievement.points
                
                # Achievement-Benachrichtigung senden
                socketio.emit('achievement_unlocked', {
                    'player_id': player_id,
                    'achievement': {
                        'name': achievement.name,
                        'description': achievement.description,
                        'icon': achievement.icon,
                        'points': achievement.points
                    }
                }, room=f'player_{player_id}')

# API-Endpunkte

@app.route('/api/players', methods=['POST'])
def register_player():
    """Neuen Spieler registrieren"""
    data = request.json
    username = data.get('username')
    device_id = data.get('device_id')
    
    with get_db() as db:
        # Prüfen ob Spieler bereits existiert
        existing = db.query(Player).filter(
            (Player.username == username) | (Player.device_id == device_id)
        ).first()
        
        if existing:
            return jsonify({
                'status': 'error',
                'message': 'Username oder Gerät bereits registriert'
            }), 400
        
        # Neuen Spieler erstellen
        player = Player(username=username, device_id=device_id)
        db.add(player)
        db.flush()
        
        # First Steps Achievement prüfen
        check_achievements(player.id)
        
        return jsonify({
            'status': 'success',
            'player': {
                'id': player.id,
                'username': player.username,
                'total_score': player.total_score
            }
        })

@app.route('/api/games', methods=['POST'])
def create_game():
    """Neues Spiel erstellen"""
    data = request.json
    player_id = data.get('player_id')
    
    with get_db() as db:
        # Neues Spiel erstellen
        game = Game(
            status='waiting',
            max_players=MAX_PLAYERS,
            duration=GAME_DURATION
        )
        db.add(game)
        db.flush()
        
        # Ersteller als Spieler hinzufügen
        game_player = GamePlayer(
            game_id=game.id,
            player_id=player_id,
            is_ready=True
        )
        db.add(game_player)
        
        # Spiel im Speicher halten
        active_games[game.id] = {
            'players': {player_id: False},  # False = nicht bereit
            'start_time': None,
            'tags': set()
        }
        
        return jsonify({
            'status': 'success',
            'game': {
                'id': game.id,
                'status': game.status,
                'max_players': game.max_players,
                'duration': game.duration
            }
        })

@app.route('/api/games/<int:game_id>/join', methods=['POST'])
def join_game(game_id):
    """Einem Spiel beitreten"""
    data = request.json
    player_id = data.get('player_id')
    
    with get_db() as db:
        game = db.query(Game).get(game_id)
        if not game:
            return jsonify({'status': 'error', 'message': 'Spiel nicht gefunden'}), 404
        
        if game.status != 'waiting':
            return jsonify({'status': 'error', 'message': 'Spiel bereits gestartet'}), 400
        
        # Prüfen ob Spieler bereits im Spiel ist
        existing = db.query(GamePlayer).filter_by(
            game_id=game_id,
            player_id=player_id
        ).first()
        
        if existing:
            return jsonify({'status': 'error', 'message': 'Bereits im Spiel'}), 400
        
        # Spieler zum Spiel hinzufügen
        game_player = GamePlayer(game_id=game_id, player_id=player_id)
        db.add(game_player)
        
        # Aktives Spiel aktualisieren
        if game_id in active_games:
            active_games[game_id]['players'][player_id] = False
        
        # Andere Spieler benachrichtigen
        socketio.emit('player_joined', {
            'game_id': game_id,
            'player': {
                'id': player_id,
                'username': db.query(Player).get(player_id).username
            }
        }, room=f'game_{game_id}')
        
        return jsonify({'status': 'success'})

@app.route('/api/games/<int:game_id>/ready', methods=['POST'])
def player_ready(game_id):
    """Spieler ist bereit"""
    data = request.json
    player_id = data.get('player_id')
    
    with get_db() as db:
        game_player = db.query(GamePlayer).filter_by(
            game_id=game_id,
            player_id=player_id
        ).first()
        
        if not game_player:
            return jsonify({'status': 'error', 'message': 'Spieler nicht im Spiel'}), 404
        
        game_player.is_ready = True
        
        # Aktives Spiel aktualisieren
        if game_id in active_games:
            active_games[game_id]['players'][player_id] = True
            
            # Prüfen ob alle Spieler bereit sind
            all_ready = all(active_games[game_id]['players'].values())
            if all_ready:
                game = db.query(Game).get(game_id)
                game.status = 'running'
                game.start_time = datetime.utcnow()
                active_games[game_id]['start_time'] = game.start_time
                
                # Alle Spieler benachrichtigen
                socketio.emit('game_started', {
                    'game_id': game_id,
                    'start_time': game.start_time.isoformat()
                }, room=f'game_{game_id}')
        
        return jsonify({'status': 'success'})

@app.route('/api/games/<int:game_id>/tag', methods=['POST'])
def scan_tag(game_id):
    """Tag scannen"""
    data = request.json
    player_id = data.get('player_id')
    tag_uid = data.get('tag_uid')
    
    with get_db() as db:
        game = db.query(Game).get(game_id)
        if not game or game.status != 'running':
            return jsonify({'status': 'error', 'message': 'Spiel nicht aktiv'}), 400
        
        # Prüfen ob Tag in diesem Spiel bereits gescannt wurde
        if tag_uid in active_games[game_id]['tags']:
            return jsonify({'status': 'error', 'message': 'Tag bereits gescannt'}), 400
        
        # Tag in Datenbank finden oder erstellen
        tag = db.query(Tag).filter_by(uid=tag_uid).first()
        if not tag:
            tag = Tag(
                uid=tag_uid,
                points=BASE_POINTS
            )
            db.add(tag)
            db.flush()
        
        # Tag-Scan speichern
        game_player = db.query(GamePlayer).filter_by(
            game_id=game_id,
            player_id=player_id
        ).first()
        
        game_tag = GameTag(
            game_id=game_id,
            tag_id=tag.id,
            player_id=game_player.id,
            points_awarded=tag.points
        )
        db.add(game_tag)
        
        # Punkte zum Spieler hinzufügen
        game_player.score += tag.points
        
        # Tag als gescannt markieren
        active_games[game_id]['tags'].add(tag_uid)
        
        # Achievement prüfen
        check_achievements(player_id, game_id)
        
        # Alle Spieler über den Scan informieren
        socketio.emit('tag_scanned', {
            'game_id': game_id,
            'player_id': player_id,
            'tag_uid': tag_uid,
            'points': tag.points,
            'new_score': game_player.score
        }, room=f'game_{game_id}')
        
        return jsonify({
            'status': 'success',
            'points': tag.points,
            'total_score': game_player.score
        })

@app.route('/api/leaderboard', methods=['GET'])
def get_leaderboard():
    """Bestenliste abrufen"""
    with get_db() as db:
        players = db.query(Player).order_by(Player.total_score.desc()).limit(100).all()
        return jsonify({
            'status': 'success',
            'leaderboard': [{
                'username': p.username,
                'total_score': p.total_score,
                'total_games': p.total_games,
                'achievements': len(p.achievements)
            } for p in players]
        })

# WebSocket Events

@socketio.on('connect')
def handle_connect():
    """Client verbunden"""
    pass

@socketio.on('join_game')
def handle_join_game(data):
    """Spieler tritt Spiel-Room bei"""
    game_id = data['game_id']
    player_id = data['player_id']
    
    join_room(f'game_{game_id}')
    join_room(f'player_{player_id}')
    
    emit('joined_game', {'game_id': game_id})

@socketio.on('leave_game')
def handle_leave_game(data):
    """Spieler verlässt Spiel-Room"""
    game_id = data['game_id']
    player_id = data['player_id']
    
    leave_room(f'game_{game_id}')
    
    with get_db() as db:
        game_player = db.query(GamePlayer).filter_by(
            game_id=game_id,
            player_id=player_id
        ).first()
        
        if game_player:
            db.delete(game_player)
            
            if game_id in active_games:
                if player_id in active_games[game_id]['players']:
                    del active_games[game_id]['players'][player_id]
                
                # Spiel beenden wenn keine Spieler mehr da sind
                if not active_games[game_id]['players']:
                    game = db.query(Game).get(game_id)
                    if game:
                        game.status = 'finished'
                        game.end_time = datetime.utcnow()
                    del active_games[game_id]
    
    emit('player_left', {'player_id': player_id}, room=f'game_{game_id}')

if __name__ == '__main__':
    socketio.run(app, host=HOST, port=PORT, debug=DEBUG)
