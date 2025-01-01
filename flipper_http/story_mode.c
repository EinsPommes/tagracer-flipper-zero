#include "game_modes.h"
#include <furi_hal.h>
#include <notification/notification_messages.h>

// Story-Kapitel Definitionen
static const StoryChapter default_chapters[] = {
    {
        .id = 1,
        .title = "Der Anfang",
        .description = "Entdecke die Grundlagen des Tag Racing...",
        .required_tags = 5,
        .required_score = 100,
        .next_chapter = 2,
        .unlocked = true
    },
    {
        .id = 2,
        .title = "Power-Up Training",
        .description = "Lerne die verschiedenen Power-Ups kennen...",
        .required_tags = 8,
        .required_score = 200,
        .next_chapter = 3
    },
    {
        .id = 3,
        .title = "Team Challenge",
        .description = "Arbeite mit anderen Spielern zusammen...",
        .required_tags = 12,
        .required_score = 500,
        .next_chapter = 4
    },
    {
        .id = 4,
        .title = "Der Meister-Test",
        .description = "Beweise dein Können in der finalen Prüfung...",
        .required_tags = 15,
        .required_score = 1000,
        .next_chapter = 0
    }
};

bool story_mode_init(GameContext* game, StoryProgress* progress) {
    if(!game || !progress) return false;
    
    // Progress initialisieren
    progress->chapter_count = sizeof(default_chapters) / sizeof(StoryChapter);
    memcpy(progress->chapters, default_chapters, sizeof(default_chapters));
    progress->current_chapter = 1;
    progress->total_score = 0;
    progress->new_content_available = false;
    
    // Spiel-Kontext für Story-Modus konfigurieren
    game->mode = GameModeStory;
    game->state = GameStateIdle;
    game->score = 0;
    game->combo_multiplier = 1;
    
    // Story-spezifische Einstellungen
    game->time_remaining = 0; // Kein Zeitlimit im Story-Modus
    snprintf(game->status_text, sizeof(game->status_text),
        "Story-Modus: Kapitel %lu", progress->current_chapter);
    
    return true;
}

bool story_mode_update(GameContext* game, StoryProgress* progress) {
    if(!game || !progress) return false;
    
    StoryChapter* current = NULL;
    
    // Aktuelles Kapitel finden
    for(uint32_t i = 0; i < progress->chapter_count; i++) {
        if(progress->chapters[i].id == progress->current_chapter) {
            current = &progress->chapters[i];
            break;
        }
    }
    
    if(!current) return false;
    
    // Fortschritt prüfen
    if(game->tag_count >= current->required_tags &&
       game->score >= current->required_score &&
       !current->completed) {
        // Kapitel abschließen
        current->completed = true;
        
        // Nächstes Kapitel freischalten
        if(current->next_chapter > 0) {
            for(uint32_t i = 0; i < progress->chapter_count; i++) {
                if(progress->chapters[i].id == current->next_chapter) {
                    progress->chapters[i].unlocked = true;
                    break;
                }
            }
        }
        
        // Belohnung vergeben
        game->score += 500; // Bonus für Kapitel-Abschluss
        progress->total_score += game->score;
        
        // Benachrichtigung
        NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
        notification_message(notifications, &sequence_success);
        notification_message(notifications, &sequence_success);
        furi_record_close(RECORD_NOTIFICATION);
        
        snprintf(game->status_text, sizeof(game->status_text),
            "Kapitel %lu abgeschlossen!", current->id);
        
        return true;
    }
    
    // Status aktualisieren
    snprintf(game->status_text, sizeof(game->status_text),
        "Tags: %lu/%lu Score: %lu/%lu",
        game->tag_count, current->required_tags,
        game->score, current->required_score);
    
    return true;
}

bool story_mode_process_tag(GameContext* game, StoryProgress* progress, TagConfig* tag) {
    if(!game || !progress || !tag) return false;
    
    StoryChapter* current = NULL;
    
    // Aktuelles Kapitel finden
    for(uint32_t i = 0; i < progress->chapter_count; i++) {
        if(progress->chapters[i].id == progress->current_chapter) {
            current = &progress->chapters[i];
            break;
        }
    }
    
    if(!current) return false;
    
    // Tag-spezifische Aktionen
    uint32_t base_points = tag->points;
    
    switch(tag->type) {
        case TagTypeStory:
            // Story-Tag gefunden
            if(tag->custom_data == current->id) {
                // Korrekter Story-Tag für aktuelles Kapitel
                base_points *= 2;
                
                // Story-Element freischalten
                // TODO: Story-Dialog oder Animation abspielen
            }
            break;
            
        case TagTypePowerUp:
            if(current->id >= 2) { // Power-Ups erst ab Kapitel 2
                game_state_activate_power_up(game, tag->power_up_id);
            }
            break;
            
        case TagTypeCheckpoint:
            if(current->id >= 3) { // Checkpoints ab Kapitel 3
                if(tag->checkpoint_id == game->tag_count + 1) {
                    base_points *= 1.5;
                }
            }
            break;
            
        default:
            break;
    }
    
    // Punkte berechnen und vergeben
    uint32_t points = game_state_calculate_points(game, base_points);
    game->score += points;
    game->tag_count++;
    
    // Combo aktualisieren
    game_state_update_combo(game, furi_get_tick());
    
    // Story-Fortschritt aktualisieren
    story_mode_update(game, progress);
    
    return true;
}

bool story_mode_complete_chapter(GameContext* game, StoryProgress* progress) {
    if(!game || !progress) return false;
    
    StoryChapter* current = NULL;
    
    // Aktuelles Kapitel finden und als abgeschlossen markieren
    for(uint32_t i = 0; i < progress->chapter_count; i++) {
        if(progress->chapters[i].id == progress->current_chapter) {
            current = &progress->chapters[i];
            current->completed = true;
            
            // Nächstes Kapitel freischalten wenn vorhanden
            if(current->next_chapter > 0) {
                for(uint32_t j = 0; j < progress->chapter_count; j++) {
                    if(progress->chapters[j].id == current->next_chapter) {
                        progress->chapters[j].unlocked = true;
                        progress->current_chapter = current->next_chapter;
                        break;
                    }
                }
            } else {
                // Story-Modus komplett abgeschlossen
                snprintf(game->status_text, sizeof(game->status_text),
                    "Story-Modus abgeschlossen!");
            }
            
            break;
        }
    }
    
    return current != NULL;
}

bool story_mode_save_progress(StoryProgress* progress, OfflineData* data) {
    if(!progress || !data) return false;
    
    // Progress in OfflineData speichern
    memcpy(&data->story_progress, progress, sizeof(StoryProgress));
    data->needs_sync = true;
    
    return offline_data_save(data);
}

bool story_mode_load_progress(StoryProgress* progress, OfflineData* data) {
    if(!progress || !data) return false;
    
    // Progress aus OfflineData laden
    memcpy(progress, &data->story_progress, sizeof(StoryProgress));
    
    return true;
}
