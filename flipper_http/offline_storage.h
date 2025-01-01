#pragma once

#include <furi.h>
#include <storage/storage.h>
#include "game_state.h"

#define OFFLINE_STORAGE_FILE_PATH EXT_PATH("apps_data/tagracer/offline_data.bin")
#define MAX_CACHED_TAGS 1000
#define MAX_CACHED_GAMES 50

typedef struct {
    uint32_t timestamp;
    char tag_uid[32];
    char game_id[32];
    uint32_t points;
    uint32_t combo;
    float latitude;
    float longitude;
} CachedTagScan;

typedef struct {
    uint32_t timestamp;
    GameMode mode;
    uint32_t duration;
    uint32_t score;
    uint32_t tag_count;
    char game_id[32];
} CachedGame;

typedef struct {
    uint32_t tag_scan_count;
    uint32_t game_count;
    CachedTagScan tag_scans[MAX_CACHED_TAGS];
    CachedGame games[MAX_CACHED_GAMES];
    bool needs_sync;
} OfflineStorage;

// Initialisierung und Speichermanagement
bool offline_storage_init(OfflineStorage* storage);
bool offline_storage_save(OfflineStorage* storage);
bool offline_storage_load(OfflineStorage* storage);

// Tag-Operationen
bool offline_storage_add_tag_scan(
    OfflineStorage* storage,
    const char* tag_uid,
    const char* game_id,
    uint32_t points,
    uint32_t combo,
    float latitude,
    float longitude
);

// Spiel-Operationen
bool offline_storage_add_game(
    OfflineStorage* storage,
    const char* game_id,
    GameMode mode,
    uint32_t duration,
    uint32_t score,
    uint32_t tag_count
);

// Synchronisation
typedef void (*SyncCallback)(bool success, void* context);
void offline_storage_sync(OfflineStorage* storage, SyncCallback callback, void* context);

// Hilfsfunktionen
uint32_t offline_storage_get_pending_count(OfflineStorage* storage);
bool offline_storage_clear(OfflineStorage* storage);
bool offline_storage_export_csv(OfflineStorage* storage, const char* path);
