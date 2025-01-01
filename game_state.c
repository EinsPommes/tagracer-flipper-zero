#include "game_state.h"
#include <furi_hal.h>
#include <notification/notification_messages.h>

#define GAME_DURATION_SEC 300 // 5 Minuten
#define TAG_COOLDOWN_MS 2000 // 2 Sekunden Cooldown zwischen Scans

static uint32_t last_tag_time = 0;

void game_state_init(GameContext* context) {
    context->score = 0;
    context->time_remaining = GAME_DURATION_SEC;
    context->tag_count = 0;
    context->state = GameStateIdle;
    memset(context->player_id, 0, sizeof(context->player_id));
    memset(context->last_tag_id, 0, sizeof(context->last_tag_id));
    snprintf(context->status_text, sizeof(context->status_text), "Bereit zum Start");
}

void game_state_reset(GameContext* context) {
    context->score = 0;
    context->time_remaining = GAME_DURATION_SEC;
    context->tag_count = 0;
    context->state = GameStateIdle;
    memset(context->last_tag_id, 0, sizeof(context->last_tag_id));
    snprintf(context->status_text, sizeof(context->status_text), "Spiel zurückgesetzt");
}

bool game_state_start(GameContext* context) {
    if(context->state != GameStateIdle) {
        return false;
    }
    
    context->state = GameStateRunning;
    context->time_remaining = GAME_DURATION_SEC;
    snprintf(context->status_text, sizeof(context->status_text), "Spiel gestartet!");
    
    // Benachrichtigung: Spiel gestartet
    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
    notification_message(notifications, &sequence_success);
    furi_record_close(RECORD_NOTIFICATION);
    
    return true;
}

void game_state_update(GameContext* context) {
    if(context->state != GameStateRunning) {
        return;
    }
    
    // Zeit aktualisieren
    if(context->time_remaining > 0) {
        context->time_remaining--;
        
        // Warnung bei wenig Zeit
        if(context->time_remaining == 30) { // 30 Sekunden übrig
            NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
            notification_message(notifications, &sequence_warning);
            furi_record_close(RECORD_NOTIFICATION);
            snprintf(context->status_text, sizeof(context->status_text), "Noch 30 Sekunden!");
        }
    } else {
        context->state = GameStateFinished;
        snprintf(context->status_text, sizeof(context->status_text), "Spiel beendet!");
        
        // Benachrichtigung: Spiel beendet
        NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
        notification_message(notifications, &sequence_success);
        furi_record_close(RECORD_NOTIFICATION);
    }
}

void game_state_process_tag(GameContext* context, TagData* tag_data) {
    if(context->state != GameStateRunning) {
        return;
    }
    
    // Prüfe Cooldown
    uint32_t current_time = furi_get_tick();
    if(current_time - last_tag_time < TAG_COOLDOWN_MS) {
        snprintf(context->status_text, sizeof(context->status_text), "Zu schnell! Warte...");
        return;
    }
    
    // Konvertiere Tag-ID in String
    char tag_id[32];
    for(uint8_t i = 0; i < tag_data->uid_len; i++) {
        snprintf(tag_id + (i * 2), 3, "%02X", tag_data->uid[i]);
    }
    
    // Prüfe ob Tag bereits gescannt wurde
    if(strcmp(context->last_tag_id, tag_id) == 0) {
        snprintf(context->status_text, sizeof(context->status_text), "Tag bereits gescannt!");
        NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
        notification_message(notifications, &sequence_error);
        furi_record_close(RECORD_NOTIFICATION);
        return;
    }
    
    // Update Spielzustand
    context->tag_count++;
    strcpy(context->last_tag_id, tag_id);
    last_tag_time = current_time;
    
    // Erfolgsbenachrichtigung
    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
    notification_message(notifications, &sequence_success);
    furi_record_close(RECORD_NOTIFICATION);
    
    snprintf(context->status_text, sizeof(context->status_text), "Tag gescannt! +10");
}

bool game_state_is_finished(GameContext* context) {
    return context->state == GameStateFinished;
}
