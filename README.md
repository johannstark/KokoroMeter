<img src="docs/KokoroMeter%20Banner.png" alt="KokoroMeter" width="1600" height="476">

A physical desktop dashboard using an ESP32-32E and a Hosyond 4.0" 320x480 Touch Screen to monitor your Antigravity/Gemini API usage in real-time.

## Features
- **Guided Setup:** Easily compile and flash the firmware to your ESP32 directly from the CLI.
- **Background Daemon:** Runs silently in the background, polling your Antigravity usage.
- **USB Serial Connection:** Highly reliable and low-latency communication with the screen.
- **Rich UI:** Built using LVGL (Light and Versatile Graphics Library) on the ESP32.
- **Gemini/Antigravity monitoring:** Using [skainguyen1412/antigravity-usage](https://github.com/skainguyen1412/antigravity-usage) under the hood.

## Requirements
- Python 3.10+
- [PlatformIO](https://platformio.org/) (Required for flashing the firmware)
- Node.js (Required for the underlying `antigravity-usage` API)

---

## 1. Installation

Use Pixi to manage the development environment:

```bash
pixi install
pixi shell
```

## 2. Firmware Setup

Plug your ESP32 into a USB port on your computer, and run the setup wizard to compile and flash the KokoroMeter firmware:

```bash
kokoro setup
```

## 3. Authentication & Verification

KokoroMeter uses `antigravity-usage` under the hood to fetch your API quota. Before starting the dashboard, you must authenticate.

1. **Authenticate:** Run the following command and follow the prompts to log in (or simply export your `GEMINI_API_KEY` environment variable):
   ```bash
   npx antigravity-usage auth
   ```

2. **Verify CLI access:** Ensure you can see your quota directly in the terminal:
   ```bash
   npx antigravity-usage quota
   ```

## 4. Running the Dashboard

Once flashed and authenticated, you can use the `kokoro` CLI to manage the background daemon that streams data to your screen.

**Start the daemon in the background:**
```bash
kokoro start
```

**Check if the daemon is running:**
```bash
kokoro status
```

**Stop the daemon:**
```bash
kokoro stop
```

---

## Developers & Contributors

If you want to modify the code, run the tests, or debug the C++ firmware, please see the [Developer Documentation](docs/DEVELOPING.md).

***

## Acknowledgments & Inspiration

- **Clawdmeter:** [HermannBjorgvin/Clawdmeter](https://github.com/HermannBjorgvin/Clawdmeter) – A huge inspiration for the physical AI quota dashboard concept.
- **antigravity-usage:** [skainguyen1412/antigravity-usage](https://github.com/skainguyen1412/antigravity-usage) – The excellent underlying Node.js API that powers KokoroMeter's quota checks.

Made with love ♥️ in Colombia 🇨🇴
