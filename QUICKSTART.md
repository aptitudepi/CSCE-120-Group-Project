# Quick Start Guide

Get the Hyperlocal Weather application up and running quickly on Ubuntu 24.04 WSL.

## Prerequisites

- Ubuntu 24.04 WSL
- Internet connection
- X server (VcXsrv or X410) for GUI, or use headless mode for testing

## Quick Setup (5 minutes)

### 1. Install Dependencies

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

### 2. Build the Application

```bash
cd CSCE-120-Group-Project
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

### 3. Run the Application

**With GUI (requires X server):**
```bash
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0.0
./HyperlocalWeather
```

**Headless (for testing):**
```bash
export QT_QPA_PLATFORM=offscreen
./HyperlocalWeather
```

### 4. Run Tests

```bash
cd build
ctest --output-on-failure -V
```

## Using the Application

1. **Enter Location**: Input latitude and longitude (e.g., 30.6272, -96.3344 for College Station, TX)
2. **Get Forecast**: Click "Get Forecast" to fetch weather data
3. **View Forecast**: Scroll through the forecast list to see upcoming weather
4. **Refresh**: Click "Refresh" to update the forecast

## Optional: Set API Keys

For enhanced features with Pirate Weather:

```bash
export PIRATE_WEATHER_API_KEY="your_api_key_here"
```

Add to `~/.bashrc` for persistence.

## Troubleshooting

**CMake can't find Qt6:**
```bash
cmake -G Ninja -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6 ..
```

**GUI not displaying:**
- Install X server (VcXsrv or X410) on Windows
- Set DISPLAY variable (see step 3 above)

**Build errors:**
- Ensure all dependencies are installed (step 1)
- Check that CMake version is 3.21 or higher: `cmake --version`

## Next Steps

- See [docs/BUILD.md](docs/BUILD.md) for detailed build instructions
- See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for system architecture
- See [docs/SETUP.md](docs/SETUP.md) for environment setup details

## Success Metrics

The application targets:
- **Latency**: < 10 seconds for forecast delivery
- **Accuracy**: > 75% precipitation hit rate
- **Uptime**: > 95% service availability
- **Test Coverage**: > 80% on critical backend modules

