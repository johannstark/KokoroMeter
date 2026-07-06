// =============================================================================
// File: cloudcode_client.cpp
// Description: Implementation of CloudCodeClient — Cloud Code API HTTP calls.
// Author: <your-name>
// Date: 2026-06-01
// =============================================================================

#include "cloudcode_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#define CLOUDCODE_LOAD_URL "https://cloudcode.googleapis.com/v1/loadCodeAssist"
#define CLOUDCODE_MODELS_URL "https://cloudcode.googleapis.com/v1/fetchAvailableModels"
#define HTTP_TIMEOUT_MS 5000
#define API_USER_AGENT "KokoroMeter/1.0"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

CloudCodeClient::CloudCodeClient(TokenStorage& storage, QuotaParser& parser)
    : storage_(storage), parser_(parser) {}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------

bool CloudCodeClient::fetchQuota(const char* accessToken, QuotaData& quota) {
  // --- Call 1: loadCodeAssist -----------------------------------------------
  String loadJson;
  if (!callLoadCodeAssist_(accessToken, loadJson)) {
    Serial.println("[ERROR] CloudCodeClient: loadCodeAssist failed.");
    return false;
  }

  if (!parser_.parseLoadCodeAssist(loadJson, quota)) {
    Serial.println("[ERROR] CloudCodeClient: failed to parse loadCodeAssist.");
    return false;
  }

  // If the projectId came back empty, try to fall back to NVS.
  if (strlen(quota.projectId) == 0) {
    if (!storage_.loadProjectId(quota.projectId, PROJECT_ID_MAX_LEN)) {
      Serial.println(
          "[ERROR] CloudCodeClient: no projectId from API or NVS.");
      return false;
    }
    Serial.println("[INFO] CloudCodeClient: projectId loaded from NVS.");
  } else {
    // Persist the fresh projectId.
    storage_.saveProjectId(quota.projectId);
  }

  // --- Call 2: fetchAvailableModels -----------------------------------------
  String modelsJson;
  if (!callFetchAvailableModels_(accessToken, quota.projectId, modelsJson)) {
    Serial.println("[ERROR] CloudCodeClient: fetchAvailableModels failed.");
    return false;
  }

  if (!parser_.parseFetchAvailableModels(modelsJson, quota)) {
    Serial.println(
        "[ERROR] CloudCodeClient: failed to parse fetchAvailableModels.");
    return false;
  }

  quota.lastFetchMs = millis();
  return true;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------

bool CloudCodeClient::callLoadCodeAssist_(const char* accessToken,
                                          String& outJson) {
  // Build JSON body.
  DynamicJsonDocument body(256);
  JsonObject metadata = body.createNestedObject("metadata");
  metadata["ideType"] = "ANTIGRAVITY";
  metadata["platform"] = "PLATFORM_UNSPECIFIED";
  metadata["pluginType"] = "GEMINI";

  String bodyStr;
  serializeJson(body, bodyStr);

  const int code = postJson_(CLOUDCODE_LOAD_URL, accessToken, bodyStr, outJson);
  return code == HTTP_CODE_OK;
}

bool CloudCodeClient::callFetchAvailableModels_(const char* accessToken,
                                                const char* projectId,
                                                String& outJson) {
  DynamicJsonDocument body(128);
  body["project"] = projectId;

  String bodyStr;
  serializeJson(body, bodyStr);

  const int code =
      postJson_(CLOUDCODE_MODELS_URL, accessToken, bodyStr, outJson);
  return code == HTTP_CODE_OK;
}

int CloudCodeClient::postJson_(const char* url, const char* accessToken,
                                const String& requestBody, String& outJson) {
  // -------------------------------------------------------------------------
  // HTTPS client setup.
  //
  // setInsecure() skips TLS certificate validation — suitable for development.
  // For production, supply the Google root CA certificate:
  //   static const char* kGoogleRootCA = "-----BEGIN CERTIFICATE-----\n..."
  //   wifiClient.setCACert(kGoogleRootCA);
  // Download current roots from: https://pki.goog/roots.pem
  // -------------------------------------------------------------------------
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure();

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);

  if (!http.begin(wifiClient, url)) {
    Serial.printf("[ERROR] CloudCodeClient: http.begin() failed for %s\n", url);
    return -1;
  }

  http.addHeader("Authorization", String("Bearer ") + accessToken);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", API_USER_AGENT);

  const int httpCode = http.POST(requestBody);

  if (httpCode > 0) {
    outJson = http.getString();
    if (httpCode != HTTP_CODE_OK) {
      Serial.printf("[ERROR] CloudCodeClient: HTTP %d for %s\n", httpCode, url);
      Serial.println(outJson);
    }
  } else {
    Serial.printf("[ERROR] CloudCodeClient: request error %s for %s\n",
                  HTTPClient::errorToString(httpCode).c_str(), url);
  }

  http.end();
  return httpCode;
}
