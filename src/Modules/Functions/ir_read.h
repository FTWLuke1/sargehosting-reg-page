#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

// Enter/exit lifecycle
void irReadReset();                         // init receiver, clear state
void irReadDrawScreen(TFT_eSPI& tft);       // full redraw (dirty-aware internally)
void irReadHandleInput(bool a, bool b, bool c, bool& requestExit);
                                            // A = clear, B = pause/resume, C = exit
