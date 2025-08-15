#pragma once
#include <TFT_eSPI.h>

// Simple “B‑Gone” picker (TV/Projector/LEDs/etc.) with a look/feel
// similar to the stopwatch: full‑screen panel; A=select, B/C=navigate,
// C=back one screen.

void bgoneReset();
void bgoneDrawScreen(TFT_eSPI& tft);
void bgoneHandleInput(bool aPressed, bool bPressed, bool cPressed, bool& exitRequested);
