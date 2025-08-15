#include "bgone.h"
#include "../../UserInterface/menus/menu_submenus.h"
#include <Arduino.h>

#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4; // same baseline as app
#else
  static constexpr uint8_t ROT_TOP = 2;
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 1) & 0x3;

static const char* kItems[] = {
  "<- Back",
  "TV B-Gone",
  "Proj B-Gone",
  "LEDs B-Gone",
  "AC B-Gone",
  "Fan B-Gone"
};
static constexpr int kCount = sizeof(kItems)/sizeof(kItems[0]);
static int selected = 1;

void bgoneReset() { selected = 1; }

static void drawList(TFT_eSPI& tft) {
  // match the layered pane look used by options/stopwatch
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // geometry tuned for 240x135 like the stopwatch UI
  const int lineH = 26;
  const int top   = 18;
  const int left  = 18;
  const int bubbleW = 204;

  for (int i = 0; i < kCount; ++i) {
    const bool sel = (i == selected);
    const int y = top + i * (lineH + 6);

    // bubble
    tft.drawRoundRect(left, y, bubbleW, lineH, 6, TFT_WHITE);
    if (sel) tft.drawRoundRect(left-1, y-1, bubbleW+2, lineH+2, 7, TFT_WHITE);

    // text
    tft.setTextSize(2);
    int tw = tft.textWidth(kItems[i]);
    int tx = left + (bubbleW - tw)/2;
    int ty = y + (lineH - 16)/2;
    tft.setCursor(tx, ty);
    tft.print(kItems[i]);
  }
}

void bgoneDrawScreen(TFT_eSPI& tft) {
  drawList(tft);
}

void bgoneHandleInput(bool aPressed, bool bPressed, bool cPressed, bool& exitRequested) {
  exitRequested = false;

  if (bPressed) {                       // next
    selected = (selected + 1) % kCount;
  }
  if (cPressed) {                       // prev / back
    if (selected == 0) {
      exitRequested = true;             // "<- Back"
      return;
    }
    selected = (selected - 1 + kCount) % kCount;
  }

  if (aPressed) {
    if (selected == 0) {
      exitRequested = true;             // A on "<- Back"
      return;
    }

    // For now, just flash a small confirm box per device type.
    // (Actual IR sending gets wired later.)
    // You can hook the chosen index here.
    // 1=TV, 2=Proj, 3=LEDs, 4=AC, 5=Fan
    // TODO: call IR routine with selected type.
  }
}
