<img src="docs/KokoroMeter%20Banner.png" alt="KokoroMeter" width="1600" height="476">

A standalone physical dashboard using an ESP32 to monitor your Antigravity/Gemini API usage in real-time over Wi-Fi.

> [!WARNING]
> THIS REPO IS A WORK IN PROGRESS

## Features
- **Standalone Wi-Fi:** Connects directly to Google's Cloud Code APIs over Wi-Fi—no PC connection required after initial setup!
- **Multi-Screen Support:** Compile for either a large Hosyond 4.0" TFT Touch Screen (using LVGL) or a small 0.96" SSD1306 OLED (using Adafruit GFX).
- **Interactive Wizard:** Easily configure Wi-Fi, OAuth credentials, and flash the firmware using a single CLI command.
- **Gemini/Antigravity monitoring:** Inspired by [skainguyen1412/antigravity-usage](https://github.com/skainguyen1412/antigravity-usage).

## Requirements
- Python 3.14+
- [PlatformIO](https://platformio.org/) (Required for flashing the firmware)
- Google Cloud OAuth2 credentials (Client ID, Secret, and Refresh Token)

---

## 1. Installation

Use Pixi to manage the development environment:

```bash
pixi install
pixi shell
```

## 2. Setup & Flashing

Plug your ESP32 into a USB port on your computer, and run the interactive setup wizard:

```bash
kokoro setup
```

The wizard will prompt you for:
1. Your **Wi-Fi SSID** and **Password**.
2. Your **Google OAuth2 Client ID, Secret, and Refresh Token** (Follow Google's standard OAuth Desktop app flow to obtain these).
3. Your **Hardware Profile** (Choose between the Hosyond 4.0" or the SSD1306 0.96" screen).

The CLI will automatically generate the `config.h` file and flash the firmware to your ESP32 via PlatformIO. 

## 3. Usage

Once flashed, KokoroMeter is completely standalone! You can unplug it from your PC and plug it into any standard 5V USB wall charger. It will automatically connect to Wi-Fi, refresh its OAuth token, and cycle through your API quotas.

## Developers & Contributors

If you want to modify the C++ firmware or run tests, please see the [Developer Documentation](docs/DEVELOPING.md).

## AI Assistants

This project contains rules for AI coding assistants. 
- The source of truth for these rules is `AGENTS.md` at the repository root.
- `CLAUDE.md` and `GEMINI.md` are symbolic links pointing to `AGENTS.md`, ensuring all agents follow the same unified rules.
- **Agent Skills:** A custom `hosyond-4in-esp32-screen` skill is included in the `.agents/skills/` folder to provide AI assistants with hardware context when developing for the Hosyond display.

## Acknowledgments & Inspiration

- **Clawdmeter:** [HermannBjorgvin/Clawdmeter](https://github.com/HermannBjorgvin/Clawdmeter) – A huge inspiration for the physical AI quota dashboard concept.
- **gemini-pulse:** [gemini-pulse](https://github.com/your-username/gemini-pulse) - The core networking and C++ JSON parsing logic used in our standalone firmware.
- **antigravity-usage:** [skainguyen1412/antigravity-usage](https://github.com/skainguyen1412/antigravity-usage) – The excellent Node.js API that inspired KokoroMeter's quota logic.

***

Made with love ♥️ in Colombia 🇨🇴
