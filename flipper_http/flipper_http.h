#pragma once

#include <furi.h>
#include <furi_hal.h>

// HTTP Methoden
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE
} HTTPMethod;

// HTTP Response
typedef struct {
    int status_code;
    char* body;
    size_t body_size;
} FlipperHTTPResponse;

// HTTP Request
typedef struct {
    const char* method;
    const char* url;
    const char* body;
    void (*callback)(FlipperHTTPResponse* response, void* context);
    void* context;
} FlipperHTTPRequest;

// HTTP Client
typedef struct FlipperHTTP FlipperHTTP;

// Öffentliche Funktionen
FlipperHTTP* flipper_http_alloc();
void flipper_http_free(FlipperHTTP* http);
bool flipper_http_init(FlipperHTTP* http);
void flipper_http_deinit(FlipperHTTP* http);
bool flipper_http_send_request(FlipperHTTP* http, FlipperHTTPRequest* request);
void flipper_http_cancel_request(FlipperHTTP* http);

// Hilfsfunktionen für JSON
char* flipper_http_json_create_object();
void flipper_http_json_add_string(char* json, const char* key, const char* value);
void flipper_http_json_add_int(char* json, const char* key, int value);
void flipper_http_json_add_bool(char* json, const char* key, bool value);
void flipper_http_json_close_object(char* json);
