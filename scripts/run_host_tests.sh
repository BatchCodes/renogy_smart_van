#! /bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Build and run the host tests in tests/host on the ESP-IDF linux target.
# Load ESP-IDF first: . ~/esp/esp-idf/export.sh

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
TESTS_DIR="$(dirname -- "${SCRIPT_DIR}")/tests/host"
TEST_BINARY="${TESTS_DIR}/build/host_tests.elf"

main() {
  if ! command -v idf.py &>/dev/null; then
    printf 'error: idf.py not found. Load ESP-IDF first with: . ~/esp/esp-idf/export.sh\n' >&2
    return 1
  fi

  idf.py -C "${TESTS_DIR}" build

  "${TEST_BINARY}"
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  set -euo pipefail
  main "$@"
fi
