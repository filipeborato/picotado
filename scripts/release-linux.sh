#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build"
DIST_DIR="${ROOT}/dist"
ARTIFACT_ROOT="${BUILD_DIR}/plugin/Picotado_artefacts/Release"
VST3_BUNDLE="${ARTIFACT_ROOT}/VST3/Picotado.vst3"
VST3_SO="${VST3_BUNDLE}/Contents/x86_64-linux/Picotado.so"
LV2_BUNDLE="${ARTIFACT_ROOT}/LV2/Picotado.lv2"
STANDALONE_EXE="${ARTIFACT_ROOT}/Standalone/Picotado"

usage() {
  cat <<'EOF'
Usage: scripts/release-linux.sh [--install-deps]

Builds Picotado Release artifacts for Linux and writes:
  dist/Picotado-v<version>-Linux-VST3.tar.gz
  dist/Picotado-v<version>-Linux-LV2.tar.gz
  dist/INSTALL-Linux.md

Set PICOTADO_VERSION to override the version.
EOF
}

install_deps=false
for arg in "$@"; do
  case "$arg" in
    --install-deps) install_deps=true ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown argument: $arg" >&2; usage >&2; exit 2 ;;
  esac
done

detect_version() {
  if [[ -n "${PICOTADO_VERSION:-}" ]]; then
    printf '%s\n' "${PICOTADO_VERSION}"
    return
  fi

  local workflow="${ROOT}/.github/workflows/build-release-artifacts.yml"
  if [[ -f "$workflow" ]]; then
    local version
    version="$(sed -n 's/^[[:space:]]*PICOTADO_VERSION:[[:space:]]*"\([^"]*\)".*/\1/p' "$workflow" | head -n 1)"
    if [[ -n "$version" ]]; then
      printf '%s\n' "$version"
      return
    fi
  fi

  printf '0.1.3\n'
}

install_ubuntu_deps() {
  if ! command -v apt-get >/dev/null 2>&1; then
    echo "--install-deps currently supports apt-based Ubuntu systems only." >&2
    exit 1
  fi

  sudo apt-get update

  local webkit_dev="libwebkit2gtk-4.1-dev"
  if ! apt-cache show "$webkit_dev" >/dev/null 2>&1; then
    webkit_dev="libwebkit2gtk-4.0-dev"
  fi

  sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    curl \
    libasound2-dev \
    libjack-jackd2-dev \
    libcurl4-openssl-dev \
    libfreetype6-dev \
    libfontconfig1-dev \
    libx11-dev \
    libxcomposite-dev \
    libxcursor-dev \
    libxext-dev \
    libxinerama-dev \
    libxrandr-dev \
    libxrender-dev \
    libgl1-mesa-dev \
    "$webkit_dev" \
    xvfb
}

write_install_notes() {
  local version="$1"
  cat > "${DIST_DIR}/INSTALL-Linux.md" <<EOF
# Picotado Linux install

These artifacts are unsigned development builds.

## Runtime dependencies

Picotado uses a JUCE WebView UI. On Ubuntu or compatible apt-based distributions, install WebKitGTK runtime packages if the plugin opens with a blank GUI:

\`\`\`bash
sudo apt update
sudo apt install -y libwebkit2gtk-4.1-0 || sudo apt install -y libwebkit2gtk-4.0-37
\`\`\`

## VST3

Extract \`Picotado-v${version}-Linux-VST3.tar.gz\` and copy the whole \`Picotado.vst3\` bundle to \`~/.vst3/\`.

\`\`\`bash
mkdir -p ~/.vst3
tar -xzf Picotado-v${version}-Linux-VST3.tar.gz
rm -rf ~/.vst3/Picotado.vst3 ~/.vst3/Picotado.so
cp -R Picotado.vst3 ~/.vst3/
chmod +x ~/.vst3/Picotado.vst3/Contents/x86_64-linux/Picotado.so
\`\`\`

Do not copy a loose \`.so\` file for VST3. Linux VST3 must remain a bundle:

\`\`\`text
Picotado.vst3/Contents/x86_64-linux/Picotado.so
\`\`\`

## LV2

Extract \`Picotado-v${version}-Linux-LV2.tar.gz\` and copy the whole \`Picotado.lv2\` bundle to \`~/.lv2/\`.

\`\`\`bash
mkdir -p ~/.lv2
tar -xzf Picotado-v${version}-Linux-LV2.tar.gz
rm -rf ~/.lv2/Picotado.lv2
cp -R Picotado.lv2 ~/.lv2/
\`\`\`

After installing either format, rescan plugins in your DAW.
EOF
}

if [[ "$install_deps" == true ]]; then
  install_ubuntu_deps
fi

PICOTADO_VERSION="$(detect_version)"
export PICOTADO_VERSION

cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --target Picotado_VST3 --parallel
timeout 20m xvfb-run -a cmake --build "$BUILD_DIR" --target Picotado_LV2 --parallel
cmake --build "$BUILD_DIR" --target Picotado_Standalone --parallel

test -d "$VST3_BUNDLE"
test -f "$VST3_SO"
test -d "$LV2_BUNDLE"
test -f "$STANDALONE_EXE"

mkdir -p "$DIST_DIR"
tar -czf "${DIST_DIR}/Picotado-v${PICOTADO_VERSION}-Linux-VST3.tar.gz" -C "${ARTIFACT_ROOT}/VST3" "Picotado.vst3"
tar -czf "${DIST_DIR}/Picotado-v${PICOTADO_VERSION}-Linux-LV2.tar.gz" -C "${ARTIFACT_ROOT}/LV2" "Picotado.lv2"
write_install_notes "$PICOTADO_VERSION"

echo
echo "Created:"
ls -la "$DIST_DIR"
echo
echo "Local install commands:"
echo "  scripts/install-vst3-linux.sh"
echo "  scripts/install-lv2-linux.sh"
