# TagRacer API Referenz

## Authentifizierung
Alle API-Endpunkte erwarten die Spieler-ID im Request-Body.

## Endpunkte

### Spieler

#### POST /api/players
Registriert einen neuen Spieler.

**Request:**
```json
{
  "username": "string",
  "device_id": "string"
}
```

**Response:**
```json
{
  "status": "success",
  "player": {
    "id": "integer",
    "username": "string",
    "total_score": "integer"
  }
}
```

### Spiele

#### POST /api/games
Erstellt ein neues Spiel.

**Request:**
```json
{
  "player_id": "integer"
}
```

**Response:**
```json
{
  "status": "success",
  "game": {
    "id": "integer",
    "status": "string",
    "max_players": "integer",
    "duration": "integer"
  }
}
```

#### POST /api/games/{game_id}/join
Tritt einem Spiel bei.

**Request:**
```json
{
  "player_id": "integer"
}
```

**Response:**
```json
{
  "status": "success"
}
```

#### POST /api/games/{game_id}/ready
Signalisiert Spielbereitschaft.

**Request:**
```json
{
  "player_id": "integer"
}
```

**Response:**
```json
{
  "status": "success"
}
```

#### POST /api/games/{game_id}/tag
Registriert einen Tag-Scan.

**Request:**
```json
{
  "player_id": "integer",
  "tag_uid": "string"
}
```

**Response:**
```json
{
  "status": "success",
  "points": "integer",
  "total_score": "integer"
}
```

### Statistiken

#### GET /api/leaderboard
Ruft die Bestenliste ab.

**Response:**
```json
{
  "status": "success",
  "leaderboard": [
    {
      "username": "string",
      "total_score": "integer",
      "total_games": "integer",
      "achievements": "integer"
    }
  ]
}
```

## WebSocket Events

### Client → Server

#### join_game
```json
{
  "game_id": "integer",
  "player_id": "integer"
}
```

#### leave_game
```json
{
  "game_id": "integer",
  "player_id": "integer"
}
```

### Server → Client

#### player_joined
```json
{
  "game_id": "integer",
  "player": {
    "id": "integer",
    "username": "string"
  }
}
```

#### player_left
```json
{
  "player_id": "integer"
}
```

#### game_started
```json
{
  "game_id": "integer",
  "start_time": "string (ISO 8601)"
}
```

#### tag_scanned
```json
{
  "game_id": "integer",
  "player_id": "integer",
  "tag_uid": "string",
  "points": "integer",
  "new_score": "integer"
}
```

#### achievement_unlocked
```json
{
  "player_id": "integer",
  "achievement": {
    "name": "string",
    "description": "string",
    "icon": "string",
    "points": "integer"
  }
}
```

## Fehler-Responses

Alle Fehler folgen diesem Format:
```json
{
  "status": "error",
  "message": "string"
}
```

Häufige Fehlercodes:
- 400: Ungültige Anfrage
- 404: Ressource nicht gefunden
- 409: Konflikt (z.B. bereits im Spiel)
- 500: Server-Fehler
