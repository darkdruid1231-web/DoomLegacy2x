#!/bin/bash
# Unified build script for Doom Legacy
# Works locally and in CI, with optional container support

set -e

# Default values
TARGET="${TARGET:-linux}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
USE_CONTAINER="${USE_CONTAINER:-false}"
CONTAINER_IMAGE="${CONTAINER_IMAGE:-doomlegacy-${TARGET}}"
WORKSPACE_DIR="${WORKSPACE_DIR:-$(pwd)}"
SOURCE_DIR="${SOURCE_DIR:-legacy/trunk}"
BUILD_DIR="${BUILD_DIR:-build-${TARGET}}"
CLEAN_BUILD="${CLEAN_BUILD:-false}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
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

usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -t, --target TARGET       Build target: linux, windows, clang (default: linux)"
    echo "  -b, --build-type TYPE     Build type: Debug, Release (default: Release)"
    echo "  -c, --container           Use Docker container for build"
    echo "  --clean                   Clean build directory before building"
    echo "  --image IMAGE             Docker image to use (default: doomlegacy-{target})"
    echo "  --workspace DIR           Workspace directory (default: current dir)"
    echo "  --source DIR              Source directory relative to workspace (default: legacy/trunk)"
    echo "  --build DIR               Build directory relative to source (default: build-{target})"
    echo "  -h, --help                Show this help"
    echo ""
    echo "Examples:"
    echo "  $0                        # Local Linux build"
    echo "  $0 -t windows -c          # Windows cross-compile in container"
    echo "  $0 --clean -b Debug       # Clean debug build"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--target)
            TARGET="$2"
            shift 2
            ;;
        -b|--build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -c|--container)
            USE_CONTAINER=true
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --image)
            CONTAINER_IMAGE="$2"
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
        --build)
            BUILD_DIR="$2"
            shift 2
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

# Validate target
case $TARGET in
    linux|windows|clang)
        ;;
    *)
        log_error "Invalid target: $TARGET"
        exit 1
        ;;
esac

# Function to run build in container
run_in_container() {
    local image="$1"
    local workspace_mount="$2"

    log_info "Running build in container: $image"

    # Build container if it doesn't exist
    if ! docker image inspect "$image" >/dev/null 2>&1; then
        log_info "Building container image: $image"
        docker build -f "Dockerfile.${TARGET}" -t "$image" .
    fi

    # Run build in container
    docker run --rm \
        -v "$workspace_mount:/workspace" \
        -w /workspace \
        -e TARGET="$TARGET" \
        -e BUILD_TYPE="$BUILD_TYPE" \
        -e CLEAN_BUILD="$CLEAN_BUILD" \
        "$image" \
        bash -c "cd $SOURCE_DIR && ../build.sh --workspace /workspace --source $SOURCE_DIR --build $BUILD_DIR"
}

# Function to perform the actual build
do_build() {
    local src_dir="$1"
    local build_dir="$2"

    log_info "Building Doom Legacy"
    log_info "Target: $TARGET"
    log_info "Build type: $BUILD_TYPE"
    log_info "Source dir: $src_dir"
    log_info "Build dir: $build_dir"

    # Clean build directory if requested
    if [[ "$CLEAN_BUILD" == "true" ]]; then
        log_info "Cleaning build directory: $build_dir"
        rm -rf "$build_dir"
    fi

    # Create build directory
    mkdir -p "$build_dir"
    cd "$build_dir" || exit 1

    # Configure with CMake based on target
    case $TARGET in
        linux)
            log_info "Configuring for Linux build"
            cmake .. \
                -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
                -DUSE_MODERN_NETWORKING=OFF \
                -DUSE_CDROM=OFF \
                -DUSE_SOUND=ON \
                -DUSE_SDL2=OFF \
                -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
            ;;
        windows)
            log_info "Configuring for Windows cross-compilation"
            cmake .. \
                -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64-x86_64.cmake \
                -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
                -DUSE_SDL2=ON \
                -DUSE_SOUND=ON \
                -DUSE_IMAGE_SUPPORT=OFF \
                -DUSE_NETWORKING=ON
            ;;
        clang)
            log_info "Configuring for Clang build"
            export CC=clang
            export CXX=clang++
            cmake .. \
                -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
                -DUSE_MODERN_NETWORKING=OFF \
                -DUSE_CDROM=OFF \
                -DUSE_SOUND=ON \
                -DUSE_SDL2=OFF \
                -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
            ;;
    esac

    # Build
    log_info "Building..."
    make -j$(nproc) doomlegacy d2h wadtool test_fixed_t test_vector test_console test_actor test_save_load_unit

    log_info "Build completed successfully"
}

# Main logic
if [[ "$USE_CONTAINER" == "true" ]]; then
    # Check if Docker is available
    if ! command -v docker >/dev/null 2>&1; then
        log_error "Docker is not available. Install Docker or run without --container"
        exit 1
    fi

    run_in_container "$CONTAINER_IMAGE" "$WORKSPACE_DIR"
else
    # Local build
    cd "$WORKSPACE_DIR" || exit 1
    if [[ ! -d "$SOURCE_DIR" ]]; then
        log_error "Source directory not found: $SOURCE_DIR"
        exit 1
    fi

    do_build "$SOURCE_DIR" "$SOURCE_DIR/$BUILD_DIR"
fi