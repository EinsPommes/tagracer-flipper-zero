#pragma once

#include "game_state.h"
#include "offline_data.h"
#include "tag_manager.h"
#include "location_manager.h"

// Story-Modus Strukturen
typedef struct {
    uint32_t id;
    char title[64];
    char description[256];
    bool unlocked;
    bool completed;
    uint32_t required_tags;
    uint32_t required_score;
    uint32_t next_chapter;
} StoryChapter;

typedef struct {
    uint32_t chapter_count;
    StoryChapter chapters[20];
    uint32_t current_chapter;
    uint32_t total_score;
    bool new_content_available;
} StoryProgress;

// Challenge-Strukturen
typedef struct {
    uint32_t id;
    char name[64];
    char description[128];
    uint32_t required_score;
    uint32_t time_limit;
    uint32_t tag_count;
    bool team_based;
    bool location_based;
    uint32_t reward_points;
    uint32_t reward_achievement;
} Challenge;

typedef struct {
    uint32_t challenge_count;
    Challenge challenges[50];
    uint32_t completed_challenges;
    uint32_t daily_progress;
    uint32_t weekly_progress;
} ChallengeProgress;

// Custom Game Rules
typedef struct {
    bool teams_enabled;
    bool power_ups_enabled;
    bool time_limit_enabled;
    bool location_required;
    uint32_t min_players;
    uint32_t max_players;
    uint32_t win_score;
    uint32_t time_limit;
    float play_area_radius;
    uint32_t tag_multiplier;
    uint32_t combo_multiplier;
    uint32_t power_up_frequency;
} GameRules;

// Training-Modus
typedef struct {
    uint32_t session_id;
    uint32_t start_time;
    uint32_t duration;
    uint32_t tags_scanned;
    uint32_t distance_covered;
    uint32_t average_speed;
    uint32_t best_combo;
    uint32_t calories_burned;
    LocationInfo checkpoints[10];
} TrainingSession;

typedef struct {
    uint32_t total_sessions;
    uint32_t total_distance;
    uint32_t total_time;
    uint32_t best_score;
    uint32_t average_speed;
    uint32_t favorite_routes[5];
} TrainingStats;

// Hauptfunktionen für Story-Modus
bool story_mode_init(GameContext* game, StoryProgress* progress);
bool story_mode_update(GameContext* game, StoryProgress* progress);
bool story_mode_process_tag(GameContext* game, StoryProgress* progress, TagConfig* tag);
bool story_mode_complete_chapter(GameContext* game, StoryProgress* progress);
bool story_mode_save_progress(StoryProgress* progress, OfflineData* data);
bool story_mode_load_progress(StoryProgress* progress, OfflineData* data);

// Challenge-Funktionen
bool challenge_mode_init(GameContext* game, ChallengeProgress* progress);
bool challenge_mode_start_challenge(GameContext* game, uint32_t challenge_id);
bool challenge_mode_update(GameContext* game, ChallengeProgress* progress);
bool challenge_mode_complete(GameContext* game, ChallengeProgress* progress);
bool challenge_mode_get_daily(Challenge* challenge);
bool challenge_mode_get_weekly(Challenge* challenge);

// Custom Game Funktionen
bool custom_game_create(GameContext* game, const GameRules* rules);
bool custom_game_start(GameContext* game);
bool custom_game_update(GameContext* game);
bool custom_game_end(GameContext* game);
bool custom_game_save_rules(const GameRules* rules, const char* name);
bool custom_game_load_rules(GameRules* rules, const char* name);

// Training-Funktionen
bool training_mode_start(GameContext* game, TrainingSession* session);
bool training_mode_update(GameContext* game, TrainingSession* session);
bool training_mode_end(GameContext* game, TrainingSession* session);
bool training_mode_save_session(TrainingSession* session, OfflineData* data);
bool training_mode_get_stats(TrainingStats* stats, OfflineData* data);
bool training_mode_analyze_performance(TrainingSession* session, TrainingStats* stats);

// Statistik-Funktionen
typedef struct {
    uint32_t games_played;
    uint32_t total_score;
    uint32_t best_score;
    uint32_t total_tags;
    uint32_t unique_tags;
    uint32_t power_ups_used;
    uint32_t distance_covered;
    uint32_t time_played;
    uint32_t achievements_unlocked;
    float win_rate;
} PlayerStats;

bool stats_update(PlayerStats* stats, GameContext* game);
bool stats_save(PlayerStats* stats, OfflineData* data);
bool stats_load(PlayerStats* stats, OfflineData* data);
bool stats_generate_report(PlayerStats* stats, const char* filename);
