#include "tagracer_nfc.h"

NFCScanner* nfc_scanner_alloc() {
    NFCScanner* scanner = malloc(sizeof(NFCScanner));
    scanner->running = false;
    return scanner;
}

void nfc_scanner_free(NFCScanner* scanner) {
    furi_assert(scanner);
    free(scanner);
}

static void nfc_scan_task(void* context) {
    NFCScanner* scanner = context;
    
    while(scanner->running) {
        // NFC-Karte erkennen
        if(furi_hal_nfc_detect(&scanner->nfc_data, 200)) {
            TagData tag_data;
            memcpy(tag_data.uid, scanner->nfc_data.uid, scanner->nfc_data.uid_len);
            tag_data.uid_len = scanner->nfc_data.uid_len;
            
            // Callback mit Tag-Daten aufrufen
            if(scanner->callback) {
                scanner->callback(&tag_data, scanner->callback_context);
            }
            
            // Warte kurz, um mehrfaches Scannen zu vermeiden
            furi_delay_ms(500);
        }
        furi_delay_ms(100);
    }
}

void nfc_scanner_start(NFCScanner* scanner, TagCallback callback, void* context) {
    furi_assert(scanner);
    
    scanner->callback = callback;
    scanner->callback_context = context;
    scanner->running = true;
    
    // NFC-Hardware initialisieren
    furi_hal_nfc_init();
    
    // Scan-Task starten
    FuriThread* thread = furi_thread_alloc();
    furi_thread_set_name(thread, "NFCScanTask");
    furi_thread_set_stack_size(thread, 1024);
    furi_thread_set_context(thread, scanner);
    furi_thread_set_callback(thread, nfc_scan_task);
    furi_thread_start(thread);
}

void nfc_scanner_stop(NFCScanner* scanner) {
    furi_assert(scanner);
    scanner->running = false;
    furi_hal_nfc_deinit();
}
