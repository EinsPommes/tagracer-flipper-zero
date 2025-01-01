#include "data_pipeline.h"
#include <toolbox/compression.h>
#include <furi_hal_rtc.h>

// Worker-Thread
static int32_t pipeline_worker(void* context) {
    DataPipeline* pipeline = (DataPipeline*)context;
    
    while(pipeline->running) {
        furi_mutex_acquire(pipeline->mutex, FuriWaitForever);
        
        uint32_t now = furi_get_tick();
        bool should_process = false;
        
        // Prüfen ob Batch-Verarbeitung nötig
        if(pipeline->batch.count >= MAX_BATCH_SIZE ||
           pipeline->batch.total_size >= PIPELINE_BUFFER_SIZE / 2 ||
           (now - pipeline->last_sync >= 5000 && pipeline->batch.count > 0)) {
            should_process = true;
        }
        
        if(should_process) {
            // Batch verarbeiten
            if(data_pipeline_process_batch(pipeline)) {
                // Batch hochladen
                if(data_pipeline_upload_batch(pipeline)) {
                    pipeline->last_sync = now;
                    pipeline->retry_count = 0;
                } else {
                    pipeline->failed_items += pipeline->batch.count;
                    pipeline->retry_count++;
                    
                    // Nach zu vielen Fehlversuchen Batch verwerfen
                    if(pipeline->retry_count >= RETRY_COUNT) {
                        pipeline->batch.count = 0;
                        pipeline->batch.total_size = 0;
                        pipeline->retry_count = 0;
                    }
                }
            }
        }
        
        furi_mutex_release(pipeline->mutex);
        furi_delay_ms(100);
    }
    
    return 0;
}

DataPipeline* data_pipeline_alloc(void) {
    DataPipeline* pipeline = malloc(sizeof(DataPipeline));
    
    // Buffer allozieren
    pipeline->input.buffer = malloc(PIPELINE_BUFFER_SIZE);
    pipeline->input.capacity = PIPELINE_BUFFER_SIZE;
    pipeline->input.size = 0;
    pipeline->input.compressed = false;
    
    pipeline->output.buffer = malloc(PIPELINE_BUFFER_SIZE);
    pipeline->output.capacity = PIPELINE_BUFFER_SIZE;
    pipeline->output.size = 0;
    pipeline->output.compressed = false;
    
    // Batch initialisieren
    pipeline->batch.count = 0;
    pipeline->batch.total_size = 0;
    
    // Filter zurücksetzen
    pipeline->filter.type = FilterTypeNone;
    pipeline->filter.value = 0;
    pipeline->filter.custom_filter = NULL;
    pipeline->filter.context = NULL;
    
    // Statistiken initialisieren
    pipeline->processed_items = 0;
    pipeline->failed_items = 0;
    pipeline->retry_count = 0;
    pipeline->last_sync = 0;
    
    // Callbacks initialisieren
    pipeline->process_callback = NULL;
    pipeline->upload_callback = NULL;
    pipeline->callback_context = NULL;
    
    pipeline->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    
    // Worker-Thread starten
    pipeline->running = true;
    pipeline->worker_thread = furi_thread_alloc();
    furi_thread_set_name(pipeline->worker_thread, "Data Pipeline");
    furi_thread_set_stack_size(pipeline->worker_thread, 2048);
    furi_thread_set_context(pipeline->worker_thread, pipeline);
    furi_thread_set_callback(pipeline->worker_thread, pipeline_worker);
    furi_thread_start(pipeline->worker_thread);
    
    return pipeline;
}

void data_pipeline_free(DataPipeline* pipeline) {
    if(!pipeline) return;
    
    // Thread beenden
    pipeline->running = false;
    furi_thread_join(pipeline->worker_thread);
    furi_thread_free(pipeline->worker_thread);
    
    // Buffer freigeben
    free(pipeline->input.buffer);
    free(pipeline->output.buffer);
    
    // Items freigeben
    for(uint32_t i = 0; i < pipeline->batch.count; i++) {
        free(pipeline->batch.items[i].data);
    }
    
    furi_mutex_free(pipeline->mutex);
    free(pipeline);
}

