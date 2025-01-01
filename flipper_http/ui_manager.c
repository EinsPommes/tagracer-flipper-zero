#include "ui_manager.h"
#include <gui/gui.h>
#include <notification/notification_messages.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define MENU_ITEMS_PER_PAGE 4

static void ui_draw_game(Canvas* canvas, void* context);
static void ui_process_game_input(InputEvent* event, void* context);

UiManager* ui_manager_alloc(void) {
    UiManager* manager = malloc(sizeof(UiManager));
    
    // GUI initialisieren
    manager->gui = furi_record_open(RECORD_GUI);
    
    // View Dispatcher erstellen
    manager->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(manager->view_dispatcher);
    view_dispatcher_attach_to_gui(
        manager->view_dispatcher,
        manager->gui,
        ViewDispatcherTypeFullscreen
    );
    
    // Game View erstellen
    manager->game_view = view_alloc();
    view_set_context(manager->game_view, manager);
    view_set_draw_callback(manager->game_view, ui_draw_game);
    view_set_input_callback(manager->game_view, ui_process_game_input);
    
    // Popup erstellen
    manager->popup = popup_alloc();
    
    // Loading-View erstellen
    manager->loading = loading_alloc();
    
    // Menü erstellen
    manager->menu = submenu_alloc();
    
    // Settings erstellen
    manager->settings = variable_item_list_alloc();
    
    // Stats-Widget erstellen
    manager->stats = widget_alloc();
    
    // Views registrieren
    view_dispatcher_add_view(
        manager->view_dispatcher,
        0,
        manager->game_view
    );
    view_dispatcher_add_view(
        manager->view_dispatcher,
        1,
        popup_get_view(manager->popup)
    );
    view_dispatcher_add_view(
        manager->view_dispatcher,
        2,
        loading_get_view(manager->loading)
    );
    view_dispatcher_add_view(
        manager->view_dispatcher,
        3,
        submenu_get_view(manager->menu)
    );
    view_dispatcher_add_view(
        manager->view_dispatcher,
        4,
        variable_item_list_get_view(manager->settings)
    );
    view_dispatcher_add_view(
        manager->view_dispatcher,
        5,
        widget_get_view(manager->stats)
    );
    
    manager->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    
    return manager;
}

void ui_manager_free(UiManager* manager) {
    if(!manager) return;
    
    // Views entfernen
    view_dispatcher_remove_view(manager->view_dispatcher, 0);
    view_dispatcher_remove_view(manager->view_dispatcher, 1);
    view_dispatcher_remove_view(manager->view_dispatcher, 2);
    view_dispatcher_remove_view(manager->view_dispatcher, 3);
    view_dispatcher_remove_view(manager->view_dispatcher, 4);
    view_dispatcher_remove_view(manager->view_dispatcher, 5);
    
    // Views freigeben
    view_free(manager->game_view);
    popup_free(manager->popup);
    loading_free(manager->loading);
    submenu_free(manager->menu);
    variable_item_list_free(manager->settings);
    widget_free(manager->stats);
    
    // View Dispatcher freigeben
    view_dispatcher_free(manager->view_dispatcher);
    
    furi_record_close(RECORD_GUI);
    furi_mutex_free(manager->mutex);
    
    free(manager);
}

void ui_manager_show_game(UiManager* manager) {
    view_dispatcher_switch_to_view(manager->view_dispatcher, 0);
}

void ui_manager_show_menu(UiManager* manager) {
    submenu_reset(manager->menu);
    
    submenu_add_item(
        manager->menu,
        "Story-Modus",
        0,
        NULL,
        NULL
    );
    submenu_add_item(
        manager->menu,
        "Multiplayer",
        1,
        NULL,
        NULL
    );
    submenu_add_item(
        manager->menu,
        "Training",
        2,
        NULL,
        NULL
    );
    submenu_add_item(
        manager->menu,
        "Statistiken",
        3,
        NULL,
        NULL
    );
    submenu_add_item(
        manager->menu,
        "Einstellungen",
        4,
        NULL,
        NULL
    );
    
    view_dispatcher_switch_to_view(manager->view_dispatcher, 3);
}

