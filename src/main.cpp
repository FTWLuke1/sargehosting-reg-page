#include <Arduino.h>

#if defined(M5CARDPUTER)
  #include <M5Cardputer.h>
#else
  #include <M5StickCPlus2.h>
#endif

#include <TFT_eSPI.h>
#include "UserInterface/menus/menu_enums.h"
#include "UserInterface/bitmaps/menu_bitmaps.h"
#include "UserInterface/menus/menu_submenus.h"
#include "UserInterface/menus/submenu_options.h"
#include "Modules/Functions/stopwatch.h"
#include "Modules/Core/buttons.h"

#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4;
#else
  static constexpr uint8_t ROT_TOP = 2;
#endif

#if defined(M5CARDPUTER)
  #define BACKLIGHT_PIN 38
#else
  #define BACKLIGHT_PIN 27
#endif

TFT_eSPI tft;
MenuState currentMenu = WIFI_MENU;
bool inOptionScreen = false;
bool inStopwatch    = false;
bool inBGone        = false;
bool inIRRead       = false;   // ← NEW: IR Reader app active flag

void setup() {
  Serial.begin(115200);
#if defined(M5CARDPUTER)
  M5Cardputer.begin();
#else
  M5.begin();
#endif

  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  tft.begin();
  tft.setRotation(ROT_TOP);
  tft.fillScreen(TFT_BLACK);
  delay(100);

  initButtons();
  initSubmenuOptions(&tft);
  drawWiFiMenu();
}

void loop() {
  updateButtons();
  // Updated call includes the new inIRRead flag (5th bool reference)
  handleAllButtonLogic(&tft, inOptionScreen, inStopwatch, inBGone, inIRRead, currentMenu);
}
