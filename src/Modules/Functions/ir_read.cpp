#include "ir_read.h"
#include "../../UserInterface/menus/menu_submenus.h"
#include <Arduino.h>
#include <TFT_eSPI.h>

// ─────────────────────────────────────────────────────────────────────────────
// Library: Arduino-IRremote
// Add to platformio.ini → lib_deps = arduino-libraries/Arduino-IRremote
// Pin is configured per-env with -DIR_RX_PIN=<num> (see notes below).
// Defaults used if not provided.
// ─────────────────────────────────────────────────────────────────────────────
#if __has_include(<IRremote.hpp>)
  #include <IRremote.hpp>
#else
  #include <IRremote.h>
#endif

#ifndef IR_RX_PIN
  // Default if env not set; matches M5StickC Plus 2 request
  #warning "IR_RX_PIN not defined! Defaulting to GPIO 33."
  #define IR_RX_PIN 33
#endif

#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4;   // consistent with rest of UI
#else
  static constexpr uint8_t ROT_TOP = 2;
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 1) & 0x3;

// ─────────────────────────────────────────────────────────────────────────────
// State
// ─────────────────────────────────────────────────────────────────────────────
enum class IRState : uint8_t { WAITING, RECEIVED };
static IRState  sState   = IRState::WAITING;
static bool     sDirty   = true;
static bool     sPaused  = false;    // toggle via B
static uint32_t sLastMs  = 0;

// Decoded fields (display)
static String   sProto = "-";
static uint32_t sAddr  = 0;
static uint32_t sCmd   = 0;
static uint64_t sValue = 0;          // some protocols carry >32 bits

// Raw waveform capture snapshot
static uint16_t sRawBuf[256];
static uint16_t sRawLen = 0;