void ui_manager_show_notification(
    UiManager* manager,
    const char* message,
    uint32_t duration
) {
    popup_reset(manager->popup);
    popup_set_text(manager->popup, message, 0, 0, AlignLeft, AlignTop);
    popup_set_timeout(manager->popup, duration);
    popup_enable_timeout(manager->popup);
    
    view_dispatcher_switch_to_view(manager->view_dispatcher, 1);
}

void ui_manager_show_loading(UiManager* manager, const char* message) {
    loading_set_text(manager->loading, message);
    view_dispatcher_switch_to_view(manager->view_dispatcher, 2);
}

void ui_manager_hide_loading(UiManager* manager) {
    view_dispatcher_switch_to_view(manager->view_dispatcher, 0);
}

void ui_manager_update_score(UiManager* manager) {
    if(!manager || !manager->game) return;
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    // Score-Anzeige aktualisieren
    char score_text[32];
    snprintf(score_text, sizeof(score_text),
             "Score: %lu", manager->game->score);
             
    // TODO: Score-Widget aktualisieren
    
    furi_mutex_release(manager->mutex);
}

void ui_manager_update_combo(UiManager* manager) {
    if(!manager || !manager->game) return;
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    // Combo-Animation
    if(manager->game->combo_multiplier > 1) {
        NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
        notification_message(notifications, &sequence_blink_yellow_100);
        furi_record_close(RECORD_NOTIFICATION);
        
        char combo_text[32];
        snprintf(combo_text, sizeof(combo_text),
                 "x%lu", manager->game->combo_multiplier);
                 
        // TODO: Combo-Widget aktualisieren
    }
    
    furi_mutex_release(manager->mutex);
}

void ui_manager_animate_tag_scan(UiManager* manager) {
    if(!manager) return;
    
    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
    notification_message(notifications, &sequence_success);
    notification_message(notifications, &sequence_blink_green_100);
    furi_record_close(RECORD_NOTIFICATION);
    
    // TODO: Scan-Animation implementieren
}

void ui_manager_animate_powerup(UiManager* manager) {
    if(!manager) return;
    
    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
    notification_message(notifications, &sequence_blink_magenta_100);
    furi_record_close(RECORD_NOTIFICATION);
    
    // TODO: Power-up-Animation implementieren
}

static void ui_draw_game(Canvas* canvas, void* context) {
    UiManager* manager = context;
    
    furi_mutex_acquire(manager->mutex, FuriWaitForever);
    
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    
    // Score anzeigen
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 12, "Score:");
    
    char score_text[32];
    snprintf(score_text, sizeof(score_text),
             "%lu", manager->game->score);
    canvas_draw_str(canvas, 50, 12, score_text);
    
    // Status anzeigen
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 24, manager->game->status_text);
    
    // Combo anzeigen
    if(manager->game->combo_multiplier > 1) {
        char combo_text[32];
        snprintf(combo_text, sizeof(combo_text),
                 "x%lu", manager->game->combo_multiplier);
        canvas_draw_str(canvas, 2, 36, combo_text);
    }
    
    // Power-ups anzeigen
    // TODO: Power-up-Icons implementieren
    
    furi_mutex_release(manager->mutex);
}

static void ui_process_game_input(InputEvent* event, void* context) {
    UiManager* manager = context;
    
    if(event->type == InputTypeShort) {
        switch(event->key) {
            case InputKeyUp:
                // TODO: Navigation implementieren
                break;
                
            case InputKeyDown:
                // TODO: Navigation implementieren
                break;
                
            case InputKeyLeft:
                // Menü öffnen
                ui_manager_show_menu(manager);
                break;
                
            case InputKeyRight:
                // Aktion ausführen
                break;
                
            case InputKeyOk:
                // Auswählen
                break;
                
            case InputKeyBack:
                // Zurück
                break;
                
            default:
                break;
        }
    }
}
