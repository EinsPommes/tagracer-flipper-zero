#include "map_manager.h"
#include <furi_hal.h>
#include <storage/storage.h>

#define MAP_TILE_SIZE 256
#define MAP_CACHE_DIR "/ext/tagracer/maps"
#define TRACK_FILE_EXT ".gpx"
#define MAP_FILE_EXT ".map"

MapManager* map_manager_alloc(LocationManager* location, OfflineData* data) {
    MapManager* manager = malloc(sizeof(MapManager));
    
    manager->location = location;
    manager->data = data;
    manager->waypoint_count = 0;
    manager->route_count = 0;
    manager->area_count = 0;
    manager->tracking_active = false;
    
    manager->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    
    // Cache-Verzeichnis erstellen
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_mkdir(storage, MAP_CACHE_DIR);
    furi_record_close(RECORD_STORAGE);
    
    return manager;
}

void map_manager_free(MapManager* manager) {
    if(!manager) return;
    
    if(manager->tracking_active) {
        map_manager_stop_tracking(manager);
    }
    
    furi_mutex_free(manager->mutex);
    free(manager);
}

bool map_manager_add_waypoint(
    MapManager* manager,
    const char* name,
    float latitude,
    float longitude
) {
    if(!manager || !name || manager->waypoint_count >= MAX_WAYPOINTS) {
        return false;
    }
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    Waypoint* wp = &manager->waypoints[manager->waypoint_count];
    wp->id = manager->waypoint_count + 1;
    strncpy(wp->name, name, sizeof(wp->name)-1);
    wp->latitude = latitude;
    wp->longitude = longitude;
    wp->visible = true;
    
    manager->waypoint_count++;
    
    furi_mutex_release(manager->mutex);
    
    return true;
}

bool map_manager_create_route(
    MapManager* manager,
    const char* name,
    uint32_t* waypoint_ids,
    uint32_t count
) {
    if(!manager || !name || !waypoint_ids || count == 0 ||
       manager->route_count >= MAX_ROUTES) {
        return false;
    }
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    Route* route = &manager->routes[manager->route_count];
    route->id = manager->route_count + 1;
    strncpy(route->name, name, sizeof(route->name)-1);
    
    // Wegpunkte kopieren
    route->waypoint_count = count;
    memcpy(route->waypoints, waypoint_ids, count * sizeof(uint32_t));
    
    // Distanz berechnen
    route->distance = 0;
    for(uint32_t i = 1; i < count; i++) {
        Waypoint* wp1 = map_manager_get_waypoint(manager, waypoint_ids[i-1]);
        Waypoint* wp2 = map_manager_get_waypoint(manager, waypoint_ids[i]);
        
        if(wp1 && wp2) {
            float distance;
            if(map_manager_calculate_distance(manager,
                wp1->latitude, wp1->longitude,
                wp2->latitude, wp2->longitude,
                &distance)) {
                route->distance += (uint32_t)distance;
            }
        }
    }
    
    // Geschätzte Zeit (4 km/h Durchschnittsgeschwindigkeit)
    route->estimated_time = route->distance / 1.11f;
    
    manager->route_count++;
    
    furi_mutex_release(manager->mutex);
    
    return true;
}

bool map_manager_create_area(
    MapManager* manager,
    const char* name,
    float center_lat,
    float center_lon,
    float radius
) {
    if(!manager || !name || manager->area_count >= MAX_AREAS) {
        return false;
    }
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    PlayArea* area = &manager->areas[manager->area_count];
    area->id = manager->area_count + 1;
    strncpy(area->name, name, sizeof(area->name)-1);
    area->center_lat = center_lat;
    area->center_lon = center_lon;
    area->radius = radius;
    area->active = true;
    
    manager->area_count++;
    
    furi_mutex_release(manager->mutex);
    
    return true;
}

bool map_manager_start_tracking(MapManager* manager, const char* name) {
    if(!manager || !name || manager->tracking_active) {
        return false;
    }
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    // Track initialisieren
    manager->current_track.id = furi_hal_random_get();
    strncpy(manager->current_track.name, name,
            sizeof(manager->current_track.name)-1);
    manager->current_track.point_count = 0;
    manager->current_track.distance = 0;
    manager->current_track.duration = 0;
    manager->current_track.max_speed = 0;
    manager->current_track.avg_speed = 0;
    
    manager->tracking_active = true;
    
    furi_mutex_release(manager->mutex);
    
    return true;
}

bool map_manager_stop_tracking(MapManager* manager) {
    if(!manager || !manager->tracking_active) {
        return false;
    }
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    // Track finalisieren
    if(manager->current_track.point_count > 0) {
        // Durchschnittsgeschwindigkeit berechnen
        manager->current_track.avg_speed = 
            manager->current_track.distance /
            (float)manager->current_track.duration;
            
        // Track exportieren
        char filename[64];
        snprintf(filename, sizeof(filename),
                "%s/track_%lu%s",
                MAP_CACHE_DIR,
                manager->current_track.id,
                TRACK_FILE_EXT);
                
        map_manager_export_gpx(manager, filename);
    }
    
    manager->tracking_active = false;
    
    furi_mutex_release(manager->mutex);
    
    return true;
}