// ─────────────────────────────────────────────────────────────────────────────
// IRremote helpers (version-compat)
// ─────────────────────────────────────────────────────────────────────────────
static void snapshotFromDecoder() {
  sProto = "-";
  sAddr = 0;
  sCmd = 0;
  sValue = 0;
  sRawLen = 0;

#if defined(IRremote_HPP) || __has_include(<IRremote.hpp>)
  // Newer IRremote
  auto &d = IrReceiver.decodedIRData;
  sProto = String(getProtocolString(d.protocol));
  sAddr  = d.address;
  sCmd   = d.command;
  sValue = (uint64_t)d.decodedRawData;

  if (d.rawDataPtr && d.rawDataPtr->rawlen > 0) {
    sRawLen = min<uint16_t>(d.rawDataPtr->rawlen, (uint16_t)(sizeof(sRawBuf)/sizeof(sRawBuf[0])));
    for (uint16_t i=0; i<sRawLen; ++i) sRawBuf[i] = d.rawDataPtr->rawbuf[i];
  }
#else
  // Legacy IRremote
  decode_results r;
  if (IrReceiver.decode(&r)) {
    switch (r.decode_type) {
      case NEC:  sProto = "NEC"; break;
      case SONY: sProto = "SONY"; break;
      case RC5:  sProto = "RC5"; break;
      case RC6:  sProto = "RC6"; break;
      default:   sProto = "UNK"; break;
    }
    sAddr  = 0;
    sCmd   = 0;
    sValue = r.value;

    sRawLen = min<uint16_t>(r.rawlen, (uint16_t)(sizeof(sRawBuf)/sizeof(sRawBuf[0])));
    for (uint16_t i=0; i<sRawLen; ++i) sRawBuf[i] = r.rawbuf[i];

    IrReceiver.resume();
  }
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Drawing helpers
// ─────────────────────────────────────────────────────────────────────────────
static void printLine(TFT_eSPI& tft, int x, int& y, const String& s, uint16_t col=TFT_WHITE) {
  tft.setTextColor(col, TFT_BLACK);
  tft.setCursor(x, y);
  tft.print(s);
  y += 14;
}

static void drawWave(TFT_eSPI& tft, int x, int y, int w, int h) {
  tft.drawRect(x, y, w, h, TFT_DARKGREY);
  if (sRawLen < 2) return;

  uint32_t sumTicks = 0;
  for (uint16_t i=1; i<sRawLen; ++i) sumTicks += sRawBuf[i];
  if (!sumTicks) return;

  float pxPerTick = float(w - 2) / float(sumTicks);
  int   cx = x + 1;
  int   mid = y + h/2;
  bool  mark = true;

  for (uint16_t i=1; i<sRawLen; ++i) {
    int pw = max(1, int(sRawBuf[i] * pxPerTick));
    int y0 = mark ? (y+2) : (mid+1);
    int y1 = mark ? (mid-1) : (y + h - 3);
    tft.drawFastHLine(cx, mark ? y1 : y0, pw, TFT_WHITE);
    for (int xx=cx; xx<cx+pw && xx < x+w-1; xx+=3) {
      tft.drawFastVLine(xx, y0, y1 - y0 + 1, mark ? TFT_WHITE : TFT_SILVER);
    }
    cx += pw;
    if (cx >= x + w - 1) break;
    mark = !mark;
  }
}

static void drawUI(TFT_eSPI& tft) {
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);

  const int SW=240, SH=135;
  const int L=10, T=10, R=SW-10, B=SH-10;
  const int W=R-L, H=B-T;

  // Info panel
  const int panelX = L + 4;
  const int panelY = T + 20;
  const int panelW = W - 8;
  const int panelH = 54;
  tft.drawRoundRect(panelX, panelY, panelW, panelH, 6, TFT_DARKGREY);

  int y = panelY + 8;
  tft.setTextSize(2);
  printLine(tft, panelX + 8, y, "Proto: " + sProto);
  tft.setTextSize(1);
  char line[64];
  snprintf(line, sizeof(line), "Addr: 0x%X   Cmd: 0x%X", (unsigned)sAddr, (unsigned)sCmd);
  printLine(tft, panelX + 8, y, String(line));
  snprintf(line, sizeof(line), "Value: 0x%llX", (unsigned long long)sValue);
  printLine(tft, panelX + 8, y, String(line));

  // Waveform
  const int waveX = L + 4;
  const int waveY = panelY + panelH + 8;
  const int waveW = W - 8;
  const int waveH = 46;
  drawWave(tft, waveX, waveY, waveW, waveH);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────
void irReadReset() {
#if defined(IRremote_HPP) || __has_include(<IRremote.hpp>)
  IrReceiver.begin(IR_RX_PIN, ENABLE_LED_FEEDBACK);
#else
  IrReceiver.begin(IR_RX_PIN, ENABLE_LED_FEEDBACK);
#endif
  sState  = IRState::WAITING;
  sPaused = false;
  sDirty  = true;
  sLastMs = millis();
  sProto  = "-"; sAddr=0; sCmd=0; sValue=0; sRawLen=0;
}

void irReadDrawScreen(TFT_eSPI& tft) {
  if (!sDirty) return;
  drawUI(tft);
  sDirty = false;
}

void irReadHandleInput(bool a, bool b, bool c, bool& requestExit) {
  requestExit = false;
  // A → clear
  if (a) {
    sProto="-"; sAddr=0; sCmd=0; sValue=0; sRawLen=0;
    sState = IRState::WAITING;
    sDirty = true;
  }
  // B → pause
  if (b) {
    sPaused = !sPaused;
    sDirty = true;
  }
  // C → exit
  if (c) {
    requestExit = true;
    return;
  }

  if (!sPaused) {
#if defined(IRremote_HPP) || __has_include(<IRremote.hpp>)
    if (IrReceiver.decode()) {
      snapshotFromDecoder();
      IrReceiver.resume();
      sState = IRState::RECEIVED;
      sDirty = true;
    }
#else
    decode_results r;
    if (IrReceiver.decode(&r)) {
      snapshotFromDecoder();
      IrReceiver.resume();
      sState = IRState::RECEIVED;
      sDirty = true;
    }
#endif
  }

  // Redraw throttled
  uint32_t now = millis();
  if (sDirty || (now - sLastMs) > 100) {
    sLastMs = now;
    // draw is performed by caller via irReadDrawScreen()
  }
}
