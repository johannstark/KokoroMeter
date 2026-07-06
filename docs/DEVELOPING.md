# Developing KokoroMeter

This guide covers day-to-day development for the Python CLI and ESP32 firmware in VS Code.

## Quick Start

1. Install dependencies:

```bash
pixi install
```

2. Open the repository in VS Code.
3. Install recommended extensions when prompted.

## Development Standards

- Python uses Google-style docstrings.
- Python formatting/linting uses `ruff`.

## VS Code Project Configuration

The repository ships with development configuration in `.vscode/`:

- `.vscode/tasks.json`: run/build/test tasks for CLI and firmware.
- `.vscode/launch.json`: Python and PlatformIO debug profiles.
- `.vscode/settings.json`: pytest discovery and Pixi-oriented Python settings.
- `.vscode/extensions.json`: recommended extensions.

## Run and Test

Use **Terminal -> Run Task** in VS Code.

### CLI tasks

- `CLI: Setup Firmware (Upload)`: run interactive firmware compile/upload wizard via CLI.

### Firmware tasks

- `Firmware: Build (Hosyond)`: compile firmware for the large TFT screen.
- `Firmware: Build (SSD1306)`: compile firmware for the small OLED screen.
- `Firmware: Upload (Hosyond)`: flash Hosyond firmware to board.
- `Firmware: Monitor Serial`: open serial monitor at 115200.

### Test tasks

- `Test: CLI`: run Python tests under `cli/tests`.
- `Test: Firmware Native`: run PlatformIO native tests.
- `Test: All`: run full test suite.

Equivalent command:

```bash
pixi run test
```

## Debug in VS Code

Open **Run and Debug** and choose one of the following profiles:

- `Python: Debug Kokoro Setup`: debug the interactive setup command.
- `Python: Debug CLI Tests`: debug pytest tests for the CLI package.
- `PlatformIO: Debug Firmware`: debug firmware with PlatformIO-capable hardware setup.
