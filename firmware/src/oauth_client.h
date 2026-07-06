// =============================================================================
// File: oauth_client.h
// Description: Google OAuth2 access token refresh flow for ESP32.
//              Uses the refresh_token grant to obtain a fresh access_token.
// Author: <your-name>
// Date: 2026-06-01
// =============================================================================

#pragma once

#include <Arduino.h>

#include "token_storage.h"

#define TOKEN_MAX_LEN 2048
#define JSON_TOKEN_DOC_SIZE 1024

/// @brief Handles Google OAuth2 token lifecycle on the ESP32.
///
/// On first boot the refresh token is read from config.h (NVS is empty).
/// After a successful refresh, the new access token and expiry are persisted
/// to NVS so subsequent boots can skip the refresh if the token is still valid.
class OAuthClient {
 public:
  /// @brief Constructs the OAuthClient with a reference to persistent storage.
  /// @param storage Reference to the TokenStorage instance used for NVS I/O.
  explicit OAuthClient(TokenStorage& storage);

  /// @brief Initialises the client. Loads any previously stored token from NVS.
  ///        If no stored access token is found, triggers an immediate refresh.
  /// @return true if a valid access token is available after init.
  bool begin();

  /// @brief Checks token expiry and refreshes if needed. Call this periodically
  ///        from the main loop (e.g., every TOKEN_REFRESH_INTERVAL_MS).
  /// @return true if the access token is valid (whether refreshed or still current).
  bool maintainToken();

  /// @brief Forces an immediate token refresh regardless of expiry.
  /// @return true if the new token was obtained and stored successfully.
  bool forceRefresh();

  /// @brief Returns a pointer to the in-memory access token buffer.
  ///        Valid until the next call to maintainToken() or forceRefresh().
  /// @return Null-terminated access token string, or empty string on error.
  const char* accessToken() const;

  /// @brief Returns true if the current access token is non-empty and not expired.
  /// @return true when the token is considered valid.
  bool isTokenValid() const;

 private:
  /// @brief Performs the HTTP POST to the token refresh endpoint and parses
  ///        the JSON response to extract access_token and expires_in.
  /// @return true on success.
  bool refreshAccessToken_();

  /// @brief Reference to NVS storage for persisting token data.
  TokenStorage& storage_;

  /// @brief In-memory buffer for the current access token.
  char accessToken_[TOKEN_MAX_LEN] = {0};

  /// @brief Unix timestamp at which the access token expires.
  uint32_t tokenExpiry_ = 0;

  /// @brief millis() timestamp of the last successful refresh.
  unsigned long lastRefreshMs_ = 0;
};
