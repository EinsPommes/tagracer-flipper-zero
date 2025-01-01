"""
TagRacer Server Hauptanwendung
"""

from flask import Flask, request, jsonify
from flask_socketio import SocketIO, emit
from flask_cors import CORS
from database import init_db, get_session, cleanup_session
from game_manager import GameManager
from models import Player, Game, GameStatus
import logging
from datetime import datetime
import json

# Logging konfigurieren
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)

# Flask und SocketIO initialisieren
app = Flask(__name__)
CORS(app)
socketio = SocketIO(app, cors_allowed_origins="*")

# Game Manager initialisieren
game_manager = GameManager()

@app.before_first_request
def setup():
    """Server-Setup vor dem ersten Request"""
    init_db()
    
@app.route('/api/register', methods=['POST'])
def register_player():
    """Neuen Spieler registrieren"""
    data = request.json
    if not data or 'device_id' not in data:
        return jsonify({'error': 'device_id required'}), 400
        
    session = get_session()
    try:
        player = session.query(Player).filter_by(device_id=data['device_id']).first()
        if not player:
            player = Player(
                device_id=data['device_id'],
                nickname=data.get('nickname')
            )
            session.add(player)
            session.commit()
        
        return jsonify({
            'player_id': player.id,
            'device_id': player.device_id,
            'nickname': player.nickname
        })
    finally:
        cleanup_session(session)

@app.route('/api/games/current', methods=['GET'])
def get_current_game():
    """Aktuelle Spielrunde abrufen"""
    session = get_session()
    try:
        game = session.query(Game).filter_by(status=GameStatus.RUNNING).first()
        if not game:
            return jsonify({'error': 'No active game'}), 404
            
        scores = game_manager.get_game_scores(game.id)
        return jsonify({
            'game_id': game.id,
            'start_time': game.start_time.isoformat(),
            'scores': scores
        })
    finally:
        cleanup_session(session)

@app.route('/api/games/start', methods=['POST'])
def start_game():
    """Neue Spielrunde starten"""
    game = game_manager.start_new_game()
    socketio.emit('game_started', {
        'game_id': game.id,
        'start_time': game.start_time.isoformat()
    })
    return jsonify({
        'game_id': game.id,
        'start_time': game.start_time.isoformat()
    })

@app.route('/api/tag', methods=['POST'])
def process_tag():
    """Tag-Scan verarbeiten"""
    data = request.json
    if not data or 'tag_id' not in data or 'player_id' not in data:
        return jsonify({'error': 'Invalid data'}), 400
        
    session = get_session()
    try:
        game = session.query(Game).filter_by(status=GameStatus.RUNNING).first()
        if not game:
            return jsonify({'error': 'No active game'}), 404
            
        score = game_manager.process_tag(
            game.id,
            data['player_id'],
            data['tag_id'],
            data.get('latitude'),
            data.get('longitude')
        )
        
        if not score:
            return jsonify({'error': 'Tag already scanned or invalid'}), 400
            
        # Aktualisierte Punktestände an alle Clients senden
        scores = game_manager.get_game_scores(game.id)
        socketio.emit('score_update', {
            'game_id': game.id,
            'scores': scores
        })
        
        return jsonify({
            'points': score.points,
            'total_score': sum(s['points'] for s in scores 
                             if s['device_id'] == score.player.device_id)
        })
    finally:
        cleanup_session(session)

@socketio.on('connect')
def handle_connect():
    """WebSocket-Verbindung behandeln"""
    logging.info(f"Client verbunden: {request.sid}")

@socketio.on('join_game')
def handle_join_game(data):
    """Spieler einer Runde hinzufügen"""
    if 'game_id' not in data or 'player_id' not in data:
        return {'error': 'Invalid data'}
        
    success = game_manager.join_game(data['game_id'], data['player_id'])
    if success:
        emit('player_joined', {
            'game_id': data['game_id'],
            'player_id': data['player_id']
        }, broadcast=True)
        return {'success': True}
    return {'error': 'Could not join game'}

def start_server():
    """Server starten"""
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)

if __name__ == '__main__':
    start_server()
