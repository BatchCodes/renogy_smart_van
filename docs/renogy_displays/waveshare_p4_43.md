# Waveshare ESP32-P4 Board Notes

This document records the hardware and software facts that `firmware/cab_dash` uses. The board is the [Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3](https://www.waveshare.com/product/iot-communication/short-range-wireless/esp32-p4-wifi6-touch-lcd-4.3.htm) (SKU 33874). It is the cab unit: a touch dashboard for the charging data and the rear camera.

The facts come from the Waveshare schematic and BSP, the Espressif `esp-hosted-mcu` and `esp-usb` repositories, and test builds with ESP-IDF v6.1. **None of this is tested on the board yet.**

## Board Summary

| Part | Value |
| --- | --- |
| Main processor | ESP32-P4, 32 MB flash (GD25Q256), 32 MB PSRAM. The P4 has no radio. |
| Radio | ESP32-C6-MINI-1 co-processor, connected to the P4 over SDIO. It supplies BLE and Wi-Fi. |
| Display | 4.3" 480 × 800 IPS, ST7701 controller, 2-lane MIPI-DSI. The dashboard turns it to 800 × 480 landscape. |
| Touch | GT911 on I2C (SDA GPIO7, SCL GPIO8). |
| Backlight | GPIO26, LEDC PWM. |
| Console | UART0 (GPIO37 TX, GPIO38 RX) through a CH343P on the Type-C port labelled "USB TO UART". On Linux it is normally `/dev/ttyACM0`. |
| USB host | Type-C port labelled "USB": the P4 USB 2.0 high-speed port. |

## Software Stack

| Component | Version | Purpose |
| --- | --- | --- |
| `waveshare/esp32_p4_wifi6_touch_lcd_4_3` | 1.0.1 | Board support: display, touch, backlight, USB host power. |
| `espressif/esp_lvgl_adapter` | 0.6.x (through the BSP) | LVGL port, rotation, tear avoidance. |
| `lvgl/lvgl` | 9.5.0 (through the BSP) | User interface. |
| `espressif/esp_hosted` | 3.0.9 | BLE through the C6. NimBLE runs on the P4, the controller on the C6. |
| `espressif/usb_host_uvc` | 2.6.0 | USB camera (planned, Phase 3). |

The versions are pinned in `firmware/cab_dash/main/idf_component.yml`.

The app partition is 8 MB (`firmware/cab_dash/partitions.csv`), for the BSP, LVGL, BLE and the camera code.

The display uses three frame buffers in PSRAM (about 2.3 MB) to avoid tearing. The 90 degree rotation is a CPU copy on each update.

## Chip Revision

The firmware is built for ESP32-P4 chip revision v3.x (`CONFIG_ESP32P4_REV_MIN_300`). An image for v3.x does not boot on an older v1.x chip. Check the revision when the board arrives:

```bash
esptool --chip esp32p4 -p /dev/ttyACM0 chip-id
```

For a v1.x chip, build with the extra defaults file:

```bash
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.rev_lt_v3" build
```

## ESP32-C6 Firmware

BLE on the P4 needs the `esp_hosted` slave firmware on the C6, with Bluetooth enabled. The firmware that Waveshare installs on the C6 is not known. It is probably an old `esp_hosted` 1.x version, and some early boards reportedly shipped without Bluetooth in the C6 firmware.

The cab app logs the C6 firmware version at start-up (`C6 esp_hosted firmware x.y.z`). If the log shows `C6 BLE controller` as an error, the C6 firmware has no Bluetooth or is too old. If the C6 firmware is too old or has no Bluetooth, flash a new one. There are two methods:

- Over the air from the P4, with the `esp-hosted-mcu` co-processor OTA example.
- With a USB-TTL adapter on header P1 "C6-UART" (TX, RX, IO9, GND). Connect IO9 to GND, and hold the P4 BOOT key at power on, so the P4 cannot reset the C6.

The SDIO pins (P4 GPIO14 to GPIO19, C6 reset on GPIO54) match the `esp_hosted` defaults for the P4, so no pin settings are necessary.

## USB Camera Power

The "USB" Type-C port has 5.1 kΩ pull-down resistors on CC and a reverse-current block on VBUS. This means the board probably **does not supply 5 V** to a device on that port. The USB capture adapter for the reversing camera then needs power from a powered hub or an OTG Y-cable. Check this on the board before you buy cables.

## Header Pins

These GPIOs on the 40-pin header are free: 2, 3, 4, 5, 21, 22, 28 to 32, 46 to 52. The planned reverse-gear input uses GPIO4 or GPIO5, through a protection circuit (Phase 3). Do not use GPIO7 and GPIO8 (I2C), GPIO37 and GPIO38 (console), GPIO34 and GPIO35 (strapping pins), or GPIO24 and GPIO25 (USB).

## Sources

- [Waveshare board repository](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-4.3), including the schematic.
- [Waveshare ESP32 components](https://github.com/waveshareteam/Waveshare-ESP32-components), for the BSP.
- [esp-hosted-mcu](https://github.com/espressif/esp-hosted-mcu), for BLE through the C6 and the migration notes.
- [esp-usb](https://github.com/espressif/esp-usb), for the UVC host driver.
