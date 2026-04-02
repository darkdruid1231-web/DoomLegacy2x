# Doom Legacy Test Suite

This directory contains the test suite for Doom Legacy.

## Structure

- `unit/` - Unit tests for core types (fixed_t, packets)
- `integration/` - Integration tests (save/load, map parsing)
- `demos/` - Demo recording and checksum files
- `scripts/` - Helper scripts for testing

## Running Tests

### CMake build:
```bash
mkdir build && cd build
cmake ..
make
./run_tests
```

### Makefile:
```bash
make test
```

## Test Coverage Goals

| Area | Target |
|------|--------|
| fixed_t operations | 100% |
| Save/Load | 80% |
| Map parsing | 70% |
| Network packets | 60% |
| Demo recording | 50% |
