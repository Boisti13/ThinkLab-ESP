#!/usr/bin/env bash
# Builds and runs the fan controller simulation: tools/fansim/build.sh
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
flags=$(sed -n 's/^ *-D\(FC_PT[0-9]_[CP]\|FAN_[A-Z_]*\|FAN1_MIN_DUTY_PCT\)=\([^ ;]*\).*/-D\1=\2/p' "$repo/platformio.ini" | tr '\n' ' ')
g++ -std=c++17 -O1 -w -DARDUINO=10819 -DUSE_FAN1=1 -DUSE_FANCTRL=1 $flags \
  -I "$repo/tools/render/shim" -I "$repo/src" \
  "$here/fansim.cpp" "$repo/src/modules/fanctrl.cpp" -o "${TMPDIR:-/tmp}/fansim"
"${TMPDIR:-/tmp}/fansim"
