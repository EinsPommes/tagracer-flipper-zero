#include "achievement_cache.h"
#include <furi_hal_rtc.h>

AchievementCache* achievement_cache_alloc(void) {
    AchievementCache* cache = malloc(sizeof(AchievementCache));
    if(!cache) return NULL;
    
    cache->entry_count = 0;
    cache->last_update = 0;
    cache->needs_sync = false;
    cache->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    
    return cache;
}

void achievement_cache_free(AchievementCache* cache) {
    if(!cache) return;
    
    furi_mutex_free(cache->mutex);
    free(cache);
}

bool achievement_cache_update(
    AchievementCache* cache,
    uint32_t achievement_id,
    uint32_t progress
) {
    if(!cache) return false;
    
    furi_mutex_acquire(cache->mutex, FuriWaitForever);
    
    // Existierenden Eintrag suchen
    CacheEntry* entry = NULL;
    for(uint32_t i = 0; i < cache->entry_count; i++) {
        if(cache->entries[i].achievement_id == achievement_id) {
            entry = &cache->entries[i];
            break;
        }
    }
    
    // Neuen Eintrag erstellen wenn nötig
    if(!entry && cache->entry_count < CACHE_MAX_ENTRIES) {
        entry = &cache->entries[cache->entry_count++];
        entry->achievement_id = achievement_id;
    }
    
    if(entry) {
        entry->progress = progress;
        entry->timestamp = furi_get_tick();
        cache->needs_sync = true;
        
        // Cache aufräumen wenn voll
        if(cache->entry_count >= CACHE_MAX_ENTRIES) {
            achievement_cache_compact(cache);
        }
        
        furi_mutex_release(cache->mutex);
        return true;
    }
    
    furi_mutex_release(cache->mutex);
    return false;
}

void achievement_cache_process_batch(
    AchievementCache* cache,
    AchievementUpdateCallback callback
) {
    if(!cache || !callback) return;
    
    furi_mutex_acquire(cache->mutex, FuriWaitForever);
    
    uint32_t now = furi_get_tick();
    
    // Nur verarbeiten wenn Update-Intervall erreicht
    if(now - cache->last_update < UPDATE_INTERVAL_MS) {
        furi_mutex_release(cache->mutex);
        return;
    }
    
    // Batch von Einträgen verarbeiten
    uint32_t processed = 0;
    for(uint32_t i = 0; i < cache->entry_count && processed < CACHE_BATCH_SIZE; i++) {
        CacheEntry* entry = &cache->entries[i];
        
        callback(entry->achievement_id, entry->progress);
        processed++;
        
        // Eintrag als verarbeitet markieren
        entry->timestamp = now;
    }
    
    cache->last_update = now;
    
    // Cache aufräumen wenn möglich
    if(processed > 0) {
        achievement_cache_compact(cache);
    }
    
    furi_mutex_release(cache->mutex);
}

bool achievement_cache_needs_sync(AchievementCache* cache) {
    if(!cache) return false;
    
    furi_mutex_acquire(cache->mutex, FuriWaitForever);
    bool needs_sync = cache->needs_sync;
    furi_mutex_release(cache->mutex);
    
    return needs_sync;
}

void achievement_cache_mark_synced(AchievementCache* cache) {
    if(!cache) return;
    
    furi_mutex_acquire(cache->mutex, FuriWaitForever);
    cache->needs_sync = false;
    furi_mutex_release(cache->mutex);
}

void achievement_cache_cleanup(AchievementCache* cache) {
    if(!cache) return;
    
    furi_mutex_acquire(cache->mutex, FuriWaitForever);
    
    uint32_t now = furi_get_tick();
    
    // Alte Einträge entfernen
    for(uint32_t i = 0; i < cache->entry_count; i++) {
        if(now - cache->entries[i].timestamp > UPDATE_INTERVAL_MS * 10) {
            // Eintrag löschen durch Verschieben der restlichen Einträge
            if(i < cache->entry_count - 1) {
                memmove(&cache->entries[i],
                        &cache->entries[i + 1],
                        (cache->entry_count - i - 1) * sizeof(CacheEntry));
            }
            cache->entry_count--;
            i--; // Index anpassen
        }
    }
    
    furi_mutex_release(cache->mutex);
}

void achievement_cache_compact(AchievementCache* cache) {
    if(!cache) return;
    
    furi_mutex_acquire(cache->mutex, FuriWaitForever);
    
    // Einträge nach ID sortieren
    for(uint32_t i = 0; i < cache->entry_count - 1; i++) {
        for(uint32_t j = 0; j < cache->entry_count - i - 1; j++) {
            if(cache->entries[j].achievement_id >
               cache->entries[j + 1].achievement_id) {
                // Einträge tauschen
                CacheEntry temp = cache->entries[j];
                cache->entries[j] = cache->entries[j + 1];
                cache->entries[j + 1] = temp;
            }
        }
    }
    
    // Doppelte Einträge entfernen
    uint32_t write = 0;
    for(uint32_t read = 1; read < cache->entry_count; read++) {
        if(cache->entries[write].achievement_id !=
           cache->entries[read].achievement_id) {
            write++;
            if(write != read) {
                cache->entries[write] = cache->entries[read];
            }
        } else if(cache->entries[read].timestamp >
                  cache->entries[write].timestamp) {
            // Neueren Eintrag behalten
            cache->entries[write] = cache->entries[read];
        }
    }
    
    cache->entry_count = write + 1;
    
    furi_mutex_release(cache->mutex);
}
