#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <notification/notification_messages.h>
#include "tagracer_nfc.h"
#include "game_state.h"
#include "flipper_http/flipper_http.h"

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    FuriMessageQueue* event_queue;
    NFCScanner* nfc_scanner;
    FlipperHTTP* http;
    GameContext* game;
    FuriTimer* timer;
} TagRacer;

// HTTP-Callback für Server-Antworten
static void http_callback(FlipperHTTPResponse* response, void* context) {
    TagRacer* tagracer = context;
    if(response->status_code == 200) {
        // Parse JSON response und aktualisiere Score
        uint32_t points;
        if(sscanf(response->body, "{\"points\":%ld}", &points) == 1) {
            tagracer->game->score += points;
        }
    }
}

// Callback für gescannte NFC-Tags
static void tag_callback(TagData* tag_data, void* context) {
    TagRacer* tagracer = context;
    
    // Tag im Spielzustand verarbeiten
    game_state_process_tag(tagracer->game, tag_data);
    
    // Tag-Daten an Server senden
    if(tagracer->http) {
        char tag_id[32];
        for(uint8_t i = 0; i < tag_data->uid_len; i++) {
            snprintf(tag_id + (i * 2), 3, "%02X", tag_data->uid[i]);
        }
        
        // HTTP-Request vorbereiten
        char body[128];
        snprintf(body, sizeof(body),
                "{\"tag_id\":\"%s\",\"player_id\":\"%s\"}",
                tag_id,
                tagracer->game->player_id);
        
        FlipperHTTPRequest request = {
            .method = "POST",
            .url = "http://localhost:5000/api/tag",
            .body = body,
            .callback = http_callback,
            .context = tagracer
        };
        
        flipper_http_send_request(tagracer->http, &request);
    }
}

// Timer-Callback für Spielzeit
static void timer_callback(void* context) {
    TagRacer* tagracer = context;
    game_state_update(tagracer->game);
    view_port_update(tagracer->view_port);
}

// Render callback für die GUI
static void render_callback(Canvas* canvas, void* ctx) {
    TagRacer* tagracer = ctx;
    GameContext* game = tagracer->game;
    
    canvas_clear(canvas);
    
    // Titel
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "TagRacer");
    
    // Spielstatus
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 22, game->status_text);
    
    // Hauptinformationen
    canvas_set_font(canvas, FontPrimary);
    
    // Score
    char score_str[32];
    snprintf(score_str, sizeof(score_str), "Score: %ld", game->score);
    canvas_draw_str(canvas, 2, 36, score_str);
    
    // Zeit
    char time_str[32];
    snprintf(time_str, sizeof(time_str), "Zeit: %ld:%02ld", 
             game->time_remaining / 60, game->time_remaining % 60);
    canvas_draw_str(canvas, 2, 50, time_str);
    
    // Tag-Zähler
    char tags_str[32];
    snprintf(tags_str, sizeof(tags_str), "Tags: %ld", game->tag_count);
    canvas_draw_str(canvas, 2, 64, tags_str);
}

// Input callback für Benutzereingaben
static void input_callback(InputEvent* input_event, void* ctx) {
    TagRacer* tagracer = ctx;
    furi_message_queue_put(tagracer->event_queue, input_event, FuriWaitForever);
}

// Haupteinstiegspunkt der Anwendung
int32_t tagracer_app_main(void* p) {
    UNUSED(p);
    TagRacer* tagracer = malloc(sizeof(TagRacer));

    // Komponenten initialisieren
    tagracer->gui = furi_record_open(RECORD_GUI);
    tagracer->view_port = view_port_alloc();
    tagracer->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    tagracer->game = malloc(sizeof(GameContext));
    
    // Spielzustand initialisieren
    game_state_init(tagracer->game);
    
    // NFC Scanner initialisieren
    tagracer->nfc_scanner = nfc_scanner_alloc();
    nfc_scanner_start(tagracer->nfc_scanner, tag_callback, tagracer);
    
    // HTTP Client initialisieren
    tagracer->http = flipper_http_alloc();
    flipper_http_init(tagracer->http);
    
    // Timer für Spielzeit einrichten
    tagracer->timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, tagracer);
    furi_timer_start(tagracer->timer, 1000);
    
    // GUI einrichten
    view_port_draw_callback_set(tagracer->view_port, render_callback, tagracer);
    view_port_input_callback_set(tagracer->view_port, input_callback, tagracer);
    gui_add_view_port(tagracer->gui, tagracer->view_port, GuiLayerFullscreen);
    
    // Hauptschleife
    InputEvent event;
    while(1) {
        if(furi_message_queue_get(tagracer->event_queue, &event, 100) == FuriStatusOk) {
            if(event.type == InputTypeShort) {
                switch(event.key) {
                    case InputKeyOk:
                        // Spiel starten/neustarten
                        if(tagracer->game->state == GameStateIdle ||
                           tagracer->game->state == GameStateFinished) {
                            game_state_start(tagracer->game);
                        }
                        break;
                        
                    case InputKeyBack:
                        // Spiel beenden
                        goto exit;
                        
                    default:
                        break;
                }
            }
        }
        
        view_port_update(tagracer->view_port);
    }

exit:
    // Aufräumen
    furi_timer_free(tagracer->timer);
    nfc_scanner_stop(tagracer->nfc_scanner);
    nfc_scanner_free(tagracer->nfc_scanner);
    flipper_http_deinit(tagracer->http);
    flipper_http_free(tagracer->http);
    view_port_enabled_set(tagracer->view_port, false);
    gui_remove_view_port(tagracer->gui, tagracer->view_port);
    view_port_free(tagracer->view_port);
    furi_message_queue_free(tagracer->event_queue);
    furi_record_close(RECORD_GUI);
    free(tagracer->game);
    free(tagracer);

    return 0;
}
