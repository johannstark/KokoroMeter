# KokoroMeter Agent Rules

## Project Context
KokoroMeter is a physical desktop dashboard running on an ESP32-32E with a Hosyond 4.0" 320x480 Touch Screen to monitor Antigravity/Gemini usage. It communicates over a wired USB Serial connection with a Python-based host daemon (the `kokoro` CLI app).

## Agent Rules
- **ALWAYS** plan the changes and check with the human user on the approach.
- **NEVER** use `git add`, `git commit`, or `git push`. The human user will do these things.
- **DOCSTRINGS:** Keep all Python code documented in Google-styled docstrings.
- **DOCUMENTATION:** Every change an agent does must be done with a check and adjustment to `README.md` and `AGENTS.md` if necessary.
- **FORMATTING:** For Python code, use `ruff` to check formats.
