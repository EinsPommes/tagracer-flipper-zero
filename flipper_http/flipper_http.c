#include "flipper_http.h"
#include <furi_hal_uart.h>
#include <string.h>

#define UART_CH UART_CH_1
#define BUFFER_SIZE 1024

struct FlipperHTTP {
    FuriThread* rx_thread;
    uint8_t rx_buffer[BUFFER_SIZE];
    size_t rx_buffer_pos;
    FuriMutex* mutex;
};

static void uart_on_irq_cb(UartIrqEvent event, uint8_t data, void* context) {
    FlipperHTTP* http = context;
    
    if(event == UartIrqEventRXNE) {
        if(http->rx_buffer_pos < BUFFER_SIZE - 1) {
            http->rx_buffer[http->rx_buffer_pos++] = data;
        }
    }
}

static int32_t uart_worker(void* context) {
    FlipperHTTP* http = context;
    
    while(1) {
        furi_delay_ms(100);
        
        if(http->rx_buffer_pos > 0) {
            furi_mutex_acquire(http->mutex, FuriWaitForever);
            
            // Parse response
            FlipperHTTPResponse response = {0};
            char* status_line = strtok((char*)http->rx_buffer, "\r\n");
            if(status_line) {
                sscanf(status_line, "HTTP/1.1 %d", &response.status_code);
                response.body = strstr((char*)http->rx_buffer, "\r\n\r\n");
                if(response.body) {
                    response.body += 4; // Skip \r\n\r\n
                }
            }
            
            // Clear buffer
            memset(http->rx_buffer, 0, BUFFER_SIZE);
            http->rx_buffer_pos = 0;
            
            furi_mutex_release(http->mutex);
        }
    }
    
    return 0;
}

FlipperHTTP* flipper_http_alloc() {
    FlipperHTTP* http = malloc(sizeof(FlipperHTTP));
    http->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    http->rx_buffer_pos = 0;
    return http;
}

void flipper_http_free(FlipperHTTP* http) {
    furi_assert(http);
    furi_mutex_free(http->mutex);
    free(http);
}

bool flipper_http_init(FlipperHTTP* http) {
    furi_assert(http);
    
    // UART initialisieren
    furi_hal_uart_init(UART_CH, 115200);
    furi_hal_uart_set_irq_cb(UART_CH, uart_on_irq_cb, http);
    
    // RX Thread starten
    http->rx_thread = furi_thread_alloc();
    furi_thread_set_name(http->rx_thread, "HTTPRxWorker");
    furi_thread_set_stack_size(http->rx_thread, 1024);
    furi_thread_set_context(http->rx_thread, http);
    furi_thread_set_callback(http->rx_thread, uart_worker);
    furi_thread_start(http->rx_thread);
    
    return true;
}

void flipper_http_deinit(FlipperHTTP* http) {
    furi_assert(http);
    
    furi_thread_free(http->rx_thread);
    furi_hal_uart_deinit(UART_CH);
}

void flipper_http_send_request(FlipperHTTP* http, FlipperHTTPRequest* request) {
    furi_assert(http);
    furi_assert(request);
    
    char buffer[BUFFER_SIZE];
    int len = snprintf(buffer, BUFFER_SIZE,
        "%s %s HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        request->method,
        request->url,
        strlen(request->body),
        request->body);
    
    furi_hal_uart_tx(UART_CH, (uint8_t*)buffer, len);
}
