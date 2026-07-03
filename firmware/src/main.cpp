#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <XPT2046_Touchscreen.h>
#include <ArduinoJson.h>

/* Display and Touch objects */
TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(TOUCH_CS);

/* LVGL Buffer */
static const uint16_t screenWidth  = 480;
static const uint16_t screenHeight = 320;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

/* LVGL UI Elements */
lv_obj_t * label_quota;
lv_obj_t * btn_refresh;

/* LVGL Display Flushing */
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp_drv);
}

/* LVGL Touch Reading */
void my_touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
    if(ts.touched()) {
        TS_Point p = ts.getPoint();
        data->state = LV_INDEV_STATE_PR;
        
        // Map touch coordinates (requires calibration for specific screen)
        // This is a generic rough mapping for Hosyond 320x480
        data->point.x = map(p.x, 200, 3800, 0, screenWidth);
        data->point.y = map(p.y, 200, 3800, 0, screenHeight);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

/* Button Event Handler */
static void refresh_btn_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        // Send command back to the Host PC Daemon
        Serial.println("ACTION_REFRESH");
    }
}

void build_ui() {
    /* Main Screen Styling */
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x1a1a1a), LV_PART_MAIN);

    /* Quota Label */
    label_quota = lv_label_create(lv_scr_act());
    lv_label_set_text(label_quota, "Waiting for PC daemon...");
    lv_obj_set_style_text_color(label_quota, lv_color_hex(0xffffff), 0);
    lv_obj_align(label_quota, LV_ALIGN_CENTER, 0, -20);

    /* Refresh Button */
    btn_refresh = lv_btn_create(lv_scr_act());
    lv_obj_align(btn_refresh, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_event_cb(btn_refresh, refresh_btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * label_btn = lv_label_create(btn_refresh);
    lv_label_set_text(label_btn, "Refresh Quota");
    lv_obj_center(label_btn);
}

void setup() {
    Serial.begin(115200);

    /* Init TFT */
    tft.begin();
    tft.setRotation(1); // Landscape

    /* Init Touch */
    ts.begin();
    ts.setRotation(1);

    /* Init LVGL */
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

    /* Register Display Driver */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* Register Touch Driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    build_ui();
}

void process_serial() {
    if (Serial.available()) {
        String payload = Serial.readStringUntil('\n');
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            String type = doc["type"];
            if (type == "quota") {
                // Update UI based on parsed antigravity-usage JSON
                // Just as an MVP example, let's dump a part of the payload or formatted string
                // The exact keys depend on the antigravity-usage JSON output structure
                // Assuming something like doc["data"]["totalQuota"] etc
                
                String ui_text = "Quota Updated!\n";
                // Add your exact JSON keys here once known
                ui_text += payload.substring(0, 50) + "..."; // Placeholder preview
                
                lv_label_set_text(label_quota, ui_text.c_str());
            }
        }
    }
}

void loop() {
    lv_timer_handler(); // let the GUI do its work
    process_serial();
    delay(5);
}
