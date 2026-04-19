# BRONTIDE.MEDIA tally light firmware

vMix tally-light firmware for the original **M5StickC** and the **M5StickC-Plus2**, extended with a 16-pixel NeoPixel status ring, an always-visible charging indicator, a Wokwi simulator for off-stick development, and a double-tap-to-poke chat protocol that lets crew sticks quietly heckle each other during the dull bits of a shoot.

Forked from [guido-visser/vMix-M5Stick-Tally-Light](https://github.com/guido-visser/vMix-M5Stick-Tally-Light) — upstream project does the heavy lifting on vMix integration, web UI, and the boot flow. Everything below that layer is unchanged; this README covers what BRONTIDE has added on top.

---

## What this fork adds

- **NeoPixel status ring** (16 px on GPIO26). Red for LIVE, green for PREVIEW, configurable via web UI. Doubles as a boot-progress indicator (white chase → blue breath while connecting Wi-Fi → 300 ms green flash on vMix-connected). Deliberately silent on disconnects — it must never distract crew during a live take.
- **BRONTIDE boot splash** replacing the upstream title card.
- **Always-on charging indicator.** The top-left battery icon now renders a yellow lightning bolt whenever the AXP reports current flowing into the cell. Works across both AXP192 (original StickC) and AXP2101 (Plus2).
- **Wokwi simulator.** Run the firmware in a browser or VS Code without hardware — useful for layout debugging and regression testing. Uses a stand-in ILI9341 display since Wokwi has no ST7735 part.
- **M5StickC-Plus2 support** via PlatformIO env switching. The upstream code already had `C_PLUS=2` conditionals for Plus2; this fork adds the build env and library resolution so it actually ships.
- **Inter-stick "BORED" chat.** Double-tap the main M5 button to UDP-broadcast a poke to every other stick on the LAN. Receivers see the cue overlaid on their tally screen (background colour preserved). A reply double-tap within 20 s sends ME TOO back. Random word pool (BORED / YAWN / HALP / HELLO / ZZZ / POKE), camera-ID attribution, fire-count badge. Does not touch the ring. Does not block live tally updates.

---

## Hardware variants + branches

| Hardware | PIO env | Branch currently flashed | Notes |
|---|---|---|---|
| Original M5StickC (ESP32-PICO-D4, ST7735 80×160, AXP192) | `device` | `bored-chat` | Vendored libs in `libs/M5StickC/` |
| M5StickC-Plus2 (ESP32-PICO-V3-02, ST7789 135×240, AXP2101) | `device_plus2` | `bored-chat-plus2` | M5Unified + M5GFX from the registry |
| Wokwi simulator (generic ESP32 + wokwi-ili9341) | `wokwi` | n/a | Shimmed M5 library under `sim/` |

M5StickC-Plus (v1) is not an active target but the `C_PLUS=1` conditionals from upstream remain in place; adding a `device_plus1` env should be straightforward.

---

## Build + flash

### Everyday (original StickC or Wokwi)

```sh
pio run -e device                              # build for original M5StickC
pio run -e device -t upload --upload-port /dev/cu.usbserial-XXXX

pio run -e wokwi                               # build for the simulator
```

### M5StickC-Plus2

The globally installed `pio` on some Macs runs under Python 3.14, which trips the pioarduino platform's Python 3.10-3.13 guard. Use PIO's own 3.11 venv instead:

```sh
/Users/tom/.platformio/penv/bin/python -m platformio run -e device_plus2
/Users/tom/.platformio/penv/bin/python -m platformio run -e device_plus2 -t upload --upload-port /dev/cu.usbserial-XXXX
```

(Replace the venv path for your machine; `platformio.ini` bakes in `upload_speed = 115200` because 460800 flakes on the CH9102 adapters the sticks ship with.)

### Clearing a stick back to AP mode

Reflashing leaves NVS (stored Wi-Fi creds) intact. To force AP-setup mode, erase first:

```sh
pio run -e device -t erase --upload-port /dev/cu.usbserial-XXXX
pio run -e device -t upload --upload-port /dev/cu.usbserial-XXXX
```

After reboot the stick joins `vMix-M5Stick-Tally <mac-suffix>` (password `12345678`). Connect a phone, browse `http://192.168.4.1`.

---

## Wokwi simulator

Two ways to run it:

- **VS Code**: install the Wokwi extension, open this folder, command palette → `Wokwi: Start Simulator`. First launch prompts for a free community licence.
- **Headless / screenshots**: `wokwi-cli` from `https://wokwi.com/ci/install.sh` (needs a `WOKWI_CLI_TOKEN` from `https://wokwi.com/dashboard/ci`). The screenshot renderer uses `rsvg-convert`, so on macOS `brew install librsvg` first.

```sh
export WOKWI_CLI_TOKEN=wok_…
PATH=/opt/homebrew/bin:$PATH wokwi-cli --timeout 15000 \
    --screenshot-part lcd --screenshot-time 12000 \
    --screenshot-file /tmp/shot.png .
sips -r -90 /tmp/shot.png --out /tmp/shot.png   # Wokwi screenshots come out portrait
open /tmp/shot.png
```

Wokwi has no ST7735 part, so drawings land on a 240×320 ILI9341 rendered at the stick's original coordinate space. Layout is approximate, functional behaviour is real.

### Simulator limitations

- No AXP / IMU / RTC emulation — stubbed. Battery reports a constant ~65%. Charging state is a `SIM_CHARGING` bool in `sim/sim_glue.cpp`.
- The M5 side button remaps from GPIO37 to GPIO35 in sim (the Wokwi DevKitC model doesn't break out GPIO37). Firmware handles the remap via `#ifdef SIM_WOKWI`.
- WiFi against the Wokwi proxy network works for basic connectivity; there's no vMix in the sim, so tally states can only be forced by patching source.

---

## NeoPixel ring wiring

Default data pin is **GPIO26** (`RING_DATA_PIN` in [`src/a_GLOBAL/l_RING.h`](src/a_GLOBAL/l_RING.h)). Wire:

- `VCC` → 5V on the HY2.0 / Grove header
- `GND` → ground
- `DIN` → GPIO26

Master toggle, brightness, preview colour enable, and LIVE-only mode are all configurable from the existing web UI.

---

## Inter-stick chat protocol

All sticks on the same L2 segment listen on UDP port `41234`. Wire format is ASCII, newline-free:

```
M5TALLY/1/MSG/<sender_mac6>/<sender_tally_nr>/<WORD>
M5TALLY/1/METOO/<from_mac6>/<from_tally_nr>/<to_mac6>
```

- `<mac6>` = last 6 hex chars of `WiFi.macAddress()` uppercase (addressing id).
- `<tally_nr>` = sender's configured camera / input number (displayed to the receiver as "from cam N").
- `<WORD>` is one of `BORED YAWN HALP HELLO ZZZ POKE`, rolled by the sender so every receiver shows the same cue.

Dismissed instantly if a real vMix TALLY state change arrives — live state always wins over the joke. Ring is never touched by chat code. See [`src/a_GLOBAL/m_CHAT.h`](src/a_GLOBAL/m_CHAT.h) for the full state machine.

UDP is unauthenticated; acceptable on a closed production LAN. There's no rate limiting beyond the 20 s reply window, so spam responsibly.

---

## Upstream compatibility

All of the upstream features and the configuration documented in the original [README_UPSTREAM.md](https://github.com/guido-visser/vMix-M5Stick-Tally-Light) still apply — multi-input, Just-Live mode, reconnect interval, brightness, tally-number override from the device, STM/REC indicators, static-IP edit path in `e_WIFI.ino`. This fork does not remove anything; everything below is additive.

---

## Credits

- [Guido Visser](https://github.com/guido-visser) — original firmware, web UI, boot flow.
- [Thomas Mout](https://github.com/ThomasMout) — the Arduino vMix-Tally project the upstream built on.
- [@ArieR1963](https://github.com/ArieR1963) — Plus2 hardware support in the upstream codebase.
- [Dirwin Clemens](https://github.com/Dirwinc) — LED HAT plugin pattern that informed the ring integration.

BRONTIDE fork maintained for in-house use at https://brontide.media.
