#pragma once

#include <furi.h>
#include "game_state.h"
#include "location_manager.h"
#include "map_manager.h"

#define OPTIMIZATION_INTERVAL_MS 100
#define MAX_PREDICTION_POINTS 16
#define CACHE_LINE_SIZE 64
#define PREFETCH_DISTANCE 2

typedef struct {
    float x;
    float y;
    float velocity;
    uint32_t timestamp;
} PredictionPoint;

typedef struct {
    PredictionPoint points[MAX_PREDICTION_POINTS];
    uint32_t point_count;
    uint32_t last_update;
} MovementPredictor;

typedef struct {
    uint8_t data[CACHE_LINE_SIZE];
    uint32_t tag_id;
    uint32_t last_access;
    bool valid;
} TagCacheLine;

typedef struct {
    TagCacheLine* lines;
    uint32_t line_count;
    uint32_t hits;
    uint32_t misses;
} TagCache;

typedef struct {
    float position_weight;
    float velocity_weight;
    float time_weight;
    float learning_rate;
} PredictionConfig;

typedef struct {
    GameContext* game;
    LocationManager* location;
    MapManager* map;
    
    MovementPredictor predictor;
    TagCache tag_cache;
    PredictionConfig config;
    
    FuriMutex* mutex;
    FuriThread* optimizer_thread;
    bool running;
    
    // Performance Metriken
    uint32_t prediction_hits;
    uint32_t prediction_misses;
    uint32_t cache_hits;
    uint32_t cache_misses;
    float avg_prediction_error;
} GameOptimizer;

// Hauptfunktionen
GameOptimizer* game_optimizer_alloc(
    GameContext* game,
    LocationManager* location,
    MapManager* map
);
void game_optimizer_free(GameOptimizer* optimizer);

// Bewegungsvorhersage
bool game_optimizer_predict_movement(
    GameOptimizer* optimizer,
    float* next_x,
    float* next_y,
    float* confidence
);

void game_optimizer_update_movement(
    GameOptimizer* optimizer,
    float x,
    float y,
    float velocity
);

// Tag-Caching
bool game_optimizer_cache_tag(
    GameOptimizer* optimizer,
    uint32_t tag_id,
    const uint8_t* data,
    size_t size
);

bool game_optimizer_get_cached_tag(
    GameOptimizer* optimizer,
    uint32_t tag_id,
    uint8_t* data,
    size_t* size
);

// Optimierungskonfiguration
void game_optimizer_set_config(
    GameOptimizer* optimizer,
    const PredictionConfig* config
);

void game_optimizer_get_config(
    GameOptimizer* optimizer,
    PredictionConfig* config
);

// Performance-Analyse
void game_optimizer_get_stats(
    GameOptimizer* optimizer,
    uint32_t* pred_hits,
    uint32_t* pred_misses,
    uint32_t* cache_hits,
    uint32_t* cache_misses,
    float* avg_error
);

// Cache-Management
void game_optimizer_clear_cache(GameOptimizer* optimizer);
void game_optimizer_prefetch_tags(
    GameOptimizer* optimizer,
    const uint32_t* tag_ids,
    size_t count
);
