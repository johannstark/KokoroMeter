// =============================================================================
// File: display_manager.cpp
// Description: SSD1306 OLED rendering implementation for Gemini Pulse.
//              Uses Adafruit_SSD1306 + Adafruit_GFX.
//
//              To swap to TFT_eSPI:
//                1. Replace #include <Adafruit_SSD1306.h> with
//                   #include <TFT_eSPI.h>
//                2. Replace the Adafruit_SSD1306 display_ member with
//                   TFT_eSPI tft_;
//                3. Adapt begin() to call tft_.init() and set rotation.
//                4. Replace display_.setTextColor(WHITE) etc. with TFT macros.
//                5. Adjust pixel coordinates for the larger resolution.
//
// Author: <your-name>
// Date: 2026-06-01
// =============================================================================

#include "ssd1306_ui.h"

#ifdef USE_SSD1306

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_I2C_ADDRESS 0x3C
#define DISPLAY_CYCLE_INTERVAL_MS 5000



// ---------------------------------------------------------------------------
// Module-local display instance (not exposed in the header to keep the
// abstraction boundary clean).
// ---------------------------------------------------------------------------

/// @brief Internal SSD1306 driver instance.
static Adafruit_SSD1306 gDisplay(OLED_WIDTH, OLED_HEIGHT, &Wire,
                                  /*resetPin=*/-1);

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------

bool DisplayManager::begin() {
  Wire.begin(PIN_SDA, PIN_SCL);

  if (!gDisplay.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println("[ERROR] DisplayManager: SSD1306 not found at I2C address "
                   "0x" + String(OLED_I2C_ADDRESS, HEX));
    return false;
  }

  gDisplay.clearDisplay();
  gDisplay.setTextColor(SSD1306_WHITE);
  gDisplay.setTextWrap(false);
  gDisplay.display();

  initialised_ = true;
  Serial.println("[INFO] DisplayManager: OLED initialised.");
  return true;
}

void DisplayManager::update(const QuotaData& quota) {
  if (!initialised_) return;

  const unsigned long now = millis();
  if ((now - lastCycleMs_) < DISPLAY_CYCLE_INTERVAL_MS) {
    return;  // Not time to advance yet.
  }
  lastCycleMs_ = now;

  // Screen 0 = summary; screens 1..modelCount = per-model.
  const int totalScreens = 1 + quota.modelCount;
  if (totalScreens <= 1) {
    drawSummaryScreen_(quota);
    return;
  }

  if (currentScreen_ == 0) {
    drawSummaryScreen_(quota);
  } else {
    const int modelIdx = currentScreen_ - 1;
    drawModelScreen_(quota.models[modelIdx]);
  }

  currentScreen_ = (currentScreen_ + 1) % totalScreens;
}

void DisplayManager::showError(const char* errorCode, const char* message) {
  if (!initialised_) return;

  gDisplay.clearDisplay();
  gDisplay.setTextSize(1);

  // Inverted header bar for error code.
  gDisplay.fillRect(0, 0, OLED_WIDTH, 12, SSD1306_WHITE);
  gDisplay.setTextColor(SSD1306_BLACK);
  gDisplay.setCursor(2, 2);
  gDisplay.printf("!! %s !!", errorCode);

  gDisplay.setTextColor(SSD1306_WHITE);
  gDisplay.setCursor(0, 16);
  gDisplay.println(message);

  gDisplay.display();
}

void DisplayManager::showConnecting() {
  if (!initialised_) return;

  gDisplay.clearDisplay();
  gDisplay.setTextSize(1);
  gDisplay.setTextColor(SSD1306_WHITE);
  gDisplay.setCursor(0, 8);
  gDisplay.println("  Gemini Pulse");
  gDisplay.setCursor(0, 28);
  gDisplay.println(" Connecting WiFi...");
  gDisplay.display();
}

void DisplayManager::showRefreshingToken() {
  if (!initialised_) return;

  gDisplay.clearDisplay();
  gDisplay.setTextSize(1);
  gDisplay.setTextColor(SSD1306_WHITE);
  gDisplay.setCursor(0, 20);
  gDisplay.println("  Refreshing token");
  gDisplay.setCursor(0, 34);
  gDisplay.println("  Please wait...");
  gDisplay.display();
}

