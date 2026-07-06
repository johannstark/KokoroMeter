// =============================================================================
// File: wifi_manager.cpp
// Description: Implementation of WifiManager — WiFi connection and retry logic.
// Author: <your-name>
// Date: 2026-06-01
// =============================================================================

#include "wifi_manager.h"

#include <WiFi.h>

#define WIFI_CONNECT_TIMEOUT_MS 10000
#define WIFI_MAX_RETRIES 3
#define WIFI_RETRY_INTERVAL_MS 5000



// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------

bool WifiManager::begin(const char* ssid, const char* password) {
  strlcpy(ssid_, ssid, sizeof(ssid_));
  strlcpy(password_, password, sizeof(password_));

  Serial.printf("[INFO] Connecting to WiFi SSID: %s\n", ssid_);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_, password_);

  retryCount_ = 0;
  return attemptConnect_();
}

bool WifiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool WifiManager::maintainConnection() {
  if (isConnected()) {
    retryCount_ = 0;
    return true;
  }

  const unsigned long now = millis();
  if ((now - lastRetryMs_) < WIFI_RETRY_INTERVAL_MS) {
    return false;
  }

  lastRetryMs_ = now;
  retryCount_++;

  if (retryCount_ > WIFI_MAX_RETRIES) {
    Serial.printf(
        "[WARN] WiFi: exceeded max retries (%d). Restarting WiFi stack.\n",
        WIFI_MAX_RETRIES);
    WiFi.disconnect(true);
    delay(500);
    WiFi.begin(ssid_, password_);
    retryCount_ = 0;
  } else {
    Serial.printf("[INFO] WiFi: reconnect attempt %d/%d\n", retryCount_,
                  WIFI_MAX_RETRIES);
    WiFi.reconnect();
  }

  return attemptConnect_();
}

String WifiManager::ipAddress() const {
  if (!isConnected()) {
    return String("");
  }
  return WiFi.localIP().toString();
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------

bool WifiManager::attemptConnect_() {
  const unsigned long deadline = millis() + WIFI_CONNECT_TIMEOUT_MS;

  while (millis() < deadline) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[INFO] WiFi connected. IP: %s\n",
                    WiFi.localIP().toString().c_str());
      lastRetryMs_ = millis();
      return true;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.printf("[ERROR] WiFi: connection timed out after %lu ms\n",
                WIFI_CONNECT_TIMEOUT_MS);
  return false;
}
