#!/bin/bash
#-----------------------------------------------------------------------------
#
# $Id$
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

/// \file
/// \brief Smoke test script for Doom Legacy build verification.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../build"

echo "========================================"
echo "Doom Legacy Smoke Test"
echo "========================================"

# Test 1: Binary exists
echo "[1/3] Checking binary exists..."
if [ ! -f "${BUILD_DIR}/doomlegacy" ]; then
    echo "FAIL: Binary not found at ${BUILD_DIR}/doomlegacy"
    exit 1
fi
echo "  ✓ Binary exists"

# Test 2: Binary responds to --help flag (validates startup)
echo "[2/3] Checking --help flag..."
if "${BUILD_DIR}/doomlegacy" --help > /dev/null 2>&1; then
    echo "  ✓ Help flag accepted"
else
    echo "FAIL: Binary failed to respond to --help"
    exit 1
fi

# Test 3: Check exit code
echo "[3/3] Checking exit code..."
EXIT_CODE=0
"${BUILD_DIR}/doomlegacy" --help > /dev/null 2>&1 || EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ]; then
    echo "  ✓ Exit code is 0"
else
    echo "FAIL: Binary exited with code $EXIT_CODE"
    exit 1
fi

echo ""
echo "========================================"
echo "All smoke tests passed!"
echo "========================================"
