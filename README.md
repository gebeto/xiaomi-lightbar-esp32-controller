# ESP32-C3 firmware — Xiaomi Lightbar over nRF24

A standalone [PlatformIO](https://platformio.org/) / Arduino firmware for an **ESP32-C3** that
toggles a Xiaomi Mi Computer Monitor Light Bar (model **MJGJD01YL**, the non-BLE variant with a
Telink TLSR8368 receiver) over an **nRF24L01(+)** module — no Raspberry Pi required.

It is a C++ port of the Python library in this repo (`xiaomi_lightbar/radio.py` +
`xiaomi_lightbar/baseband.py`). The bar has a single toggle command, so calling `on_off()` turns it
on or off depending on its current state, exactly like pressing the remote knob.

The firmware exposes a tiny WiFi HTTP endpoint: `GET /on_off` toggles the bar.

## Wiring (nRF24L01 → ESP32-C3 DevKitM-1)

| nRF24L01 | ESP32-C3 |
|----------|----------|
| VCC      | 3V3      |
| GND      | GND      |
| CE       | GPIO10   |
| CSN      | GPIO7    |
| SCK      | GPIO4    |
| MOSI     | GPIO6    |
| MISO     | GPIO5    |
| IRQ      | (not connected) |

> The nRF24L01 is power-hungry on transmit. Solder a **10 µF** (or larger) capacitor across the
> module's VCC/GND, or use an adapter board with a regulator, to avoid brown-outs. Power the module
> from **3.3 V**, not 5 V.

### Wiring (nRF24L01 → WEMOS LOLIN32 Lite rev1)

The LOLIN32 Lite is a classic **ESP32** (not a C3). GPIO6–11 are reserved for the on-chip SPI
flash and must not be used, so this board uses different pins (set via `build_flags` in the
`lolin32-lite` env):

| nRF24L01 | LOLIN32 Lite |
|----------|--------------|
| VCC      | 3V3          |
| GND      | GND          |
| CE       | GPIO17       |
| CSN      | GPIO5        |
| SCK      | GPIO18       |
| MOSI     | GPIO23       |
| MISO     | GPIO19       |
| IRQ      | (not connected) |

Pins can be changed: control pins (`NRF_CE_PIN`, `NRF_CSN_PIN`) in `src/main.cpp`; SPI bus pins
(`NRF_SCK_PIN`, `NRF_MISO_PIN`, `NRF_MOSI_PIN`) at the top of `src/Lightbar.cpp` — or override any
of them per board in `platformio.ini` `build_flags`.

## Configure

Edit the defines at the top of `src/main.cpp`:

- `WIFI_SSID` / `WIFI_PASS` — your network credentials.
- `REMOTE_ID` — the 3-byte id of your remote (e.g. `0xABCDEF`). Capture yours with
  [`scripts/scan_lightbar_remote.py`](../scripts/scan_lightbar_remote.py), or set an arbitrary id and
  re-pair the bar: unplug/replug the bar and trigger `/on_off` within 20 s (the bar will flash).

## Build, flash, monitor

```sh
cd esp32c3
pio run                 # compile (default env: esp32-c3-devkitm-1)
pio run -t upload       # flash over USB
pio device monitor      # serial console @ 115200
```

For the WEMOS LOLIN32 Lite, select its environment:

```sh
pio run -e lolin32-lite               # compile
pio run -e lolin32-lite -t upload     # flash over USB
```

On boot the serial console prints a self-test packet, whether the nRF24 chip is connected, and the
assigned WiFi IP address.

## Usage

```sh
curl http://<esp-ip>/on_off
```

Or open `http://<esp-ip>/` in a browser and tap the **On / Off** button. Call it again to toggle
back.

## Verifying without hardware

With `PACKET_SELFTEST 1` (the default) and `REMOTE_ID 0xABCDEF`, the serial console prints:

```
Self-test packet: 533914DD1C493412ABCDEFFF720100FAD4
```

This is the gold test vector from `tests/test_baseband.py` (`id=0xABCDEF, command=0x0100,
counter=0x72`) and proves the packet byte-order and CRC16 are correct.

## Troubleshooting

If the bar does not react even though the radio and WiFi are up:

- The bar rejects duplicate consecutive packets; the firmware auto-increments a rolling counter, so
  just call `/on_off` again.
- Try a different channel — edit `radio.setChannel(6)` in `src/Lightbar.cpp` to `15`, `43`, or `68`.
- Some nRF24 clones need the Enhanced ShockBurst framing disabled to match the Telink receiver. Add
  these to `Lightbar::begin()` in `src/Lightbar.cpp`, after `radio.begin(...)`:

  ```cpp
  radio.setAutoAck(false);
  radio.setCRCLength(RF24_CRC_DISABLED);
  ```

- Move the nRF24 close to the bar and confirm 3.3 V power with a decoupling capacitor.
