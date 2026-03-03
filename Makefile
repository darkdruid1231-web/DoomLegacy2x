# Doom Legacy CI/CD Pipeline Makefile
# Provides quick local feedback and unified commands for CI

.PHONY: help build test clean docker-build docker-test ci-linux ci-windows ci-clang all

# Default target
help:
	@echo "Doom Legacy CI/CD Pipeline"
	@echo ""
	@echo "Local Development Targets:"
	@echo "  build          Build for Linux (default target)"
	@echo "  build-debug    Build for Linux with debug symbols"
	@echo "  build-windows  Cross-compile for Windows"
	@echo "  build-clang    Build with Clang compiler"
	@echo "  test           Run all tests"
	@echo "  test-quick     Run only quick unit tests"
	@echo "  clean          Clean all build artifacts"
	@echo ""
	@echo "Container Targets:"
	@echo "  docker-build   Build using Docker container"
	@echo "  docker-test    Run tests using Docker container"
	@echo ""
	@echo "CI Targets (used by GitHub Actions):"
	@echo "  ci-linux       Full Linux CI pipeline"
	@echo "  ci-windows     Full Windows CI pipeline"
	@echo "  ci-clang       Full Clang CI pipeline"
	@echo ""
	@echo "Utility Targets:"
	@echo "  all            Build and test everything"
	@echo "  deps           Install local dependencies (Ubuntu/Debian)"
	@echo "  format         Format code using clang-format"
	@echo "  lint           Run clang-tidy checks"

# Local build targets
build:
	./build.sh -t linux -b Release

build-debug:
	./build.sh -t linux -b Debug

build-windows:
	./build.sh -t windows -b Release

build-clang:
	./build.sh -t clang -b Release

# Local test targets
test: build
	./test.sh

test-quick: build
	./test.sh -q

# Clean targets
clean:
	./build.sh --clean -t linux
	./build.sh --clean -t windows
	./build.sh --clean -t clang

# Docker targets
docker-build:
	./build.sh --container -t linux -b Release

docker-test: docker-build
	./test.sh

# CI targets (used by GitHub Actions)
ci-linux:
	@echo "=== Linux CI Pipeline ==="
	./build.sh -t linux -b Release --clean --build build-linux
	./test.sh -t linux --build build-linux

ci-windows:
	@echo "=== Windows CI Pipeline ==="
	./build.sh -t windows -b Release --clean --build build-windows
	./test.sh -t windows --build build-windows

ci-clang:
	@echo "=== Clang CI Pipeline ==="
	./build.sh -t clang -b Release --clean --build build-clang
	./test.sh -t clang --build build-clang

# Install local dependencies (Ubuntu/Debian)
deps:
	@echo "Installing local dependencies..."
	sudo apt-get update
	sudo apt-get install -y \
		cmake \
		g++ \
		clang \
		clang-tidy \
		clang-format \
		libsdl1.2-dev \
		libsdl-mixer1.2-dev \
		libpng-dev \
		libjpeg-dev \
		libenet-dev \
		libglu1-mesa-dev \
		flex \
		lemon \
		cppcheck
	# Create SDL symlink if missing
	sudo ln -sf /usr/include/SDL /usr/include/SDL || true
	@echo "Dependencies installed."

# Code formatting
format:
	@echo "Formatting code..."
	find legacy/trunk -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Code linting
lint: build-debug
	@echo "Running clang-tidy..."
	# Generate compile_commands.json for clang-tidy
	cd legacy/trunk/build-linux && cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	# Run clang-tidy on key files
	clang-tidy legacy/trunk/engine/g_state.cpp legacy/trunk/net/n_connection.cpp -p legacy/trunk/build-linux

# Build and test everything
all: build test build-windows build-clang
	@echo "All builds completed successfully!"