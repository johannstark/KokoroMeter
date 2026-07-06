// =============================================================================
// File: cloudcode_client.h
// Description: HTTP client for the Google Cloud Code private API.
//              Performs loadCodeAssist and fetchAvailableModels calls.
// Author: <your-name>
// Date: 2026-06-01
// =============================================================================

#pragma once

#include <Arduino.h>

#include "quota_parser.h"
#include "token_storage.h"

/// @brief Performs authenticated HTTPS calls to the Cloud Code internal API.
class CloudCodeClient {
 public:
  /// @brief Constructs the client with shared storage and parser dependencies.
  /// @param storage Reference to TokenStorage for reading project ID.
  /// @param parser  Reference to QuotaParser for interpreting responses.
  CloudCodeClient(TokenStorage& storage, QuotaParser& parser);

  /// @brief Executes both API calls in sequence to populate a QuotaData struct.
  ///        Call 1: loadCodeAssist  — credits + projectId
  ///        Call 2: fetchAvailableModels — per-model quotas
  /// @param accessToken  Valid OAuth2 bearer token string.
  /// @param quota        Output QuotaData struct to populate.
  /// @return true if both calls succeeded and data was parsed.
  bool fetchQuota(const char* accessToken, QuotaData& quota);

 private:
  /// @brief Posts to the loadCodeAssist endpoint and stores the raw JSON.
  /// @param accessToken  Bearer token.
  /// @param outJson      Output String to write the response body into.
  /// @return true on HTTP 200.
  bool callLoadCodeAssist_(const char* accessToken, String& outJson);

  /// @brief Posts to the fetchAvailableModels endpoint and stores the raw JSON.
  /// @param accessToken  Bearer token.
  /// @param projectId    Cloud Code project ID obtained from loadCodeAssist.
  /// @param outJson      Output String to write the response body into.
  /// @return true on HTTP 200.
  bool callFetchAvailableModels_(const char* accessToken,
                                 const char* projectId, String& outJson);

  /// @brief Shared helper: performs a JSON POST request and returns the body.
  /// @param url          HTTPS endpoint URL.
  /// @param accessToken  Bearer token.
  /// @param requestBody  JSON-encoded request body string.
  /// @param outJson      Output String for the response body.
  /// @return HTTP status code (200 = success).
  int postJson_(const char* url, const char* accessToken,
                const String& requestBody, String& outJson);

  TokenStorage& storage_;
  QuotaParser& parser_;
};
