# BRONTIDE.MEDIA tally light — Waveshare ESP32-C6-LCD-1.47 fork

vMix tally-light firmware ported to the **Waveshare ESP32-C6-LCD-1.47**:
single RISC-V core with Wi-Fi 6, a 1.47" 172×320 IPS LCD, USB-C, on-board
LiPo charging, and a WS2812 status pixel. Drop-in replacement for the
M5StickC tallies in the BRONTIDE camera-op kit, with a bigger screen, a
rec/stream/clock status banner, and an optional **live preview thumbnail**
of the cued/live input pulled from a tiny companion proxy on the vMix host.

This branch is C6-only on purpose. The original M5StickC v1 / Plus2 builds
and the Wokwi simulator live on `master` / `plus2` and have not been
modified.

Forked from [guido-visser/vMix-M5Stick-Tally-Light](https://github.com/guido-visser/vMix-M5Stick-Tally-Light) — upstream
project does the heavy lifting on vMix integration, web UI, and the boot
flow. The BRONTIDE NeoPixel + boot-splash work from the M5 branches has
been carried forward.

---

## What this fork adds vs. the M5 build

- **Native 320×172 layout.** Bigger tally text, dedicated bottom status
  strip (REC + elapsed | STREAM + elapsed | wall clock from NTP).
- **Live preview thumbnail (optional).** A 320×110 JPEG of the configured
  vMix input, polled at 5 fps by default, decoded with JPEGDEC and
  blitted via Arduino_GFX. Tally state never blocks on the preview poll;
  if the proxy is unreachable the tally keeps working and the preview
  area shows "preview offline" over the last frame.
- **Single-button UX.** The board has only one user-pressable button
  (BOOT / GPIO9). Short-press cycles screens, long-press is the action
  (reconnect on the network screen, brightness step on the brightness
  screen, settings reset on the network screen when offline).
- **Configurable WS2812 count.** Onboard pixel (GPIO8) doubles as a
  status indicator; daisy-chain an external ring off the same line and
  bump LED count in the web UI.

---

## Hardware (Waveshare ESP32-C6-LCD-1.47)

| Block      | Pins / IC                                            |
|------------|------------------------------------------------------|
| LCD        | ST7789V 172×320 IPS, SPI MOSI=6 SCLK=7 CS=14 DC=15 RST=21, BL=GPIO22 (PWM) |
| WS2812     | 1 onboard pixel on GPIO8 (chain externals off it)    |
| SD card    | CS=4 MISO=5 (shares SPI bus with LCD)                |
| Buttons    | BOOT on GPIO9 (active low) — only user button        |
| Battery    | LiPo connector, on-board TP4056-class charger, ADC on GPIO0 (1:2 divider) |
| USB        | USB-C, native USB CDC                                |
| Radios     | Wi-Fi 6 / BLE 5 / 802.15.4 (last two unused)         |

> Pin numbers come from the Waveshare wiki. Verify against the schematic
> for your exact board revision before first flash; if the battery ADC
> sits on a different GPIO, change `BATT_ADC` in [src/c6/n_HAL.h](src/c6/n_HAL.h).

---

## Build + flash

```sh
pio run -e c6
pio run -e c6 -t upload --upload-port /dev/cu.usbmodemXXXX
pio device monitor -e c6
```

Arduino-ESP32 3.x is required for the C6 (`platform = espressif32@^6.7.0`
in [platformio.ini](platformio.ini) pulls it in). USB-CDC is enabled so
Serial works over the native USB-C port.

### Clearing back to AP mode

Reflashing leaves NVS (stored Wi-Fi creds) intact. To force first-run AP
setup:

```sh
pio run -e c6 -t erase --upload-port /dev/cu.usbmodemXXXX
pio run -e c6 -t upload --upload-port /dev/cu.usbmodemXXXX
```

On boot the device joins `BRONTIDE-Tally-XXXXX` (password `12345678`).
Connect a phone, browse `http://192.168.4.1`.

---

## Live preview feature

Receiving SRT, NDI, or RTMP directly on an ESP32-C6 isn't realistic — no
PSRAM, no H.264 decoder, no congestion-controlled transport. Instead, a
~100-line Python script on the vMix host exposes per-input JPEG snapshots
over HTTP, and the tally polls those.

1. Run the proxy on the vMix host:

   ```sh
   pip install pillow
   python tools/vmix-snapshot-proxy/proxy.py --vmix http://127.0.0.1:8088 --port 8089
   ```

2. In the tally's web UI, set **Proxy URL** to e.g.
   `http://10.0.0.5:8089/preview/1?w=320&h=110` and pick a poll rate
   (5 Hz is comfortable, 10 Hz is achievable, 15 Hz is the edge).

3. Save & reboot. The middle band of the tally screen now mirrors the
   selected input.

See [tools/vmix-snapshot-proxy/README.md](tools/vmix-snapshot-proxy/README.md)
for details.

---

## Status strip

The bottom 22 px shows REC / STREAM elapsed timers (driven by vMix's
`ACTS` events, same protocol the M5 build subscribes to) and a wall
clock from NTP. Each is independently toggleable from the web UI. Default
on, except disable the wall clock if you find it distracting during
takes.

NTP is configured at WiFi-connect time using the **NTP offset** field
from the web UI (minutes from UTC; no DST handling — set it for the
show's timezone). The clock is tolerant of brief Wi-Fi blips.

---

## NeoPixel ring

Same FastLED-based driver as the M5 build, on GPIO8. Default `RING_NUM=1`
just animates the onboard pixel. If you wire an external 16-pixel ring
off GPIO8, set `LED count` to 16 in the web UI — no recompile.

Tally colours: red = LIVE (always), green = PREVIEW (optional). The
"only on LIVE" flag mutes the green entirely. During boot the ring runs
white → blue breath → 300 ms green flash on vMix-connect, then hands
control to the tally state. No animations on disconnect — quiet during a
live take is the rule.

---

## Upstream compatibility

The vMix-side configuration (multi-input, Just-Live, reconnect interval,
brightness, tally-number override, STM/REC indicators) is unchanged. The
TCP protocol against vMix:8099 is identical to the upstream firmware.

---

## Credits

- [Guido Visser](https://github.com/guido-visser) — original firmware, web UI, boot flow.
- [Thomas Mout](https://github.com/ThomasMout) — the Arduino vMix-Tally project the upstream built on.
- [@ArieR1963](https://github.com/ArieR1963) — Plus2 hardware support in the upstream codebase.

BRONTIDE C6 fork maintained for in-house use at https://brontide.media.
