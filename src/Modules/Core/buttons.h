#pragma once
#include <TFT_eSPI.h>
#include "././UserInterface/menus/menu_enums.h"

void initButtons();
void updateButtons();
bool btnAPressed();
bool btnBPressed();
bool btnCPressed();
void finalizeButtons();

void handleAllButtonLogic(
  TFT_eSPI* tft,
  bool& inOptionScreen,
  bool& inStopwatch,
  bool& inBGone,
  bool& inIRRead,     // ← ensure this exists here
  MenuState& currentMenu
);
