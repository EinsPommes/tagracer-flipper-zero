#include "offline_storage.h"
#include <furi_hal_rtc.h>
#include <storage/storage.h>
#include <toolbox/compression.h>

#define STORAGE_FOLDER "/ext/tagracer"
#define CACHE_SIZE 8192
#define COMPRESSION_LEVEL 9

// Optimierter Speicher-Cache
typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
    bool dirty;
    uint32_t last_access;
} StorageCache;

typedef struct {
    Storage* storage;
    StorageCache cache;
    FuriMutex* mutex;
    char current_file[256];
} StorageManager;

static StorageManager* storage = NULL;

// Optimierte Kompression
static bool compress_data(const void* data, size_t size, void* compressed, size_t* compressed_size) {
    // DEFLATE mit optimiertem Dictionary
    compression_init();
    
    bool success = compression_encode(
        data,
        size,
        compressed,
        compressed_size,
        COMPRESSION_LEVEL
    );
    
    compression_free();
    return success;
}

static bool decompress_data(const void* compressed, size_t compressed_size, void* data, size_t* size) {
    compression_init();
    
    bool success = compression_decode(
        compressed,
        compressed_size,
        data,
        size
    );
    
    compression_free();
    return success;
}

// Cache-Management
static bool cache_init(StorageCache* cache) {
    cache->data = malloc(CACHE_SIZE);
    if(!cache->data) return false;
    
    cache->capacity = CACHE_SIZE;
    cache->size = 0;
    cache->dirty = false;
    cache->last_access = 0;
    
    return true;
}

static void cache_free(StorageCache* cache) {
    if(cache->data) {
        free(cache->data);
        cache->data = NULL;
    }
}

static bool cache_flush(StorageManager* manager) {
    if(!manager->cache.dirty) return true;
    
    File* file = storage_file_alloc(manager->storage);
    if(!storage_file_open(file, manager->current_file,
                         FSAM_WRITE, FSOM_OPEN_EXISTING)) {
        storage_file_free(file);
        return false;
    }
    
    // Daten komprimieren
    size_t compressed_size = manager->cache.size * 2; // Worst case
    void* compressed = malloc(compressed_size);
    if(!compressed) {
        storage_file_free(file);
        return false;
    }
    
    bool success = compress_data(
        manager->cache.data,
        manager->cache.size,
        compressed,
        &compressed_size
    );
    
    if(success) {
        success = storage_file_write(
            file,
            compressed,
            compressed_size
        ) == compressed_size;
    }
    
    free(compressed);
    storage_file_close(file);
    storage_file_free(file);
    
    if(success) {
        manager->cache.dirty = false;
    }
    
    return success;
}

// Optimierte Dateiverwaltung
bool offline_storage_init(void) {
    if(storage) return true;
    
    storage = malloc(sizeof(StorageManager));
    if(!storage) return false;
    
    storage->storage = furi_record_open(RECORD_STORAGE);
    storage->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    
    if(!cache_init(&storage->cache)) {
        free(storage);
        storage = NULL;
        return false;
    }
    
    // Verzeichnis erstellen
    storage_mkdir(storage->storage, STORAGE_FOLDER);
    
    return true;
}

void offline_storage_deinit(void) {
    if(!storage) return;
    
    furi_mutex_acquire(storage->mutex, FuriWaitForever);
    
    if(storage->cache.dirty) {
        cache_flush(storage);
    }
    
    cache_free(&storage->cache);
    
    furi_mutex_free(storage->mutex);
    furi_record_close(RECORD_STORAGE);
    
    free(storage);
    storage = NULL;
}

