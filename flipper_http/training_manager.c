#include "training_manager.h"
#include <furi_hal_rtc.h>
#include <notification/notification_messages.h>

// Kalorien pro Minute bei verschiedenen Geschwindigkeiten
static const float calories_per_minute[] = {
    2.0f,  // Gehen (3 km/h)
    3.5f,  // Schnelles Gehen (5 km/h)
    5.0f,  // Joggen (7 km/h)
    7.0f,  // Laufen (9 km/h)
    9.0f   // Schnelles Laufen (11+ km/h)
};

TrainingManager* training_manager_alloc(
    GameContext* game,
    LocationManager* location,
    MapManager* map
) {
    TrainingManager* manager = malloc(sizeof(TrainingManager));
    
    manager->game = game;
    manager->location = location;
    manager->map = map;
    manager->session_count = 0;
    manager->route_count = 0;
    manager->current_session = NULL;
    manager->current_route = NULL;
    
    // Stats initialisieren
    memset(&manager->stats, 0, sizeof(TrainingStats));
    
    manager->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    
    return manager;
}

void training_manager_free(TrainingManager* manager) {
    if(!manager) return;
    
    if(manager->current_session) {
        training_manager_end_session(manager);
    }
    
    furi_mutex_free(manager->mutex);
    free(manager);
}

bool training_manager_start_session(TrainingManager* manager) {
    if(!manager || manager->current_session ||
       manager->session_count >= MAX_TRAINING_SESSIONS) {
        return false;
    }
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    // Neue Session erstellen
    TrainingSession* session = &manager->sessions[manager->session_count];
    session->id = manager->session_count + 1;
    session->start_time = furi_get_tick();
    session->duration = 0;
    session->distance = 0;
    session->avg_speed = 0;
    session->max_speed = 0;
    session->calories = 0;
    session->tags_scanned = 0;
    session->score = 0;
    session->best_combo = 0;
    session->route_completion = 0;
    session->checkpoint_count = 0;
    
    manager->current_session = session;
    
    // Map-Tracking starten
    if(manager->map) {
        char track_name[32];
        snprintf(track_name, sizeof(track_name),
                 "Training_%lu", session->id);
        map_manager_start_tracking(manager->map, track_name);
    }
    
    furi_mutex_release(manager->mutex);
    
    return true;
}

bool training_manager_end_session(TrainingManager* manager) {
    if(!manager || !manager->current_session) return false;
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    TrainingSession* session = manager->current_session;
    
    // Finale Statistiken berechnen
    session->duration = (furi_get_tick() - session->start_time) / 1000;
    
    if(session->duration > 0) {
        session->avg_speed = session->distance / session->duration;
    }
    
    // Route-Completion berechnen
    if(manager->current_route) {
        session->route_completion = training_manager_get_route_progress(manager);
    }
    
    // Map-Tracking beenden
    if(manager->map) {
        map_manager_stop_tracking(manager->map);
    }
    
    // Session speichern
    manager->session_count++;
    manager->current_session = NULL;
    
    // Gesamtstatistik aktualisieren
    training_manager_update_stats(manager);
    
    furi_mutex_release(manager->mutex);
    
    return true;
}

void training_manager_update_session(TrainingManager* manager) {
    if(!manager || !manager->current_session) return;
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    TrainingSession* session = manager->current_session;
    
    // Aktuelle Position abrufen
    LocationInfo location;
    if(location_manager_get_location(manager->location, &location)) {
        // Distanz zum letzten Punkt berechnen
        if(session->checkpoint_count > 0) {
            LocationInfo* last = &session->checkpoints[session->checkpoint_count-1];
            float distance;
            
            if(location_manager_calculate_distance(
                manager->location,
                last->latitude, last->longitude,
                location.latitude, location.longitude,
                &distance)) {
                session->distance += distance;
            }
            
            // Maximale Geschwindigkeit aktualisieren
            if(location.speed > session->max_speed) {
                session->max_speed = location.speed;
            }
        }
        
        // Checkpoint speichern
        if(session->checkpoint_count < MAX_CHECKPOINTS) {
            memcpy(&session->checkpoints[session->checkpoint_count],
                   &location, sizeof(LocationInfo));
            session->checkpoint_count++;
        }
        
        // Kalorien berechnen
        float speed = location.speed * 3.6f; // m/s zu km/h
        float calories = 0;
        
        if(speed < 5.0f) {
            calories = calories_per_minute[0];
        } else if(speed < 7.0f) {
            calories = calories_per_minute[1];
        } else if(speed < 9.0f) {
            calories = calories_per_minute[2];
        } else if(speed < 11.0f) {
            calories = calories_per_minute[3];
        } else {
            calories = calories_per_minute[4];
        }
        
        session->calories += calories / 60.0f; // Pro Sekunde
    }
    
    // Spieldaten aktualisieren
    if(manager->game) {
        session->score = manager->game->score;
        session->tags_scanned = manager->game->tag_count;
        
        if(manager->game->combo_multiplier > session->best_combo) {
            session->best_combo = manager->game->combo_multiplier;
        }
    }
    
    furi_mutex_release(manager->mutex);
}

