#pragma once

#include <furi.h>
#include "tagracer_nfc.h"

typedef enum {
    GameStateIdle,
    GameStateWaitingForStart,
    GameStateRunning,
    GameStateFinished
} GameState;

typedef struct {
    uint32_t score;
    uint32_t time_remaining;
    uint32_t tag_count;
    char player_id[32];
    char last_tag_id[32];
    char status_text[64];
    GameState state;
} GameContext;

void game_state_init(GameContext* context);
void game_state_reset(GameContext* context);
bool game_state_start(GameContext* context);
void game_state_update(GameContext* context);
void game_state_process_tag(GameContext* context, TagData* tag_data);
bool game_state_is_finished(GameContext* context);