void DisplayManager::showFetching() {
  if (!initialised_) return;

  gDisplay.clearDisplay();
  gDisplay.setTextSize(1);
  gDisplay.setTextColor(SSD1306_WHITE);
  gDisplay.setCursor(0, 20);
  gDisplay.println("  Fetching quota...");
  gDisplay.display();
}

void DisplayManager::resetCycle() {
  currentScreen_ = 0;
  lastCycleMs_ = 0;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------

void DisplayManager::drawSummaryScreen_(const QuotaData& quota) {
  gDisplay.clearDisplay();
  gDisplay.setTextColor(SSD1306_WHITE);

  // Row 0: title
  gDisplay.setTextSize(1);
  gDisplay.setCursor(0, 0);
  gDisplay.println("Antigravity Quota");

  // Row 1: credits
  gDisplay.setCursor(0, 12);
  gDisplay.printf("Credits: %d / %d", quota.availableCredits,
                  quota.monthlyCredits);

  // Row 2: progress bar
  const float fraction = (quota.monthlyCredits > 0)
                             ? static_cast<float>(quota.availableCredits) /
                                   static_cast<float>(quota.monthlyCredits)
                             : 0.0f;
  drawProgressBar_(0, 26, OLED_WIDTH - 1, 8, fraction);

  // Percentage label inside / beside bar
  gDisplay.setCursor(OLED_WIDTH - 30, 27);
  gDisplay.printf("%3d%%", static_cast<int>(fraction * 100.0f));

  // Row 3: last updated
  gDisplay.setCursor(0, 40);
  if (quota.lastFetchMs == 0) {
    gDisplay.println("Updated: never");
  } else {
    const unsigned long ageSec = (millis() - quota.lastFetchMs) / 1000UL;
    if (ageSec < 60) {
      gDisplay.printf("Updated: %lus ago", ageSec);
    } else {
      gDisplay.printf("Updated: %lum ago", ageSec / 60UL);
    }
  }

  gDisplay.display();
}

void DisplayManager::drawModelScreen_(const ModelQuota& model) {
  gDisplay.clearDisplay();
  gDisplay.setTextColor(SSD1306_WHITE);

  // Row 0: model name (truncate to fit 128px at textSize=1 = 21 chars)
  gDisplay.setTextSize(1);
  gDisplay.setCursor(0, 0);
  char nameBuf[22] = {0};
  strlcpy(nameBuf, model.displayName, sizeof(nameBuf));
  gDisplay.println(nameBuf);

  // Row 1: progress bar
  drawProgressBar_(0, 14, OLED_WIDTH - 1, 8, model.remainingFraction);
  gDisplay.setCursor(OLED_WIDTH - 30, 15);
  gDisplay.printf("%3d%%",
                  static_cast<int>(model.remainingFraction * 100.0f));

  // Row 2: reset countdown
  gDisplay.setCursor(0, 28);
  char countdownBuf[16] = {0};
  QuotaParser::formatTimeUntilReset(model.resetTimeIso, countdownBuf,
                                    sizeof(countdownBuf));
  gDisplay.printf("Resets in: %s", countdownBuf);

  // Row 3: status — inverted banner if exhausted
  if (model.isExhausted) {
    gDisplay.fillRect(0, 40, OLED_WIDTH, 12, SSD1306_WHITE);
    gDisplay.setTextColor(SSD1306_BLACK);
    gDisplay.setCursor(10, 42);
    gDisplay.println("!! EXHAUSTED !!");
    gDisplay.setTextColor(SSD1306_WHITE);
  } else {
    gDisplay.setCursor(0, 42);
    gDisplay.println("Status: OK");
  }

  gDisplay.display();
}

void DisplayManager::drawProgressBar_(const int x, const int y,
                                       const int width, const int height,
                                       const float fraction) {
  // Outer border.
  gDisplay.drawRect(x, y, width, height, SSD1306_WHITE);

  // Inner fill proportional to fraction (clamped 0–1).
  const float clamped = fraction < 0.0f ? 0.0f : (fraction > 1.0f ? 1.0f : fraction);
  const int fillWidth = static_cast<int>((width - 2) * clamped);
  if (fillWidth > 0) {
    gDisplay.fillRect(x + 1, y + 1, fillWidth, height - 2, SSD1306_WHITE);
  }
}

#endif // USE_SSD1306
