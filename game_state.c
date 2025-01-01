#include "game_state.h"
#include <furi_hal.h>
#include <notification/notification_messages.h>
#include <math.h>

#define GAME_DURATION_SEC 300
#define TAG_COOLDOWN_MS 2000
#define COMBO_TIMEOUT_MS 5000
#define MAX_COMBO_MULTIPLIER 8
#define POWER_UP_DURATION_MS 30000

static uint32_t last_tag_time = 0;
static uint32_t power_up_end_times[4] = {0};

void game_state_init(GameContext* context) {
    context->score = 0;
    context->time_remaining = GAME_DURATION_SEC;
    context->tag_count = 0;
    context->combo_multiplier = 1;
    context->team_id = 0;
    context->state = GameStateIdle;
    context->mode = GameModeClassic;
    memset(context->player_id, 0, sizeof(context->player_id));
    memset(context->last_tag_id, 0, sizeof(context->last_tag_id));
    memset(context->power_ups_active, 0, sizeof(context->power_ups_active));
    snprintf(context->status_text, sizeof(context->status_text), "Bereit zum Start");
}

void game_state_reset(GameContext* context) {
    context->score = 0;
    context->time_remaining = GAME_DURATION_SEC;
    context->tag_count = 0;
    context->combo_multiplier = 1;
    context->state = GameStateIdle;
    memset(context->last_tag_id, 0, sizeof(context->last_tag_id));
    memset(context->power_ups_active, 0, sizeof(context->power_ups_active));
    snprintf(context->status_text, sizeof(context->status_text), "Spiel zurückgesetzt");
}

bool game_state_start(GameContext* context, GameMode mode) {
    if(context->state != GameStateIdle) {
        return false;
    }
    
    context->state = GameStateRunning;
    context->mode = mode;
    context->time_remaining = GAME_DURATION_SEC;
    
    // Modus-spezifische Initialisierung
    switch(mode) {
        case GameModeRelay:
            snprintf(context->status_text, sizeof(context->status_text), "Staffel gestartet!");
            break;
        case GameModeCapture:
            snprintf(context->status_text, sizeof(context->status_text), "Capture gestartet!");
            break;
        case GameModeSprint:
            context->time_remaining = 120; // 2 Minuten für Sprint
            snprintf(context->status_text, sizeof(context->status_text), "Sprint gestartet!");
            break;
        case GameModeHunter:
            context->time_remaining = 600; // 10 Minuten für Hunter
            snprintf(context->status_text, sizeof(context->status_text), "Jagd beginnt!");
            break;
        default:
            snprintf(context->status_text, sizeof(context->status_text), "Spiel gestartet!");
            break;
    }
    
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
        
        // Power-ups aktualisieren
        uint32_t current_time = furi_get_tick();
        for(int i = 0; i < 4; i++) {
            if(context->power_ups_active[i] && current_time >= power_up_end_times[i]) {
                context->power_ups_active[i] = false;
                NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
                notification_message(notifications, &sequence_error);
                furi_record_close(RECORD_NOTIFICATION);
            }
        }
        
        // Modus-spezifische Updates
        switch(context->mode) {
            case GameModeCapture:
                // Territorium alle 10 Sekunden neu berechnen
                if(context->time_remaining % 10 == 0) {
                    game_state_update_territory(context, NULL);
                }
                break;
            case GameModeSprint:
                // Bonus für schnelles Scannen
                if(context->time_remaining > 60) {
                    context->score += context->combo_multiplier;
                }
                break;
            default:
                break;
        }
        
        // Zeitwarnungen
        if(context->time_remaining == 30) {
            NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
            notification_message(notifications, &sequence_warning);
            furi_record_close(RECORD_NOTIFICATION);
            snprintf(context->status_text, sizeof(context->status_text), "Noch 30 Sekunden!");
        }
    } else {
        context->state = GameStateFinished;
        snprintf(context->status_text, sizeof(context->status_text), "Spiel beendet!");
        
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
    
    // Tag-ID konvertieren
    char tag_id[32];
    for(uint8_t i = 0; i < tag_data->uid_len; i++) {
        snprintf(tag_id + (i * 2), 3, "%02X", tag_data->uid[i]);
    }
    
    // Modus-spezifische Verarbeitung
    uint32_t points = 10;
    bool valid_scan = true;
    
    switch(context->mode) {
        case GameModeRelay:
            valid_scan = game_state_check_relay_sequence(context, tag_id);
            points = valid_scan ? 20 : 0;
            break;
            
        case GameModeCapture:
            game_state_update_territory(context, tag_id);
            points = 15;
            break;
            
        case GameModeSprint:
            points = 10 + (context->time_remaining / 2);
            break;
            
        case GameModeHunter:
            // Verstecker bekommt Punkte für nicht gefundene Tags
            if(context->team_id == 0) {
                points = 5;
            } else {
                points = 25;
            }
            break;
            
        default:
            // Klassischer Modus
            if(strcmp(context->last_tag_id, tag_id) == 0) {
                snprintf(context->status_text, sizeof(context->status_text), "Tag bereits gescannt!");
                NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
                notification_message(notifications, &sequence_error);
                furi_record_close(RECORD_NOTIFICATION);
                return;
            }
            break;
    }
    
    if(valid_scan) {
        // Combo aktualisieren
        game_state_update_combo(context, current_time - last_tag_time);
        
        // Punkte berechnen und vergeben
        uint32_t final_points = game_state_calculate_points(context, points);
        context->score += final_points;
        context->tag_count++;
        strcpy(context->last_tag_id, tag_id);
        last_tag_time = current_time;
        
        // Erfolgsbenachrichtigung
        NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
        notification_message(notifications, &sequence_success);
        furi_record_close(RECORD_NOTIFICATION);
        
        snprintf(context->status_text, sizeof(context->status_text), 
                "+%ld (x%ld)", final_points, context->combo_multiplier);
    }
}

