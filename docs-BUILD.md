# Build Guide

Instructions for building and running the Hyperlocal Weather application on Ubuntu 24.04 WSL.

## Build Instructions

### 1. Install Dependencies (Ubuntu 24.04 WSL)

First, ensure all dependencies are installed:

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    qt6-base-dev \
    qt6-declarative-dev \
    qt6-tools-dev \
    qml6-module-qtquick \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts \
    qml6-module-qtquick-window \
    libqt6sql6-sqlite \
    libsqlite3-dev \
    libgtest-dev \
    pkg-config
```

### 2. Create Build Directory

```bash
cd CSCE-120-Group-Project
mkdir build && cd build
```

### 3. Configure with CMake

For Ubuntu 24.04 with system Qt6 packages:
```bash
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
```

If CMake cannot find Qt6, specify the path:
```bash
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6 ..
```

For Debug build:
```bash
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
```

### 4. Build

```bash
ninja
# or
cmake --build . --parallel $(nproc)
```

### 5. Run

For WSL with X server (VcXsrv or X410):
```bash
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0.0
./HyperlocalWeather
```

For headless testing (no GUI):
```bash
export QT_QPA_PLATFORM=offscreen
./HyperlocalWeather
```

## Running Tests

### Build Tests

Tests are built automatically with the main application when you configure CMake.

### Run All Tests

```bash
cd build
ctest --output-on-failure -V
```

Note: Some integration tests require network connectivity and may be skipped if offline.

### Run Specific Test Suites

```bash
# Run unit tests only
./tests/HyperlocalWeatherTests --gtest_filter="*Unit*"

# Run integration tests only
./tests/HyperlocalWeatherTests --gtest_filter="*Integration*"

# Run specific test
./tests/HyperlocalWeatherTests --gtest_filter="WeatherServiceTest.*"
```

### Generate Test Coverage Report

Install coverage tools:
```bash
sudo apt install -y gcovr lcov
```

Build with coverage:
```bash
mkdir build-coverage && cd build-coverage
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
ninja
ctest --output-on-failure
ninja coverage
```

View coverage report:
```bash
# HTML report will be in build-coverage/coverage/index.html
# Or view in terminal:
lcov --summary coverage.info
```

## Installation

### Install to System

```bash
cd build
sudo ninja install
```

Default installation paths:
- Binary: `/usr/local/bin/HyperlocalWeather`
- Resources: `/usr/local/share/HyperlocalWeather`

### Custom Installation Path

```bash
cmake -G Ninja -DCMAKE_INSTALL_PREFIX=/opt/hyperlocal-weather ..
ninja
sudo ninja install
```

## Development Workflow

### Quick Rebuild

```bash
cd build
ninja
```

### Clean Build

```bash
cd build
ninja clean
ninja
```

### Complete Rebuild

```bash
rm -rf build
mkdir build && cd build
cmake -G Ninja ..
ninja
```

## Qt Creator Integration

### Open Project in Qt Creator

1. Open Qt Creator
2. File → Open File or Project
3. Select `CMakeLists.txt` from project root
4. Configure Kit (select Qt 6.x Desktop kit)
5. Build → Run CMake
6. Build → Build Project

### Run from Qt Creator

1. Projects → Run Settings
2. Set working directory to project root
3. Click Run button (Ctrl+R)

### Debug from Qt Creator

1. Set breakpoints in code
2. Debug → Start Debugging (F5)

## Troubleshooting

### CMake can't find Qt6

On Ubuntu 24.04, Qt6 is typically installed in `/usr/lib/x86_64-linux-gnu/cmake/Qt6`. Try:

```bash
cmake -G Ninja -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6 ..
```

Or verify Qt6 installation:
```bash
dpkg -l | grep qt6
pkg-config --modversion Qt6Core
```

### Linker errors with Qt libraries

```bash
export LD_LIBRARY_PATH=$Qt6_DIR/lib:$LD_LIBRARY_PATH
```

### QML module not found

Install missing QML modules:
```bash
sudo apt install -y \
    qml6-module-qtquick \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts \
    qml6-module-qtquick-window
```

Verify QML modules are installed:
```bash
ls /usr/lib/x86_64-linux-gnu/qml/
```

### Google Test not found

CMake will try to download it automatically via FetchContent. If it fails:

```bash
sudo apt install -y libgtest-dev libgmock-dev
```

Or let CMake download it (default behavior).

### Build fails on WSL

**GUI Issues:**
- Install X server on Windows (VcXsrv or X410)
- Set DISPLAY variable:
  ```bash
  export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0.0
  ```

**Headless Testing:**
```bash
export QT_QPA_PLATFORM=offscreen
```

**SQLite Issues:**
```bash
sudo apt install -y libsqlite3-dev
```

**Missing Development Headers:**
```bash
sudo apt install -y qt6-base-dev qt6-declarative-dev
```

### Performance Issues

Use parallel builds:
```bash
ninja -j$(nproc)
# or
cmake --build . -j$(nproc)
```

## Build Options

Available CMake options:

```bash
# Enable tests (default: ON)
cmake -DBUILD_TESTS=ON ..

# Enable coverage (default: OFF)
cmake -DENABLE_COVERAGE=ON ..

# Set build type
cmake -DCMAKE_BUILD_TYPE=Debug ..    # Debug build
cmake -DCMAKE_BUILD_TYPE=Release ..  # Optimized build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..  # Release with debug info

# Verbose build
cmake -DCMAKE_VERBOSE_MAKEFILE=ON ..
```

## Performance Benchmarks

After building, run performance tests:

```bash
./tests/HyperlocalWeatherTests --gtest_filter="*Performance*"
```

Expected metrics:
- Forecast API latency: < 10 seconds (p95)
- Cache lookup: < 100ms
- UI render time: < 16ms (60 FPS)
- Database query: < 50ms

## Continuous Integration

### GitHub Actions Example

Create `.github/workflows/build.yml`:

```yaml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-24.04
    steps:
    - uses: actions/checkout@v3
    - name: Install dependencies
      run: |
        sudo apt update
        sudo apt install -y \
          build-essential \
          cmake \
          ninja-build \
          qt6-base-dev \
          qt6-declarative-dev \
          qt6-tools-dev \
          qml6-module-qtquick \
          qml6-module-qtquick-controls \
          qml6-module-qtquick-layouts \
          libqt6sql6-sqlite \
          libsqlite3-dev \
          libgtest-dev \
          pkg-config
    - name: Build
      run: |
        mkdir build && cd build
        cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
        ninja
    - name: Test
      run: |
        cd build
        export QT_QPA_PLATFORM=offscreen
        ctest --output-on-failure -V
```

## Next Steps

See [ARCHITECTURE.md](ARCHITECTURE.md) for system design details.
