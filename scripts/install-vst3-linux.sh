#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG="${1:-Release}"
SRC="${ROOT}/build/plugin/Picotado_artefacts/${CONFIG}/VST3/Picotado.vst3"
DST="${HOME}/.vst3/Picotado.vst3"
SO="${DST}/Contents/x86_64-linux/Picotado.so"

if [[ ! -d "$SRC" ]]; then
  echo "VST3 bundle not found: $SRC" >&2
  echo "Run scripts/release-linux.sh first." >&2
  exit 1
fi

rm -rf "$DST"
rm -f "${HOME}/.vst3/Picotado.so"
mkdir -p "${HOME}/.vst3"
cp -R "$SRC" "${HOME}/.vst3/"
chmod +x "$SO"

echo "Installed VST3 bundle to $DST"
