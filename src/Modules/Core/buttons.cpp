#include "buttons.h"

#if defined(M5CARDPUTER)
  #include <M5Cardputer.h>
  static constexpr uint8_t ROT_TOP = 4;   // Cardputer base UI
#else
  #include <M5StickCPlus2.h>
  static constexpr uint8_t ROT_TOP = 2;   // M5Stick base UI
#endif

#include <Arduino.h>
#include "././UserInterface/bitmaps/menu_bitmaps.h"
#include "././UserInterface/menus/menu_submenus.h"
#include "././UserInterface/menus/submenu_options.h"
#include "././Modules/Functions/stopwatch.h"
#include "././Modules/Functions/bgone.h"
#include "././UserInterface/menus/menu_enums.h"
#include "././Modules/Functions/ir_read.h"

// --------- Inputs (Cardputer keyboard vs Stick pins) ----------
#if defined(M5CARDPUTER)
#ifndef KEY_ENTER
#define KEY_ENTER 0x28
#endif
#ifndef KEY_SEMICOLON
#define KEY_SEMICOLON 0x33
#endif
#ifndef KEY_DOT
#define KEY_DOT 0x37
#endif
#ifndef KEY_BACKTICK
#define KEY_BACKTICK 0x35
#endif
#else
#ifndef BTN_B_PIN
#define BTN_B_PIN 39
#endif
#ifndef BTN_C_PIN
#define BTN_C_PIN 35
#endif
#endif
// --------------------------------------------------------------

static bool lastA=false,lastB=false,lastC=false;
static bool currA=false,currB=false,currC=false;

static bool lastExitSpecial=false;
static bool currExitSpecial=false;

void initButtons() {
#if defined(M5CARDPUTER)
  // no GPIO init for Cardputer keyboard
#else
  pinMode(BTN_B_PIN, INPUT_PULLUP);
  pinMode(BTN_C_PIN, INPUT_PULLUP);
#endif
}

void updateButtons() {
#if defined(M5CARDPUTER)
  M5Cardputer.update();
  bool aNow =
      M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) ||
      M5Cardputer.Keyboard.isKeyPressed('\r');
  bool bNow =
      M5Cardputer.Keyboard.isKeyPressed('.') ||
      M5Cardputer.Keyboard.isKeyPressed(KEY_DOT);
  bool cNow =
      M5Cardputer.Keyboard.isKeyPressed(';') ||
      M5Cardputer.Keyboard.isKeyPressed(KEY_SEMICOLON);
  bool exitNow =
      M5Cardputer.Keyboard.isKeyPressed('`') ||
      M5Cardputer.Keyboard.isKeyPressed(KEY_BACKTICK);

  currA = aNow; currB = bNow; currC = cNow;
  currExitSpecial = exitNow;
#else
  M5.update();
  currA = M5.BtnA.wasPressed();
  currB = !digitalRead(BTN_B_PIN);
  currC = !digitalRead(BTN_C_PIN);
  currExitSpecial = false;
#endif
}

bool btnAPressed(){ return currA && !lastA; }
bool btnBPressed(){ return currB && !lastB; }
bool btnCPressed(){ return currC && !lastC; }
static bool btnExitSpecialPressed(){ return currExitSpecial && !lastExitSpecial; }

void finalizeButtons() {
  lastA = currA; lastB = currB; lastC = currC;
  lastExitSpecial = currExitSpecial;
}

// Centralize “entering a submenu row” behavior
static void handleSubmenuAction(
  MenuState currentMenu,
  int idx,
  TFT_eSPI* tft,
  bool& inStopwatch,
  bool& inOptionScreen,
  bool& inBGone,
  bool& inIRRead
) {
  // EXTRAS → Stopwatch opens dedicated UI
  if (currentMenu == EXTRAS_SUBMENU && idx == 1) {
    resetStopwatch();
    drawStopwatchScreen(*tft);
    inStopwatch = true;
    return;
  }

  // IR → B-Gone
  if (currentMenu == IR_SUBMENU && idx == 1) {
    bgoneReset();
    bgoneDrawScreen(*tft);
    inBGone = true;
    return;
  }

  // IR → Read (index 4)
  if (currentMenu == IR_SUBMENU && idx == 4) {
    irReadReset();
    irReadDrawScreen(*tft);
    inIRRead = true;         // FIX: use the proper flag (was 'IrRead')
    return;
  }

  // default: layered option screen
  drawOptionsLayerBackground(*tft);
  inOptionScreen = true;
}

