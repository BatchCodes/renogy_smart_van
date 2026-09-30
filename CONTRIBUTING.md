# Contributing to the Renogy BT-2 E-paper Display

This document tells you how to build and test the project, which code style to use, and how to add a new board or a new Renogy device. Install the tools first. Refer to the installation section in [README.md](README.md).

## Repository Layout

| Path | Contents |
| --- | --- |
| `components/` | Shared ESP-IDF components. All firmware apps use them. |
| `firmware/` | One ESP-IDF app for each board, for example `firmware/rear_eink/`. |
| `tests/host/` | Host tests. They run on your computer with the ESP-IDF `linux` target. |
| `scripts/` | Install script and the pinned ESP-IDF version. |
| `docs/renogy_displays/` | Design notes and BT-2 discovery results. |

The shared components follow these rules:

- `charging_data` and `renogy_protocol` must not include ESP-IDF headers. The host tests compile them on your computer.
- The display code reads `ChargingData` through the `ChargingDataSource` interface. It does not include BLE or protocol headers.
- Board-specific values (pins, display, BLE controller) live in the board app and its `sdkconfig.defaults`. They do not live in the shared components.
- Timing and device values are Kconfig options. They are not constants in the code.

## Build

Load ESP-IDF, then build each app that your change touches:

```bash
. ~/esp/esp-idf/export.sh
cd firmware/rear_eink
idf.py set-target esp32s3
idf.py build
```

## Test

The host tests use Unity on the ESP-IDF `linux` target. The test commands are added with the first test in `tests/host/`. Run the host tests before you send a change.

To test on hardware, flash the app and read the serial monitor:

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

## Code Style

- Format C and C++ code with `clang-format`. The project file is [.clang-format](.clang-format).
- Format Markdown, JSON and YAML with Prettier. The project file is [.prettierrc.json](.prettierrc.json).
- Check the spelling with cspell. Add a correct project word to [cspell.json](cspell.json).
- Use British English in documentation, for example "colour" and "licence".
- Write a commit message in the Conventional Commits format, for example `feat: add partial refresh`.

Start each new source file with an SPDX licence header:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later
```

In a shell script, use `# SPDX-License-Identifier: GPL-3.0-or-later` on the line after the shebang.

## Add a Board

1. Create a new app directory in `firmware/`, for example `firmware/rear_eink_<board>/`. Copy the top-level `CMakeLists.txt` from an existing app. It must set `EXTRA_COMPONENT_DIRS` to `../../components`.
2. Put the board pins and settings in the app's `sdkconfig.defaults` and Kconfig. Do not change the shared components for one board.
3. If the board has a new e-paper panel controller, add a driver for it next to the existing driver in `components/epaper_panel/`.
4. Add the board to the supported hardware table in [README.md](README.md). Mark it as "Untested" until someone confirms that it works.

## Add a Renogy Device

Other Renogy controllers use the same BT-2 protocol but can use a different register map.

1. Build the firmware with the BT-2 data source. The firmware logs each raw BT-2 response as hex.
2. Compare the decoded values with the Renogy DC Home app.
3. Add the register map for the device to `components/renogy_protocol/`.
4. Save the logged hex responses as test vectors in `tests/host/`, with the values that the app showed.
5. Add the device to the supported hardware table in [README.md](README.md).

## See Also

- [README.md](README.md): project overview, installation, build and flash.
- [LICENSE](LICENSE): GPL-3.0 licence text.
