#pragma once

#include <furi.h>
#include "location_manager.h"
#include "offline_data.h"

#define MAX_WAYPOINTS 100
#define MAX_ROUTES 20
#define MAX_AREAS 10

typedef struct {
    uint32_t id;
    char name[32];
    float latitude;
    float longitude;
    char description[64];
    uint32_t icon;
    bool visible;
} Waypoint;

typedef struct {
    uint32_t id;
    char name[32];
    uint32_t waypoint_count;
    uint32_t waypoints[MAX_WAYPOINTS];
    uint32_t distance;
    uint32_t estimated_time;
    bool circular;
} Route;

typedef struct {
    uint32_t id;
    char name[32];
    float center_lat;
    float center_lon;
    float radius;
    uint32_t type;
    bool active;
} PlayArea;

typedef struct {
    float latitude;
    float longitude;
    float speed;
    float bearing;
    uint32_t timestamp;
} TrackPoint;

typedef struct {
    uint32_t id;
    char name[32];
    uint32_t point_count;
    TrackPoint points[1000];
    uint32_t distance;
    uint32_t duration;
    uint32_t max_speed;
    uint32_t avg_speed;
} Track;

typedef struct {
    Waypoint waypoints[MAX_WAYPOINTS];
    uint32_t waypoint_count;
    
    Route routes[MAX_ROUTES];
    uint32_t route_count;
    
    PlayArea areas[MAX_AREAS];
    uint32_t area_count;
    
    Track current_track;
    bool tracking_active;
    
    LocationManager* location;
    OfflineData* data;
    FuriMutex* mutex;
} MapManager;

// Hauptfunktionen
MapManager* map_manager_alloc(LocationManager* location, OfflineData* data);
void map_manager_free(MapManager* manager);

// Wegpunkt-Funktionen
bool map_manager_add_waypoint(
    MapManager* manager,
    const char* name,
    float latitude,
    float longitude
);
bool map_manager_remove_waypoint(MapManager* manager, uint32_t id);
Waypoint* map_manager_get_waypoint(MapManager* manager, uint32_t id);
bool map_manager_update_waypoint(MapManager* manager, const Waypoint* waypoint);

// Routen-Funktionen
bool map_manager_create_route(
    MapManager* manager,
    const char* name,
    uint32_t* waypoint_ids,
    uint32_t count
);
bool map_manager_delete_route(MapManager* manager, uint32_t id);
Route* map_manager_get_route(MapManager* manager, uint32_t id);
bool map_manager_calculate_route(MapManager* manager, Route* route);

// Spielbereich-Funktionen
bool map_manager_create_area(
    MapManager* manager,
    const char* name,
    float center_lat,
    float center_lon,
    float radius
);
bool map_manager_delete_area(MapManager* manager, uint32_t id);
PlayArea* map_manager_get_area(MapManager* manager, uint32_t id);
bool map_manager_check_in_area(MapManager* manager, uint32_t area_id);

// Tracking-Funktionen
bool map_manager_start_tracking(MapManager* manager, const char* name);
bool map_manager_stop_tracking(MapManager* manager);
bool map_manager_add_track_point(MapManager* manager, const LocationInfo* location);
bool map_manager_get_track_stats(MapManager* manager, Track* stats);

// Import/Export
bool map_manager_export_gpx(MapManager* manager, const char* filename);
bool map_manager_import_gpx(MapManager* manager, const char* filename);
bool map_manager_export_kml(MapManager* manager, const char* filename);
bool map_manager_import_kml(MapManager* manager, const char* filename);

// Offline-Karten
bool map_manager_cache_area(
    MapManager* manager,
    float center_lat,
    float center_lon,
    float radius,
    uint8_t zoom
);
bool map_manager_clear_cache(MapManager* manager);
uint32_t map_manager_get_cache_size(MapManager* manager);

// Routing
bool map_manager_find_nearest_waypoint(
    MapManager* manager,
    float latitude,
    float longitude,
    Waypoint* nearest
);
bool map_manager_calculate_distance(
    MapManager* manager,
    float lat1,
    float lon1,
    float lat2,
    float lon2,
    float* distance
);
bool map_manager_get_bearing(
    MapManager* manager,
    float lat1,
    float lon1,
    float lat2,
    float lon2,
    float* bearing
);
