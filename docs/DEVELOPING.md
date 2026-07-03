# Developing KokoroMeter

This guide covers day-to-day development for the Python CLI/daemon and ESP32 firmware in VS Code.

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

### CLI and daemon tasks

- `CLI: Status`: check daemon status.
- `CLI: Start Daemon`: start the daemon in background mode.
- `CLI: Stop Daemon`: stop the daemon.
- `CLI: Run Daemon Foreground`: run daemon in foreground for logs/debugging.
- `CLI: Setup Firmware (Upload)`: run guided firmware compile/upload via CLI.

### Firmware tasks

- `Firmware: Build (esp32dev)`: compile firmware for ESP32.
- `Firmware: Upload (esp32dev)`: flash firmware to board.
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

- `Python: Debug Kokoro Command`: debug one command (`status`, `start`, `stop`, `setup`).
- `Python: Debug Daemon Foreground`: debug daemon loop behavior.
- `Python: Debug CLI Tests`: debug pytest tests for the CLI package.
- `PlatformIO: Debug Firmware (esp32dev)`: debug firmware with PlatformIO-capable hardware setup.
