#pragma once

#include <furi.h>
#include "game_state.h"

#define CACHE_MAX_ENTRIES 32
#define CACHE_BATCH_SIZE 16
#define UPDATE_INTERVAL_MS 1000

typedef struct {
    uint32_t achievement_id;
    uint32_t progress;
    uint32_t timestamp;
} CacheEntry;

typedef struct {
    CacheEntry entries[CACHE_MAX_ENTRIES];
    uint32_t entry_count;
    uint32_t last_update;
    bool needs_sync;
    FuriMutex* mutex;
} AchievementCache;

typedef void (*AchievementUpdateCallback)(uint32_t achievement_id, uint32_t progress);

// Cache-Management
AchievementCache* achievement_cache_alloc(void);
void achievement_cache_free(AchievementCache* cache);

// Asynchrone Updates
bool achievement_cache_update(
    AchievementCache* cache,
    uint32_t achievement_id,
    uint32_t progress
);

// Batch-Verarbeitung
void achievement_cache_process_batch(
    AchievementCache* cache,
    AchievementUpdateCallback callback
);

// Cache-Synchronisation
bool achievement_cache_needs_sync(AchievementCache* cache);
void achievement_cache_mark_synced(AchievementCache* cache);

// Cache-Wartung
void achievement_cache_cleanup(AchievementCache* cache);
void achievement_cache_compact(AchievementCache* cache);
