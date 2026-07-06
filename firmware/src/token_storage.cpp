// =============================================================================
// File: token_storage.cpp
// Description: Implementation of TokenStorage
// Author: KokoroMeter
// Date: 2026-06-01
// =============================================================================

#include "token_storage.h"

// NVS Keys
#define NVS_NAMESPACE "kokoro"
#define NVS_KEY_WIFI_SSID "wifi_ssid"
#define NVS_KEY_WIFI_PASS "wifi_pass"
#define NVS_KEY_CLIENT_ID "client_id"
#define NVS_KEY_CLIENT_SEC "client_sec"
#define NVS_KEY_ACCESS_TOKEN "acc_token"
#define NVS_KEY_REFRESH_TOKEN "ref_token"
#define NVS_KEY_TOKEN_EXPIRY "tok_expiry"
#define NVS_KEY_PROJECT_ID "proj_id"

bool TokenStorage::begin() {
  return prefs_.begin(NVS_NAMESPACE, false);
}

void TokenStorage::end() {
  prefs_.end();
}

// --- Write methods ---
void TokenStorage::saveWifiSsid(const char* ssid) { prefs_.putString(NVS_KEY_WIFI_SSID, ssid); }
void TokenStorage::saveWifiPassword(const char* password) { prefs_.putString(NVS_KEY_WIFI_PASS, password); }
void TokenStorage::saveClientId(const char* clientId) { prefs_.putString(NVS_KEY_CLIENT_ID, clientId); }
void TokenStorage::saveClientSecret(const char* clientSecret) { prefs_.putString(NVS_KEY_CLIENT_SEC, clientSecret); }
void TokenStorage::saveAccessToken(const char* token) { prefs_.putString(NVS_KEY_ACCESS_TOKEN, token); }
void TokenStorage::saveRefreshToken(const char* token) { prefs_.putString(NVS_KEY_REFRESH_TOKEN, token); }
void TokenStorage::saveTokenExpiry(uint32_t unixTime) { prefs_.putUInt(NVS_KEY_TOKEN_EXPIRY, unixTime); }
void TokenStorage::saveProjectId(const char* projectId) { prefs_.putString(NVS_KEY_PROJECT_ID, projectId); }

// --- Read methods ---
bool TokenStorage::loadWifiSsid(char* buf, size_t len) const {
  String val = prefs_.getString(NVS_KEY_WIFI_SSID, "");
  if (val.isEmpty()) return false;
  strlcpy(buf, val.c_str(), len);
  return true;
}

bool TokenStorage::loadWifiPassword(char* buf, size_t len) const {
  String val = prefs_.getString(NVS_KEY_WIFI_PASS, "");
  if (val.isEmpty()) return false;
  strlcpy(buf, val.c_str(), len);
  return true;
}

bool TokenStorage::loadClientId(char* buf, size_t len) const {
  String val = prefs_.getString(NVS_KEY_CLIENT_ID, "");
  if (val.isEmpty()) return false;
  strlcpy(buf, val.c_str(), len);
  return true;
}

bool TokenStorage::loadClientSecret(char* buf, size_t len) const {
  String val = prefs_.getString(NVS_KEY_CLIENT_SEC, "");
  if (val.isEmpty()) return false;
  strlcpy(buf, val.c_str(), len);
  return true;
}

bool TokenStorage::loadAccessToken(char* buf, size_t len) const {
  String val = prefs_.getString(NVS_KEY_ACCESS_TOKEN, "");
  if (val.isEmpty()) return false;
  strlcpy(buf, val.c_str(), len);
  return true;
}

bool TokenStorage::loadRefreshToken(char* buf, size_t len) const {
  String val = prefs_.getString(NVS_KEY_REFRESH_TOKEN, "");
  if (val.isEmpty()) return false;
  strlcpy(buf, val.c_str(), len);
  return true;
}

bool TokenStorage::loadTokenExpiry(uint32_t& outExpiry) const {
  outExpiry = prefs_.getUInt(NVS_KEY_TOKEN_EXPIRY, 0);
  return outExpiry > 0;
}

bool TokenStorage::loadProjectId(char* buf, size_t len) const {
  String val = prefs_.getString(NVS_KEY_PROJECT_ID, "");
  if (val.isEmpty()) return false;
  strlcpy(buf, val.c_str(), len);
  return true;
}

void TokenStorage::clear() {
  prefs_.clear();
}