bool data_pipeline_add_item(
    DataPipeline* pipeline,
    DataType type,
    uint32_t id,
    const uint8_t* data,
    uint32_t size,
    uint32_t priority
) {
    if(!pipeline || !data || size == 0 ||
       pipeline->batch.count >= MAX_BATCH_SIZE) {
        return false;
    }
    
    furi_mutex_acquire(pipeline->mutex, FuriWaitForever);
    
    // Filter anwenden
    bool should_add = true;
    
    switch(pipeline->filter.type) {
        case FilterTypeTimestamp:
            should_add = (furi_get_tick() - pipeline->filter.value) > 0;
            break;
            
        case FilterTypePriority:
            should_add = priority >= pipeline->filter.value;
            break;
            
        case FilterTypeSize:
            should_add = size <= pipeline->filter.value;
            break;
            
        case FilterTypeCustom:
            if(pipeline->filter.custom_filter) {
                DataItem temp = {
                    .type = type,
                    .id = id,
                    .timestamp = furi_get_tick(),
                    .size = size,
                    .data = (uint8_t*)data,
                    .compressed = false,
                    .priority = priority
                };
                should_add = pipeline->filter.custom_filter(
                    &temp,
                    pipeline->filter.context
                );
            }
            break;
            
        default:
            break;
    }
    
    if(!should_add) {
        furi_mutex_release(pipeline->mutex);
        return false;
    }
    
    // Item erstellen
    DataItem* item = &pipeline->batch.items[pipeline->batch.count];
    item->type = type;
    item->id = id;
    item->timestamp = furi_get_tick();
    item->size = size;
    item->compressed = false;
    item->priority = priority;
    
    // Daten kopieren
    item->data = malloc(size);
    if(!item->data) {
        furi_mutex_release(pipeline->mutex);
        return false;
    }
    
    memcpy(item->data, data, size);
    
    pipeline->batch.count++;
    pipeline->batch.total_size += size;
    
    furi_mutex_release(pipeline->mutex);
    return true;
}

bool data_pipeline_process_batch(DataPipeline* pipeline) {
    if(!pipeline || pipeline->batch.count == 0) return false;
    
    furi_mutex_acquire(pipeline->mutex, FuriWaitForever);
    
    bool success = true;
    
    // Items nach Priorität sortieren
    for(uint32_t i = 0; i < pipeline->batch.count - 1; i++) {
        for(uint32_t j = 0; j < pipeline->batch.count - i - 1; j++) {
            if(pipeline->batch.items[j].priority <
               pipeline->batch.items[j + 1].priority) {
                // Items tauschen
                DataItem temp = pipeline->batch.items[j];
                pipeline->batch.items[j] = pipeline->batch.items[j + 1];
                pipeline->batch.items[j + 1] = temp;
            }
        }
    }
    
    // Items verarbeiten
    for(uint32_t i = 0; i < pipeline->batch.count; i++) {
        DataItem* item = &pipeline->batch.items[i];
        
        // Komprimierung wenn sinnvoll
        if(!item->compressed && item->size > COMPRESSION_CHUNK) {
            size_t comp_size = item->size * 2; // Worst case
            uint8_t* comp_data = malloc(comp_size);
            
            if(comp_data) {
                if(compression_encode(
                    item->data,
                    item->size,
                    comp_data,
                    &comp_size,
                    9 // Max compression
                )) {
                    // Komprimierte Daten verwenden wenn kleiner
                    if(comp_size < item->size) {
                        free(item->data);
                        item->data = comp_data;
                        item->size = comp_size;
                        item->compressed = true;
                    } else {
                        free(comp_data);
                    }
                } else {
                    free(comp_data);
                }
            }
        }
        
        // Callback aufrufen
        if(pipeline->process_callback) {
            if(!pipeline->process_callback(item, pipeline->callback_context)) {
                success = false;
                break;
            }
        }
        
        pipeline->processed_items++;
    }
    
    furi_mutex_release(pipeline->mutex);
    return success;
}

bool data_pipeline_upload_batch(DataPipeline* pipeline) {
    if(!pipeline || !pipeline->upload_callback ||
       pipeline->batch.count == 0) {
        return false;
    }
    
    furi_mutex_acquire(pipeline->mutex, FuriWaitForever);
    
    bool success = pipeline->upload_callback(
        &pipeline->batch,
        pipeline->callback_context
    );
    
    if(success) {
        // Batch leeren
        for(uint32_t i = 0; i < pipeline->batch.count; i++) {
            free(pipeline->batch.items[i].data);
        }
        
        pipeline->batch.count = 0;
        pipeline->batch.total_size = 0;
    }
    
    furi_mutex_release(pipeline->mutex);
    return success;
}

void data_pipeline_get_stats(
    DataPipeline* pipeline,
    uint32_t* processed,
    uint32_t* failed,
    uint32_t* retries
) {
    if(!pipeline) return;
    
    furi_mutex_acquire(pipeline->mutex, FuriWaitForever);
    
    if(processed) *processed = pipeline->processed_items;
    if(failed) *failed = pipeline->failed_items;
    if(retries) *retries = pipeline->retry_count;
    
    furi_mutex_release(pipeline->mutex);
}