void handleAllButtonLogic(
  TFT_eSPI* tft,
  bool& inOptionScreen,
  bool& inStopwatch,
  bool& inBGone,
  bool& inIRRead,
  MenuState& currentMenu
) {
  unsigned long now = millis();

  // ---------- IR Read mode ----------
  if (inIRRead) {
    bool exitReq = false;
#if defined(M5CARDPUTER)
    // A=Clear, B=Pause/Resume, C or backtick=Exit
    irReadHandleInput(btnAPressed(), btnBPressed(), btnCPressed() || btnExitSpecialPressed(), exitReq);
#else
    irReadHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
#endif
    if (exitReq) {
      inIRRead = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawIrSubmenu();
      finalizeButtons();
      return;
    }
    irReadDrawScreen(*tft);
    finalizeButtons();
    delay(30);
    return;
  }

  // ---------- B-Gone mode ----------
  if (inBGone) {
    bool exitReq = false;
    bgoneHandleInput(btnAPressed(), btnBPressed(), btnCPressed(), exitReq);
    if (exitReq) {
      inBGone = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawIrSubmenu();
      finalizeButtons();
      return;
    }
    bgoneDrawScreen(*tft);
    finalizeButtons();
    delay(30);
    return;
  }

  // ---------- Stopwatch mode ----------
  if (inStopwatch) {
#if defined(M5CARDPUTER)
    if (btnExitSpecialPressed()) {
#else
    if (btnCPressed()) {
#endif
      inStopwatch = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
#if defined(M5CARDPUTER)
      drawOptionsLayerBackground(*tft);
#endif
      drawExtrasSubmenu();
      finalizeButtons();
      return;
    }

    if (btnAPressed()) {
      handleStopwatchInput(true);
      drawStopwatchScreen(*tft);
    } else {
      if (isStopwatchRunning()) {
        drawStopwatchTimeOnly(*tft);
      }
    }
    finalizeButtons();
    delay(30);
    return;
  }

  // ---------- Option screen active ----------
  if (inOptionScreen && btnCPressed()) {
    inOptionScreen = false;
    tft->fillScreen(TFT_BLACK);
    tft->setRotation(ROT_TOP);
    switch(currentMenu) {
      case WIFI_SUBMENU:       drawWiFisubmenu(); break;
      case BLUETOOTH_SUBMENU:  drawBluetoothSubmenu(); break;
      case IR_SUBMENU:         drawIrSubmenu(); break;
      case RF_SUBMENU:         drawRfSubmenu(); break;
      case NRF_SUBMENU:        drawNrfSubmenu(); break;
      case RADIO_SUBMENU:      drawRadioSubmenu(); break;
      case GPS_SUBMENU:        drawGpsSubmenu(); break;
      case RPI_SUBMENU:        drawRpiSubmenu(); break;
      case BADUSB_SUBMENU:     drawBadUsbSubmenu(); break;
      case SETTINGS_SUBMENU:   drawSettingsSubmenu(); break;
      case EXTRAS_SUBMENU:     drawExtrasSubmenu(); break;
      case FILES_SUBMENU:      drawFilesSubmenu(); break;
      case RFID_SUBMENU:       drawRfidSubmenu(); break;
      case NFC_SUBMENU:        drawNfcSubmenu(); break;
      default: break;
    }
    finalizeButtons();
    return;
  }

  if (inOptionScreen && btnAPressed()) {
    int idx = getSubmenuOptionIndex();
    if (idx == 0) {
      inOptionScreen = false;
      tft->fillScreen(TFT_BLACK);
      tft->setRotation(ROT_TOP);
      switch(currentMenu) {
        case WIFI_SUBMENU:       drawWiFisubmenu(); break;
        case BLUETOOTH_SUBMENU:  drawBluetoothSubmenu(); break;
        case IR_SUBMENU:         drawIrSubmenu(); break;
        case RF_SUBMENU:         drawRfSubmenu(); break;
        case NRF_SUBMENU:        drawNrfSubmenu(); break;
        case RADIO_SUBMENU:      drawRadioSubmenu(); break;
        case GPS_SUBMENU:        drawGpsSubmenu(); break;
        case RPI_SUBMENU:        drawRpiSubmenu(); break;
        case BADUSB_SUBMENU:     drawBadUsbSubmenu(); break;
        case SETTINGS_SUBMENU:   drawSettingsSubmenu(); break;
        case EXTRAS_SUBMENU:     drawExtrasSubmenu(); break;
        case FILES_SUBMENU:      drawFilesSubmenu(); break;
        case RFID_SUBMENU:       drawRfidSubmenu(); break;
        case NFC_SUBMENU:        drawNfcSubmenu(); break;
        default: break;
      }
      finalizeButtons();
      return;
    }
  }

  // ---------- Submenu browsing ----------
  if (!inOptionScreen && !inStopwatch && !inIRRead && !inBGone) {
    switch(currentMenu) {
      case WIFI_SUBMENU: case BLUETOOTH_SUBMENU: case IR_SUBMENU:
      case RF_SUBMENU: case NRF_SUBMENU: case RADIO_SUBMENU:
      case GPS_SUBMENU: case RPI_SUBMENU: case BADUSB_SUBMENU:
      case SETTINGS_SUBMENU: case EXTRAS_SUBMENU: case FILES_SUBMENU:
      case RFID_SUBMENU: case NFC_SUBMENU:
        if (btnBPressed()) { nextSubmenuOption();     drawSubmenuOptions(); }
        if (btnCPressed()) { previousSubmenuOption(); drawSubmenuOptions(); }
        if (btnAPressed()) {
          int idx = getSubmenuOptionIndex();
          if (idx == 0) {
            switch(currentMenu) {
              case WIFI_SUBMENU:       currentMenu = WIFI_MENU;       drawWiFiMenu(); break;
              case BLUETOOTH_SUBMENU:  currentMenu = BLUETOOTH_MENU;  drawBluetoothMenu(); break;
              case IR_SUBMENU:         currentMenu = IR_MENU;         drawIRMenu(); break;
              case RF_SUBMENU:         currentMenu = RF_MENU;         drawRFMenu(); break;
              case NRF_SUBMENU:        currentMenu = NRF_MENU;        drawNRFMenu(); break;
              case RADIO_SUBMENU:      currentMenu = RADIO_MENU;      drawRadioMenu(); break;
              case GPS_SUBMENU:        currentMenu = GPS_MENU;        drawGPSMenu(); break;
              case RPI_SUBMENU:        currentMenu = RPI_MENU;        drawRPIMenu(); break;
              case BADUSB_SUBMENU:     currentMenu = BADUSB_MENU;     drawBadUSBMenu(); break;
              case SETTINGS_SUBMENU:   currentMenu = SETTINGS_MENU;   drawSettingsMenu(); break;
              case EXTRAS_SUBMENU:     currentMenu = EXTRAS_MENU;     drawExtrasMenu(); break;
              case FILES_SUBMENU:      currentMenu = FILES_MENU;      drawFilesMenu(); break;
              case RFID_SUBMENU:       currentMenu = RFID_MENU;       drawRfidMenu(); break;
              case NFC_SUBMENU:        currentMenu = NFC_MENU;        drawNfcMenu(); break;
              default: break;
            }
          } else {
            handleSubmenuAction(currentMenu, idx, tft, inStopwatch, inOptionScreen, inBGone, inIRRead);
          }
        }
        finalizeButtons();
        return;
      default: break;
    }
  }

  // ---------- Top-level ring ----------
  if (!inOptionScreen && !inStopwatch && !inIRRead && !inBGone) {
    static unsigned long last = 0;
    if (now - last > 200) {
      if (btnBPressed()) {
        last = now;
        switch(currentMenu) {
          case WIFI_MENU:        currentMenu=NFC_MENU;         drawNfcMenu();        break;
          case BLUETOOTH_MENU:   currentMenu=WIFI_MENU;        drawWiFiMenu();       break;
          case IR_MENU:          currentMenu=BLUETOOTH_MENU;   drawBluetoothMenu();  break;
          case RF_MENU:          currentMenu=IR_MENU;          drawIRMenu();         break;
          case NRF_MENU:         currentMenu=RF_MENU;          drawRFMenu();         break;
          case RADIO_MENU:       currentMenu=NRF_MENU;         drawNRFMenu();        break;
          case GPS_MENU:         currentMenu=RADIO_MENU;       drawRadioMenu();      break;
          case RPI_MENU:         currentMenu=GPS_MENU;         drawGPSMenu();        break;
          case BADUSB_MENU:      currentMenu=RPI_MENU;         drawRPIMenu();        break;
          case SETTINGS_MENU:    currentMenu=BADUSB_MENU;      drawBadUSBMenu();     break;
          case EXTRAS_MENU:      currentMenu=SETTINGS_MENU;    drawSettingsMenu();   break;
          case FILES_MENU:       currentMenu=EXTRAS_MENU;      drawExtrasMenu();     break;
          case RFID_MENU:        currentMenu=FILES_MENU;       drawFilesMenu();      break;
          case NFC_MENU:         currentMenu=RFID_MENU;        drawRfidMenu();       break;
          default: break;
        }
      }
      if (btnCPressed()) {
        last = now;
        switch(currentMenu) {
          case WIFI_MENU:        currentMenu=BLUETOOTH_MENU;  drawBluetoothMenu();   break;
          case BLUETOOTH_MENU:   currentMenu=IR_MENU;         drawIRMenu();          break;
          case IR_MENU:          currentMenu=RF_MENU;         drawRFMenu();          break;
          case RF_MENU:          currentMenu=NRF_MENU;        drawNRFMenu();         break;
          case NRF_MENU:         currentMenu=RADIO_MENU;      drawRadioMenu();       break;
          case RADIO_MENU:       currentMenu=GPS_MENU;        drawGPSMenu();         break;
          case GPS_MENU:         currentMenu=RPI_MENU;        drawRPIMenu();         break;
          case RPI_MENU:         currentMenu=BADUSB_MENU;     drawBadUSBMenu();      break;
          case BADUSB_MENU:      currentMenu=SETTINGS_MENU;   drawSettingsMenu();    break;
          case SETTINGS_MENU:    currentMenu=EXTRAS_MENU;     drawExtrasMenu();      break;
          case EXTRAS_MENU:      currentMenu=FILES_MENU;      drawFilesMenu();       break;
          case FILES_MENU:       currentMenu=RFID_MENU;       drawRfidMenu();        break;
          case RFID_MENU:        currentMenu=NFC_MENU;        drawNfcMenu();         break;
          case NFC_MENU:         currentMenu=WIFI_MENU;       drawWiFiMenu();        break;
          default: break;
        }
      }
    }
  }

  // ---------- Enter submenu from top-level ----------
  if (!inOptionScreen && !inStopwatch && !inIRRead && !inBGone && btnAPressed()) {
    tft->fillScreen(TFT_BLACK);
    tft->setRotation(ROT_TOP);
    switch (currentMenu) {
      case WIFI_MENU:        currentMenu = WIFI_SUBMENU;       setSubmenuType(SUBMENU_WIFI);       drawWiFisubmenu();       break;
      case BLUETOOTH_MENU:   currentMenu = BLUETOOTH_SUBMENU;  setSubmenuType(SUBMENU_BLUETOOTH);  drawBluetoothSubmenu();  break;
      case IR_MENU:          currentMenu = IR_SUBMENU;         setSubmenuType(SUBMENU_IR);         drawIrSubmenu();         break;
      case RF_MENU:          currentMenu = RF_SUBMENU;         setSubmenuType(SUBMENU_RF);         drawRfSubmenu();         break;
      case NRF_MENU:         currentMenu = NRF_SUBMENU;        setSubmenuType(SUBMENU_NRF);        drawNrfSubmenu();        break;
      case RADIO_MENU:       currentMenu = RADIO_SUBMENU;      setSubmenuType(SUBMENU_RADIO);      drawRadioSubmenu();      break;
      case GPS_MENU:         currentMenu = GPS_SUBMENU;        setSubmenuType(SUBMENU_GPS);        drawGpsSubmenu();        break;
      case RPI_MENU:         currentMenu = RPI_SUBMENU;        setSubmenuType(SUBMENU_RPI);        drawRpiSubmenu();        break;
      case BADUSB_MENU:      currentMenu = BADUSB_SUBMENU;     setSubmenuType(SUBMENU_BADUSB);     drawBadUsbSubmenu();     break;
      case SETTINGS_MENU:    currentMenu = SETTINGS_SUBMENU;   setSubmenuType(SUBMENU_SETTINGS);   drawSettingsSubmenu();   break;
      case EXTRAS_MENU:      currentMenu = EXTRAS_SUBMENU;     setSubmenuType(SUBMENU_EXTRAS);     drawExtrasSubmenu();     break;
      case FILES_MENU:       currentMenu = FILES_SUBMENU;      setSubmenuType(SUBMENU_FILES);      drawFilesSubmenu();      break;
      case RFID_MENU:        currentMenu = RFID_SUBMENU;       setSubmenuType(SUBMENU_RFID);       drawRfidSubmenu();       break;
      case NFC_MENU:         currentMenu = NFC_SUBMENU;        setSubmenuType(SUBMENU_NFC);        drawNfcSubmenu();        break;
      default: break;
    }
  }

  finalizeButtons();
}
