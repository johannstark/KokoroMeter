// =============================================================================
// File: oauth_client.cpp
// Description: Implementation of OAuthClient — Google OAuth2 token refresh.
// Author: <your-name>
// Date: 2026-06-01
// =============================================================================

#include "oauth_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

#define OAUTH_TOKEN_URL "https://oauth2.googleapis.com/token"
#define HTTP_TIMEOUT_MS 5000
#define TOKEN_REFRESH_INTERVAL_MS 3300000 // 55 minutes

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

OAuthClient::OAuthClient(TokenStorage& storage) : storage_(storage) {}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------

bool OAuthClient::begin() {
  // Try to load a previously stored access token from NVS.
  const bool hasToken = storage_.loadAccessToken(accessToken_, TOKEN_MAX_LEN);
  storage_.loadTokenExpiry(tokenExpiry_);

  if (hasToken && isTokenValid()) {
    Serial.println("[INFO] OAuthClient: loaded valid token from NVS.");
    return true;
  }

  Serial.println("[INFO] OAuthClient: no valid cached token, refreshing now.");
  return forceRefresh();
}

bool OAuthClient::maintainToken() {
  const unsigned long now = millis();
  if ((now - lastRefreshMs_) >= TOKEN_REFRESH_INTERVAL_MS || !isTokenValid()) {
    return forceRefresh();
  }
  return true;
}

bool OAuthClient::forceRefresh() {
  Serial.println("[INFO] OAuthClient: refreshing access token...");
  const bool ok = refreshAccessToken_();
  if (ok) {
    lastRefreshMs_ = millis();
  }
  return ok;
}

const char* OAuthClient::accessToken() const {
  return accessToken_;
}

bool OAuthClient::isTokenValid() const {
  if (accessToken_[0] == '\0') return false;
  if (tokenExpiry_ == 0) return false;

  time_t now;
  time(&now);
  // Consider valid if expiry is more than 60 seconds in the future.
  return static_cast<uint32_t>(now) < (tokenExpiry_ - 60);
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------

bool OAuthClient::refreshAccessToken_() {
  // Load refresh token from NVS
  char refreshToken[TOKEN_MAX_LEN] = {0};
  if (!storage_.loadRefreshToken(refreshToken, TOKEN_MAX_LEN)) {
    Serial.println("[ERROR] OAuthClient: no refresh token in NVS.");
    return false;
  }

  char clientId[128] = {0};
  if (!storage_.loadClientId(clientId, sizeof(clientId))) {
    Serial.println("[ERROR] OAuthClient: no client ID in NVS.");
    return false;
  }

  char clientSecret[128] = {0};
  if (!storage_.loadClientSecret(clientSecret, sizeof(clientSecret))) {
    Serial.println("[ERROR] OAuthClient: no client secret in NVS.");
    return false;
  }

  // Build URL-encoded POST body.
  String body = "grant_type=refresh_token";
  body += "&refresh_token=";
  body += refreshToken;
  body += "&client_id=";
  body += clientId;
  body += "&client_secret=";
  body += clientSecret;

  // -------------------------------------------------------------------------
  // HTTPS client setup.
  //
  // setInsecure() skips certificate validation — suitable for development.
  // For production, replace with:
  //   static const char* kGoogleRootCA = "-----BEGIN CERTIFICATE-----\n..."
  //   client.setCACert(kGoogleRootCA);
  // The Google Trust Services root certificates can be downloaded from:
  //   https://pki.goog/roots.pem
  // -------------------------------------------------------------------------
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure();

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);

  if (!http.begin(wifiClient, OAUTH_TOKEN_URL)) {
    Serial.println("[ERROR] OAuthClient: http.begin() failed.");
    return false;
  }

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  const int httpCode = http.POST(body);
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[ERROR] OAuthClient: token refresh HTTP %d\n", httpCode);
    Serial.println(http.getString());
    http.end();
    return false;
  }

  const String payload = http.getString();
  http.end();

  // Parse JSON response.
  DynamicJsonDocument doc(JSON_TOKEN_DOC_SIZE);
  const DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[ERROR] OAuthClient: JSON parse error: %s\n",
                  err.c_str());
    return false;
  }

  const char* newToken = doc["access_token"] | "";
  const int expiresIn = doc["expires_in"] | 0;

  if (strlen(newToken) < 10) {
    Serial.println("[ERROR] OAuthClient: access_token missing in response.");
    return false;
  }

  // Store token in memory.
  strlcpy(accessToken_, newToken, TOKEN_MAX_LEN);

  // Compute and store expiry timestamp.
  time_t now;
  time(&now);
  tokenExpiry_ = static_cast<uint32_t>(now) + static_cast<uint32_t>(expiresIn);

  // Persist to NVS.
  storage_.saveAccessToken(accessToken_);
  storage_.saveTokenExpiry(tokenExpiry_);

  Serial.printf("[INFO] OAuthClient: token refreshed, expires in %d s\n",
                expiresIn);
  return true;
}
