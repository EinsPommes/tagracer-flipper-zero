#pragma once

#include <furi.h>
#include <furi_hal.h>
#include "game_state.h"
#include "offline_data.h"

#define P2P_PACKET_SIZE 64
#define P2P_MAX_PLAYERS 8
#define P2P_CHANNEL 10
#define P2P_TIMEOUT 1000

typedef enum {
    P2pMessageTypeBeacon,
    P2pMessageTypeJoinRequest,
    P2pMessageTypeJoinResponse,
    P2pMessageTypeGameState,
    P2pMessageTypeTagScan,
    P2pMessageTypeChat,
    P2pMessageTypeChallenge,
    P2pMessageTypeSync
} P2pMessageType;

typedef struct {
    uint8_t player_id;
    char name[32];
    uint32_t score;
    uint32_t team_id;
    bool is_host;
} P2pPlayer;

typedef struct {
    P2pMessageType type;
    uint8_t sender_id;
    uint8_t sequence;
    uint32_t timestamp;
    uint8_t data[P2P_PACKET_SIZE - 8];
} P2pMessage;

typedef struct {
    bool enabled;
    uint8_t player_id;
    bool is_host;
    uint32_t game_id;
    P2pPlayer players[P2P_MAX_PLAYERS];
    uint8_t player_count;
    FuriThread* rx_thread;
    FuriThread* tx_thread;
    FuriMutex* mutex;
    void (*message_callback)(P2pMessage* message, void* context);
    void* callback_context;
} P2pManager;

// Hauptfunktionen
P2pManager* p2p_manager_alloc(void);
void p2p_manager_free(P2pManager* manager);

// Verbindungsmanagement
bool p2p_manager_start_host(P2pManager* manager, const char* player_name);
bool p2p_manager_join_game(P2pManager* manager, const char* player_name, uint32_t game_id);
void p2p_manager_stop(P2pManager* manager);

// Nachrichtenversand
bool p2p_manager_send_game_state(P2pManager* manager, GameContext* game);
bool p2p_manager_send_tag_scan(P2pManager* manager, const CachedTagScan* tag);
bool p2p_manager_send_chat(P2pManager* manager, const char* message);
bool p2p_manager_send_challenge(P2pManager* manager, uint8_t target_id, uint32_t challenge_type);

// Spieler-Management
P2pPlayer* p2p_manager_get_player(P2pManager* manager, uint8_t player_id);
uint8_t p2p_manager_get_player_count(P2pManager* manager);
bool p2p_manager_is_host(P2pManager* manager);

// Team-Funktionen
bool p2p_manager_set_team(P2pManager* manager, uint8_t player_id, uint32_t team_id);
uint32_t p2p_manager_get_team_score(P2pManager* manager, uint32_t team_id);

// Callback-Management
void p2p_manager_set_message_callback(
    P2pManager* manager,
    void (*callback)(P2pMessage* message, void* context),
    void* context
);
