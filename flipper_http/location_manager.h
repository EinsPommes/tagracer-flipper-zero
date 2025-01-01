#pragma once

#include <furi.h>
#include "offline_data.h"

typedef struct {
    float latitude;
    float longitude;
    float distance;
    float bearing;
} LocationInfo;

typedef struct {
    uint32_t tile_x;
    uint32_t tile_y;
    uint8_t zoom;
} MapTileInfo;

typedef void (*LocationUpdateCallback)(LocationInfo* location, void* context);

typedef struct {
    bool enabled;
    bool has_fix;
    LocationInfo current_location;
    LocationUpdateCallback callback;
    void* callback_context;
    FuriThread* worker_thread;
    FuriMutex* mutex;
} LocationManager;

// Hauptfunktionen
LocationManager* location_manager_alloc(void);
void location_manager_free(LocationManager* manager);

// GPS-Management
bool location_manager_start(LocationManager* manager);
void location_manager_stop(LocationManager* manager);
bool location_manager_get_location(LocationManager* manager, LocationInfo* location);
void location_manager_set_callback(LocationManager* manager, LocationUpdateCallback callback, void* context);

// Distanz und Richtung
float location_manager_calculate_distance(
    float lat1, float lon1,
    float lat2, float lon2
);
float location_manager_calculate_bearing(
    float lat1, float lon1,
    float lat2, float lon2
);

// Kartenfunktionen
void location_manager_get_tile_info(
    float latitude,
    float longitude,
    uint8_t zoom,
    MapTileInfo* tile_info
);
bool location_manager_cache_current_area(
    LocationManager* manager,
    OfflineData* data,
    uint8_t zoom,
    uint32_t radius
);

// Geofencing
bool location_manager_check_in_area(
    LocationManager* manager,
    float center_lat,
    float center_lon,
    float radius
);

// Hilfsfunktionen
bool location_manager_format_coordinates(
    float latitude,
    float longitude,
    char* buffer,
    size_t buffer_size
);
void location_manager_get_readable_bearing(
    float bearing,
    char* buffer,
    size_t buffer_size
);