// Optimierte Lese-/Schreiboperationen
bool offline_storage_save(const char* filename, const void* data, size_t size) {
    if(!storage || !filename || !data) return false;
    
    furi_mutex_acquire(storage->mutex, FuriWaitForever);
    
    char full_path[512];
    snprintf(full_path, sizeof(full_path),
             "%s/%s", STORAGE_FOLDER, filename);
             
    // Cache leeren wenn anderes File
    if(strcmp(full_path, storage->current_file) != 0) {
        if(storage->cache.dirty) {
            cache_flush(storage);
        }
        strncpy(storage->current_file, full_path,
                sizeof(storage->current_file)-1);
    }
    
    // In Cache schreiben
    if(size > storage->cache.capacity) {
        // Direkt auf Disk schreiben bei großen Daten
        File* file = storage_file_alloc(storage->storage);
        if(!storage_file_open(file, full_path,
                             FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
            storage_file_free(file);
            furi_mutex_release(storage->mutex);
            return false;
        }
        
        // Komprimieren und schreiben
        size_t compressed_size = size * 2;
        void* compressed = malloc(compressed_size);
        if(!compressed) {
            storage_file_free(file);
            furi_mutex_release(storage->mutex);
            return false;
        }
        
        bool success = compress_data(
            data,
            size,
            compressed,
            &compressed_size
        );
        
        if(success) {
            success = storage_file_write(
                file,
                compressed,
                compressed_size
            ) == compressed_size;
        }
        
        free(compressed);
        storage_file_close(file);
        storage_file_free(file);
        
        furi_mutex_release(storage->mutex);
        return success;
    }
    
    // In Cache kopieren
    memcpy(storage->cache.data, data, size);
    storage->cache.size = size;
    storage->cache.dirty = true;
    storage->cache.last_access = furi_get_tick();
    
    furi_mutex_release(storage->mutex);
    return true;
}

bool offline_storage_load(const char* filename, void* data, size_t* size) {
    if(!storage || !filename || !data || !size) return false;
    
    furi_mutex_acquire(storage->mutex, FuriWaitForever);
    
    char full_path[512];
    snprintf(full_path, sizeof(full_path),
             "%s/%s", STORAGE_FOLDER, filename);
             
    bool success = false;
    
    // Aus Cache lesen wenn möglich
    if(strcmp(full_path, storage->current_file) == 0 &&
       !storage->cache.dirty) {
        memcpy(data, storage->cache.data, storage->cache.size);
        *size = storage->cache.size;
        storage->cache.last_access = furi_get_tick();
        success = true;
    } else {
        // Von Disk lesen
        File* file = storage_file_alloc(storage->storage);
        if(!storage_file_open(file, full_path,
                             FSAM_READ, FSOM_OPEN_EXISTING)) {
            storage_file_free(file);
            furi_mutex_release(storage->mutex);
            return false;
        }
        
        // Größe ermitteln
        uint64_t file_size = storage_file_size(file);
        if(file_size == 0) {
            storage_file_free(file);
            furi_mutex_release(storage->mutex);
            return false;
        }
        
        // Komprimierte Daten lesen
        void* compressed = malloc(file_size);
        if(!compressed) {
            storage_file_free(file);
            furi_mutex_release(storage->mutex);
            return false;
        }
        
        if(storage_file_read(file, compressed, file_size) == file_size) {
            // Dekomprimieren
            success = decompress_data(
                compressed,
                file_size,
                data,
                size
            );
            
            // In Cache speichern
            if(success && *size <= storage->cache.capacity) {
                memcpy(storage->cache.data, data, *size);
                storage->cache.size = *size;
                storage->cache.dirty = false;
                storage->cache.last_access = furi_get_tick();
                strncpy(storage->current_file, full_path,
                        sizeof(storage->current_file)-1);
            }
        }
        
        free(compressed);
        storage_file_close(file);
        storage_file_free(file);
    }
    
    furi_mutex_release(storage->mutex);
    return success;
}

// Optimierte Hilfsfunktionen
bool offline_storage_delete(const char* filename) {
    if(!storage || !filename) return false;
    
    char full_path[512];
    snprintf(full_path, sizeof(full_path),
             "%s/%s", STORAGE_FOLDER, filename);
             
    // Cache leeren wenn betroffen
    if(strcmp(full_path, storage->current_file) == 0) {
        storage->cache.size = 0;
        storage->cache.dirty = false;
        storage->current_file[0] = '\0';
    }
    
    return storage_file_delete(
        storage->storage,
        full_path
    );
}

bool offline_storage_exists(const char* filename) {
    if(!storage || !filename) return false;
    
    char full_path[512];
    snprintf(full_path, sizeof(full_path),
             "%s/%s", STORAGE_FOLDER, filename);
             
    return storage_file_exists(
        storage->storage,
        full_path
    );
}
