// =============================================================================
// File: quota_parser.h
// Description: Data structures and JSON parsing helpers for Cloud Code API
//              responses (loadCodeAssist and fetchAvailableModels).
// Author: <your-name>
// Date: 2026-06-01
// =============================================================================

#pragma once

#include <Arduino.h>

#define MAX_MODELS 20
#define PROJECT_ID_MAX_LEN 128
#define MODEL_NAME_MAX_LEN 64
#include <ArduinoJson.h>



/// @brief Quota data for a single AI model returned by fetchAvailableModels.
struct ModelQuota {
  /// @brief Human-readable model display name (e.g. "Gemini 3 Flash").
  char displayName[MODEL_NAME_MAX_LEN] = {0};

  /// @brief Fraction of quota remaining: 0.0 (exhausted) to 1.0 (full).
  float remainingFraction = 1.0f;

  /// @brief ISO 8601 reset time string as returned by the API.
  char resetTimeIso[32] = {0};

  /// @brief True if the model quota is fully exhausted.
  bool isExhausted = false;
};

/// @brief Aggregated quota data fetched from both Cloud Code API calls.
struct QuotaData {
  /// @brief Available prompt credits from loadCodeAssist.
  int availableCredits = 0;

  /// @brief Monthly credit limit from loadCodeAssist.
  int monthlyCredits = 0;

  /// @brief Cloud Code project ID extracted from loadCodeAssist.
  char projectId[PROJECT_ID_MAX_LEN] = {0};

  /// @brief Array of per-model quota entries.
  ModelQuota models[MAX_MODELS];

  /// @brief Number of valid entries in the models array.
  int modelCount = 0;

  /// @brief Unix timestamp (millis) of last successful data fetch.
  unsigned long lastFetchMs = 0;
};

/// @brief Parses JSON responses from the Cloud Code API into QuotaData structs.
class QuotaParser {
 public:
  /// @brief Parses the loadCodeAssist JSON response body.
  ///        Populates availableCredits, monthlyCredits, and projectId in quota.
  /// @param json  Raw JSON string from the API response.
  /// @param quota Output QuotaData struct to populate.
  /// @return true if parsing succeeded and required fields were found.
  bool parseLoadCodeAssist(const String& json, QuotaData& quota) const;

  /// @brief Parses the fetchAvailableModels JSON response body.
  ///        Populates the models array in quota.
  /// @param json  Raw JSON string from the API response.
  /// @param quota Output QuotaData struct to populate.
  /// @return true if parsing succeeded and at least one model was found.
  bool parseFetchAvailableModels(const String& json, QuotaData& quota) const;

  /// @brief Converts an ISO 8601 reset time string into a human-readable
  ///        countdown string such as "3h 24m" or "45m" relative to now.
  /// @param isoTime  ISO 8601 date-time string (e.g. "2026-06-01T15:30:00Z").
  /// @param buf      Output buffer to write the formatted string into.
  /// @param bufLen   Size of the output buffer in bytes.
  static void formatTimeUntilReset(const char* isoTime, char* buf,
                                   size_t bufLen);
};
