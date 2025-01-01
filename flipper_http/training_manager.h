#pragma once

#include <furi.h>
#include "game_state.h"
#include "location_manager.h"
#include "map_manager.h"

#define MAX_TRAINING_SESSIONS 100
#define MAX_TRAINING_ROUTES 20
#define MAX_CHECKPOINTS 10

typedef struct {
    uint32_t id;
    uint32_t start_time;
    uint32_t duration;
    uint32_t distance;
    uint32_t avg_speed;
    uint32_t max_speed;
    uint32_t calories;
    uint32_t tags_scanned;
    uint32_t score;
    uint32_t best_combo;
    float route_completion;
    LocationInfo checkpoints[MAX_CHECKPOINTS];
    uint32_t checkpoint_count;
} TrainingSession;

typedef struct {
    uint32_t id;
    char name[32];
    uint32_t best_time;
    uint32_t best_score;
    uint32_t play_count;
    float avg_completion;
    LocationInfo checkpoints[MAX_CHECKPOINTS];
    uint32_t checkpoint_count;
    bool active;
} TrainingRoute;

typedef struct {
    uint32_t total_sessions;
    uint32_t total_distance;
    uint32_t total_duration;
    uint32_t total_calories;
    uint32_t total_tags;
    uint32_t best_score;
    uint32_t best_combo;
    float avg_speed;
    float max_speed;
    uint32_t favorite_route;
} TrainingStats;

typedef struct {
    TrainingSession sessions[MAX_TRAINING_SESSIONS];
    uint32_t session_count;
    
    TrainingRoute routes[MAX_TRAINING_ROUTES];
    uint32_t route_count;
    
    TrainingStats stats;
    
    TrainingSession* current_session;
    TrainingRoute* current_route;
    
    GameContext* game;
    LocationManager* location;
    MapManager* map;
    FuriMutex* mutex;
} TrainingManager;

// Hauptfunktionen
TrainingManager* training_manager_alloc(
    GameContext* game,
    LocationManager* location,
    MapManager* map
);
void training_manager_free(TrainingManager* manager);

// Session-Management
bool training_manager_start_session(TrainingManager* manager);
bool training_manager_end_session(TrainingManager* manager);
bool training_manager_pause_session(TrainingManager* manager);
bool training_manager_resume_session(TrainingManager* manager);
void training_manager_update_session(TrainingManager* manager);

// Routen-Management
bool training_manager_create_route(
    TrainingManager* manager,
    const char* name,
    LocationInfo* checkpoints,
    uint32_t count
);
bool training_manager_start_route(TrainingManager* manager, uint32_t route_id);
bool training_manager_complete_checkpoint(TrainingManager* manager);
float training_manager_get_route_progress(TrainingManager* manager);

// Statistik-Funktionen
void training_manager_update_stats(TrainingManager* manager);
bool training_manager_get_session_stats(
    TrainingManager* manager,
    uint32_t session_id,
    TrainingSession* session
);
bool training_manager_get_route_stats(
    TrainingManager* manager,
    uint32_t route_id,
    TrainingRoute* route
);
void training_manager_get_total_stats(
    TrainingManager* manager,
    TrainingStats* stats
);

// Leistungsanalyse
typedef struct {
    float speed_consistency;
    float route_optimization;
    float stamina_score;
    float overall_rating;
    char recommendations[256];
} PerformanceAnalysis;

bool training_manager_analyze_session(
    TrainingManager* manager,
    uint32_t session_id,
    PerformanceAnalysis* analysis
);
bool training_manager_compare_sessions(
    TrainingManager* manager,
    uint32_t session1_id,
    uint32_t session2_id,
    char* comparison,
    size_t size
);

// Export/Import
bool training_manager_export_session(
    TrainingManager* manager,
    uint32_t session_id,
    const char* filename
);
bool training_manager_import_session(
    TrainingManager* manager,
    const char* filename
);
bool training_manager_export_route(
    TrainingManager* manager,
    uint32_t route_id,
    const char* filename
);
bool training_manager_import_route(
    TrainingManager* manager,
    const char* filename
);
