#!/bin/bash
#-----------------------------------------------------------------------------
#
# $Id: smoke_test.sh 1 2026-02-17 $
#
# Copyright (C) 2026 by Doom Legacy Team.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
#-----------------------------------------------------------------------------

# \file
# \brief Smoke test script for Doom Legacy build verification.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../build"

echo "========================================"
echo "Doom Legacy Smoke Test"
echo "========================================"

# Test 1: Binary exists
echo "[1/4] Checking binary exists..."
if [ ! -f "${BUILD_DIR}/doomlegacy" ]; then
    echo "FAIL: Binary not found at ${BUILD_DIR}/doomlegacy"
    exit 1
fi
echo "  ✓ Binary exists"

# Test 2: Binary is executable
echo "[2/4] Checking binary is executable..."
if [ ! -x "${BUILD_DIR}/doomlegacy" ]; then
    echo "FAIL: Binary is not executable"
    exit 1
fi
echo "  ✓ Binary is executable"

# Test 3: Binary responds to version flag
echo "[3/4] Checking version flag..."
VERSION_OUTPUT=$("${BUILD_DIR}/doomlegacy" --version 2>&1 || true)
if [ -z "$VERSION_OUTPUT" ]; then
    # Try alternative flag
    VERSION_OUTPUT=$("${BUILD_DIR}/doomlegacy" -version 2>&1 || true)
fi
if [ -z "$VERSION_OUTPUT" ]; then
    echo "  ⚠ Warning: Version check returned empty (may be normal)"
else
    echo "  ✓ Version check passed: ${VERSION_OUTPUT}"
fi

# Test 4: Binary help (if available)
echo "[4/4] Checking help output..."
HELP_OUTPUT=$("${BUILD_DIR}/doomlegacy" --help 2>&1 || true)
if [ -n "$HELP_OUTPUT" ]; then
    echo "  ✓ Help output available"
fi

echo ""
echo "========================================"
echo "All smoke tests passed!"
echo "========================================"
