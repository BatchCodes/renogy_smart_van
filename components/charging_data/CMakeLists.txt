# SPDX-License-Identifier: GPL-3.0-or-later
# Pure C++ component. It must not depend on ESP-IDF, so the host tests can build it.
idf_component_register(
    SRCS "src/fake_source.cpp" "src/latest_reading.cpp"
    INCLUDE_DIRS "include"
)
