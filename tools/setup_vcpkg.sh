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

if command -v python3 >/dev/null 2>&1; then
  VCPKG_BASELINE="$(python3 -c 'import json; print(json.load(open("vcpkg.json", "r", encoding="utf-8")).get("builtin-baseline", ""))')"
  if [[ -n "${VCPKG_BASELINE}" ]]; then
    if ! git -C "${VCPKG_ROOT_PATH}" cat-file -e "${VCPKG_BASELINE}^{commit}" 2>/dev/null; then
      echo "[setup_vcpkg] Fetching pinned baseline ${VCPKG_BASELINE}"
      git -C "${VCPKG_ROOT_PATH}" fetch --depth 1 origin "${VCPKG_BASELINE}"
    fi
    git -C "${VCPKG_ROOT_PATH}" checkout "${VCPKG_BASELINE}"
  fi
fi

echo "[setup_vcpkg] Bootstrapping toolchain"
"${VCPKG_ROOT_PATH}/bootstrap-vcpkg.sh" -disableMetrics

cat <<EOF
[setup_vcpkg] Done.
Export before running CMake presets:
  export VCPKG_ROOT="${VCPKG_ROOT_PATH}"
EOF
