# Heltec Wireless Paper Hardware Notes

This document records the hardware facts that the `epaper_panel` driver and the `rear_eink` app use. The board is the [Heltec Wireless Paper](https://heltec.org/project/wireless-paper/): an ESP32-S3FN8 with 8 MB flash, an SX1262 LoRa radio (not used by this project) and a 2.13" black and white e-paper panel.

## Hardware Versions

Heltec changed the e-paper panel, and with it the controller IC, between hardware versions. Each controller needs a different driver.

| Version | Panel | Controller | BUSY level while busy | Driver class |
| --- | --- | --- | --- | --- |
| V1.0 | DEPG0213BNS800 | SSD1680 | High | `Ssd1680Panel` |
| V1.1 | LCMEN2R13EFC1 | Fitipower JD79656 | Low | `Jd79656Panel` |
| V1.1.1 and V1.2 | E0213A367-BW | SSD1682 | High | `Ssd1682Panel` |

All versions have 122 × 250 pixels.

To identify your version:

- A V1.1 panel has a green protective film label.
- V1.1 boards have a white "V1.1" sticker on the back. V1.0 boards have no sticker.
- V1.1.1 has the V1.1 PCB with the new panel.
- The firmware can tell V1.1 from V1.1.1/V1.2 at start-up: it holds RST low and reads BUSY. BUSY low means JD79656, high means SSD1682. This test cannot tell V1.0 from V1.1.1/V1.2, so select V1.0 manually in `idf.py menuconfig`.

Refer to the [Heltec hardware update log](https://wiki.heltec.org/docs/devices/open-source-hardware/esp32-series/lora-32/wireless-paper/hardware-update-log) and the [panel datasheets](https://resource.heltec.cn/download/Wireless_Paper/E-Ink%20Datasheet).

## Pins

| Function | GPIO | Notes |
| --- | --- | --- |
| E-paper SCK | 3 | Own SPI bus (`SPI2_HOST`). No MISO. |
| E-paper MOSI | 2 | |
| E-paper CS | 4 | |
| E-paper DC | 5 | Low for a command, high for data. |
| E-paper RST | 6 | Active low. |
| E-paper BUSY | 7 | Level depends on the controller. Refer to the table above. |
| Vext (panel power) | 45 | Active low. Wait 50 ms after power on. |
| LED | 18 | |
| PRG button | 0 | Also the boot mode pin. |
| Battery ADC | 20 | Enabled by GPIO19 driven low. Divider about 1:2. |

The board has a CP2102 USB-UART bridge, so it appears as `/dev/ttyUSB0` on Linux. GPIO19 and GPIO20, the ESP32-S3 native USB pins, are used for the battery ADC.

## Frame Memory Layout

All three controllers use the same frame layout. `mono_gfx::FrameBuffer` stores frames in this layout:

- 250 rows of 16 bytes, 4000 bytes in total.
- The 122 pixel side is the byte axis. The last 6 bits of each row are not used.
- The most significant bit is the first pixel.
- Bit value 1 is white.

The drawing surface is landscape, 250 × 122. The `Display rotation` option in `idf.py menuconfig` turns it by 180 degrees.

## Refresh Behaviour

| Controller | Full refresh | Fast refresh | Source |
| --- | --- | --- | --- |
| SSD1680 (V1.0) | About 4.0 s | About 0.74 s | Measured by the GxEPD2 author. |
| JD79656 (V1.1) | At least 3.65 s | At least 0.72 s | Meshtastic polling delays, not measurements. |
| SSD1682 (V1.1.1/V1.2) | At least 1.5 s | At least 0.5 s | Meshtastic polling delays, not measurements. |

The test pattern mode of `rear_eink` logs the measured time of each refresh.

The driver keeps a copy of the last frame. A fast refresh sends it to the controller as the "old" image. This makes fast refreshes work after the controller sleeps and loses its image memory. The first refresh after start-up is always a full refresh.

## Sources

The command sequences come from these GPL-3.0 projects:

- [GxEPD2](https://github.com/ZinggJM/GxEPD2), `GxEPD2_213_BN` (SSD1680).
- [Meshtastic firmware](https://github.com/meshtastic/firmware), `LCMEN2R13EFC1` (JD79656) and `E0213A367` (SSD1682), with values supplied by Heltec.
- [heltec-eink-modules](https://github.com/todd-herbert/heltec-eink-modules), for version identification.
