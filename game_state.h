#pragma once

#include <furi.h>
#include "tagracer_nfc.h"

typedef enum {
    GameStateIdle,
    GameStateWaitingForStart,
    GameStateRunning,
    GameStateFinished
} GameState;

typedef enum {
    GameModeClassic,    // Standard-Modus: Punkte für jeden Tag
    GameModeRelay,      // Staffel: Tags müssen in bestimmter Reihenfolge gescannt werden
    GameModeCapture,    // Capture the Flag: Teams kämpfen um Tag-Kontrolle
    GameModeSprint,     // Sprint: Wer scannt am schnellsten alle Tags?
    GameModeHunter      // Jäger: Ein Spieler versteckt Tags, andere suchen
} GameMode;

typedef struct {
    uint32_t score;
    uint32_t time_remaining;
    uint32_t tag_count;
    uint32_t combo_multiplier;  // Multiplikator für Combos
    uint32_t team_id;          // Team-ID für Team-Modi
    char player_id[32];
    char last_tag_id[32];
    char status_text[64];
    GameState state;
    GameMode mode;
    bool power_ups_active[4];  // Aktive Power-ups
} GameContext;

// Power-up IDs
#define POWERUP_DOUBLE_POINTS 0
#define POWERUP_SPEED_BOOST  1
#define POWERUP_SHIELD       2
#define POWERUP_RADAR        3

void game_state_init(GameContext* context);
void game_state_reset(GameContext* context);
bool game_state_start(GameContext* context, GameMode mode);
void game_state_update(GameContext* context);
void game_state_process_tag(GameContext* context, TagData* tag_data);
bool game_state_is_finished(GameContext* context);

// Neue Funktionen
void game_state_activate_power_up(GameContext* context, uint8_t power_up_id);
void game_state_update_combo(GameContext* context, uint32_t time_since_last_tag);
uint32_t game_state_calculate_points(GameContext* context, uint32_t base_points);
bool game_state_check_relay_sequence(GameContext* context, const char* tag_id);
void game_state_update_territory(GameContext* context, const char* tag_id);
bool game_state_radar_ping(GameContext* context, float* distance, float* angle);
