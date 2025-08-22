#pragma once
#include <TFT_eSPI.h>

// “B‑Gone” full‑screen panel, A=select, B/C=navigate, C on “<- Back” exits.
// Matches the layered look (stopwatch/options).

void bgoneReset();
void bgoneDrawScreen(TFT_eSPI& tft);
void bgoneHandleInput(bool aPressed, bool bPressed, bool cPressed, bool& exitRequested);
