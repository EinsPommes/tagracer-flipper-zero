#include "achievement_manager.h"
#include <notification/notification_messages.h>
#include <furi_hal_rtc.h>

// Standard-Achievements
static const Achievement default_achievements[] = {
    {
        .id = 1,
        .name = "Erste Schritte",
        .description = "Scanne deinen ersten Tag",
        .type = AchievementTypeTags,
        .requirement = 1,
        .reward_points = 10,
        .secret = false
    },
    // ... weitere Standard-Achievements
};

// Update-Thread
static int32_t achievement_update_thread(void* context) {
    AchievementManager* manager = (AchievementManager*)context;
    
    while(manager->running) {
        // Cache-Updates verarbeiten
        achievement_cache_process_batch(
            manager->cache,
            (AchievementUpdateCallback)achievement_manager_update_progress
        );
        
        // Challenges prüfen
        achievement_manager_check_challenges(manager);
        
        // Fortschritt speichern wenn nötig
        if(achievement_cache_needs_sync(manager->cache)) {
            achievement_manager_save_progress(manager);
            achievement_cache_mark_synced(manager->cache);
        }
        
        furi_delay_ms(UPDATE_INTERVAL_MS);
    }
    
    return 0;
}

AchievementManager* achievement_manager_alloc(GameContext* game, OfflineData* data) {
    AchievementManager* manager = malloc(sizeof(AchievementManager));
    
    manager->game = game;
    manager->data = data;
    manager->achievement_count = 0;
    manager->challenge_count = 0;
    manager->total_points = 0;
    manager->completed_achievements = 0;
    manager->completed_challenges = 0;
    manager->unlock_callback = NULL;
    manager->callback_context = NULL;
    
    manager->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    manager->cache = achievement_cache_alloc();
    
    // Standard-Achievements laden
    for(uint32_t i = 0; i < sizeof(default_achievements) / sizeof(Achievement); i++) {
        memcpy(&manager->achievements[i], &default_achievements[i], sizeof(Achievement));
        manager->achievement_count++;
    }
    
    // Update-Thread starten
    manager->running = true;
    manager->update_thread = furi_thread_alloc();
    furi_thread_set_name(manager->update_thread, "Achievement Updates");
    furi_thread_set_stack_size(manager->update_thread, 1024);
    furi_thread_set_context(manager->update_thread, manager);
    furi_thread_set_callback(manager->update_thread, achievement_update_thread);
    furi_thread_start(manager->update_thread);
    
    // Tägliche Challenge erstellen
    achievement_manager_create_daily_challenge(manager);
    
    return manager;
}

void achievement_manager_free(AchievementManager* manager) {
    if(!manager) return;
    
    // Thread beenden
    manager->running = false;
    furi_thread_join(manager->update_thread);
    furi_thread_free(manager->update_thread);
    
    achievement_cache_free(manager->cache);
    furi_mutex_free(manager->mutex);
    free(manager);
}

bool achievement_manager_unlock_achievement(
    AchievementManager* manager,
    uint32_t id
) {
    if(!manager) return false;
    
    Achievement* achievement = NULL;
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    // Achievement suchen
    for(uint32_t i = 0; i < manager->achievement_count; i++) {
        if(manager->achievements[i].id == id) {
            achievement = &manager->achievements[i];
            break;
        }
    }
    
    if(!achievement || achievement->unlocked) {
        furi_mutex_release(manager->mutex);
        return false;
    }
    
    // Achievement freischalten
    achievement->unlocked = true;
    achievement->unlock_time = furi_get_tick();
    manager->total_points += achievement->reward_points;
    manager->completed_achievements++;
    
    // Benachrichtigung
    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
    notification_message(notifications, &sequence_success);
    notification_message(notifications, &sequence_blink_magenta_100);
    furi_record_close(RECORD_NOTIFICATION);
    
    // Callback aufrufen
    if(manager->unlock_callback) {
        manager->unlock_callback(achievement, manager->callback_context);
    }
    
    furi_mutex_release(manager->mutex);
    
    return true;
}

void achievement_manager_update_progress(
    AchievementManager* manager,
    AchievementType type,
    uint32_t value
) {
    if(!manager) return;
    
    // Update in Cache speichern
    achievement_cache_update(manager->cache, type, value);
}

bool achievement_manager_save_progress(AchievementManager* manager) {
    if(!manager || !manager->data) return false;
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    // Achievement-Daten in OfflineData speichern
    memcpy(&manager->data->achievements, manager->achievements,
           sizeof(Achievement) * manager->achievement_count);
    manager->data->achievement_count = manager->achievement_count;
    
    memcpy(&manager->data->challenges, manager->challenges,
           sizeof(Challenge) * manager->challenge_count);
    manager->data->challenge_count = manager->challenge_count;
    
    manager->data->achievement_points = manager->total_points;
    
    bool success = offline_data_save(manager->data);
    
    furi_mutex_release(manager->mutex);
    
    return success;
}

bool achievement_manager_load_progress(AchievementManager* manager) {
    if(!manager || !manager->data) return false;
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    // Achievement-Daten aus OfflineData laden
    memcpy(manager->achievements, &manager->data->achievements,
           sizeof(Achievement) * manager->data->achievement_count);
    manager->achievement_count = manager->data->achievement_count;
    
    memcpy(manager->challenges, &manager->data->challenges,
           sizeof(Challenge) * manager->data->challenge_count);
    manager->challenge_count = manager->data->challenge_count;
    
    manager->total_points = manager->data->achievement_points;
    
    // Completed zählen
    manager->completed_achievements = 0;
    manager->completed_challenges = 0;
    
    for(uint32_t i = 0; i < manager->achievement_count; i++) {
        if(manager->achievements[i].unlocked) {
            manager->completed_achievements++;
        }
    }
    
    for(uint32_t i = 0; i < manager->challenge_count; i++) {
        if(manager->challenges[i].completed) {
            manager->completed_challenges++;
        }
    }
    
    furi_mutex_release(manager->mutex);
    
    return true;
}
