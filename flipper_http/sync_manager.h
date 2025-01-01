#pragma once

#include <furi.h>
#include "offline_data.h"
#include "http_client.h"

typedef enum {
    SyncStateIdle,
    SyncStateUploading,
    SyncStateDownloading,
    SyncStateConflict,
    SyncStateError
} SyncState;

typedef struct {
    uint32_t timestamp;
    char hash[64];
    bool is_local;
} SyncVersion;

typedef struct {
    uint32_t id;
    uint32_t local_version;
    uint32_t server_version;
    char path[256];
    bool needs_upload;
    bool needs_download;
    bool has_conflict;
} SyncItem;

typedef struct {
    HttpClient* client;
    OfflineData* data;
    FuriThread* worker;
    FuriMutex* mutex;
    
    SyncState state;
    uint32_t total_items;
    uint32_t processed_items;
    uint32_t conflicts;
    
    char status_text[128];
    float progress;
    
    bool auto_sync;
    uint32_t sync_interval;
    uint32_t last_sync;
    
    void (*progress_callback)(float progress, const char* status, void* context);
    void* callback_context;
} SyncManager;

// Hauptfunktionen
SyncManager* sync_manager_alloc(HttpClient* client, OfflineData* data);
void sync_manager_free(SyncManager* manager);

// Sync-Steuerung
bool sync_manager_start_sync(SyncManager* manager);
void sync_manager_stop_sync(SyncManager* manager);
bool sync_manager_is_syncing(SyncManager* manager);

// Auto-Sync
void sync_manager_set_auto_sync(SyncManager* manager, bool enabled);
void sync_manager_set_sync_interval(SyncManager* manager, uint32_t seconds);

// Konfliktbehandlung
typedef enum {
    SyncResolveUseLocal,
    SyncResolveUseServer,
    SyncResolveMerge
} SyncResolveStrategy;

bool sync_manager_resolve_conflict(
    SyncManager* manager,
    SyncItem* item,
    SyncResolveStrategy strategy
);

// Callback-Management
void sync_manager_set_progress_callback(
    SyncManager* manager,
    void (*callback)(float progress, const char* status, void* context),
    void* context
);

// Status-Abfragen
SyncState sync_manager_get_state(SyncManager* manager);
float sync_manager_get_progress(SyncManager* manager);
void sync_manager_get_status(SyncManager* manager, char* buffer, size_t size);

// Daten-Management
bool sync_manager_queue_upload(SyncManager* manager, const char* path);
bool sync_manager_queue_download(SyncManager* manager, const char* path);
bool sync_manager_clear_queue(SyncManager* manager);

// Kompression
bool sync_manager_compress_data(
    const void* data,
    size_t size,
    void* compressed,
    size_t* compressed_size
);
bool sync_manager_decompress_data(
    const void* compressed,
    size_t compressed_size,
    void* data,
    size_t* size
);
