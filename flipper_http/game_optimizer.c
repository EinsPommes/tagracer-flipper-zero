#include "game_optimizer.h"
#include <math.h>
#include <furi_hal_rtc.h>

// Standardkonfiguration
static const PredictionConfig default_config = {
    .position_weight = 0.5f,
    .velocity_weight = 0.3f,
    .time_weight = 0.2f,
    .learning_rate = 0.1f
};

// Optimierungs-Thread
static int32_t optimizer_thread(void* context) {
    GameOptimizer* optimizer = (GameOptimizer*)context;
    
    while(optimizer->running) {
        furi_mutex_acquire(optimizer->mutex, FuriWaitForever);
        
        // Bewegungsvorhersage aktualisieren
        float x, y, confidence;
        if(game_optimizer_predict_movement(optimizer, &x, &y, &confidence)) {
            // Prefetching für nahe Tags
            uint32_t nearby_tags[PREFETCH_DISTANCE];
            size_t tag_count = 0;
            
            // Nächste Tags finden
            if(optimizer->map) {
                map_manager_find_nearby_tags(
                    optimizer->map,
                    x, y,
                    PREFETCH_DISTANCE,
                    nearby_tags,
                    &tag_count
                );
                
                // Tags vorladen
                if(tag_count > 0) {
                    game_optimizer_prefetch_tags(
                        optimizer,
                        nearby_tags,
                        tag_count
                    );
                }
            }
        }
        
        // Cache bereinigen
        uint32_t now = furi_get_tick();
        TagCacheLine* lines = optimizer->tag_cache.lines;
        
        for(uint32_t i = 0; i < optimizer->tag_cache.line_count; i++) {
            if(lines[i].valid && 
               now - lines[i].last_access > OPTIMIZATION_INTERVAL_MS * 10) {
                lines[i].valid = false;
            }
        }
        
        furi_mutex_release(optimizer->mutex);
        furi_delay_ms(OPTIMIZATION_INTERVAL_MS);
    }
    
    return 0;
}

GameOptimizer* game_optimizer_alloc(
    GameContext* game,
    LocationManager* location,
    MapManager* map
) {
    GameOptimizer* optimizer = malloc(sizeof(GameOptimizer));
    
    optimizer->game = game;
    optimizer->location = location;
    optimizer->map = map;
    
    // Bewegungsvorhersage initialisieren
    optimizer->predictor.point_count = 0;
    optimizer->predictor.last_update = 0;
    
    // Tag-Cache initialisieren
    optimizer->tag_cache.line_count = 256; // Power of 2 für schnelles Mapping
    optimizer->tag_cache.lines = malloc(
        sizeof(TagCacheLine) * optimizer->tag_cache.line_count
    );
    optimizer->tag_cache.hits = 0;
    optimizer->tag_cache.misses = 0;
    
    // Cache leeren
    memset(optimizer->tag_cache.lines, 0,
           sizeof(TagCacheLine) * optimizer->tag_cache.line_count);
    
    // Konfiguration setzen
    optimizer->config = default_config;
    
    // Metriken zurücksetzen
    optimizer->prediction_hits = 0;
    optimizer->prediction_misses = 0;
    optimizer->cache_hits = 0;
    optimizer->cache_misses = 0;
    optimizer->avg_prediction_error = 0;
    
    optimizer->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    
    // Optimizer-Thread starten
    optimizer->running = true;
    optimizer->optimizer_thread = furi_thread_alloc();
    furi_thread_set_name(optimizer->optimizer_thread, "Game Optimizer");
    furi_thread_set_stack_size(optimizer->optimizer_thread, 1024);
    furi_thread_set_context(optimizer->optimizer_thread, optimizer);
    furi_thread_set_callback(optimizer->optimizer_thread, optimizer_thread);
    furi_thread_start(optimizer->optimizer_thread);
    
    return optimizer;
}

void game_optimizer_free(GameOptimizer* optimizer) {
    if(!optimizer) return;
    
    // Thread beenden
    optimizer->running = false;
    furi_thread_join(optimizer->optimizer_thread);
    furi_thread_free(optimizer->optimizer_thread);
    
    // Cache freigeben
    free(optimizer->tag_cache.lines);
    
    furi_mutex_free(optimizer->mutex);
    free(optimizer);
}

bool game_optimizer_predict_movement(
    GameOptimizer* optimizer,
    float* next_x,
    float* next_y,
    float* confidence
) {
    if(!optimizer || !next_x || !next_y || !confidence ||
       optimizer->predictor.point_count < 2) {
        return false;
    }
    
    furi_mutex_acquire(optimizer->mutex, FuriWaitForever);
    
    // Letzte zwei Punkte verwenden
    PredictionPoint* p1 = &optimizer->predictor.points[
        optimizer->predictor.point_count - 2
    ];
    PredictionPoint* p2 = &optimizer->predictor.points[
        optimizer->predictor.point_count - 1
    ];
    
    // Zeit seit letztem Update
    uint32_t now = furi_get_tick();
    float dt = (float)(now - p2->timestamp) / 1000.0f;
    
    // Geschwindigkeit und Richtung berechnen
    float dx = p2->x - p1->x;
    float dy = p2->y - p1->y;
    float dist = sqrtf(dx * dx + dy * dy);
    
    if(dist > 0) {
        dx /= dist;
        dy /= dist;
    }
    
    // Nächste Position vorhersagen
    *next_x = p2->x + dx * p2->velocity * dt;
    *next_y = p2->y + dy * p2->velocity * dt;
    
    // Konfidenz berechnen
    float time_factor = expf(-dt / 1.0f); // Exponentieller Abfall
    float velocity_factor = p2->velocity > 0.1f ? 1.0f : 0.5f;
    
    *confidence = time_factor * velocity_factor;
    
    furi_mutex_release(optimizer->mutex);
    return true;
}

