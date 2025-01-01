#pragma once

#include <furi.h>
#include "game_state.h"
#include "offline_data.h"

#define PIPELINE_BUFFER_SIZE 4096
#define MAX_BATCH_SIZE 32
#define COMPRESSION_CHUNK 512
#define RETRY_COUNT 3

typedef enum {
    DataTypeGameState,
    DataTypeAchievement,
    DataTypeChallenge,
    DataTypeTraining,
    DataTypeTag,
    DataTypeRoute,
    DataTypeCustom
} DataType;

typedef struct {
    DataType type;
    uint32_t id;
    uint32_t timestamp;
    uint32_t size;
    uint8_t* data;
    bool compressed;
    uint32_t priority;
} DataItem;

typedef struct {
    DataItem items[MAX_BATCH_SIZE];
    uint32_t count;
    uint32_t total_size;
} DataBatch;

typedef enum {
    FilterTypeNone,
    FilterTypeTimestamp,
    FilterTypePriority,
    FilterTypeSize,
    FilterTypeCustom
} FilterType;

typedef struct {
    FilterType type;
    uint32_t value;
    bool (*custom_filter)(const DataItem* item, void* context);
    void* context;
} DataFilter;

typedef struct {
    uint8_t* buffer;
    size_t size;
    size_t capacity;
    bool compressed;
} DataBuffer;

typedef struct {
    DataBuffer input;
    DataBuffer output;
    DataBatch batch;
    DataFilter filter;
    
    uint32_t processed_items;
    uint32_t failed_items;
    uint32_t retry_count;
    uint32_t last_sync;
    
    bool (*process_callback)(DataItem* item, void* context);
    bool (*upload_callback)(DataBatch* batch, void* context);
    void* callback_context;
    
    FuriMutex* mutex;
    FuriThread* worker_thread;
    bool running;
} DataPipeline;

// Hauptfunktionen
DataPipeline* data_pipeline_alloc(void);
void data_pipeline_free(DataPipeline* pipeline);

// Item-Management
bool data_pipeline_add_item(
    DataPipeline* pipeline,
    DataType type,
    uint32_t id,
    const uint8_t* data,
    uint32_t size,
    uint32_t priority
);

bool data_pipeline_get_item(
    DataPipeline* pipeline,
    DataType type,
    uint32_t id,
    uint8_t* data,
    uint32_t* size
);

// Batch-Verarbeitung
bool data_pipeline_process_batch(DataPipeline* pipeline);
bool data_pipeline_upload_batch(DataPipeline* pipeline);

// Filter-Management
void data_pipeline_set_filter(
    DataPipeline* pipeline,
    FilterType type,
    uint32_t value
);

void data_pipeline_set_custom_filter(
    DataPipeline* pipeline,
    bool (*filter)(const DataItem* item, void* context),
    void* context
);

// Callback-Management
void data_pipeline_set_process_callback(
    DataPipeline* pipeline,
    bool (*callback)(DataItem* item, void* context),
    void* context
);

void data_pipeline_set_upload_callback(
    DataPipeline* pipeline,
    bool (*callback)(DataBatch* batch, void* context),
    void* context
);

// Statistik und Status
void data_pipeline_get_stats(
    DataPipeline* pipeline,
    uint32_t* processed,
    uint32_t* failed,
    uint32_t* retries
);

bool data_pipeline_is_busy(DataPipeline* pipeline);
void data_pipeline_clear(DataPipeline* pipeline);
