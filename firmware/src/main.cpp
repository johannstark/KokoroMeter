#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>
#include <WiFi.h>

#include "cloudcode_client.h"
#include "oauth_client.h"
#include "quota_parser.h"
#include "token_storage.h"
#include "wifi_manager.h"

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
#define QUOTA_POLL_INTERVAL_MS 60000
#define TOKEN_REFRESH_INTERVAL_MS 3300000
#define ERROR_RETRY_INTERVAL_MS 10000
#define NTP_SERVER "pool.ntp.org"
#define NTP_UTC_OFFSET_SEC 0
#define NTP_DST_OFFSET_SEC 0

// ---------------------------------------------------------------------------
// Hardware Profiles
// ---------------------------------------------------------------------------
#ifdef USE_HOSYOND
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(TOUCH_CS);

static const uint16_t screenWidth  = 480;
static const uint16_t screenHeight = 320;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];
lv_obj_t * label_quota;
lv_obj_t * label_status;

void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp_drv);
}

void my_touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
    if(ts.touched()) {
        TS_Point p = ts.getPoint();
        data->state = LV_INDEV_STATE_PR;
        data->point.x = map(p.x, 200, 3800, 0, screenWidth);
        data->point.y = map(p.y, 200, 3800, 0, screenHeight);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void init_hosyond_ui() {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x1a1a1a), LV_PART_MAIN);

    label_quota = lv_label_create(lv_scr_act());
    lv_label_set_text(label_quota, "Booting...");
    lv_obj_set_style_text_color(label_quota, lv_color_hex(0xffffff), 0);
    lv_obj_align(label_quota, LV_ALIGN_CENTER, 0, -20);

    label_status = lv_label_create(lv_scr_act());
    lv_label_set_text(label_status, "Initialising KokoroMeter...");
    lv_obj_set_style_text_color(label_status, lv_color_hex(0xaaaaaa), 0);
    lv_obj_align(label_status, LV_ALIGN_CENTER, 0, 40);
}

void set_hosyond_status(const char* text) {
    lv_label_set_text(label_status, text);
}
#endif

#ifdef USE_SSD1306
#include "ssd1306_ui.h"
static DisplayManager gDisplay;
#endif


// ---------------------------------------------------------------------------
// State machine definition
// ---------------------------------------------------------------------------
enum class AppState {
  INIT,
  PROVISIONING,
  WIFI_CONNECTING,
  TOKEN_REFRESHING,
  FETCHING_QUOTA,
  DISPLAYING,
  ERROR,
};

static AppState gState = AppState::INIT;
static TokenStorage gStorage;
static WifiManager gWifi;
static OAuthClient gOAuth(gStorage);
static QuotaParser gParser;
static CloudCodeClient gCloudCode(gStorage, gParser);
static QuotaData gQuota;

static unsigned long gLastQuotaFetchMs = 0;
static unsigned long gLastTokenRefreshMs = 0;
static unsigned long gErrorEnteredMs = 0;
static char gErrorCode[16] = {0};
static char gErrorMsg[48] = {0};

static void enterError_(const char* code, const char* msg) {
  Serial.printf("[ERROR] State → ERROR  code=%s  msg=%s\n", code, msg);
  strlcpy(gErrorCode, code, sizeof(gErrorCode));
  strlcpy(gErrorMsg, msg, sizeof(gErrorMsg));
  gErrorEnteredMs = millis();
  gState = AppState::ERROR;
}

