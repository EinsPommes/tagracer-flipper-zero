#pragma once

#include <furi.h>
#include <nfc/nfc.h>
#include "game_state.h"

// Tag-Typen
typedef enum {
    TagTypeStandard,    // Normaler Punkt-Tag
    TagTypePowerUp,     // Power-up Tag
    TagTypeCheckpoint,  // Checkpoint für Rennen
    TagTypeKey,         // Schlüssel für spezielle Aktionen
    TagTypeBonus,       // Bonus-Punkte
    TagTypeStory,       // Story-Element
    TagTypeCustom       // Benutzerdefiniert
} TagType;

// Tag-Daten
typedef struct {
    TagType type;
    uint32_t id;
    uint32_t points;
    uint32_t power_up_id;
    uint32_t checkpoint_id;
    uint32_t custom_data;
    char name[32];
    char description[64];
    uint8_t encryption_key[16];
    bool is_encrypted;
} TagConfig;

typedef struct {
    Nfc* nfc;
    FuriThread* worker;
    FuriMutex* mutex;
    bool is_running;
    
    void (*scan_callback)(TagConfig* tag, void* context);
    void* callback_context;
    
    uint8_t encryption_key[16];
    bool encryption_enabled;
} TagManager;

// Hauptfunktionen
TagManager* tag_manager_alloc(void);
void tag_manager_free(TagManager* manager);

// Scanner-Funktionen
bool tag_manager_start_scan(TagManager* manager);
void tag_manager_stop_scan(TagManager* manager);
void tag_manager_set_callback(
    TagManager* manager,
    void (*callback)(TagConfig* tag, void* context),
    void* context
);

// Tag-Konfiguration
bool tag_manager_write_config(TagManager* manager, NfcTag* tag, const TagConfig* config);
bool tag_manager_read_config(TagManager* manager, NfcTag* tag, TagConfig* config);
bool tag_manager_format_tag(TagManager* manager, NfcTag* tag, TagType type);

// Verschlüsselung
void tag_manager_set_encryption_key(TagManager* manager, const uint8_t* key);
void tag_manager_enable_encryption(TagManager* manager, bool enabled);
bool tag_manager_encrypt_tag(TagManager* manager, NfcTag* tag);
bool tag_manager_decrypt_tag(TagManager* manager, NfcTag* tag);

// Power-up Management
bool tag_manager_create_power_up(
    TagManager* manager,
    NfcTag* tag,
    uint32_t power_up_id,
    uint32_t duration
);
bool tag_manager_activate_power_up(TagManager* manager, uint32_t power_up_id);

// Checkpoint-System
bool tag_manager_create_checkpoint(
    TagManager* manager,
    NfcTag* tag,
    uint32_t checkpoint_id,
    uint32_t next_checkpoint
);
bool tag_manager_verify_checkpoint(
    TagManager* manager,
    uint32_t checkpoint_id,
    uint32_t expected_id
);

// Story-Modus
bool tag_manager_create_story_tag(
    TagManager* manager,
    NfcTag* tag,
    uint32_t story_id,
    const char* content
);
bool tag_manager_unlock_story(TagManager* manager, uint32_t story_id);

// Benutzerdefinierte Tags
bool tag_manager_create_custom_tag(
    TagManager* manager,
    NfcTag* tag,
    uint32_t custom_id,
    const void* data,
    size_t size
);
bool tag_manager_read_custom_data(
    TagManager* manager,
    NfcTag* tag,
    void* data,
    size_t* size
);
