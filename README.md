# M5DIAL Color CountDown Timer

[日本語はこちら](README.ja.md)

A visual countdown timer sketch for M5Dial with a segmented ring UI.

## Photos

![M5Dial collage](images/collage.jpeg)


## Features

- 20-segment visual ring timer
- Fast encoder handling (multi-step minute changes)
- Catch-up tick logic for better time accuracy under load
- Half-time notification with selectable sound pattern
- Non-blocking completion alarm sequence
- Optional auto-dim while in DONE state
- Optional auto-reset after DONE timeout
- Long-press mute toggle in READY state
- Persist last configured duration in NVS and restore on boot
- Optional preset stepping (5/15/25/50 minutes)

## Controls

- Rotate: set time (READY/PAUSED)
- Short press:
	- READY -> start
	- RUNNING -> pause
	- PAUSED -> resume
	- DONE -> reset
- Long press:
	- READY -> toggle mute (when `ENABLE_LONG_PRESS_MUTE_TOGGLE` is enabled)
	- Other states -> reset

## Demo Video

[![Watch on YouTube](https://img.youtube.com/vi/b-_D3QaZPbk/hqdefault.jpg)](https://youtube.com/shorts/b-_D3QaZPbk)

## Configuration

Edit [config.h](config.h). Key options include:

- `HALF_NOTIFY_PATTERN`
- `USE_PRESET_STEPS`
- `PERSIST_LAST_DURATION`
- `MUTE_MODE_DEFAULT`
- `DONE_DIM_START_MS`, `DONE_DIM_EVERY_MS`, `DONE_DIM_STEP`, `DONE_MIN_BRIGHTNESS`
- `DONE_AUTO_RESET_MS`

## Using config.example.h

Recommended workflow for sharing/reuse:

1. Copy [config.example.h](config.example.h) to [config.h](config.h)
2. Adjust values in [config.h](config.h) for your device/use case
3. Optionally keep [config.h](config.h) out of version control

## License

This project is licensed under the MIT License.

- Copyright: 2026 omiya-bonsai
- Details: [LICENSE](LICENSE)
