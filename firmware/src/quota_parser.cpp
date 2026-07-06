// =============================================================================
// File: quota_parser.cpp
// Description: Implementation of QuotaParser — JSON parsing for Cloud Code API.
// Author: <your-name>
// Date: 2026-06-01
// =============================================================================

#include "quota_parser.h"

#include <time.h>
#include <ArduinoJson.h>

#define JSON_LOAD_DOC_SIZE 1024
#define JSON_MODELS_DOC_SIZE 4096

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------

bool QuotaParser::parseLoadCodeAssist(const String& json,
                                      QuotaData& quota) const {
  DynamicJsonDocument doc(JSON_LOAD_DOC_SIZE);
  const DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.printf("[ERROR] QuotaParser::parseLoadCodeAssist JSON error: %s\n",
                  err.c_str());
    return false;
  }

  quota.availableCredits = doc["availablePromptCredits"] | 0;
  quota.monthlyCredits =
      doc["planInfo"]["monthlyPromptCredits"] | 0;

  const char* projectId = doc["cloudaicompanionProject"] | "";
  if (strlen(projectId) == 0) {
    Serial.println(
        "[WARN] QuotaParser: cloudaicompanionProject field missing.");
  }
  strlcpy(quota.projectId, projectId, PROJECT_ID_MAX_LEN);

  Serial.printf(
      "[INFO] QuotaParser: credits=%d/%d  projectId=%s\n",
      quota.availableCredits, quota.monthlyCredits, quota.projectId);
  return true;
}

bool QuotaParser::parseFetchAvailableModels(const String& json,
                                            QuotaData& quota) const {
  DynamicJsonDocument doc(JSON_MODELS_DOC_SIZE);
  const DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.printf(
        "[ERROR] QuotaParser::parseFetchAvailableModels JSON error: %s\n",
        err.c_str());
    return false;
  }

  const JsonObject models = doc["models"];
  if (models.isNull()) {
    Serial.println("[WARN] QuotaParser: 'models' object missing in response.");
    return false;
  }

  quota.modelCount = 0;
  for (const JsonPair kv : models) {
    if (quota.modelCount >= MAX_MODELS) {
      Serial.printf("[WARN] QuotaParser: MAX_MODELS (%d) reached, skipping.\n",
                    MAX_MODELS);
      break;
    }

    ModelQuota& m = quota.models[quota.modelCount];

    const char* name = kv.value()["displayName"] | "";
    strlcpy(m.displayName, name, MODEL_NAME_MAX_LEN);

    const JsonObject quotaInfo = kv.value()["quotaInfo"];
    if (!quotaInfo.isNull()) {
      m.remainingFraction = quotaInfo["remainingFraction"] | 1.0f;
      m.isExhausted = quotaInfo["isExhausted"] | false;
      const char* resetTime = quotaInfo["resetTime"] | "";
      strlcpy(m.resetTimeIso, resetTime, sizeof(m.resetTimeIso));
    }

    quota.modelCount++;
  }

  Serial.printf("[INFO] QuotaParser: parsed %d models.\n", quota.modelCount);
  return quota.modelCount > 0;
}

void QuotaParser::formatTimeUntilReset(const char* isoTime, char* buf,
                                       const size_t bufLen) {
  if (buf == nullptr || bufLen == 0) return;

  if (isoTime == nullptr || strlen(isoTime) == 0) {
    strlcpy(buf, "N/A", bufLen);
    return;
  }

  // Parse ISO 8601 string: "2026-06-01T15:30:00Z"
  struct tm tm_reset = {};
  // strptime is available in ESP-IDF via time.h.
  const char* parsed = strptime(isoTime, "%Y-%m-%dT%H:%M:%SZ", &tm_reset);
  if (parsed == nullptr) {
    strlcpy(buf, "N/A", bufLen);
    return;
  }

  const time_t resetEpoch = mktime(&tm_reset);
  time_t now;
  time(&now);

  const long diffSec = static_cast<long>(resetEpoch) - static_cast<long>(now);

  if (diffSec <= 0) {
    strlcpy(buf, "now", bufLen);
    return;
  }

  const long hours = diffSec / 3600;
  const long minutes = (diffSec % 3600) / 60;

  if (hours > 0) {
    snprintf(buf, bufLen, "%ldh %ldm", hours, minutes);
  } else {
    snprintf(buf, bufLen, "%ldm", minutes);
  }
}
