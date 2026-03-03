#!/bin/bash
# Unified test script for Doom Legacy
# Works locally and in CI

set -e

# Default values
TARGET="${TARGET:-linux}"
BUILD_DIR="${BUILD_DIR:-build-${TARGET}}"
WORKSPACE_DIR="${WORKSPACE_DIR:-$(pwd)}"
SOURCE_DIR="${SOURCE_DIR:-legacy/trunk}"
VERBOSE="${VERBOSE:-false}"
QUICK="${QUICK:-false}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_test() {
    echo -e "${BLUE}[TEST]${NC} $1"
}

usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -t, --target TARGET       Build target: linux, windows, clang (default: linux)"
    echo "  --build DIR               Build directory relative to source (default: build-{target})"
    echo "  --workspace DIR           Workspace directory (default: current dir)"
    echo "  --source DIR              Source directory relative to workspace (default: legacy/trunk)"
    echo "  -v, --verbose             Verbose output"
    echo "  -q, --quick               Run only quick tests (skip integration tests)"
    echo "  -h, --help                Show this help"
    echo ""
    echo "Examples:"
    echo "  $0                        # Run all tests for Linux build"
    echo "  $0 -t windows -q          # Run quick tests for Windows build"
    echo "  $0 --verbose              # Run tests with verbose output"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--target)
            TARGET="$2"
            shift 2
            ;;
        --build)
            BUILD_DIR="$2"
            shift 2
            ;;
        --workspace)
            WORKSPACE_DIR="$2"
            shift 2
            ;;
        --source)
            SOURCE_DIR="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -q|--quick)
            QUICK=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Function to run a test
run_test() {
    local test_name="$1"
    local test_cmd="$2"
    local description="$3"

    log_test "Running $description..."

    if [[ "$VERBOSE" == "true" ]]; then
        if eval "$test_cmd"; then
            log_info "$description PASSED"
            return 0
        else
            log_error "$description FAILED"
            return 1
        fi
    else
        if eval "$test_cmd" >/dev/null 2>&1; then
            log_info "$description PASSED"
            return 0
        else
            log_error "$description FAILED"
            return 1
        fi
    fi
}

# Function to run tests
run_tests() {
    local build_dir="$1"
    local failed_tests=()

    log_info "Running Doom Legacy Test Suite"
    log_info "Target: $TARGET"
    log_info "Build dir: $build_dir"
    echo

    # Check if build directory exists
    if [[ ! -d "$build_dir" ]]; then
        log_error "Build directory not found: $build_dir"
        log_error "Run build.sh first to build the project"
        exit 1
    fi

    cd "$build_dir"

    # Quick unit tests (always run)
    log_test "=== Unit Tests ==="

    # Fixed-point arithmetic tests
    if [[ -f "test_fixed_t" ]]; then
        if ! run_test "fixed_t" "./test_fixed_t" "Fixed-point arithmetic tests"; then
            failed_tests+=("fixed_t")
        fi
    else
        log_warn "test_fixed_t not found, skipping"
    fi

    # Vector math tests
    if [[ -f "test_vector" ]]; then
        if ! run_test "vector" "./test_vector" "Vector math tests"; then
            failed_tests+=("vector")
        fi
    else
        log_warn "test_vector not found, skipping"
    fi

    # Console tests
    if [[ -f "test_console" ]]; then
        if ! run_test "console" "./test_console" "Console output tests"; then
            failed_tests+=("console")
        fi
    else
        log_warn "test_console not found, skipping"
    fi

    # Actor system tests
    if [[ -f "test_actor" ]]; then
        if ! run_test "actor" "./test_actor" "Actor/mobj system tests"; then
            failed_tests+=("actor")
        fi
    else
        log_warn "test_actor not found, skipping"
    fi

    # Skip integration tests if quick mode
    if [[ "$QUICK" == "true" ]]; then
        log_info "Quick mode: skipping integration tests"
    else
        echo
        log_test "=== Integration Tests ==="

        # Save/load tests
        if [[ -f "test_save_load_unit" ]]; then
            if ! run_test "save_load" "./test_save_load_unit" "Save/load serialization tests"; then
                failed_tests+=("save_load")
            fi
        else
            log_warn "test_save_load_unit not found, skipping"
        fi

        # E2E tests (Phase 2: headless testing with visual regression)
        if [[ -f "../../../test_runner.py" ]]; then
            if ! run_test "e2e" "cd ../../.. && python3 test_runner.py" "End-to-end headless tests with visual regression"; then
                failed_tests+=("e2e")
            fi
        else
            log_warn "test_runner.py not found, skipping e2e tests"
        fi
    fi

    echo
    log_test "=== Test Results ==="

    if [[ ${#failed_tests[@]} -eq 0 ]]; then
        log_info "All tests PASSED!"
        return 0
    else
        log_error "Failed tests: ${failed_tests[*]}"
        return 1
    fi
}

# Main logic
cd "$WORKSPACE_DIR" || exit 1
if [[ ! -d "$SOURCE_DIR" ]]; then
    log_error "Source directory not found: $SOURCE_DIR"
    exit 1
fi

run_tests "$SOURCE_DIR/$BUILD_DIR"