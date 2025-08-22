#include "bgone.h"
#include "../../UserInterface/menus/menu_submenus.h"
#include <Arduino.h>

#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4;
#else
  static constexpr uint8_t ROT_TOP = 2;
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 1) & 0x3;

enum class BGoneMode : uint8_t { MENU, TV_REMOTE };
static BGoneMode mode = BGoneMode::MENU;

// ============================ MAIN MENU ===========================
static const char* kItems[] = {
  "<- Back",
  "Add",
  "TV",
  "Proj",
  "LEDs",
  "AC",
  "Fan"
};
static constexpr int kCount = sizeof(kItems)/sizeof(kItems[0]);
static int  selected  = 1;    // default on Add
static int  pageStart = 0;
static bool dirty     = true; // redraw only when dirty

// ---------------- menu icons ----------------
static void iconAdd (TFT_eSPI& t, int x, int y, int w, int h, uint16_t c) {
  int cx = x + w/2, cy = y + h/2, r = min(w,h)/3;
  t.drawLine(cx-r, cy, cx+r, cy, c);
  t.drawLine(cx, cy-r, cx, cy+r, c);
}
static void iconTV  (TFT_eSPI& t, int x, int y, int w, int h, uint16_t c){
  t.drawRoundRect(x, y, w, h-4, 3, c);
  t.drawFastHLine(x + w/4, y + h-3, w/2, c);
}
static void iconProj(TFT_eSPI& t, int x, int y, int w, int h, uint16_t c){
  int r = min(w, h) / 4;
  t.drawRoundRect(x, y + 2, w, h-4, 3, c);
  t.drawCircle(x + w/2, y + h/2, r, c);
  t.drawCircle(x + w - r - 5, y + h/2, r/2, c);
}
static void iconLED (TFT_eSPI& t, int x, int y, int w, int h, uint16_t c){
  int pad = 6;
  t.drawRoundRect(x, y, w, h, 3, c);
  for (int i=0;i<4;i++) {
    int bx = x + pad + i*( (w-2*pad)/4 );
    t.drawRect(bx, y + h/2 - 4, 8, 8, c);
  }
}
static void iconAC  (TFT_eSPI& t, int x, int y, int w, int h, uint16_t c){
  int cx = x + w/2, cy = y + h/2, r = min(w,h)/3;
  t.drawLine(cx-r, cy, cx+r, cy, c);
  t.drawLine(cx, cy-r, cx, cy+r, c);
  t.drawLine(cx-int(r*0.7), cy-int(r*0.7), cx+int(r*0.7), cy+int(r*0.7), c);
  t.drawLine(cx-int(r*0.7), cy+int(r*0.7), cx+int(r*0.7), cy-int(r*0.7), c);
}
static void iconFan (TFT_eSPI& t, int x, int y, int w, int h, uint16_t c){
  int cx = x + w/2, cy = y + h/2, r = min(w,h)/4;
  t.drawCircle(cx, cy, 2, c);
  t.drawCircle(cx + r, cy, r, c);
  t.drawCircle(cx - r/2, cy + int(0.866*r), r, c);
  t.drawCircle(cx - r/2, cy - int(0.866*r), r, c);
}

static void drawMenuGrid(TFT_eSPI& tft){
  // Background boundary
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);

  const int cols   = 2;
  const int rows   = 2;
  const int tileW  = 100;
  const int tileH  = 38;
  const int gapX   = 12;
  const int gapY   = 12;
  const int startX = 14;
  const int startY = 28;    
  const int perPage = cols * rows;

  auto drawTile = [&](int idx, int x, int y) {
    bool sel = (idx == selected);
    uint16_t frame = sel ? TFT_WHITE : TFT_DARKGREY;
    uint16_t label = sel ? TFT_WHITE : TFT_SILVER;

    tft.drawRoundRect(x, y, tileW, tileH, 6, frame);
    if (sel) tft.drawRoundRect(x-1, y-1, tileW+2, tileH+2, 7, frame);

    int iconX = x + 8, iconY = y + 6, iconW = 22, iconH = tileH - 12;
    switch (idx) {
      case 0: break; // Back has its own arrow text
      case 1: iconAdd (tft, iconX, iconY, iconW, iconH, frame); break;
      case 2: iconTV  (tft, iconX, iconY, iconW, iconH, frame); break;
      case 3: iconProj(tft, iconX, iconY, iconW, iconH, frame); break;
      case 4: iconLED (tft, iconX, iconY, iconW, iconH, frame); break;
      case 5: iconAC  (tft, iconX, iconY, iconW, iconH, frame); break;
      case 6: iconFan (tft, iconX, iconY, iconW, iconH, frame); break;
    }

    tft.setTextColor(label, TFT_BLACK);
    tft.setTextSize(2);
    String name = kItems[idx];
    int lw = tft.textWidth(name);
    tft.setCursor(x + tileW - lw - 8, y + (tileH - 16)/2);
    tft.print(name);
  };

  const int end = min(pageStart + perPage, kCount);
  int visibleIdx = 0;
  for (int i = pageStart; i < end; ++i, ++visibleIdx) {
    int r = visibleIdx / cols;
    int c = visibleIdx % cols;
    int x = startX + c*(tileW + gapX);
    int y = startY + r*(tileH + gapY);
    drawTile(i, x, y);
  }
}

