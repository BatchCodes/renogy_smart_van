#! /bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Install the system packages and the pinned ESP-IDF release that this project needs.
# Safe to run more than once.

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
REPO_DIR="$(dirname -- "${SCRIPT_DIR}")"
PACKAGES_FILE="${REPO_DIR}/system_packages.txt"
IDF_VERSION_FILE="${SCRIPT_DIR}/esp_idf_version.txt"
IDF_REPO_URL="https://github.com/espressif/esp-idf.git"
IDF_TARGETS="esp32,esp32s3"
SERIAL_GROUP="dialout"

usage() {
  cat <<USAGE
Usage: scripts/install.sh [options]

Options:
  --idf-dir DIR      Where to put ESP-IDF (default: ~/esp/esp-idf).
  --skip-packages    Do not install the system packages with apt-get.
  --skip-group       Do not add the user to the '${SERIAL_GROUP}' group.
  -h, --help         Show this help.
USAGE
}

require_command() {
  local command_name="$1"
  local reason="$2"

  if ! command -v "${command_name}" &>/dev/null; then
    printf 'error: "%s" is not installed. It is needed to %s.\n' "${command_name}" "${reason}" >&2
    return 1
  fi
}

read_packages() {
  grep -vE '^\s*(#|$)' "${PACKAGES_FILE}"
}

install_packages() {
  local packages

  if ! command -v apt-get &>/dev/null; then
    printf 'error: apt-get not found. This script supports Debian and Ubuntu only.\n' >&2
    printf 'Install the packages in %s with your package manager, then run again with --skip-packages.\n' \
      "${PACKAGES_FILE}" >&2
    return 1
  fi

  mapfile -t packages < <(read_packages)
  printf 'Installing %d system packages with apt-get (needs sudo).\n' "${#packages[@]}"
  if ! sudo apt-get update; then
    printf 'warning: apt-get update reported errors. A third-party repository is often the cause.\n' >&2
    printf 'The package install continues with the package lists that did update.\n' >&2
  fi

  sudo apt-get install -y "${packages[@]}"
}

clone_or_update_idf() {
  local idf_dir="$1"
  local idf_version="$2"

  if [[ ! -d "${idf_dir}/.git" ]]; then
    printf 'Cloning ESP-IDF %s to %s.\n' "${idf_version}" "${idf_dir}"
    mkdir -p "$(dirname -- "${idf_dir}")"
    git clone \
      --branch "${idf_version}" \
      --depth 1 \
      --recursive \
      --shallow-submodules \
      "${IDF_REPO_URL}" \
      "${idf_dir}"
    return 0
  fi

  if [[ -n "$(git -C "${idf_dir}" status --porcelain --untracked-files=no)" ]]; then
    printf 'error: %s has local changes. Commit or remove them, then run again.\n' "${idf_dir}" >&2
    return 1
  fi

  printf 'Updating ESP-IDF in %s to %s.\n' "${idf_dir}" "${idf_version}"
  git -C "${idf_dir}" fetch --depth 1 origin tag "${idf_version}"
  git -C "${idf_dir}" checkout --quiet "${idf_version}"
  git -C "${idf_dir}" submodule update --init --recursive --depth 1
}

install_idf_tools() {
  local idf_dir="$1"

  printf 'Installing ESP-IDF tools for targets: %s.\n' "${IDF_TARGETS}"
  "${idf_dir}/install.sh" "${IDF_TARGETS}"
}

add_serial_group() {
  local user_name
  user_name="$(id -un)"

  if id -nG "${user_name}" | grep -qw "${SERIAL_GROUP}"; then
    printf 'User %s is already in the %s group.\n' "${user_name}" "${SERIAL_GROUP}"
    return 0
  fi

  printf 'Adding %s to the %s group for serial port access (needs sudo).\n' "${user_name}" "${SERIAL_GROUP}"
  sudo usermod -aG "${SERIAL_GROUP}" "${user_name}"
  printf 'Log out and log in again to apply the group change.\n'
}

main() {
  local idf_dir="${HOME}/esp/esp-idf"
  local skip_packages=0
  local skip_group=0
  local idf_version

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --idf-dir)
        idf_dir="$2"
        shift 2
        ;;
      --idf-dir=*)
        idf_dir="${1#*=}"
        shift
        ;;
      --skip-packages)
        skip_packages=1
        shift
        ;;
      --skip-group)
        skip_group=1
        shift
        ;;
      -h | --help)
        usage
        return 0
        ;;
      *)
        printf 'error: unknown argument %s\n' "$1" >&2
        usage >&2
        return 1
        ;;
    esac
  done

  idf_version="$(tr -d '[:space:]' <"${IDF_VERSION_FILE}")"

  if [[ "${skip_packages}" -eq 0 ]]; then
    install_packages
  fi

  require_command git "download ESP-IDF"
  require_command python3 "run the ESP-IDF installer"

  clone_or_update_idf "${idf_dir}" "${idf_version}"

  install_idf_tools "${idf_dir}"

  if [[ "${skip_group}" -eq 0 ]]; then
    add_serial_group
  fi

  printf '\nDone. In each new terminal, load ESP-IDF with:\n\n  . %s/export.sh\n\n' "${idf_dir}"
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  set -euo pipefail
  main "$@"
fi
