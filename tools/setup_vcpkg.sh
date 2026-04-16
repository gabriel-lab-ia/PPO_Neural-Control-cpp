#!/usr/bin/env bash
set -euo pipefail

VCPKG_ROOT_DEFAULT="${HOME}/.vcpkg"
VCPKG_ROOT_PATH="${VCPKG_ROOT:-${VCPKG_ROOT_DEFAULT}}"

if [[ ! -d "${VCPKG_ROOT_PATH}/.git" ]]; then
  echo "[setup_vcpkg] Cloning vcpkg into ${VCPKG_ROOT_PATH}"
  git clone --depth 1 https://github.com/microsoft/vcpkg.git "${VCPKG_ROOT_PATH}"
else
  echo "[setup_vcpkg] Reusing existing vcpkg at ${VCPKG_ROOT_PATH}"
fi

echo "[setup_vcpkg] Bootstrapping toolchain"
"${VCPKG_ROOT_PATH}/bootstrap-vcpkg.sh" -disableMetrics

cat <<EOF
[setup_vcpkg] Done.
Export before running CMake presets:
  export VCPKG_ROOT="${VCPKG_ROOT_PATH}"
EOF
