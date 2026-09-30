# Renogy BT-2 E-paper Display

This project is ESP32 firmware that shows live data from a Renogy solar or DC-DC charger on a small e-paper display. The firmware connects to the charger's Renogy BT-2 Bluetooth module over Bluetooth Low Energy (BLE). The official Renogy DC Home app continues to work at the same time.

The first target is a van with a Renogy RBC50D1S DC-DC charger with MPPT. The display unit is in the sleeping area, so it uses reflective e-paper with no light. It updates 24 hours a day from a 5 V USB supply.

`van_sensors` is a working name. The project gets a final name when it is published.

> **Status: early development.** The repository set-up is complete. The firmware apps do not exist yet. The build and flash commands below show the planned workflow for `firmware/rear_eink/`.

## How It Works

```text
Renogy charger --RS485-- BT-2 module --BLE--+-- Renogy DC Home app (phone)
                                            |
                                            +-- ESP32-S3 + e-paper display (this project)
```

The BT-2 is a bridge between the charger's RS485 port and BLE. The firmware sends Modbus read requests to the BT-2 over BLE and decodes the responses. The display shows these values:

- solar (PV) power, voltage and current
- house battery voltage and current
- charging state
- energy for the day, in kWh
- the data age, or "offline" when the BT-2 does not respond

## Supported Hardware

| Part | Status | Notes |
| --- | --- | --- |
| Renogy RBC50D1S DC-DC charger with MPPT | Target | Connected to a BT-2 module. |
| Other Renogy controllers with a BT-2 or BT-1 module | Untested | The protocol is the same. The register map can be different. |
| [Heltec Wireless Paper](https://heltec.org/project/wireless-paper/) (ESP32-S3, 2.13" e-paper) | Target | The first display board. |
| Other ESP32-S3 boards with an e-paper panel | Untested | Refer to [CONTRIBUTING.md](CONTRIBUTING.md) to add a board. |
| Any ESP32 development board (for example ESP-WROOM-32) | Development only | Use it to test the BLE connection without a display. |

## Required Tools

- A Linux computer. The install script supports Debian and Ubuntu. Other systems can install the tools manually.
- The system packages in [system_packages.txt](system_packages.txt).
- ESP-IDF, the Espressif IoT Development Framework. The project pins the version in [scripts/esp_idf_version.txt](scripts/esp_idf_version.txt).
- A USB-C data cable. Some cables supply power only and cannot flash the board.
- Optional: [Visual Studio Code](https://code.visualstudio.com/) with the Espressif ESP-IDF extension.
- Optional: nRF Connect for Mobile on a phone, to inspect the BT-2.

## Installation

### Install with the script

Run the install script from the repository root:

```bash
./scripts/install.sh
```

The script does these steps:

1. It installs the packages in `system_packages.txt` with `apt-get`. This step needs `sudo`.
2. It clones the pinned ESP-IDF release to `~/esp/esp-idf`, or updates an existing clone to that release.
3. It installs the ESP-IDF tools for the `esp32` and `esp32s3` targets.
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
   ~/esp/esp-idf/install.sh esp32,esp32s3
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

Flash the firmware and open the serial monitor. Replace `/dev/ttyACM0` with your port:

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

To exit the serial monitor, press `Ctrl+]`.

## Configuration

Device-specific values are Kconfig options. They are not hard-coded. Open the configuration menu in the app directory:

```bash
idf.py menuconfig
```

The planned options are:

| Option | Purpose |
| --- | --- |
| Data source | Fake data (display test with no BT-2) or the BT-2. |
| BT-2 name prefix | The BLE name to look for. |
| BT-2 MAC address | Optional. It stops the unit from connecting to another BT-2 nearby, for example at a campsite. |
| Poll interval | Time between two reads from the BT-2. The test default is 5 s. |
| Stale and offline timeouts | Time before the display shows the data age or "offline". |

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
