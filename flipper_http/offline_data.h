#pragma once

#include <furi.h>
#include <storage/storage.h>
#include "game_state.h"

// Datei-Pfade
#define OFFLINE_DATA_DIR EXT_PATH("apps_data/tagracer")
#define GAME_DATA_FILE OFFLINE_DATA_DIR "/games.bin"
#define TAG_DATA_FILE OFFLINE_DATA_DIR "/tags.bin"
#define LEADERBOARD_FILE OFFLINE_DATA_DIR "/leaderboard.bin"
#define MAP_CACHE_FILE OFFLINE_DATA_DIR "/maps.bin"
#define MESSAGE_FILE OFFLINE_DATA_DIR "/messages.bin"
#define BACKUP_DIR OFFLINE_DATA_DIR "/backups"

// Maximale Anzahl gespeicherter Elemente
#define MAX_OFFLINE_GAMES 100
#define MAX_OFFLINE_TAGS 2000
#define MAX_LEADERBOARD_ENTRIES 100
#define MAX_CACHED_MESSAGES 500
#define MAX_MAP_TILES 200

// Datenstrukturen für Offline-Speicherung
typedef struct {
    uint32_t timestamp;
    float latitude;
    float longitude;
    float accuracy;
    bool valid;
} GpsData;

typedef struct {
    char id[32];
    char name[64];
    uint32_t score;
    uint32_t rank;
    uint32_t last_updated;
} LeaderboardEntry;

typedef struct {
    char sender[32];
    char receiver[32];
    char content[256];
    uint32_t timestamp;
    bool read;
} OfflineMessage;

typedef struct {
    uint32_t tile_id;
    int16_t zoom;
    int32_t x;
    int32_t y;
    uint32_t last_updated;
    uint8_t data[4096];
} MapTile;

typedef struct {
    char id[32];
    char name[64];
    uint32_t start_time;
    uint32_t duration;
    GameMode mode;
    uint32_t player_count;
    uint32_t team_count;
    bool is_local;
} OfflineTournament;

// Hauptspeicherstruktur
typedef struct {
    // Spieldaten
    uint32_t game_count;
    CachedGame games[MAX_OFFLINE_GAMES];
    
    // Tag-Daten
    uint32_t tag_count;
    CachedTagScan tags[MAX_OFFLINE_TAGS];
    
    // Bestenliste
    uint32_t leaderboard_count;
    LeaderboardEntry leaderboard[MAX_LEADERBOARD_ENTRIES];
    
    // Nachrichten
    uint32_t message_count;
    OfflineMessage messages[MAX_CACHED_MESSAGES];
    
    // Kartendaten
    uint32_t map_tile_count;
    MapTile map_tiles[MAX_MAP_TILES];
    
    // GPS-Cache
    GpsData last_gps;
    
    // Turniere
    OfflineTournament current_tournament;
    
    // Status
    bool needs_sync;
    uint32_t last_sync;
    uint32_t last_backup;
} OfflineData;

// Hauptfunktionen
bool offline_data_init(OfflineData* data);
bool offline_data_save(OfflineData* data);
bool offline_data_load(OfflineData* data);
bool offline_data_backup(OfflineData* data);
bool offline_data_restore(OfflineData* data, const char* backup_path);

// Spiel-Management
bool offline_data_add_game(OfflineData* data, const CachedGame* game);
bool offline_data_update_game(OfflineData* data, const char* game_id, uint32_t score);
CachedGame* offline_data_get_game(OfflineData* data, const char* game_id);

// Tag-Management
bool offline_data_add_tag(OfflineData* data, const CachedTagScan* tag);
bool offline_data_get_tag_stats(OfflineData* data, const char* game_id, uint32_t* count, uint32_t* points);

// Bestenliste
bool offline_data_update_leaderboard(OfflineData* data, const LeaderboardEntry* entry);
bool offline_data_get_top_players(OfflineData* data, LeaderboardEntry* entries, uint32_t count);

// Messaging
bool offline_data_add_message(OfflineData* data, const OfflineMessage* message);
bool offline_data_get_unread_messages(OfflineData* data, OfflineMessage* messages, uint32_t* count);
bool offline_data_mark_message_read(OfflineData* data, uint32_t message_id);

// Kartendaten
bool offline_data_cache_map_tile(OfflineData* data, const MapTile* tile);
MapTile* offline_data_get_map_tile(OfflineData* data, uint32_t tile_id);
bool offline_data_clear_old_tiles(OfflineData* data, uint32_t max_age);

// GPS und Location
bool offline_data_update_gps(OfflineData* data, const GpsData* gps);
bool offline_data_get_last_location(OfflineData* data, GpsData* gps);

// Turnier-Management
bool offline_data_start_tournament(OfflineData* data, const OfflineTournament* tournament);
bool offline_data_end_tournament(OfflineData* data);
bool offline_data_get_tournament_stats(OfflineData* data, uint32_t* player_count, uint32_t* total_score);

// Export/Import
bool offline_data_export_csv(OfflineData* data, const char* path);
bool offline_data_import_csv(OfflineData* data, const char* path);

// Hilfsfunktionen
uint32_t offline_data_get_storage_usage(OfflineData* data);
bool offline_data_cleanup(OfflineData* data, uint32_t min_free_space);
void offline_data_print_stats(OfflineData* data);