void game_optimizer_update_movement(
    GameOptimizer* optimizer,
    float x,
    float y,
    float velocity
) {
    if(!optimizer) return;
    
    furi_mutex_acquire(optimizer->mutex, FuriWaitForever);
    
    // Vorhersage prüfen
    float pred_x, pred_y, confidence;
    if(game_optimizer_predict_movement(optimizer, &pred_x, &pred_y, &confidence)) {
        float error = sqrtf(
            (pred_x - x) * (pred_x - x) +
            (pred_y - y) * (pred_y - y)
        );
        
        // Metriken aktualisieren
        if(error < 1.0f) {
            optimizer->prediction_hits++;
        } else {
            optimizer->prediction_misses++;
        }
        
        // Durchschnittlichen Fehler aktualisieren
        optimizer->avg_prediction_error = 
            optimizer->avg_prediction_error * 0.9f + error * 0.1f;
    }
    
    // Neuen Punkt hinzufügen
    if(optimizer->predictor.point_count >= MAX_PREDICTION_POINTS) {
        // Alte Punkte verschieben
        memmove(&optimizer->predictor.points[0],
                &optimizer->predictor.points[1],
                (MAX_PREDICTION_POINTS - 1) * sizeof(PredictionPoint));
        optimizer->predictor.point_count--;
    }
    
    PredictionPoint* point = &optimizer->predictor.points[
        optimizer->predictor.point_count++
    ];
    
    point->x = x;
    point->y = y;
    point->velocity = velocity;
    point->timestamp = furi_get_tick();
    
    furi_mutex_release(optimizer->mutex);
}

bool game_optimizer_cache_tag(
    GameOptimizer* optimizer,
    uint32_t tag_id,
    const uint8_t* data,
    size_t size
) {
    if(!optimizer || !data || size > CACHE_LINE_SIZE) return false;
    
    furi_mutex_acquire(optimizer->mutex, FuriWaitForever);
    
    // Cache-Line finden
    uint32_t line_index = tag_id % optimizer->tag_cache.line_count;
    TagCacheLine* line = &optimizer->tag_cache.lines[line_index];
    
    // Daten cachen
    memcpy(line->data, data, size);
    line->tag_id = tag_id;
    line->last_access = furi_get_tick();
    line->valid = true;
    
    furi_mutex_release(optimizer->mutex);
    return true;
}

bool game_optimizer_get_cached_tag(
    GameOptimizer* optimizer,
    uint32_t tag_id,
    uint8_t* data,
    size_t* size
) {
    if(!optimizer || !data || !size) return false;
    
    furi_mutex_acquire(optimizer->mutex, FuriWaitForever);
    
    // Cache-Line finden
    uint32_t line_index = tag_id % optimizer->tag_cache.line_count;
    TagCacheLine* line = &optimizer->tag_cache.lines[line_index];
    
    if(line->valid && line->tag_id == tag_id) {
        // Cache-Hit
        memcpy(data, line->data, CACHE_LINE_SIZE);
        *size = CACHE_LINE_SIZE;
        line->last_access = furi_get_tick();
        optimizer->cache_hits++;
        
        furi_mutex_release(optimizer->mutex);
        return true;
    }
    
    // Cache-Miss
    optimizer->cache_misses++;
    
    furi_mutex_release(optimizer->mutex);
    return false;
}

void game_optimizer_prefetch_tags(
    GameOptimizer* optimizer,
    const uint32_t* tag_ids,
    size_t count
) {
    if(!optimizer || !tag_ids || count == 0) return;
    
    furi_mutex_acquire(optimizer->mutex, FuriWaitForever);
    
    // Tags vorladen
    for(size_t i = 0; i < count; i++) {
        uint32_t tag_id = tag_ids[i];
        
        // Prüfen ob Tag bereits im Cache
        uint32_t line_index = tag_id % optimizer->tag_cache.line_count;
        TagCacheLine* line = &optimizer->tag_cache.lines[line_index];
        
        if(!line->valid || line->tag_id != tag_id) {
            // Tag laden und cachen
            uint8_t data[CACHE_LINE_SIZE];
            size_t size = CACHE_LINE_SIZE;
            
            if(optimizer->game && optimizer->game->load_tag_callback) {
                if(optimizer->game->load_tag_callback(
                    tag_id, data, &size,
                    optimizer->game->callback_context)) {
                    game_optimizer_cache_tag(optimizer, tag_id, data, size);
                }
            }
        }
    }
    
    furi_mutex_release(optimizer->mutex);
}
