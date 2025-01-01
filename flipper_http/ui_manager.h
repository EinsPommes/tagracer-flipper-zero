#pragma once

#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/popup.h>
#include <gui/modules/loading.h>
#include <gui/modules/submenu.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/widget.h>
#include "game_state.h"
#include "offline_data.h"

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    View* game_view;
    Popup* popup;
    Loading* loading;
    Submenu* menu;
    VariableItemList* settings;
    Widget* stats;
    
    GameContext* game;
    OfflineData* data;
    
    FuriMutex* mutex;
} UiManager;

// Hauptfunktionen
UiManager* ui_manager_alloc(void);
void ui_manager_free(UiManager* manager);
void ui_manager_run(UiManager* manager);

// View Management
void ui_manager_show_game(UiManager* manager);
void ui_manager_show_menu(UiManager* manager);
void ui_manager_show_settings(UiManager* manager);
void ui_manager_show_stats(UiManager* manager);

// Benachrichtigungen
void ui_manager_show_notification(
    UiManager* manager,
    const char* message,
    uint32_t duration
);
void ui_manager_show_error(
    UiManager* manager,
    const char* message
);
void ui_manager_show_loading(
    UiManager* manager,
    const char* message
);
void ui_manager_hide_loading(UiManager* manager);

// Status Updates
void ui_manager_update_score(UiManager* manager);
void ui_manager_update_time(UiManager* manager);
void ui_manager_update_combo(UiManager* manager);
void ui_manager_update_powerups(UiManager* manager);

// Animation
void ui_manager_animate_tag_scan(UiManager* manager);
void ui_manager_animate_powerup(UiManager* manager);
void ui_manager_animate_achievement(UiManager* manager);

// Einstellungen
typedef struct {
    bool sound_enabled;
    bool vibration_enabled;
    bool backlight_enabled;
    uint8_t contrast;
    bool power_save_enabled;
} UiSettings;

void ui_manager_load_settings(UiManager* manager, UiSettings* settings);
void ui_manager_save_settings(UiManager* manager, const UiSettings* settings);

// Themes
typedef enum {
    UiThemeDefault,
    UiThemeDark,
    UiThemeHigh,
    UiThemeCustom
} UiTheme;

void ui_manager_set_theme(UiManager* manager, UiTheme theme);
void ui_manager_customize_theme(
    UiManager* manager,
    uint32_t background,
    uint32_t foreground,
    uint32_t accent
);
