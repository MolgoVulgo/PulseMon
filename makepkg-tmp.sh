#!/usr/bin/env bash
set -euo pipefail

SOURCE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TMP_BUILD_ROOT="${PULSEMON_TMP_BUILD_ROOT:-/tmp/pulsemon-api-build}"
TMP_PKGDEST="${PULSEMON_TMP_PKGDEST:-/tmp/pulsemon-api-pkgs}"

rm -rf "${TMP_BUILD_ROOT}"
mkdir -p "${TMP_BUILD_ROOT}" "${TMP_PKGDEST}"

rsync -a --delete --exclude ".git" "${SOURCE_ROOT}/" "${TMP_BUILD_ROOT}/"
cd "${TMP_BUILD_ROOT}"

if [[ $# -eq 0 ]]; then
  set -- -si
fi

PKGDEST="${TMP_PKGDEST}" makepkg "$@"
