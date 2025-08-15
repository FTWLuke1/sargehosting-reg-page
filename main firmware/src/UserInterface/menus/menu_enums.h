#pragma once

enum MenuState {
    WIFI_MENU, WIFI_SUBMENU,
    BLUETOOTH_MENU, BLUETOOTH_SUBMENU,
    IR_MENU, IR_SUBMENU,
    RF_MENU, RF_SUBMENU,
    NRF_MENU, NRF_SUBMENU,
    RADIO_MENU, RADIO_SUBMENU,
    GPS_MENU, GPS_SUBMENU,
    RPI_MENU, RPI_SUBMENU,
    BADUSB_MENU, BADUSB_SUBMENU,
    SETTINGS_MENU, SETTINGS_SUBMENU,
    EXTRAS_MENU, EXTRAS_SUBMENU,
    FILES_MENU, FILES_SUBMENU,
    RFID_MENU, RFID_SUBMENU,
    NFC_MENU, NFC_SUBMENU
};

void drawWiFiMenu();
void drawWiFisubmenu();
void drawBluetoothMenu();
void drawBluetoothSubmenu();
void drawIRMenu();
void drawIrSubmenu();
void drawRFMenu();
void drawRfSubmenu();
void drawNRFMenu();
void drawNrfSubmenu();
void drawRadioMenu();
void drawRadioSubmenu();
void drawGPSMenu();
void drawGpsSubmenu();
void drawRPIMenu();
void drawRpiSubmenu();
void drawBadUSBMenu();
void drawBadUsbSubmenu();
void drawSettingsMenu();
void drawSettingsSubmenu();
void drawExtrasMenu();
void drawExtrasSubmenu();
void drawFilesMenu();
void drawFilesSubmenu();
void drawRfidMenu();   
void drawRfidSubmenu(); 
void drawNfcMenu();     
void drawNfcSubmenu();  