static void syncNtp_() {
  Serial.println("[INFO] Syncing time via NTP...");
  configTime(NTP_UTC_OFFSET_SEC, NTP_DST_OFFSET_SEC, NTP_SERVER);
  const unsigned long deadline = millis() + 10000UL;
  while (millis() < deadline) {
    time_t now;
    time(&now);
    if (now > 1577836800UL) return; // 2020-01-01 sanity check
    delay(200);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

#ifdef USE_HOSYOND
  tft.begin();
  tft.setRotation(1);
  ts.begin();
  ts.setRotation(1);
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
  init_hosyond_ui();
#endif

#ifdef USE_SSD1306
  gDisplay.begin();
  gDisplay.showConnecting();
#endif

  if (!gStorage.begin()) {
    enterError_("NVS_ERR", "NVS open failed");
    return;
  }
  gState = AppState::INIT;
}

void loop() {
#ifdef USE_HOSYOND
  lv_timer_handler();
#endif

  switch (gState) {
    case AppState::INIT: {
      char ssid[64] = {0};
      if (!gStorage.loadWifiSsid(ssid, sizeof(ssid))) {
        gState = AppState::PROVISIONING;
        Serial.println("{\"status\":\"waiting_provision\"}");
#ifdef USE_HOSYOND
        set_hosyond_status("Waiting for USB Provisioning");
#endif
      } else {
        gState = AppState::WIFI_CONNECTING;
      }
      break;
    }

    case AppState::PROVISIONING: {
      if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        DynamicJsonDocument doc(512);
        DeserializationError err = deserializeJson(doc, line);
        if (!err) {
          const char* cmd = doc["cmd"] | "";
          if (strcmp(cmd, "scan") == 0) {
            WiFi.mode(WIFI_STA);
            WiFi.disconnect();
            delay(100);
            int n = WiFi.scanNetworks();
            DynamicJsonDocument resp(2048);
            JsonArray arr = resp.createNestedArray("networks");
            for (int i = 0; i < n; ++i) {
              arr.add(WiFi.SSID(i));
            }
            serializeJson(resp, Serial);
            Serial.println();
          } else if (strcmp(cmd, "provision") == 0) {
            gStorage.saveWifiSsid(doc["ssid"] | "");
            gStorage.saveWifiPassword(doc["password"] | "");
            gStorage.saveClientId(doc["client_id"] | "");
            gStorage.saveClientSecret(doc["client_secret"] | "");
            gStorage.saveRefreshToken(doc["refresh_token"] | "");
            Serial.println("{\"status\":\"success\"}");
            delay(100);
            ESP.restart();
          }
        }
      }
      break;
    }

    case AppState::WIFI_CONNECTING: {
#ifdef USE_HOSYOND
      set_hosyond_status("Connecting to Wi-Fi...");
#endif
#ifdef USE_SSD1306
      gDisplay.showConnecting();
#endif
      char ssid[64] = {0};
      char pass[64] = {0};
      gStorage.loadWifiSsid(ssid, sizeof(ssid));
      gStorage.loadWifiPassword(pass, sizeof(pass));

      if (!gWifi.begin(ssid, pass)) {
        enterError_("WIFI_ERR", "Cannot connect to WiFi");
        break;
      }
      syncNtp_();
      gState = AppState::TOKEN_REFRESHING;
      break;
    }

    case AppState::TOKEN_REFRESHING:
#ifdef USE_HOSYOND
      set_hosyond_status("Refreshing OAuth Token...");
#endif
#ifdef USE_SSD1306
      gDisplay.showRefreshingToken();
#endif
      if (!gOAuth.begin()) {
        enterError_("TOKEN_ERR", "Token refresh failed");
        break;
      }
      gLastTokenRefreshMs = millis();
      gState = AppState::FETCHING_QUOTA;
      break;

    case AppState::FETCHING_QUOTA:
#ifdef USE_HOSYOND
      set_hosyond_status("Fetching API Quota...");
#endif
#ifdef USE_SSD1306
      gDisplay.showFetching();
#endif
      if (!gCloudCode.fetchQuota(gOAuth.accessToken(), gQuota)) {
        enterError_("API_ERR", "Quota fetch failed");
        break;
      }
      gLastQuotaFetchMs = millis();
#ifdef USE_HOSYOND
      set_hosyond_status("Active");
      // Format simple UI for Hosyond MVP
      {
          String qText = "Credits: " + String(gQuota.availableCredits) + " / " + String(gQuota.monthlyCredits);
          lv_label_set_text(label_quota, qText.c_str());
      }
#endif
#ifdef USE_SSD1306
      gDisplay.resetCycle();
#endif
      gState = AppState::DISPLAYING;
      break;

    case AppState::DISPLAYING:
      if (!gWifi.maintainConnection()) {
        gState = AppState::WIFI_CONNECTING;
        break;
      }
      if ((millis() - gLastTokenRefreshMs) >= TOKEN_REFRESH_INTERVAL_MS) {
        gState = AppState::TOKEN_REFRESHING;
        break;
      }
      if ((millis() - gLastQuotaFetchMs) >= QUOTA_POLL_INTERVAL_MS) {
        gState = AppState::FETCHING_QUOTA;
        break;
      }

#ifdef USE_SSD1306
      gDisplay.update(gQuota);
#endif
      break;

    case AppState::ERROR:
#ifdef USE_HOSYOND
      set_hosyond_status(gErrorMsg);
      lv_label_set_text(label_quota, gErrorCode);
#endif
#ifdef USE_SSD1306
      gDisplay.showError(gErrorCode, gErrorMsg);
#endif
      if ((millis() - gErrorEnteredMs) >= ERROR_RETRY_INTERVAL_MS) {
        gState = AppState::WIFI_CONNECTING;
      }
      delay(500);
      break;
  }
  delay(5);
}
