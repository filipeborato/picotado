#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG="${1:-Release}"
SRC="${ROOT}/build/plugin/Picotado_artefacts/${CONFIG}/LV2/Picotado.lv2"
DST="${HOME}/.lv2/Picotado.lv2"

if [[ ! -d "$SRC" ]]; then
  echo "LV2 bundle not found: $SRC" >&2
  echo "Run scripts/release-linux.sh first." >&2
  exit 1
fi

rm -rf "$DST"
mkdir -p "${HOME}/.lv2"
cp -R "$SRC" "${HOME}/.lv2/"

echo "Installed LV2 bundle to $DST"
