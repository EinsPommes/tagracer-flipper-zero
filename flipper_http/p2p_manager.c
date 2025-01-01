#include "p2p_manager.h"
#include <furi_hal_random.h>
#include <notification/notification_messages.h>

#define P2P_PROTOCOL_VERSION 1
#define P2P_MAX_RETRIES 3
#define P2P_BEACON_INTERVAL 1000
#define P2P_SYNC_INTERVAL 5000

typedef struct {
    uint8_t protocol_version;
    uint32_t game_id;
    uint8_t player_count;
    uint32_t timestamp;
} P2pBeacon;

typedef struct {
    uint8_t player_id;
    char name[32];
    uint32_t game_id;
} P2pJoinRequest;

typedef struct {
    uint8_t accepted;
    uint8_t assigned_id;
    uint32_t game_state;
} P2pJoinResponse;

static int32_t p2p_rx_thread(void* context);
static int32_t p2p_tx_thread(void* context);
static void p2p_process_message(P2pManager* manager, P2pMessage* message);
static bool p2p_send_message(P2pManager* manager, P2pMessage* message);

P2pManager* p2p_manager_alloc(void) {
    P2pManager* manager = malloc(sizeof(P2pManager));
    
    manager->enabled = false;
    manager->player_id = 0;
    manager->is_host = false;
    manager->game_id = 0;
    manager->player_count = 0;
    manager->message_callback = NULL;
    manager->callback_context = NULL;
    
    manager->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    
    // RX Thread erstellen
    manager->rx_thread = furi_thread_alloc_ex(
        "P2pRx",
        2048,
        p2p_rx_thread,
        manager
    );
    
    // TX Thread erstellen
    manager->tx_thread = furi_thread_alloc_ex(
        "P2pTx",
        2048,
        p2p_tx_thread,
        manager
    );
    
    return manager;
}

void p2p_manager_free(P2pManager* manager) {
    if(!manager) return;
    
    if(manager->enabled) {
        p2p_manager_stop(manager);
    }
    
    furi_thread_free(manager->rx_thread);
    furi_thread_free(manager->tx_thread);
    furi_mutex_free(manager->mutex);
    
    free(manager);
}

bool p2p_manager_start_host(P2pManager* manager, const char* player_name) {
    if(!manager || !player_name) return false;
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    manager->enabled = true;
    manager->is_host = true;
    manager->player_id = 0; // Host ist immer ID 0
    manager->game_id = furi_hal_random_get();
    
    // Host als ersten Spieler hinzufügen
    P2pPlayer* host = &manager->players[0];
    strncpy(host->name, player_name, sizeof(host->name)-1);
    host->player_id = 0;
    host->score = 0;
    host->team_id = 0;
    host->is_host = true;
    manager->player_count = 1;
    
    // Radio initialisieren
    // TODO: Radio-Initialisierung
    
    furi_mutex_release(manager->mutex);
    
    // Threads starten
    furi_thread_start(manager->rx_thread);
    furi_thread_start(manager->tx_thread);
    
    return true;
}

bool p2p_manager_join_game(P2pManager* manager, const char* player_name, uint32_t game_id) {
    if(!manager || !player_name) return false;
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    manager->enabled = true;
    manager->is_host = false;
    manager->game_id = game_id;
    
    // Join-Request senden
    P2pMessage msg = {
        .type = P2pMessageTypeJoinRequest,
        .sender_id = 0xFF, // Noch keine ID
        .sequence = 0,
        .timestamp = furi_get_tick()
    };
    
    P2pJoinRequest* req = (P2pJoinRequest*)msg.data;
    req->game_id = game_id;
    strncpy(req->name, player_name, sizeof(req->name)-1);
    
    furi_mutex_release(manager->mutex);
    
    // Auf Antwort warten
    // TODO: Timeout implementieren
    
    return true;
}

void p2p_manager_stop(P2pManager* manager) {
    if(!manager || !manager->enabled) return;
    
    manager->enabled = false;
    
    // Threads stoppen
    furi_thread_join(manager->rx_thread);
    furi_thread_join(manager->tx_thread);
    
    // Radio deaktivieren
    // TODO: Radio-Deinitialisierung
}

bool p2p_manager_send_game_state(P2pManager* manager, GameContext* game) {
    if(!manager || !game) return false;
    
    P2pMessage msg = {
        .type = P2pMessageTypeGameState,
        .sender_id = manager->player_id,
        .sequence = 0,
        .timestamp = furi_get_tick()
    };
    
    // Game State in Nachricht packen
    memcpy(msg.data, game, sizeof(GameContext));
    
    return p2p_send_message(manager, &msg);
}

