# Wokwi simulator

Debug the M5StickC tally firmware without plugging in a stick.

## Prerequisites

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) (`pio` on PATH)
- VS Code with the [Wokwi for VS Code](https://docs.wokwi.com/vscode/getting-started) extension (free community licence signs in from the command palette)

## Build

```sh
pio run -e wokwi
```

Artifacts land in `.pio/build/wokwi/firmware.{elf,bin}`.

## Run

1. Open this folder in VS Code.
2. Command palette → **Wokwi: Start Simulator**.

The extension reads `wokwi.toml` and `diagram.json`.

## What you should see

- **Boot**: "BRONTIDE.MEDIA" splash; NeoPixel ring chases a single white pixel.
- **No saved Wi-Fi**: ring breathes blue, then the AP screen renders.
- **Hardware buttons**:
  - Blue pushbutton (GPIO35) — cycles screens tally → network → tally-number → brightness.
  - Red pushbutton (GPIO39) — actions tied to the active screen (reconnect to vMix on the tally screen; brightness cycle on the brightness screen; tally-number increment on the tally-number screen).

## What differs from the real stick

- **Display**: Wokwi has no ST7735 part. The sim renders into an offscreen 160×80 sprite — the native StickC landscape resolution — and pushes it centred onto a Wokwi ILI9341 (240×320). Layout is 1:1; the ILI9341 bezel just shows around it.
- **M5 button**: real stick wires BtnA to GPIO37 which the Wokwi DevKitC model doesn't expose. Sim remaps it to GPIO35. Device build keeps GPIO37.
- **AXP192, IMU, RTC**: stubbed. Battery returns ~65%. Accelerometer is fixed in landscape.
- **Charging state**: toggle `SIM_CHARGING` in `sim/sim_glue.cpp` (or set it at runtime) to exercise the bolt overlay.

## Wire for real hardware

The device build (`pio run -e device`) still targets the original M5StickC (C_PLUS=0) and pulls the libraries from `libs/`. The NeoPixel ring data line is GPIO26; VCC 5 V, GND to ground. Ring settings live in the web UI under "NeoPixel Ring".