bool map_manager_add_track_point(MapManager* manager, const LocationInfo* location) {
    if(!manager || !location || !manager->tracking_active ||
       manager->current_track.point_count >= 1000) {
        return false;
    }
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    uint32_t idx = manager->current_track.point_count;
    TrackPoint* point = &manager->current_track.points[idx];
    
    point->latitude = location->latitude;
    point->longitude = location->longitude;
    point->speed = location->speed;
    point->bearing = location->bearing;
    point->timestamp = furi_get_tick();
    
    // Distanz zum vorherigen Punkt berechnen
    if(idx > 0) {
        TrackPoint* prev = &manager->current_track.points[idx-1];
        float distance;
        
        if(map_manager_calculate_distance(manager,
            prev->latitude, prev->longitude,
            point->latitude, point->longitude,
            &distance)) {
            manager->current_track.distance += (uint32_t)distance;
        }
        
        // Maximale Geschwindigkeit aktualisieren
        if(point->speed > manager->current_track.max_speed) {
            manager->current_track.max_speed = point->speed;
        }
        
        // Dauer aktualisieren
        manager->current_track.duration =
            (point->timestamp - manager->current_track.points[0].timestamp) / 1000;
    }
    
    manager->current_track.point_count++;
    
    furi_mutex_release(manager->mutex);
    
    return true;
}

bool map_manager_export_gpx(MapManager* manager, const char* filename) {
    if(!manager || !filename) return false;
    
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    
    if(!storage_file_open(file, filename,
                         FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    
    // GPX Header schreiben
    storage_file_write_string(file,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<gpx version=\"1.1\">\n"
        "<trk><name>");
    storage_file_write_string(file, manager->current_track.name);
    storage_file_write_string(file,
        "</name><trkseg>\n");
    
    // Track-Punkte schreiben
    char buffer[128];
    for(uint32_t i = 0; i < manager->current_track.point_count; i++) {
        TrackPoint* point = &manager->current_track.points[i];
        
        snprintf(buffer, sizeof(buffer),
            "<trkpt lat=\"%.6f\" lon=\"%.6f\">"
            "<ele>0</ele><time>%lu</time>"
            "<speed>%.1f</speed><course>%.1f</course>"
            "</trkpt>\n",
            point->latitude, point->longitude,
            point->timestamp,
            point->speed, point->bearing);
            
        storage_file_write_string(file, buffer);
    }
    
    // GPX Footer schreiben
    storage_file_write_string(file,
        "</trkseg></trk></gpx>\n");
    
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    
    return true;
}

bool map_manager_cache_area(
    MapManager* manager,
    float center_lat,
    float center_lon,
    float radius,
    uint8_t zoom
) {
    if(!manager) return false;
    
    MapTileInfo center_tile;
    location_manager_get_tile_info(
        center_lat,
        center_lon,
        zoom,
        &center_tile
    );
    
    // Umgebende Tiles berechnen
    uint32_t tile_radius = (radius / (40075016.686 * cos(center_lat * DEG_TO_RAD) / (1 << zoom))) + 1;
    
    Storage* storage = furi_record_open(RECORD_STORAGE);
    
    for(int32_t dx = -tile_radius; dx <= (int32_t)tile_radius; dx++) {
        for(int32_t dy = -tile_radius; dy <= (int32_t)tile_radius; dy++) {
            uint32_t tile_x = center_tile.tile_x + dx;
            uint32_t tile_y = center_tile.tile_y + dy;
            
            // Tile-Datei erstellen
            char filename[128];
            snprintf(filename, sizeof(filename),
                    "%s/%d_%d_%d%s",
                    MAP_CACHE_DIR,
                    zoom, tile_x, tile_y,
                    MAP_FILE_EXT);
            
            // Prüfen ob Tile schon existiert
            if(!storage_file_exists(storage, filename)) {
                // TODO: Tile-Daten herunterladen
                // Hier würde der Code zum Herunterladen der Kartendaten stehen
            }
        }
    }
    
    furi_record_close(RECORD_STORAGE);
    
    return true;
}

bool map_manager_calculate_distance(
    MapManager* manager,
    float lat1,
    float lon1,
    float lat2,
    float lon2,
    float* distance
) {
    if(!manager || !distance) return false;
    
    // Haversine-Formel
    float lat1_rad = lat1 * DEG_TO_RAD;
    float lon1_rad = lon1 * DEG_TO_RAD;
    float lat2_rad = lat2 * DEG_TO_RAD;
    float lon2_rad = lon2 * DEG_TO_RAD;
    
    float dlat = lat2_rad - lat1_rad;
    float dlon = lon2_rad - lon1_rad;
    
    float a = sin(dlat/2) * sin(dlat/2) +
              cos(lat1_rad) * cos(lat2_rad) *
              sin(dlon/2) * sin(dlon/2);
    float c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    *distance = EARTH_RADIUS * c;
    
    return true;
}