void game_state_activate_power_up(GameContext* context, uint8_t power_up_id) {
    if(power_up_id >= 4) return;
    
    context->power_ups_active[power_up_id] = true;
    power_up_end_times[power_up_id] = furi_get_tick() + POWER_UP_DURATION_MS;
    
    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
    notification_message(notifications, &sequence_success);
    furi_record_close(RECORD_NOTIFICATION);
    
    switch(power_up_id) {
        case POWERUP_DOUBLE_POINTS:
            snprintf(context->status_text, sizeof(context->status_text), "2x Punkte aktiv!");
            break;
        case POWERUP_SPEED_BOOST:
            snprintf(context->status_text, sizeof(context->status_text), "Speed Boost!");
            break;
        case POWERUP_SHIELD:
            snprintf(context->status_text, sizeof(context->status_text), "Schild aktiv!");
            break;
        case POWERUP_RADAR:
            snprintf(context->status_text, sizeof(context->status_text), "Radar aktiviert!");
            break;
    }
}

void game_state_update_combo(GameContext* context, uint32_t time_since_last_tag) {
    if(time_since_last_tag < COMBO_TIMEOUT_MS) {
        context->combo_multiplier = MIN(context->combo_multiplier + 1, MAX_COMBO_MULTIPLIER);
    } else {
        context->combo_multiplier = 1;
    }
}

uint32_t game_state_calculate_points(GameContext* context, uint32_t base_points) {
    uint32_t points = base_points;
    
    // Combo-Multiplikator anwenden
    points *= context->combo_multiplier;
    
    // Power-up Effekte
    if(context->power_ups_active[POWERUP_DOUBLE_POINTS]) {
        points *= 2;
    }
    
    return points;
}

bool game_state_check_relay_sequence(GameContext* context, const char* tag_id) {
    // TODO: Implementiere Sequenz-Prüfung für Relay-Modus
    return true;
}

void game_state_update_territory(GameContext* context, const char* tag_id) {
    // TODO: Implementiere Territorium-Update für Capture-Modus
}

bool game_state_radar_ping(GameContext* context, float* distance, float* angle) {
    if(!context->power_ups_active[POWERUP_RADAR]) {
        return false;
    }
    
    // TODO: Implementiere Radar-Funktionalität
    *distance = 0.0f;
    *angle = 0.0f;
    return true;
}

bool game_state_is_finished(GameContext* context) {
    return context->state == GameStateFinished;
}
