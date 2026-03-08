# Doom Legacy

A modern, enhanced port of the classic Doom engine with advanced features, automated testing, and cross-platform support.

## Features

### Core Engine Enhancements
- **Dynamic GL Node Generation**: Automatically generates OpenGL-optimized BSP data for enhanced rendering performance
- **Enhanced Testability**: Comprehensive testing framework with performance monitoring and visual regression detection
- **Cross-Platform Support**: Runs on Linux, Windows, and macOS with consistent behavior
- **Object-Oriented Architecture**: Modern C++ design with proper encapsulation and memory management

### Rendering & Graphics
- **OpenGL Support**: Hardware-accelerated rendering with dynamic node generation
- **Software Rendering**: Classic software renderer for compatibility
- **Enhanced Assets**: Improved textures, sprites, and UI elements via legacy.wad
- **Visual Quality**: Better lighting, textures, and particle effects

### Testing & Quality Assurance
- **Automated E2E Testing**: Map loading, actor spawning, demo playback verification
- **Performance Monitoring**: FPS tracking, memory usage, load time measurement
- **Visual Regression Testing**: Screenshot comparison with perceptual hashing
- **CI/CD Pipeline**: Automated builds and tests on every commit

## Building

### Prerequisites
- **CMake** (3.16+)
- **C++ Compiler** (GCC 9+, Clang 10+, MSVC 2019+)
- **SDL2 Development Libraries**
- **Git**

### Quick Start

#### Linux
```bash
# Install dependencies
sudo apt-get update
sudo apt-get install build-essential cmake libsdl2-dev git

# Clone and build
git clone <repository-url>
cd doomlegacy-legacy2/legacy/trunk
./build.sh
```

#### Windows
```cmd
REM Install Visual Studio 2022 with C++ workload
REM Install CMake and SDL2

git clone <repository-url>
cd doomlegacy-legacy2\legacy\trunk
build.bat
```

#### macOS
```bash
# Install Xcode and Homebrew
brew install cmake sdl2

git clone <repository-url>
cd doomlegacy-legacy2/legacy/trunk
mkdir build && cd build
cmake ..
make
```

## Running

### Basic Usage
```bash
# Software rendering
./doomlegacy -iwad doom1.wad

# OpenGL rendering
./doomlegacy -opengl -iwad doom1.wad

# Test mode (for development)
./doomlegacy -testmode -iwad doom1.wad -warp 1 1
```

### Command Line Options
- `-iwad <file>`: Specify IWAD file (doom1.wad, doom.wad, etc.)
- `-opengl`: Enable OpenGL rendering
- `-testmode`: Enable test features and verbose logging
- `-warp <episode> <level>`: Jump directly to a level
- `-playdemo <demo>`: Play back a demo file

## Testing

### Automated Testing
The project includes a comprehensive testing framework:

```bash
# Run all tests
python3 test/test_runner.py

# Run with visual validation
python3 test/test_runner.py --visual

# Run performance tests
python3 test/test_runner.py --performance
```

### Test Coverage
- **Map Loading**: BSP verification, geometry validation
- **Actor Systems**: Monster spawning, AI behavior
- **Rendering**: OpenGL and software mode compatibility
- **Performance**: FPS, memory usage, load times
- **Visual Regression**: Screenshot comparison with baselines

### CI/CD
Automated testing runs on every push via GitHub Actions:
- Multi-platform builds (Linux/Windows/macOS)
- Full test suite execution
- Performance regression detection
- Visual diff reporting

## Development

### Architecture
- **Object-Oriented Design**: Map, Actor, and rendering classes
- **Modular Components**: Separate modules for input, audio, video
- **Memory Management**: Zone-based allocation system
- **Extensibility**: Plugin architecture for custom features

### Key Components
- `engine/`: Core game logic and systems
- `interface/`: SDL input/output handling
- `video/`: Rendering pipeline and graphics
- `test/`: Testing framework and utilities

### Contributing
1. Fork the repository
2. Create a feature branch
3. Make your changes with tests
4. Run the full test suite
5. Submit a pull request

## Requirements

### System Requirements
- **OS**: Linux, Windows 10+, macOS 10.14+
- **CPU**: x86_64 compatible
- **RAM**: 256MB minimum, 512MB recommended
- **GPU**: OpenGL 2.0+ support (optional)

### Game Data
Requires original Doom WAD files:
- **Shareware**: `doom1.wad` (free)
- **Registered**: `doom.wad`
- **Ultimate Doom**: `doomu.wad`

## License

This project is based on the Doom Legacy engine and includes enhancements under compatible open-source licenses.

## Credits

- Original Doom engine by id Software
- Doom Legacy development team
- Enhanced features and testing framework by current maintainers

## Support

- **Issues**: Report bugs on GitHub
- **Discussions**: Join the community Discord
- **Documentation**: See `docs/` directory for detailed guides

---

*Built with modern C++ and SDL2 for cross-platform compatibility*