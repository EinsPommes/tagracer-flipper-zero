#include "location_manager.h"
#include <furi_hal_uart.h>
#include <math.h>

#define EARTH_RADIUS 6371000.0
#define DEG_TO_RAD (M_PI / 180.0)
#define RAD_TO_DEG (180.0 / M_PI)

#define GPS_UART_CHANNEL FuriHalUartIdUSART1
#define GPS_BAUD_RATE 9600

#define LOCATION_THREAD_STACK_SIZE 2048
#define LOCATION_THREAD_PRIORITY 21

typedef struct {
    uint8_t buffer[128];
    uint8_t position;
} NmeaParser;

static int32_t location_worker_thread(void* context);
static void parse_nmea(LocationManager* manager, uint8_t byte);
static bool parse_gga(LocationManager* manager, const char* sentence);
static double parse_coordinate(const char* str, char dir);

LocationManager* location_manager_alloc(void) {
    LocationManager* manager = malloc(sizeof(LocationManager));
    
    manager->enabled = false;
    manager->has_fix = false;
    manager->callback = NULL;
    manager->callback_context = NULL;
    manager->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    
    // Worker Thread erstellen
    manager->worker_thread = furi_thread_alloc_ex(
        "LocationWorker",
        LOCATION_THREAD_STACK_SIZE,
        location_worker_thread,
        manager
    );
    furi_thread_set_priority(manager->worker_thread, LOCATION_THREAD_PRIORITY);
    
    return manager;
}

void location_manager_free(LocationManager* manager) {
    if(manager->enabled) {
        location_manager_stop(manager);
    }
    
    furi_thread_free(manager->worker_thread);
    furi_mutex_free(manager->mutex);
    free(manager);
}

bool location_manager_start(LocationManager* manager) {
    if(manager->enabled) return true;
    
    // UART für GPS konfigurieren
    furi_hal_uart_set_br(GPS_UART_CHANNEL, GPS_BAUD_RATE);
    furi_hal_uart_init(GPS_UART_CHANNEL, GPS_BAUD_RATE);
    
    manager->enabled = true;
    furi_thread_start(manager->worker_thread);
    
    return true;
}

void location_manager_stop(LocationManager* manager) {
    if(!manager->enabled) return;
    
    manager->enabled = false;
    furi_thread_join(manager->worker_thread);
    
    furi_hal_uart_deinit(GPS_UART_CHANNEL);
}

bool location_manager_get_location(LocationManager* manager, LocationInfo* location) {
    if(!manager->has_fix) return false;
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    memcpy(location, &manager->current_location, sizeof(LocationInfo));
    furi_mutex_release(manager->mutex);
    
    return true;
}

void location_manager_set_callback(
    LocationManager* manager,
    LocationUpdateCallback callback,
    void* context
) {
    manager->callback = callback;
    manager->callback_context = context;
}

float location_manager_calculate_distance(
    float lat1, float lon1,
    float lat2, float lon2
) {
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
    
    return EARTH_RADIUS * c;
}

float location_manager_calculate_bearing(
    float lat1, float lon1,
    float lat2, float lon2
) {
    float lat1_rad = lat1 * DEG_TO_RAD;
    float lon1_rad = lon1 * DEG_TO_RAD;
    float lat2_rad = lat2 * DEG_TO_RAD;
    float lon2_rad = lon2 * DEG_TO_RAD;
    
    float dlon = lon2_rad - lon1_rad;
    
    float y = sin(dlon) * cos(lat2_rad);
    float x = cos(lat1_rad) * sin(lat2_rad) -
              sin(lat1_rad) * cos(lat2_rad) * cos(dlon);
    
    float bearing = atan2(y, x) * RAD_TO_DEG;
    return fmod(bearing + 360, 360);
}

void location_manager_get_tile_info(
    float latitude,
    float longitude,
    uint8_t zoom,
    MapTileInfo* tile_info
) {
    int32_t n = 1 << zoom;
    
    double lat_rad = latitude * DEG_TO_RAD;
    
    tile_info->zoom = zoom;
    tile_info->tile_x = (uint32_t)((longitude + 180.0) / 360.0 * n);
    tile_info->tile_y = (uint32_t)((1.0 - log(tan(lat_rad) + 1.0/cos(lat_rad)) / M_PI) / 2.0 * n);
}

