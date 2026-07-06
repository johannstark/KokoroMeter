---
name: hosyond-esp32-4in-screen
description: Development guide and hardware reference for the Hosyond 4-inch ESP32-WROOM-32E display (ST7796S driver, XPT2046 touch). Trigger this skill when the user is working with this specific Hosyond display, modifying its UI, or setting up a new project for it.
---

# Hosyond 4" ESP32 Display Skill

This skill provides essential hardware details and implementation patterns for the Hosyond 4.0-inch ESP32-WROOM-32E Touch Display. It ensures you use the correct initialization sequence, hardware mappings, and development constraints.

## Development Constraints

1. **PlatformIO ONLY:** All development must be done using PlatformIO. Never use Arduino IDE directly.
2. **NO Auto-Flashing:** You may use `pio run` to build the code, but you must **NEVER** use `pio run --target upload`. The user must manually handle flashing to prevent accidentally flashing the wrong device.

## Hardware Specifications

- **Microcontroller:** ESP32-WROOM-32E
- **Display Driver:** ST7796S (480x320 resolution)
- **Touch Controller:** XPT2046 (Resistive)

### Pin Mapping

**Display (ST7796S):**
- **CS:** IO15
- **DC (RS):** IO2
- **RST:** -1 (Shared with ESP32 EN)
- **MOSI:** IO13
- **SCLK:** IO14
- **MISO:** IO12
- **Backlight (BL):** IO27

**Touch Screen (XPT2046):**
- **CS:** IO33
- **IRQ:** IO36
- **DIN/DOUT/CLK:** Shared with display SPI pins

### Free Pinouts (for Expansion)

Based on the official ESP32E Resource Allocation:
- **Completely Free:** IO39/IO35 (Input only), IO32, IO25, IO21
- **Conditionally Free:** IO18, IO19, IO23 (if no MicroSD card used), IO1, IO3 (if no Hardware Serial used).

### Using the docs/ Directory

The `docs/` directory contains crucial original hardware information directly from the manufacturer. When working on advanced features or troubleshooting, always reference:
- `ST7796S_Datasheet.pdf` / `ESP32E_Datasheet.pdf` for low-level register behaviors.
- `ESP32E_ResourceAllocation.xlsx` for complex pin reassignments.
- `4.in_ESP32E_UserManual.pdf` for default hardware capabilities.

### 1. TFT_eSPI Setup & Initialization

You must manually enable the backlight and apply the custom manufacturer initialization sequence after calling `tft.init()`. Here is the precise sequence translated to `TFT_eSPI` commands.

```cpp
#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// Typical calibration values for landscape (Rotation 1)
uint16_t calData[5] = {252, 3653, 243, 3485, 7};

// Manufacturer's customized initialization for the Hosyond 4.0 ST7796S Display
void applyHosyondInit(TFT_eSPI& tft) {
  delay(120);
  tft.writecommand(0x11);     
  delay(120);
  tft.writecommand(0x36); tft.writedata(0x48);   
  tft.writecommand(0x3A); tft.writedata(0x55);   
  tft.writecommand(0xF0); tft.writedata(0xC3);   
  tft.writecommand(0xF0); tft.writedata(0x96);   
  tft.writecommand(0xB4); tft.writedata(0x01);   
  tft.writecommand(0xB7); tft.writedata(0xC6);   
  tft.writecommand(0xC0); tft.writedata(0x80); tft.writedata(0x45);   
  tft.writecommand(0xC1); tft.writedata(0x13);   
  tft.writecommand(0xC2); tft.writedata(0xA7);   
  tft.writecommand(0xC5); tft.writedata(0x20);   
  tft.writecommand(0xE8); tft.writedata(0x40); tft.writedata(0x8A); tft.writedata(0x00); tft.writedata(0x00); tft.writedata(0x29); tft.writedata(0x19); tft.writedata(0xA5); tft.writedata(0x33);
  tft.writecommand(0xE0); tft.writedata(0xD0); tft.writedata(0x08); tft.writedata(0x0F); tft.writedata(0x06); tft.writedata(0x06); tft.writedata(0x33); tft.writedata(0x30); tft.writedata(0x33); tft.writedata(0x47); tft.writedata(0x17); tft.writedata(0x13); tft.writedata(0x13); tft.writedata(0x2B); tft.writedata(0x31);
  tft.writecommand(0xE1); tft.writedata(0xD0); tft.writedata(0x0A); tft.writedata(0x11); tft.writedata(0x0B); tft.writedata(0x09); tft.writedata(0x07); tft.writedata(0x2F); tft.writedata(0x33); tft.writedata(0x47); tft.writedata(0x38); tft.writedata(0x15); tft.writedata(0x16); tft.writedata(0x2C); tft.writedata(0x32);
  tft.writecommand(0xF0); tft.writedata(0x3C);   
  tft.writecommand(0xF0); tft.writedata(0x69);   
  delay(120);
  tft.writecommand(0x29); 
}

void setup() {
  // 1. Enable backlight
  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);

  // 2. Initialize TFT
  tft.init();
  
  // 3. Apply custom ST7796 init sequence
  applyHosyondInit(tft);
  
  // 4. Set orientation and touch calibration
  tft.setRotation(1);
  tft.setTouch(calData);
  
  tft.fillScreen(TFT_BLACK);
}
```

### 2. LVGL Integration

When integrating LVGL, you must provide a display flush callback and a touch read callback.

```cpp
#include <lvgl.h>

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[480 * 20]; // 20-line draw buffer

// Flush callback (LVGL -> TFT)
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp_drv);
}

// Touch read callback (TFT -> LVGL)
void my_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  uint16_t touchX, touchY;
  bool touched = tft.getTouch(&touchX, &touchY);

  if (!touched) {
    data->state = LV_INDEV_STATE_REL;
  } else {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = touchX;
    data->point.y = touchY;
  }
}

void init_lvgl() {
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, 480 * 20);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 480;
  disp_drv.ver_res = 320;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touch_read;
  lv_indev_drv_register(&indev_drv);
}
```

Remember to call `lv_tick_inc()` and `lv_timer_handler()` in your `loop()` to keep the LVGL engine running.
