// =============================================================================
// File: display_manager.h
// Description: Display abstraction layer for Gemini Pulse.
//              Default: SSD1306 128x64 OLED via I2C (Adafruit_SSD1306).
//              To swap to TFT_eSPI, replace the implementation in
//              display_manager.cpp — the header contract stays the same.
// Author: <your-name>
// Date: 2026-06-01
// =============================================================================

#pragma once

#include <Arduino.h>

#include "quota_parser.h"

/// @brief Manages all display rendering for the Gemini Pulse project.
///
/// Call begin() once in setup(), then call update() from the main loop.
/// The manager cycles through a summary screen and one screen per model,
/// advancing every DISPLAY_CYCLE_INTERVAL_MS milliseconds.
class DisplayManager {
 public:
  /// @brief Initialises the I2C bus and OLED driver.
  /// @return true if the display was found and initialised successfully.
  bool begin();

  /// @brief Non-blocking update. Advances the screen cycle when the interval
  ///        has elapsed and redraws the current screen.
  /// @param quota  Latest quota data to display.
  void update(const QuotaData& quota);

  /// @brief Renders a full-screen error message with an error code.
  /// @param errorCode Short code string shown on the display (max ~10 chars).
  /// @param message   Descriptive message (up to 2 lines at small font).
  void showError(const char* errorCode, const char* message);

  /// @brief Renders a "connecting…" splash screen shown during WIFI_CONNECTING.
  void showConnecting();

  /// @brief Renders a "refreshing token…" splash screen.
  void showRefreshingToken();

  /// @brief Renders a "fetching…" splash screen.
  void showFetching();

  /// @brief Forces the current screen index back to 0 (summary screen).
  void resetCycle();

 private:
  /// @brief Draws the summary screen: overall credits and progress bar.
  /// @param quota Latest quota data.
  void drawSummaryScreen_(const QuotaData& quota);

  /// @brief Draws a per-model quota screen.
  /// @param model  Model quota data to render.
  void drawModelScreen_(const ModelQuota& model);

  /// @brief Draws a horizontal progress bar.
  /// @param x         Left edge pixel x.
  /// @param y         Top edge pixel y.
  /// @param width     Total bar width in pixels.
  /// @param height    Bar height in pixels.
  /// @param fraction  Fill fraction 0.0–1.0.
  void drawProgressBar_(int x, int y, int width, int height, float fraction);

  /// @brief Index of the currently displayed screen.
  ///        0 = summary, 1..modelCount = model screens.
  int currentScreen_ = 0;

  /// @brief millis() timestamp when the current screen was last drawn.
  unsigned long lastCycleMs_ = 0;

  /// @brief True after begin() succeeds.
  bool initialised_ = false;
};
