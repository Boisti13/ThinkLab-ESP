#!/usr/bin/env bash
# Renders the e-paper pages on a PC with made-up data:
#   tools/render/build.sh [out-dir]   (default docs/screenshots)
# Compiles the page code from src/ against small stand-ins for Arduino,
# WiFi and GxEPD2 (shim/) plus the real Adafruit GFX library and fonts.
# Needs g++, git and ImageMagick.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
out="${1:-$repo/docs/screenshots}"
gfx_tag="1.13.0"   # same as the firmware's lib_deps resolve to
cache="${XDG_CACHE_HOME:-$HOME/.cache}/thinklab-render"
gfx="$cache/Adafruit-GFX-Library-$gfx_tag"
build="$cache/build"

if [[ ! -d "$gfx" ]]; then
  git -c advice.detachedHead=false clone -q --depth 1 --branch "$gfx_tag" https://github.com/adafruit/Adafruit-GFX-Library "$gfx"
fi
mkdir -p "$build" "$out"

g++ -std=c++17 -O1 -w -DARDUINO=10819 \
  -DUSE_WIFI=1 -DUSE_DALLAS=1 -DUSE_FAN1=1 -DUSE_FAN2_TACH=1 \
  -DFW_VERSION="\"$(sed -n 's/.*-DFW_VERSION=\\"\([^\\]*\)\\".*/\1/p' "$repo/platformio.ini")\"" \
  -I "$here/shim" -I "$repo/src" -I "$gfx" \
  "$here/render.cpp" "$gfx/Adafruit_GFX.cpp" \
  "$repo/src/ui_theme.cpp" "$repo"/src/pages/page_*.cpp \
  -o "$build/render"

"$build/render" "$build"
magick_cmd=$(command -v magick || command -v convert)
for f in "$build"/eink-*.pbm; do
  name="$(basename "$f" .pbm)"
  # 2x, nearest neighbour, on a pale e-paper grey with a thin frame
  "$magick_cmd" "$f" -scale 200% -fill '#f2f0e8' -opaque white \
    -bordercolor '#f2f0e8' -border 12 -bordercolor '#9a9a9a' -border 1 "$out/$name.png"
  echo "$out/$name.png"
done
