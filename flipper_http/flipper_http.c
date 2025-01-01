#include "flipper_http.h"
#include <furi_hal_uart.h>
#include <string.h>
#include <stdlib.h>

#define HTTP_BUFFER_SIZE 2048
#define JSON_BUFFER_SIZE 512

struct FlipperHTTP {
    FuriThread* worker_thread;
    FuriStreamBuffer* rx_stream;
    FuriStreamBuffer* tx_stream;
    FlipperHTTPRequest* current_request;
    bool is_running;
    bool request_pending;
};

typedef struct {
    FlipperHTTP* http;
    FlipperHTTPRequest request;
} WorkerContext;

// Worker Thread
static int32_t http_worker(void* context) {
    WorkerContext* worker_ctx = context;
    FlipperHTTP* http = worker_ctx->http;
    uint8_t rx_buffer[HTTP_BUFFER_SIZE];
    size_t rx_len;
    
    while(http->is_running) {
        if(http->request_pending) {
            // Request senden
            const char* method = worker_ctx->request.method;
            const char* url = worker_ctx->request.url;
            const char* body = worker_ctx->request.body;
            
            // HTTP Request formatieren
            char request_buffer[HTTP_BUFFER_SIZE];
            snprintf(request_buffer, sizeof(request_buffer),
                    "%s %s HTTP/1.1\r\n"
                    "Host: localhost\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: %d\r\n"
                    "\r\n"
                    "%s",
                    method, url,
                    body ? strlen(body) : 0,
                    body ? body : "");
            
            // Request über UART senden
            furi_stream_buffer_send(
                http->tx_stream,
                request_buffer,
                strlen(request_buffer),
                FuriWaitForever);
            
            // Auf Antwort warten
            rx_len = furi_stream_buffer_receive(
                http->rx_stream,
                rx_buffer,
                sizeof(rx_buffer),
                1000);
            
            if(rx_len > 0) {
                // Response parsen
                FlipperHTTPResponse response = {0};
                char* status_line = strtok((char*)rx_buffer, "\r\n");
                if(status_line) {
                    char* status_code_str = strchr(status_line, ' ');
                    if(status_code_str) {
                        response.status_code = atoi(status_code_str + 1);
                    }
                }
                
                // Body finden
                char* body_start = strstr((char*)rx_buffer, "\r\n\r\n");
                if(body_start) {
                    body_start += 4;
                    response.body = body_start;
                    response.body_size = rx_len - (body_start - (char*)rx_buffer);
                }
                
                // Callback aufrufen
                if(worker_ctx->request.callback) {
                    worker_ctx->request.callback(&response, worker_ctx->request.context);
                }
            }
            
            http->request_pending = false;
        }
        
        furi_delay_ms(10);
    }
    
    free(worker_ctx);
    return 0;
}

// Öffentliche Funktionen
FlipperHTTP* flipper_http_alloc() {
    FlipperHTTP* http = malloc(sizeof(FlipperHTTP));
    http->rx_stream = furi_stream_buffer_alloc(HTTP_BUFFER_SIZE);
    http->tx_stream = furi_stream_buffer_alloc(HTTP_BUFFER_SIZE);
    http->is_running = false;
    http->request_pending = false;
    return http;
}

void flipper_http_free(FlipperHTTP* http) {
    if(http->is_running) {
        flipper_http_deinit(http);
    }
    furi_stream_buffer_free(http->rx_stream);
    furi_stream_buffer_free(http->tx_stream);
    free(http);
}

bool flipper_http_init(FlipperHTTP* http) {
    if(http->is_running) {
        return false;
    }
    
    // Worker Thread erstellen
    WorkerContext* worker_ctx = malloc(sizeof(WorkerContext));
    worker_ctx->http = http;
    
    http->worker_thread = furi_thread_alloc();
    furi_thread_set_name(http->worker_thread, "HTTPWorker");
    furi_thread_set_stack_size(http->worker_thread, 2048);
    furi_thread_set_context(http->worker_thread, worker_ctx);
    furi_thread_set_callback(http->worker_thread, http_worker);
    
    http->is_running = true;
    furi_thread_start(http->worker_thread);
    
    return true;
}

void flipper_http_deinit(FlipperHTTP* http) {
    if(!http->is_running) {
        return;
    }
    
    http->is_running = false;
    furi_thread_join(http->worker_thread);
    furi_thread_free(http->worker_thread);
}

bool flipper_http_send_request(FlipperHTTP* http, FlipperHTTPRequest* request) {
    if(!http->is_running || http->request_pending) {
        return false;
    }
    
    WorkerContext* worker_ctx = furi_thread_get_context(http->worker_thread);
    memcpy(&worker_ctx->request, request, sizeof(FlipperHTTPRequest));
    http->request_pending = true;
    
    return true;
}

void flipper_http_cancel_request(FlipperHTTP* http) {
    http->request_pending = false;
}

// JSON Hilfsfunktionen
char* flipper_http_json_create_object() {
    char* json = malloc(JSON_BUFFER_SIZE);
    strcpy(json, "{");
    return json;
}

void flipper_http_json_add_string(char* json, const char* key, const char* value) {
    char buffer[JSON_BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "\"%s\":\"%s\",", key, value);
    strcat(json, buffer);
}

void flipper_http_json_add_int(char* json, const char* key, int value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "\"%s\":%d,", key, value);
    strcat(json, buffer);
}

void flipper_http_json_add_bool(char* json, const char* key, bool value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "\"%s\":%s,", key, value ? "true" : "false");
    strcat(json, buffer);
}

void flipper_http_json_close_object(char* json) {
    size_t len = strlen(json);
    if(json[len-1] == ',') {
        json[len-1] = '}';
    } else {
        strcat(json, "}");
    }
}