bool training_manager_create_route(
    TrainingManager* manager,
    const char* name,
    LocationInfo* checkpoints,
    uint32_t count
) {
    if(!manager || !name || !checkpoints || count == 0 ||
       count > MAX_CHECKPOINTS || manager->route_count >= MAX_TRAINING_ROUTES) {
        return false;
    }
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    TrainingRoute* route = &manager->routes[manager->route_count];
    route->id = manager->route_count + 1;
    strncpy(route->name, name, sizeof(route->name)-1);
    
    route->best_time = 0;
    route->best_score = 0;
    route->play_count = 0;
    route->avg_completion = 0;
    route->checkpoint_count = count;
    route->active = true;
    
    memcpy(route->checkpoints, checkpoints,
           count * sizeof(LocationInfo));
    
    manager->route_count++;
    
    furi_mutex_release(manager->mutex);
    
    return true;
}

bool training_manager_analyze_session(
    TrainingManager* manager,
    uint32_t session_id,
    PerformanceAnalysis* analysis
) {
    if(!manager || !analysis) return false;
    
    TrainingSession* session = NULL;
    
    // Session finden
    for(uint32_t i = 0; i < manager->session_count; i++) {
        if(manager->sessions[i].id == session_id) {
            session = &manager->sessions[i];
            break;
        }
    }
    
    if(!session) return false;
    
    // Geschwindigkeitskonsistenz (0-100%)
    float speed_variance = 0;
    float avg_speed = session->avg_speed;
    
    for(uint32_t i = 0; i < session->checkpoint_count; i++) {
        float diff = session->checkpoints[i].speed - avg_speed;
        speed_variance += diff * diff;
    }
    
    if(session->checkpoint_count > 0) {
        speed_variance /= session->checkpoint_count;
    }
    
    analysis->speed_consistency = 100.0f * (1.0f - MIN(1.0f, speed_variance / 100.0f));
    
    // Routen-Optimierung (0-100%)
    if(session->route_completion > 0) {
        analysis->route_optimization = 100.0f * session->route_completion;
    } else {
        analysis->route_optimization = 0;
    }
    
    // Ausdauer (0-100%)
    float expected_dropoff = session->duration / 3600.0f; // Pro Stunde
    float actual_dropoff = 0;
    
    if(session->checkpoint_count >= 2) {
        float start_speed = session->checkpoints[0].speed;
        float end_speed = session->checkpoints[session->checkpoint_count-1].speed;
        actual_dropoff = (start_speed - end_speed) / start_speed;
    }
    
    analysis->stamina_score = 100.0f * (1.0f - MIN(1.0f, actual_dropoff / expected_dropoff));
    
    // Gesamtbewertung (0-100%)
    analysis->overall_rating = (
        analysis->speed_consistency * 0.3f +
        analysis->route_optimization * 0.4f +
        analysis->stamina_score * 0.3f
    );
    
    // Empfehlungen
    char* recommendations = analysis->recommendations;
    size_t size = sizeof(analysis->recommendations);
    
    if(analysis->speed_consistency < 70.0f) {
        strncat(recommendations,
                "Versuche ein gleichmäßigeres Tempo zu halten. ",
                size);
    }
    
    if(analysis->route_optimization < 70.0f) {
        strncat(recommendations,
                "Optimiere deine Route zwischen den Checkpoints. ",
                size);
    }
    
    if(analysis->stamina_score < 70.0f) {
        strncat(recommendations,
                "Arbeite an deiner Ausdauer für längere Sessions. ",
                size);
    }
    
    if(strlen(recommendations) == 0) {
        strncat(recommendations,
                "Großartige Leistung! Versuche neue Routen für mehr Herausforderung.",
                size);
    }
    
    return true;
}
