#pragma once

#include <furi.h>
#include <furi_hal_nfc.h>
#include <gui/gui.h>

typedef struct {
    uint8_t uid[10];
    uint8_t uid_len;
} TagData;

typedef void (*TagCallback)(TagData* tag_data, void* context);

typedef struct {
    FuriHalNfcDevData nfc_data;
    FuriHalNfcTxRxContext tx_rx;
    TagCallback callback;
    void* callback_context;
    bool running;
} NFCScanner;

NFCScanner* nfc_scanner_alloc();
void nfc_scanner_free(NFCScanner* scanner);
void nfc_scanner_start(NFCScanner* scanner, TagCallback callback, void* context);
void nfc_scanner_stop(NFCScanner* scanner);
