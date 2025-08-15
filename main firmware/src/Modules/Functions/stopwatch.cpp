#include "stopwatch.h"
#include "../../UserInterface/menus/menu_submenus.h"

#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4; // Cardputer base
#else
  static constexpr uint8_t ROT_TOP = 2; // Stick base
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 1) & 0x3; // rotated view for time

static unsigned long stopwatchStart   = 0;
static unsigned long stopwatchPausedAt= 0;
static bool          stopwatchRunning = false;

void resetStopwatch() {
  stopwatchStart    = millis();
  stopwatchPausedAt = 0;
  stopwatchRunning  = false;
}

static void drawPlayButton(TFT_eSPI& tft, int x, int y, int w, int h, bool active) {
  tft.drawRoundRect(x, y, w, h, 8, TFT_WHITE);
  tft.fillRoundRect(x, y, w, h, 8, TFT_BLACK);
  int cx = x + w/2, cy = y + h/2;
  int half = h/3;
  tft.fillTriangle(cx - half, cy - half, cx - half, cy + half, cx + half, cy, TFT_WHITE);
  if (active) tft.drawRoundRect(x-2, y-2, w+4, h+4, 10, TFT_WHITE);
}

static void drawPauseButton(TFT_eSPI& tft, int x, int y, int w, int h, bool active) {
  tft.drawRoundRect(x, y, w, h, 8, TFT_WHITE);
  tft.fillRoundRect(x, y, w, h, 8, TFT_BLACK);
  int barW = w / 6;
  int barH = h * 3/5;
  int barY = y + (h - barH)/2;
  tft.fillRect(x + w/3 - barW,    barY, barW, barH, TFT_WHITE);
  tft.fillRect(x + 2*w/3,         barY, barW, barH, TFT_WHITE);
  if (active) tft.drawRoundRect(x-2, y-2, w+4, h+4, 10, TFT_WHITE);
}

void drawStopwatchScreen(TFT_eSPI& tft) {
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);

  unsigned long elapsed = stopwatchRunning ? (millis() - stopwatchStart) : stopwatchPausedAt;
  unsigned long cs = (elapsed / 10) % 100;
  unsigned long s  = (elapsed / 1000) % 60;
  unsigned long m  = (elapsed / 60000);
  char buf[12];
  sprintf(buf, "%02lu:%02lu.%02lu", m, s, cs);

  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int textW = tft.textWidth(buf);
  int textX = (240 - textW) / 2;
  int textY = 28;
  tft.setCursor(textX, textY);
  tft.print(buf);

  int btnW = 48, btnH = 48;
  int btnX = (240 - btnW) / 2;
  int btnY = 75;

  if (stopwatchRunning)
    drawPauseButton(tft, btnX, btnY, btnW, btnH, true);
  else
    drawPlayButton(tft, btnX, btnY, btnW, btnH, true);
}

void drawStopwatchTimeOnly(TFT_eSPI& tft) {
  tft.setRotation(ROT_ALT);

  unsigned long elapsed = stopwatchRunning ? (millis() - stopwatchStart) : stopwatchPausedAt;
  unsigned long cs = (elapsed / 10) % 100;
  unsigned long s  = (elapsed / 1000) % 60;
  unsigned long m  = (elapsed / 60000);
  char buf[12];
  sprintf(buf, "%02lu:%02lu.%02lu", m, s, cs);

  tft.setTextSize(3);
  int textW = tft.textWidth(buf);
  int textX = (240 - textW) / 2;
  int textY = 28;

  tft.fillRect(textX - 4, textY - 4, textW + 8, 36, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(textX, textY);
  tft.print(buf);
}

void handleStopwatchInput(bool btnA) {
  if (btnA) {
    if (stopwatchRunning) {
      stopwatchRunning  = false;
      stopwatchPausedAt = millis() - stopwatchStart;
    } else {
      stopwatchRunning  = true;
      stopwatchStart    = millis() - stopwatchPausedAt;
    }
  }
}

bool isStopwatchRunning() {
  return stopwatchRunning;
}
