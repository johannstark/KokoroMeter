// =============================================================================
// File: token_storage.h
// Description: NVS (Non-Volatile Storage) read/write helpers for Config & Tokens
// Author: KokoroMeter
// Date: 2026-06-01
// =============================================================================

#pragma once

#include <Arduino.h>
#include <Preferences.h>

class TokenStorage {
 public:
  bool begin();
  void end();

  // --- Write methods --------------------------------------------------------
  void saveWifiSsid(const char* ssid);
  void saveWifiPassword(const char* password);
  void saveClientId(const char* clientId);
  void saveClientSecret(const char* clientSecret);
  void saveAccessToken(const char* token);
  void saveRefreshToken(const char* token);
  void saveTokenExpiry(uint32_t unixTime);
  void saveProjectId(const char* projectId);

  // --- Read methods ---------------------------------------------------------
  bool loadWifiSsid(char* buf, size_t len) const;
  bool loadWifiPassword(char* buf, size_t len) const;
  bool loadClientId(char* buf, size_t len) const;
  bool loadClientSecret(char* buf, size_t len) const;
  bool loadAccessToken(char* buf, size_t len) const;
  bool loadRefreshToken(char* buf, size_t len) const;
  bool loadTokenExpiry(uint32_t& outExpiry) const;
  bool loadProjectId(char* buf, size_t len) const;

  void clear();

 private:
  mutable Preferences prefs_;
};
