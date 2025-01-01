#pragma once

#include <furi.h>
#include "game_state.h"
#include "offline_data.h"
#include "achievement_cache.h"

#define MAX_ACHIEVEMENTS 50
#define MAX_CHALLENGES 20

typedef enum {
    AchievementTypeScore,
    AchievementTypeTags,
    AchievementTypeCombo,
    AchievementTypeDistance,
    AchievementTypePowerUp,
    AchievementTypeTime,
    AchievementTypeWins,
    AchievementTypeStory,
    AchievementTypeSecret
} AchievementType;

typedef struct {
    uint32_t id;
    char name[32];
    char description[128];
    AchievementType type;
    uint32_t requirement;
    uint32_t reward_points;
    bool secret;
    bool unlocked;
    uint32_t unlock_time;
    uint32_t progress;
} Achievement;

typedef enum {
    ChallengeTypeSprint,
    ChallengeTypeRelay,
    ChallengeTypeHunter,
    ChallengeTypeExplorer,
    ChallengeTypeCollector,
    ChallengeTypeCombo,
    ChallengeTypeScore,
    ChallengeTypeCustom
} ChallengeType;

typedef struct {
    uint32_t id;
    char name[32];
    char description[128];
    ChallengeType type;
    uint32_t start_time;
    uint32_t end_time;
    uint32_t target_score;
    uint32_t reward_points;
    uint32_t reward_achievement;
    bool completed;
    uint32_t best_score;
    uint32_t current_score;
} Challenge;

typedef struct {
    Achievement achievements[MAX_ACHIEVEMENTS];
    uint32_t achievement_count;
    
    Challenge challenges[MAX_CHALLENGES];
    uint32_t challenge_count;
    
    uint32_t total_points;
    uint32_t completed_achievements;
    uint32_t completed_challenges;
    
    GameContext* game;
    OfflineData* data;
    AchievementCache* cache;
    FuriMutex* mutex;
    FuriThread* update_thread;
    bool running;
    
    void (*unlock_callback)(Achievement* achievement, void* context);
    void* callback_context;
} AchievementManager;

// Hauptfunktionen
AchievementManager* achievement_manager_alloc(GameContext* game, OfflineData* data);
void achievement_manager_free(AchievementManager* manager);

// Achievement-Funktionen
bool achievement_manager_add_achievement(
    AchievementManager* manager,
    const char* name,
    const char* description,
    AchievementType type,
    uint32_t requirement,
    uint32_t reward_points,
    bool secret
);

Achievement* achievement_manager_get_achievement(
    AchievementManager* manager,
    uint32_t id
);

bool achievement_manager_unlock_achievement(
    AchievementManager* manager,
    uint32_t id
);

void achievement_manager_update_progress(
    AchievementManager* manager,
    AchievementType type,
    uint32_t value
);

// Challenge-Funktionen
bool achievement_manager_add_challenge(
    AchievementManager* manager,
    const char* name,
    const char* description,
    ChallengeType type,
    uint32_t duration,
    uint32_t target_score,
    uint32_t reward_points
);

Challenge* achievement_manager_get_challenge(
    AchievementManager* manager,
    uint32_t id
);

bool achievement_manager_complete_challenge(
    AchievementManager* manager,
    uint32_t id
);

void achievement_manager_update_challenge(
    AchievementManager* manager,
    uint32_t id,
    uint32_t score
);

// Tägliche/Wöchentliche Challenges
bool achievement_manager_create_daily_challenge(AchievementManager* manager);
bool achievement_manager_create_weekly_challenge(AchievementManager* manager);
void achievement_manager_check_challenges(AchievementManager* manager);

// Statistik und Fortschritt
uint32_t achievement_manager_get_total_points(AchievementManager* manager);
float achievement_manager_get_completion_rate(AchievementManager* manager);
void achievement_manager_get_stats(
    AchievementManager* manager,
    uint32_t* completed,
    uint32_t* total,
    uint32_t* points
);

// Callback-Management
void achievement_manager_set_unlock_callback(
    AchievementManager* manager,
    void (*callback)(Achievement* achievement, void* context),
    void* context
);

// Speichern/Laden
bool achievement_manager_save_progress(AchievementManager* manager);
bool achievement_manager_load_progress(AchievementManager* manager);
