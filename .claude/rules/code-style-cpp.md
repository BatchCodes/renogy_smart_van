---
paths:
  - "**/*.cpp"
  - "**/*.hpp"
  - "**/*.h"
  - "**/CMakeLists.txt"
  - "**/Kconfig*"
---

# C++ and ESP-IDF Rules

The firmware is C++ on ESP-IDF, at the version in `scripts/esp_idf_version.txt`.

- Format C++ with `clang-format` and the project file `.clang-format`.
- Start each source file with `// SPDX-License-Identifier: GPL-3.0-or-later`. In `CMakeLists.txt` and `Kconfig` files, use `#`.
- Naming: PascalCase for types, snake_case for functions and variables, `kPascalCase` for constants, a trailing underscore for private members (`config_`), and one namespace per component that matches the component name.
- Keep `charging_data`, `renogy_protocol`, `mono_gfx` and `eink_view` free of ESP-IDF headers. The host tests in `tests/host/` build them on the ESP-IDF `linux` target. Put hardware access in separate components, such as `epaper_panel` and `renogy_ble`.
- Add a host test in `tests/host/main/` for each change to a pure component. Run `./scripts/run_host_tests.sh` before you commit.
- Build each app in `firmware/` that a change touches. The build must have no warnings from this repo's code. ESP-IDF builds with `-Werror`.
- Put device and timing values in Kconfig options with documented defaults. Do not hard-code them. Put board pins in the board app, not in shared components.
- Return `esp_err_t` from functions that can fail on hardware, and use `ESP_RETURN_ON_ERROR` with a log message. Do not use exceptions. The apps build with C++ exceptions and RTTI turned off.
- Value-initialise ESP-IDF configuration structs (`spi_bus_config_t config = {};`) and set the fields one by one. Designated initialisers fail with `-Wmissing-field-initializers` when a struct has more fields than you set.
- Write a comment only for a reason or a fact that the code cannot show, for example a datasheet value or a protocol quirk. Name the source of a register map, a command sequence or a magic value.
