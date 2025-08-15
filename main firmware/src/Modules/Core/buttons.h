#pragma once
#include <TFT_eSPI.h>
#include "../../UserInterface/menus/menu_enums.h"

void initButtons();
void updateButtons();
void finalizeButtons();

// edge-trigger helpers
bool btnAPressed();
bool btnBPressed();
bool btnCPressed();

// main input handler
void handleAllButtonLogic(
  TFT_eSPI* tft,
  bool& inOptionScreen,
  bool& inStopwatch,
  bool& inBGone,
  MenuState& currentMenu
);
