#!/usr/bin/env bash
# Build legacy.wad from source assets and tools
# Usage: build_wad.sh [output_path]
# Default output: src/resources/legacy.wad

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOOLS_SRC="$SCRIPT_DIR/../common/tools"
INCLUDE_DIR="$SCRIPT_DIR/include"
MD5_SRC="$SCRIPT_DIR/util/md5.cpp"
RESOURCES="$SCRIPT_DIR/resources"
INVENTORY="$(cd "$SCRIPT_DIR" && pwd)/resources/legacy.wad.inventory"
OUTPUT_WAD="${1:-}"
if [[ -z "$OUTPUT_WAD" ]]; then
    OUTPUT_WAD="$SCRIPT_DIR/resources/legacy.wad"
else
    # Resolve to absolute path
    OUTPUT_WAD="$(cd "$(dirname "$OUTPUT_WAD")" && pwd)/$(basename "$OUTPUT_WAD")"
fi
mkdir -p "$(dirname "$OUTPUT_WAD")"
STAGE_DIR="$(mktemp -d)"

# Windows (MSYS2) needs .exe suffix; Linux does not
if [[ "${MSYSTEM:-}" != "" ]] || [[ "${OS:-}" == "Windows_NT" ]]; then
    EXT=".exe"
else
    EXT=""
fi

WADTOOL="$STAGE_DIR/wadtool$EXT"
D2H="$STAGE_DIR/d2h$EXT"

echo "[build_wad] Compiling tools..."
g++ -O2 -I"$INCLUDE_DIR" -o "$WADTOOL" "$TOOLS_SRC/wadtool.cpp" "$MD5_SRC"
g++ -O2 -I"$INCLUDE_DIR" -o "$D2H"     "$TOOLS_SRC/d2h.cpp"

echo "[build_wad] Staging resources..."
cp "$RESOURCES"/*.txt  "$STAGE_DIR/"
cp "$RESOURCES"/*.png  "$STAGE_DIR/"
cp "$RESOURCES"/*.h    "$STAGE_DIR/"
cp "$RESOURCES"/*.lmp  "$STAGE_DIR/"
cp "$RESOURCES"/*.ttf  "$STAGE_DIR/"

echo "[build_wad] Generating conversion table lumps..."
cd "$STAGE_DIR"
"$D2H" doom2hexen.txt    XDOOM.lmp
"$D2H" heretic2hexen.txt XHERETIC.lmp

echo "[build_wad] Packing legacy.wad..."
"$WADTOOL" -c "$OUTPUT_WAD" "$INVENTORY"

echo "[build_wad] Cleaning up..."
rm -rf "$STAGE_DIR"

echo "[build_wad] Done -> $OUTPUT_WAD"