bool location_manager_cache_current_area(
    LocationManager* manager,
    OfflineData* data,
    uint8_t zoom,
    uint32_t radius
) {
    LocationInfo location;
    if(!location_manager_get_location(manager, &location)) {
        return false;
    }
    
    MapTileInfo center_tile;
    location_manager_get_tile_info(
        location.latitude,
        location.longitude,
        zoom,
        &center_tile
    );
    
    // Umgebende Tiles berechnen und cachen
    uint32_t tile_radius = (radius / (40075016.686 * cos(location.latitude * DEG_TO_RAD) / (1 << zoom))) + 1;
    
    for(int32_t dx = -tile_radius; dx <= (int32_t)tile_radius; dx++) {
        for(int32_t dy = -tile_radius; dy <= (int32_t)tile_radius; dy++) {
            MapTile tile;
            tile.zoom = zoom;
            tile.x = center_tile.tile_x + dx;
            tile.y = center_tile.tile_y + dy;
            tile.tile_id = (tile.zoom << 28) | (tile.x << 14) | tile.y;
            tile.last_updated = furi_get_tick();
            
            // TODO: Implementiere Tile-Daten-Download
            // Hier würden die tatsächlichen Kartendaten geladen
            
            offline_data_cache_map_tile(data, &tile);
        }
    }
    
    return true;
}

bool location_manager_check_in_area(
    LocationManager* manager,
    float center_lat,
    float center_lon,
    float radius
) {
    LocationInfo current;
    if(!location_manager_get_location(manager, &current)) {
        return false;
    }
    
    float distance = location_manager_calculate_distance(
        current.latitude, current.longitude,
        center_lat, center_lon
    );
    
    return distance <= radius;
}

bool location_manager_format_coordinates(
    float latitude,
    float longitude,
    char* buffer,
    size_t buffer_size
) {
    int32_t lat_deg = (int32_t)fabs(latitude);
    int32_t lat_min = (int32_t)((fabs(latitude) - lat_deg) * 60);
    float lat_sec = (float)((fabs(latitude) - lat_deg - lat_min/60.0) * 3600);
    
    int32_t lon_deg = (int32_t)fabs(longitude);
    int32_t lon_min = (int32_t)((fabs(longitude) - lon_deg) * 60);
    float lon_sec = (float)((fabs(longitude) - lon_deg - lon_min/60.0) * 3600);
    
    return snprintf(
        buffer,
        buffer_size,
        "%d°%d'%.1f\"%c %d°%d'%.1f\"%c",
        lat_deg, lat_min, lat_sec, latitude >= 0 ? 'N' : 'S',
        lon_deg, lon_min, lon_sec, longitude >= 0 ? 'E' : 'W'
    ) < buffer_size;
}

void location_manager_get_readable_bearing(
    float bearing,
    char* buffer,
    size_t buffer_size
) {
    const char* directions[] = {
        "N", "NNE", "NE", "ENE",
        "E", "ESE", "SE", "SSE",
        "S", "SSW", "SW", "WSW",
        "W", "WNW", "NW", "NNW"
    };
    
    int32_t index = (int32_t)((bearing + 11.25) / 22.5) % 16;
    strncpy(buffer, directions[index], buffer_size);
    buffer[buffer_size-1] = '\0';
}

static int32_t location_worker_thread(void* context) {
    LocationManager* manager = (LocationManager*)context;
    NmeaParser parser = {0};
    
    while(manager->enabled) {
        uint16_t available = furi_hal_uart_rx_available(GPS_UART_CHANNEL);
        
        if(available > 0) {
            uint8_t data;
            if(furi_hal_uart_rx(GPS_UART_CHANNEL, &data, 1) == 1) {
                parse_nmea(manager, data);
            }
        }
        
        furi_delay_ms(10);
    }
    
    return 0;
}

static void parse_nmea(LocationManager* manager, uint8_t byte) {
    // TODO: Implementiere NMEA-Parser
    // Hier würde der tatsächliche NMEA-Parsing-Code stehen
}

static bool parse_gga(LocationManager* manager, const char* sentence) {
    // TODO: Implementiere GGA-Parsing
    // Hier würde der Code zum Parsen von GGA-Nachrichten stehen
    return false;
}

static double parse_coordinate(const char* str, char dir) {
    // TODO: Implementiere Koordinaten-Parsing
    // Hier würde der Code zum Parsen von Koordinaten stehen
    return 0.0;
}
