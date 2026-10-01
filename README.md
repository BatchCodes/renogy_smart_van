# Renogy Smart Van

[![CI](https://github.com/BatchCodes/renogy_smart_van/actions/workflows/ci.yml/badge.svg)](https://github.com/BatchCodes/renogy_smart_van/actions/workflows/ci.yml)

This project is ESP32 firmware that shows live data from a Renogy solar or DC-DC charger on a small e-paper display. The firmware connects to the charger's Renogy BT-2 Bluetooth module over Bluetooth Low Energy (BLE). The official Renogy DC Home app continues to work at the same time.

The first target is a van with a Renogy RBC50D1S DC-DC charger with MPPT. The display unit is in the sleeping area, so it uses reflective e-paper with no light. It updates 24 hours a day from a 5 V USB supply.

The source code is at [github.com/BatchCodes/renogy_smart_van](https://github.com/BatchCodes/renogy_smart_van). Report problems and ideas as GitHub issues.

> **Status: early development.** `firmware/rear_eink/` shows the charging layout. It works with fake data on a Heltec Wireless Paper V1.1.1/V1.2. The BT-2 data source is written from the reference projects but is not yet tested with a real BT-2. V1.0 and V1.1 boards are not tested yet.

## How It Works

```text
Renogy charger --RS485-- BT-2 module --BLE--+-- Renogy DC Home app (phone)
                                            |
                                            +-- ESP32-S3 + e-paper display (this project)
```

The BT-2 is a bridge between the charger's RS485 port and BLE. The firmware sends Modbus read requests to the BT-2 over BLE and decodes the responses. The display shows these values:

- solar (PV) power, voltage and current
- auxiliary (house) battery voltage and current
- starter battery voltage and current (the charger's alternator input)
- charging state
- energy for the day, in kWh
- the time since the last update, or "offline" when the BT-2 does not respond

## Supported Hardware

| Part | Status | Notes |
| --- | --- | --- |
| Renogy RBC50D1S DC-DC charger with MPPT | Target | Connected to a BT-2 module. |
| Other Renogy controllers with a BT-2 or BT-1 module | Untested | The protocol is the same. The register map can be different. |
| [Heltec Wireless Paper](https://heltec.org/project/wireless-paper/) (ESP32-S3, 2.13" e-paper) | Tested on V1.1.1/V1.2 | The first display board. Hardware versions V1.0, V1.1, V1.1.1 and V1.2 use different panels. The firmware supports all of them. Refer to [Heltec Wireless Paper hardware notes](docs/renogy_displays/heltec_wireless_paper.md). |
| Other ESP32-S3 boards with an e-paper panel | Untested | Refer to [CONTRIBUTING.md](CONTRIBUTING.md) to add a board. |
| [Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3](https://www.waveshare.com/product/iot-communication/short-range-wireless/esp32-p4-wifi6-touch-lcd-4.3.htm) (cab unit) | In development | `firmware/cab_dash`. Builds, not tested on hardware. Refer to [Waveshare ESP32-P4 board notes](docs/renogy_displays/waveshare_p4_43.md). |
| Any ESP32 development board (for example ESP-WROOM-32) | Development only | Use it to test the BLE connection without a display. |

## Required Tools

- A Linux computer. The install script supports Debian and Ubuntu. Other systems can install the tools manually.
- The system packages in [system_packages.txt](system_packages.txt).
- ESP-IDF, the Espressif IoT Development Framework. The project pins the version in [scripts/esp_idf_version.txt](scripts/esp_idf_version.txt).
- A USB-C data cable. Some cables supply power only and cannot flash the board.
- Optional: [Visual Studio Code](https://code.visualstudio.com/) with the Espressif ESP-IDF extension.
- Optional: nRF Connect for Mobile on a phone, to inspect the BT-2.
- Alternative to the native tools: Docker. Refer to [Build with Docker](#build-with-docker).

## Get the Code

```bash
git clone https://github.com/BatchCodes/renogy_smart_van.git
cd renogy_smart_van
```

Run all commands in this README from the repository root, unless a step says otherwise.

## Installation

### Install with the script

Run the install script from the repository root:

```bash
./scripts/install.sh
```

The script does these steps:

1. It installs the packages in `system_packages.txt` with `apt-get`. This step needs `sudo`.
2. It clones the pinned ESP-IDF release to `~/esp/esp-idf`, or updates an existing clone to that release.
3. It installs the ESP-IDF tools for the `esp32`, `esp32s3` and `esp32p4` targets.
4. It adds your user to the `dialout` group, so you can use the serial port without `sudo`.

After the script completes, log out and log in again to apply the group change. You can run the script again at any time. It skips work that is already done.

Use `./scripts/install.sh --help` to see the options. For example, `--idf-dir` sets a different ESP-IDF location, and `--skip-packages` skips `apt-get`.

### Install manually

Use this procedure on a system without `apt-get`, or if you want to control each step.

1. Install the packages in `system_packages.txt` with your package manager.
2. Clone ESP-IDF at the pinned version:

   ```bash
   git clone --recursive --branch "$(cat scripts/esp_idf_version.txt)" https://github.com/espressif/esp-idf.git ~/esp/esp-idf
   ```

3. Install the ESP-IDF tools:

   ```bash
   ~/esp/esp-idf/install.sh esp32,esp32s3,esp32p4
   ```

4. Add your user to the `dialout` group, then log out and log in again:

   ```bash
   sudo usermod -aG dialout "$USER"
   ```

ESP-IDF also has a new graphical installer, the ESP-IDF Installation Manager (`eim`). You can use it in place of steps 2 and 3. Select the version in `scripts/esp_idf_version.txt`. Refer to the [ESP-IDF Linux set-up guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/linux-setup.html).

## Build and Flash

Load ESP-IDF into each new terminal before you use `idf.py`:

```bash
. ~/esp/esp-idf/export.sh
```

Build the rear display app:

```bash
cd firmware/rear_eink
idf.py set-target esp32s3
idf.py build
```

Connect the board with USB. Find its serial port:

```bash
ls /dev/ttyACM* /dev/ttyUSB*
```

Flash the firmware and open the serial monitor. The Heltec Wireless Paper appears as `/dev/ttyUSB0`. Replace it with your port if it is different:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

To exit the serial monitor, press `Ctrl+]`.

## Build with Docker

Docker is an alternative to the native installation. You do not need the system packages or ESP-IDF on your computer. You need only Docker. The wrapper script uses the official `espressif/idf` image at the version in `scripts/esp_idf_version.txt`.

Build the rear display app:

```bash
./scripts/idf_docker.sh firmware/rear_eink set-target esp32s3
./scripts/idf_docker.sh firmware/rear_eink build
```

On Linux, flash the firmware and open the serial monitor from the container. The `-p` option passes the serial port into the container:

```bash
./scripts/idf_docker.sh -p /dev/ttyUSB0 firmware/rear_eink flash monitor
```

The script runs the container as your user, so the files in `build/` belong to you and not to `root`. The first run downloads the image, which is several GB.

> **USB pass-through works on Linux only.** Docker Desktop on macOS and Windows cannot give a container access to a USB serial port. On these systems, build in the container, then flash from the host with `esptool` (`pip install esptool`) or with the [Espressif web flasher](https://espressif.github.io/esptool-js/). The flash command and the file offsets are in `firmware/rear_eink/build/flash_args` after a build.

### VS Code Dev Container

The repository has a Dev Container configuration in [.devcontainer/devcontainer.json](.devcontainer/devcontainer.json). It uses the same image.

1. Install the VS Code extension `ms-vscode-remote.remote-containers`.
2. Open the repository in VS Code.
3. Open the command palette and run "Dev Containers: Reopen in Container".

The Dev Container runs as `root`. On Linux, files that you build in it belong to `root`. Use `./scripts/idf_docker.sh` if this is a problem. To flash from the Dev Container on Linux, remove the comment markers from the `runArgs` line in `devcontainer.json` and set your serial port.

## Configuration

Device-specific values are Kconfig options. They are not hard-coded. Open the configuration menu in the app directory:

```bash
idf.py menuconfig
```

The options are in the "Rear e-paper display" menu:

| Option | Purpose |
| --- | --- |
| Display mode | "Charging data" is the normal display, from the selected data source. "Test pattern" draws a fixed pattern and logs the refresh times. Use it to bring up a board. |
| Data source | "Fake data" simulates a five minute day, with values that change at every refresh. Use it to test the display with no BT-2. "Renogy BT-2" reads the charger. |
| Fake data: simulate BT-2 outages | The fake source stops sending for three minutes of each five minute day, so the stale box and the offline screen appear. Off by default. |
| Panel model | Heltec Wireless Paper hardware version. "Auto detect" works for V1.1, V1.1.1 and V1.2. Select V1.0 manually. |
| Display rotation | Turns the image by 180 degrees. |
| Panel pins | The e-paper pins. The defaults are for the Heltec Wireless Paper. |
| Refresh interval | Time between two display updates. The test default is 5 s. |
| Show the data age after | When the newest reading is older than this, the display shows its age in an inverted box. |
| Show offline after | When the newest reading is older than this, the display shows the offline screen. |
| Full refresh of an unchanged image after | The display refreshes only when the image changes, and does a full refresh after this time with no change. |
| Full refresh after this many fast refreshes | A full refresh removes ghosting, but the panel flashes. |

The "Renogy BT-2" menu sets the BLE connection:

| Option | Purpose |
| --- | --- |
| BT-2 name prefix | The source connects to the first device whose name starts with this text. The default is `BT-TH-`. |
| BT-2 MAC address | Optional. Connect only to this BT-2, for example at a campsite with other vans. The log shows the address of each BT-2 found. |
| Modbus device ID | 255 reaches the charger connected to the BT-2. Change it only for a hub or a daisy chain. |
| Poll interval | Time between two reads of the charger values. The default is 5 s. |
| Response timeout, failed reads | When to count a read as failed, and when to disconnect and scan again. |
| Log every named BLE device found | Logs each named device in the scan, not only the BT-2. Use it to check that BLE works, or to find the BT-2 name. |

## Test the BT-2 Connection

`firmware/ble_probe/` connects to the BT-2 and logs each raw response in hex and each decoded reading. It needs no display, so it runs on any ESP32 board with BLE. Use it to check the connection and the values before you use the display.

```bash
cd firmware/ble_probe
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Compare the logged values with the Renogy DC Home app. The BT-2 support is new and not yet tested with a real BT-2.

## Cab Dashboard

`firmware/cab_dash/` is a touch dashboard for the cab, for the [Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3](https://www.waveshare.com/product/iot-communication/short-range-wireless/esp32-p4-wifi6-touch-lcd-4.3.htm). It shows the same charging data on four pages: solar, batteries, temperatures, and energy with a one-hour solar power chart. The status bar turns amber when the data is old and red when the BT-2 is offline.

> **Not tested on hardware yet.** Read [Waveshare ESP32-P4 board notes](docs/renogy_displays/waveshare_p4_43.md) before the first flash. It explains how to check the chip revision and the ESP32-C6 firmware.

```bash
cd firmware/cab_dash
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Use the Type-C port labelled "USB TO UART". The options are in the "Cab dashboard" menu of `idf.py menuconfig`:

| Option | Purpose |
| --- | --- |
| Screen | "Dashboard" is the normal screen. "Test screen" shows the resolution, a tap counter and a colour bar, to check the display and the touch input. "Camera test" shows only the camera and logs the frame rate and the decode time, to check the USB capture adapter. |
| Camera resolution | The MJPEG resolution to ask the capture adapter for: 640 × 480, 720 × 480 or 720 × 576 (PAL). |
| Reverse input GPIO, active low, debounce | The reverse-gear input. Refer to [Reverse gear input](docs/renogy_displays/reverse_input.md) for the circuit. |
| Data source | "Fake data" or "Renogy BT-2". The BT-2 options are in the "Renogy BT-2" menu. |
| Backlight off after no touch for | Turns the backlight off after this time with no touch. A touch turns it on again. 0 keeps it on. |
| Display rotation | 90 or 270 degrees, for the mounting direction. |
| Backlight brightness | In percent. |
| Data poll interval, stale and offline timeouts | When the dashboard shows the data age or the offline message. |

To see the dashboard pages with no board, run `./scripts/ci.sh previews`. Refer to [CONTRIBUTING.md](CONTRIBUTING.md).

## Visual Studio Code

1. Install the extension `espressif.esp-idf-extension`.
2. Open the command palette and run "ESP-IDF: Configure ESP-IDF Extension".
3. Select "Use existing setup" and select `~/esp/esp-idf`.
4. Open an app directory, for example `firmware/rear_eink/`, as the workspace folder.

## Troubleshooting

| Problem | Cause | Fix |
| --- | --- | --- |
| `Permission denied` on `/dev/ttyUSB0` or `/dev/ttyACM0` | Your user is not in the `dialout` group. | Run `sudo usermod -aG dialout "$USER"`, then log out and log in again. |
| The port for a CH340 board disappears after a few seconds (Ubuntu) | The `brltty` braille service takes the CH340 USB ID. | Run `sudo apt-get remove brltty` if you do not use a braille display. |
| `Failed to connect to ESP32` during flash | The board is not in download mode. | Hold the `BOOT` button, push and release the `RST` button, then release `BOOT`. Flash again. |
| No serial port appears | The USB cable supplies power only. | Use a USB data cable. |
| `apt-get update` shows `is not signed` or `OpenPGP signature verification failed` for a third-party repository | That repository uses an old signing key. Debian 13 rejects SHA1 keys. | The install script shows a warning and continues. To remove the error, disable that repository in `/etc/apt/sources.list.d/`. |
| CMake reports that the cache directory is different, or that the source does not match | The `build/` directory was made in Docker and is now used natively, or the opposite. The paths in the container are different. | Delete the `build/` directory in the app, then build again. |
| `idf.py: command not found` | ESP-IDF is not loaded in this terminal. | Run `. ~/esp/esp-idf/export.sh`. |

## Credits

Other people decoded the Renogy BT-2 protocol. This project uses their work as its reference:

- [cyrils/renogy-bt](https://github.com/cyrils/renogy-bt): Python library for Renogy devices with BT-1 and BT-2 modules.
- [neilsheps/Renogy-BT2-Reader](https://github.com/neilsheps/Renogy-BT2-Reader): Arduino library for Renogy devices with a BT-2 module.
- [mateuszdrab esphome-renogy-ble.yaml](https://gist.github.com/mateuszdrab/922c760582fce29d63608a1a405c541b): ESPHome configuration for Renogy BLE.

This project is not affiliated with or endorsed by Renogy.

## Licence

This project is licensed under the GNU General Public License v3.0 or later (GPL-3.0-or-later). Refer to [LICENSE](LICENSE).

## See Also

- [CONTRIBUTING.md](CONTRIBUTING.md): build, test and code style rules, and how to add a board.
- [docs/renogy_displays/](docs/renogy_displays/): design notes and BT-2 discovery results.
