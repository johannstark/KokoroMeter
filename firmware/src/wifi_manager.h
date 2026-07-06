// =============================================================================
// File: wifi_manager.h
// Description: WiFi connection management with retry logic for ESP32.
//              Provides blocking connect, status check, and reconnection helpers.
// Author: <your-name>
// Date: 2026-06-01
// =============================================================================

#pragma once

#include <Arduino.h>

/// @brief Manages WiFi connectivity with retry logic.
class WifiManager {
 public:
  /// @brief Initialises the WiFi radio and attempts to connect using credentials
  ///        defined in config.h.
  /// @return true if connected within the timeout, false otherwise.
  bool begin(const char* ssid, const char* password);

  /// @brief Returns true if the ESP32 currently has a WiFi connection.
  /// @return true when WiFi status is WL_CONNECTED.
  bool isConnected() const;

  /// @brief Non-blocking check; re-attempts connection if disconnected and the
  ///        retry interval has elapsed.
  /// @return true if connected (or was already connected), false if still down.
  bool maintainConnection();

  /// @brief Returns the current IP address as a String (empty if not connected).
  /// @return IP address string, e.g. "192.168.1.42".
  String ipAddress() const;

 private:
  /// @brief SSID of the target network.
  char ssid_[64] = {0};

  /// @brief Password of the target network.
  char password_[64] = {0};

  /// @brief Timestamp (millis) of the last connection attempt.
  unsigned long lastRetryMs_ = 0;

  /// @brief Number of consecutive failed connection attempts.
  int retryCount_ = 0;

  /// @brief Performs a single blocking connection attempt up to
  ///        WIFI_CONNECT_TIMEOUT_MS.
  /// @return true if connected, false on timeout.
  bool attemptConnect_();
};
