from flask import Blueprint, request, jsonify
from sqlalchemy import func
from models import db, Player, Game, GamePlayer, Tag, GameTag
from datetime import datetime

sync_bp = Blueprint('sync', __name__)

@sync_bp.route('/sync/tag', methods=['POST'])
def sync_tag():
    data = request.get_json()
    
    try:
        # Daten validieren
        required_fields = ['tag_uid', 'game_id', 'points', 'combo', 'timestamp']
        if not all(field in data for field in required_fields):
            return jsonify({
                'status': 'error',
                'message': 'Missing required fields'
            }), 400
        
        # Tag in Datenbank finden oder erstellen
        tag = Tag.query.filter_by(uid=data['tag_uid']).first()
        if not tag:
            tag = Tag(uid=data['tag_uid'], points=10)
            db.session.add(tag)
        
        # Spiel finden
        game = Game.query.filter_by(id=data['game_id']).first()
        if not game:
            return jsonify({
                'status': 'error',
                'message': 'Game not found'
            }), 404
        
        # GamePlayer finden
        game_player = GamePlayer.query.filter_by(
            game_id=game.id
        ).first()
        
        if not game_player:
            return jsonify({
                'status': 'error',
                'message': 'Player not found in game'
            }), 404
        
        # Tag-Scan speichern
        game_tag = GameTag(
            game_id=game.id,
            tag_id=tag.id,
            player_id=game_player.id,
            points_awarded=data['points'],
            combo_multiplier=data['combo'],
            scan_time=datetime.fromtimestamp(data['timestamp'] / 1000.0)
        )
        
        # Position speichern falls vorhanden
        if 'latitude' in data and 'longitude' in data:
            game_tag.location = {
                'lat': data['latitude'],
                'lng': data['longitude']
            }
        
        db.session.add(game_tag)
        
        # Spieler-Score aktualisieren
        game_player.score += data['points']
        
        # Statistiken aktualisieren
        player = game_player.player
        player.total_score += data['points']
        player.total_tags += 1
        
        if data['combo'] > player.longest_combo:
            player.longest_combo = data['combo']
        
        # Rang neu berechnen
        player.rank = db.session.query(func.count(Player.id)).filter(
            Player.total_score > player.total_score
        ).scalar() + 1
        
        db.session.commit()
        
        return jsonify({
            'status': 'success',
            'message': 'Tag sync successful'
        })
        
    except Exception as e:
        db.session.rollback()
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500

@sync_bp.route('/sync/game', methods=['POST'])
def sync_game():
    data = request.get_json()
    
    try:
        # Daten validieren
        required_fields = ['game_id', 'mode', 'duration', 'score', 'tag_count']
        if not all(field in data for field in required_fields):
            return jsonify({
                'status': 'error',
                'message': 'Missing required fields'
            }), 400
        
        # Spiel finden oder erstellen
        game = Game.query.filter_by(id=data['game_id']).first()
        if not game:
            game = Game(
                id=data['game_id'],
                mode=data['mode'],
                duration=data['duration'],
                status='finished'
            )
            db.session.add(game)
        
        # Spieldaten aktualisieren
        game.end_time = datetime.fromtimestamp(data['timestamp'] / 1000.0)
        
        # GamePlayer aktualisieren
        game_player = GamePlayer.query.filter_by(
            game_id=game.id
        ).first()
        
        if game_player:
            game_player.score = data['score']
            
            # Spieler-Statistiken aktualisieren
            player = game_player.player
            player.games_played += 1
            
            db.session.commit()
            
            return jsonify({
                'status': 'success',
                'message': 'Game sync successful'
            })
        else:
            return jsonify({
                'status': 'error',
                'message': 'Player not found in game'
            }), 404
            
    except Exception as e:
        db.session.rollback()
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500
