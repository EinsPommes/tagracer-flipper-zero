#pragma once

#include <furi.h>
#include <furi_hal.h>

typedef struct {
    int status_code;
    char* body;
} FlipperHTTPResponse;

typedef void (*FlipperHTTPCallback)(FlipperHTTPResponse* response, void* context);

typedef struct {
    const char* method;
    const char* url;
    const char* body;
    FlipperHTTPCallback callback;
    void* context;
} FlipperHTTPRequest;

typedef struct FlipperHTTP FlipperHTTP;

FlipperHTTP* flipper_http_alloc();
void flipper_http_free(FlipperHTTP* http);
bool flipper_http_init(FlipperHTTP* http);
void flipper_http_deinit(FlipperHTTP* http);
void flipper_http_send_request(FlipperHTTP* http, FlipperHTTPRequest* request);