static void ensureMenuSelectionVisible(){
  const int perPage = 4;
  if (selected < pageStart || selected >= pageStart + perPage) {
    pageStart = (selected / perPage) * perPage;
    dirty = true;
  }
}

// ========================= TV REMOTE VIEW ========================
// Only: Power, Mute, Vol+, Vol-, Ch+, Ch-
// - No icons for Power/Mute
// - Vol/Ch show text only (with +/-)
// - Moved the bezel/box and its contents up a bit (per request)
// - Btn C exits (no Back button drawn)

enum TVIdx {
  TV_POWER = 0,
  TV_MUTE,
  TV_VOL_UP, TV_VOL_DOWN,
  TV_CH_UP,  TV_CH_DOWN,
  TV_COUNT
};
static int tvSel = TV_POWER;

static void drawTVRemote(TFT_eSPI& tft){
  // Outer background (boundary)
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);

  // Bezel/box moved UP a little from previous version
  const int bezelX = 18;
  const int bezelY = 30;      // was 36 → 32 → now 30 (slightly higher)
  const int bezelW = 204;
  const int bezelH = 100;
  tft.drawRoundRect(bezelX, bezelY, bezelW, bezelH, 8, TFT_DARKGREY);

  // Two columns, three rows, fit fully inside the bezel
  const int colGap = 16;
  const int keyW   = 84;       // 84*2 + 16 = 184 < 204
  const int keyH   = 22;
  const int rowGap = 8;

  const int leftX  = bezelX + (bezelW - (keyW*2 + colGap))/2; // centered columns
  const int rightX = leftX + keyW + colGap;

  // Center vertically inside bezel
  const int contentH = keyH*3 + rowGap*2;
  const int topPad   = (bezelH - contentH)/2;
  int rowY = bezelY + topPad;

  auto key = [&](int idx, int x, int y, const char* label){
    bool sel = (tvSel == idx);
    uint16_t frame = sel ? TFT_WHITE : TFT_DARKGREY;
    uint16_t fill  = TFT_BLACK;
    uint16_t text  = sel ? TFT_WHITE : TFT_SILVER;

    tft.fillRoundRect(x, y, keyW, keyH, 6, fill);
    tft.drawRoundRect(x, y, keyW, keyH, 6, frame);
    if (sel) tft.drawRoundRect(x-1, y-1, keyW+2, keyH+2, 7, frame);

    tft.setTextColor(text, fill);
    tft.setTextSize(2);
    int lw = tft.textWidth(label);
    tft.setCursor(x + (keyW - lw)/2, y + (keyH - 16)/2);
    tft.print(label);
  };

  // Row 1: Power | Mute (no icons)
  key(TV_POWER, leftX,  rowY, "Power");
  key(TV_MUTE,  rightX, rowY, "Mute");
  rowY += keyH + rowGap;

  // Row 2: Vol+ | Ch+
  key(TV_VOL_UP, leftX,  rowY, "Vol+");
  key(TV_CH_UP,  rightX, rowY, "Ch+");
  rowY += keyH + rowGap;

  // Row 3: Vol- | Ch-
  key(TV_VOL_DOWN, leftX,  rowY, "Vol-");
  key(TV_CH_DOWN,  rightX, rowY, "Ch-");
}

// ============================= LIFECYCLE =========================
void bgoneReset(){
  mode = BGoneMode::MENU;
  selected = 1;
  pageStart = 0;
  tvSel = TV_POWER;
  dirty = true;
}
static void renderIfDirty(TFT_eSPI& tft){
  if (!dirty) return;
  if (mode == BGoneMode::MENU) drawMenuGrid(tft);
  else                         drawTVRemote(tft);
  dirty = false;
}
void bgoneDrawScreen(TFT_eSPI& tft){ renderIfDirty(tft); }

// ============================== INPUT ============================
void bgoneHandleInput(bool aPressed,bool bPressed,bool cPressed,bool& exitRequested){
  exitRequested = false;

  if (mode == BGoneMode::MENU){
    if (bPressed){ selected = (selected+1)%kCount; ensureMenuSelectionVisible(); dirty=true; }
    if (cPressed){ if (selected==0){exitRequested=true;return;} selected=(selected-1+kCount)%kCount; ensureMenuSelectionVisible(); dirty=true; }
    if (aPressed){
      if (selected==0){ exitRequested = true; return; }
      if (selected==2){ mode = BGoneMode::TV_REMOTE; tvSel = TV_POWER; dirty = true; return; }
      // TODO: wire Add / Proj / LEDs / AC / Fan if needed
    }
    return;
  }

  // TV remote: B cycles selection; C exits (no Back button drawn)
  if (mode == BGoneMode::TV_REMOTE){
    if (bPressed){ tvSel = (tvSel + 1) % TV_COUNT; dirty = true; }
    if (cPressed){ mode = BGoneMode::MENU; dirty = true; return; } // exit with Btn C
    if (aPressed){
      // TODO: Send IR based on tvSel (Power, Mute, Vol+/-, Ch+/-)
    }
  }
}