bool p2p_manager_send_tag_scan(P2pManager* manager, const CachedTagScan* tag) {
    if(!manager || !tag) return false;
    
    P2pMessage msg = {
        .type = P2pMessageTypeTagScan,
        .sender_id = manager->player_id,
        .sequence = 0,
        .timestamp = furi_get_tick()
    };
    
    // Tag-Daten in Nachricht packen
    memcpy(msg.data, tag, sizeof(CachedTagScan));
    
    return p2p_send_message(manager, &msg);
}

bool p2p_manager_send_chat(P2pManager* manager, const char* message) {
    if(!manager || !message) return false;
    
    P2pMessage msg = {
        .type = P2pMessageTypeChat,
        .sender_id = manager->player_id,
        .sequence = 0,
        .timestamp = furi_get_tick()
    };
    
    // Chat-Nachricht kopieren
    strncpy((char*)msg.data, message, sizeof(msg.data)-1);
    
    return p2p_send_message(manager, &msg);
}

static int32_t p2p_rx_thread(void* context) {
    P2pManager* manager = (P2pManager*)context;
    P2pMessage msg;
    
    while(manager->enabled) {
        // Auf Nachrichten warten
        // TODO: Radio RX implementieren
        
        // Nachricht verarbeiten
        p2p_process_message(manager, &msg);
        
        furi_delay_ms(10);
    }
    
    return 0;
}

static int32_t p2p_tx_thread(void* context) {
    P2pManager* manager = (P2pManager*)context;
    uint32_t last_beacon = 0;
    uint32_t last_sync = 0;
    
    while(manager->enabled) {
        uint32_t now = furi_get_tick();
        
        if(manager->is_host) {
            // Beacon senden
            if(now - last_beacon >= P2P_BEACON_INTERVAL) {
                P2pMessage msg = {
                    .type = P2pMessageTypeBeacon,
                    .sender_id = 0,
                    .sequence = 0,
                    .timestamp = now
                };
                
                P2pBeacon* beacon = (P2pBeacon*)msg.data;
                beacon->protocol_version = P2P_PROTOCOL_VERSION;
                beacon->game_id = manager->game_id;
                beacon->player_count = manager->player_count;
                beacon->timestamp = now;
                
                p2p_send_message(manager, &msg);
                last_beacon = now;
            }
        }
        
        // Spielstand synchronisieren
        if(now - last_sync >= P2P_SYNC_INTERVAL) {
            // TODO: Sync implementieren
            last_sync = now;
        }
        
        furi_delay_ms(10);
    }
    
    return 0;
}

static void p2p_process_message(P2pManager* manager, P2pMessage* message) {
    if(!manager || !message) return;
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    switch(message->type) {
        case P2pMessageTypeBeacon:
            // Beacon verarbeiten
            break;
            
        case P2pMessageTypeJoinRequest:
            if(manager->is_host) {
                // Join-Request verarbeiten
                P2pJoinRequest* req = (P2pJoinRequest*)message->data;
                if(req->game_id == manager->game_id &&
                   manager->player_count < P2P_MAX_PLAYERS) {
                    // Neuen Spieler hinzufügen
                    P2pPlayer* player = &manager->players[manager->player_count];
                    player->player_id = manager->player_count;
                    strncpy(player->name, req->name, sizeof(player->name)-1);
                    player->score = 0;
                    player->team_id = 0;
                    player->is_host = false;
                    
                    // Join-Response senden
                    P2pMessage response = {
                        .type = P2pMessageTypeJoinResponse,
                        .sender_id = 0,
                        .sequence = 0,
                        .timestamp = furi_get_tick()
                    };
                    
                    P2pJoinResponse* resp = (P2pJoinResponse*)response.data;
                    resp->accepted = 1;
                    resp->assigned_id = manager->player_count;
                    
                    p2p_send_message(manager, &response);
                    
                    manager->player_count++;
                }
            }
            break;
            
        case P2pMessageTypeJoinResponse:
            if(!manager->is_host) {
                // Join-Response verarbeiten
                P2pJoinResponse* resp = (P2pJoinResponse*)message->data;
                if(resp->accepted) {
                    manager->player_id = resp->assigned_id;
                }
            }
            break;
            
        default:
            // Callback aufrufen wenn registriert
            if(manager->message_callback) {
                manager->message_callback(message, manager->callback_context);
            }
            break;
    }
    
    furi_mutex_release(manager->mutex);
}

static bool p2p_send_message(P2pManager* manager, P2pMessage* message) {
    if(!manager || !message) return false;
    
    // TODO: Radio TX implementieren
    
    return true;
}
