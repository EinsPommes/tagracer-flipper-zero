#include "offline_data.h"
#include <furi_hal.h>
#include <toolbox/path.h>
#include <toolbox/compress.h>
#include <notification/notification_messages.h>

// Interne Hilfsfunktionen
static bool create_directories(Storage* storage);
static bool compress_data(const void* data, size_t size, uint8_t* out, size_t* out_size);
static bool decompress_data(const uint8_t* data, size_t size, void* out, size_t* out_size);
static void generate_backup_name(char* path, size_t path_size);

bool offline_data_init(OfflineData* data) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(!storage) return false;
    
    // Verzeichnisse erstellen
    if(!create_directories(storage)) {
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    
    // Daten initialisieren
    memset(data, 0, sizeof(OfflineData));
    data->last_sync = 0;
    data->last_backup = 0;
    data->needs_sync = false;
    
    // Gespeicherte Daten laden
    bool success = offline_data_load(data);
    
    // Automatisches Backup wenn nötig
    uint32_t now = furi_get_tick();
    if(success && (now - data->last_backup) > (24 * 60 * 60 * 1000)) { // 24h
        offline_data_backup(data);
    }
    
    furi_record_close(RECORD_STORAGE);
    return success;
}

bool offline_data_save(OfflineData* data) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(!storage) return false;
    
    bool success = true;
    File* file;
    
    // Daten komprimieren
    uint8_t* compressed = malloc(sizeof(OfflineData));
    size_t compressed_size = 0;
    
    if(!compress_data(data, sizeof(OfflineData), compressed, &compressed_size)) {
        free(compressed);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    
    // Spieldaten speichern
    file = storage_file_alloc(storage);
    if(storage_file_open(file, GAME_DATA_FILE, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        success &= storage_file_write(file, compressed, compressed_size) == compressed_size;
    } else {
        success = false;
    }
    storage_file_close(file);
    
    // Bestenliste separat speichern
    if(success && data->leaderboard_count > 0) {
        if(storage_file_open(file, LEADERBOARD_FILE, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
            success &= storage_file_write(
                file,
                data->leaderboard,
                sizeof(LeaderboardEntry) * data->leaderboard_count
            ) == sizeof(LeaderboardEntry) * data->leaderboard_count;
        } else {
            success = false;
        }
    }
    storage_file_close(file);
    
    storage_file_free(file);
    free(compressed);
    furi_record_close(RECORD_STORAGE);
    
    if(success) {
        NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
        notification_message(notifications, &sequence_success);
        furi_record_close(RECORD_NOTIFICATION);
    }
    
    return success;
}

bool offline_data_load(OfflineData* data) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(!storage) return false;
    
    bool success = true;
    File* file = storage_file_alloc(storage);
    
    // Spieldaten laden
    if(storage_file_open(file, GAME_DATA_FILE, FSAM_READ, FSOM_OPEN_EXISTING)) {
        size_t file_size = storage_file_size(file);
        uint8_t* compressed = malloc(file_size);
        
        if(storage_file_read(file, compressed, file_size) == file_size) {
            size_t data_size = sizeof(OfflineData);
            success = decompress_data(compressed, file_size, data, &data_size);
        } else {
            success = false;
        }
        
        free(compressed);
    } else {
        // Keine gespeicherten Daten - nicht unbedingt ein Fehler
        memset(data, 0, sizeof(OfflineData));
    }
    storage_file_close(file);
    
    // Bestenliste laden wenn vorhanden
    if(success && storage_file_open(file, LEADERBOARD_FILE, FSAM_READ, FSOM_OPEN_EXISTING)) {
        size_t entries = storage_file_size(file) / sizeof(LeaderboardEntry);
        if(entries > 0 && entries <= MAX_LEADERBOARD_ENTRIES) {
            success = storage_file_read(
                file,
                data->leaderboard,
                entries * sizeof(LeaderboardEntry)
            ) == entries * sizeof(LeaderboardEntry);
            data->leaderboard_count = entries;
        }
    }
    storage_file_close(file);
    
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return success;
}

bool offline_data_backup(OfflineData* data) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(!storage) return false;
    
    char backup_path[256];
    generate_backup_name(backup_path, sizeof(backup_path));
    
    bool success = true;
    File* file = storage_file_alloc(storage);
    
    // Backup-Verzeichnis erstellen
    storage_mkdir(storage, BACKUP_DIR);
    
    // Daten komprimieren
    uint8_t* compressed = malloc(sizeof(OfflineData));
    size_t compressed_size = 0;
    
    if(!compress_data(data, sizeof(OfflineData), compressed, &compressed_size)) {
        free(compressed);
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    
    // Backup schreiben
    if(storage_file_open(file, backup_path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        success = storage_file_write(file, compressed, compressed_size) == compressed_size;
    } else {
        success = false;
    }
    
    if(success) {
        data->last_backup = furi_get_tick();
        
        // Alte Backups aufräumen
        // TODO: Implementiere Backup-Rotation
    }
    
    storage_file_close(file);
    storage_file_free(file);
    free(compressed);
    furi_record_close(RECORD_STORAGE);
    
    return success;
}

// Implementierung der weiteren Funktionen...
// Der Code ist zu lang für eine einzelne Nachricht, ich zeige die wichtigsten Teile

bool offline_data_add_game(OfflineData* data, const CachedGame* game) {
    if(data->game_count >= MAX_OFFLINE_GAMES) {
        // Ältestes Spiel entfernen wenn Cache voll
        memmove(
            &data->games[0],
            &data->games[1],
            sizeof(CachedGame) * (MAX_OFFLINE_GAMES - 1)
        );
        data->game_count--;
    }
    
    memcpy(&data->games[data->game_count], game, sizeof(CachedGame));
    data->game_count++;
    data->needs_sync = true;
    
    return offline_data_save(data);
}

bool offline_data_add_tag(OfflineData* data, const CachedTagScan* tag) {
    if(data->tag_count >= MAX_OFFLINE_TAGS) {
        // Älteste Tags entfernen wenn Cache voll
        memmove(
            &data->tags[0],
            &data->tags[1],
            sizeof(CachedTagScan) * (MAX_OFFLINE_TAGS - 1)
        );
        data->tag_count--;
    }
    
    memcpy(&data->tags[data->tag_count], tag, sizeof(CachedTagScan));
    data->tag_count++;
    data->needs_sync = true;
    
    return offline_data_save(data);
}

bool offline_data_update_leaderboard(OfflineData* data, const LeaderboardEntry* entry) {
    // Existierenden Eintrag suchen und aktualisieren
    for(uint32_t i = 0; i < data->leaderboard_count; i++) {
        if(strcmp(data->leaderboard[i].id, entry->id) == 0) {
            memcpy(&data->leaderboard[i], entry, sizeof(LeaderboardEntry));
            return offline_data_save(data);
        }
    }
    
    // Neuen Eintrag hinzufügen
    if(data->leaderboard_count < MAX_LEADERBOARD_ENTRIES) {
        memcpy(
            &data->leaderboard[data->leaderboard_count],
            entry,
            sizeof(LeaderboardEntry)
        );
        data->leaderboard_count++;
        return offline_data_save(data);
    }
    
    return false;
}

bool offline_data_add_message(OfflineData* data, const OfflineMessage* message) {
    if(data->message_count >= MAX_CACHED_MESSAGES) {
        // Älteste Nachricht entfernen
        memmove(
            &data->messages[0],
            &data->messages[1],
            sizeof(OfflineMessage) * (MAX_CACHED_MESSAGES - 1)
        );
        data->message_count--;
    }
    
    memcpy(&data->messages[data->message_count], message, sizeof(OfflineMessage));
    data->message_count++;
    data->needs_sync = true;
    
    return offline_data_save(data);
}

bool offline_data_cache_map_tile(OfflineData* data, const MapTile* tile) {
    // Existierenden Tile aktualisieren
    for(uint32_t i = 0; i < data->map_tile_count; i++) {
        if(data->map_tiles[i].tile_id == tile->tile_id) {
            memcpy(&data->map_tiles[i], tile, sizeof(MapTile));
            return offline_data_save(data);
        }
    }
    
    // Neuen Tile hinzufügen
    if(data->map_tile_count < MAX_MAP_TILES) {
        memcpy(
            &data->map_tiles[data->map_tile_count],
            tile,
            sizeof(MapTile)
        );
        data->map_tile_count++;
        return offline_data_save(data);
    }
    
    return false;
}

// Hilfsfunktionen
static bool create_directories(Storage* storage) {
    if(!storage_mkdir(storage, OFFLINE_DATA_DIR)) return false;
    if(!storage_mkdir(storage, BACKUP_DIR)) return false;
    return true;
}

static void generate_backup_name(char* path, size_t path_size) {
    FuriHalRtcDateTime datetime;
    furi_hal_rtc_get_datetime(&datetime);
    
    snprintf(
        path,
        path_size,
        "%s/backup_%02d%02d%02d_%02d%02d.bin",
        BACKUP_DIR,
        datetime.year,
        datetime.month,
        datetime.day,
        datetime.hour,
        datetime.minute
    );
}
