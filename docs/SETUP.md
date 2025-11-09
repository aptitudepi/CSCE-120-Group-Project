# Setup Guide

This guide covers setting up the development environment for the Hyperlocal Weather application on Ubuntu 24.04 WSL.

## Prerequisites

### Update System
```bash
sudo apt update && sudo apt upgrade -y
```

### Install Build Tools
```bash
sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    pkg-config
```

### Install Qt6

#### Option 1: From Ubuntu Repositories (Recommended for Ubuntu 24.04)
```bash
sudo apt install -y \
    qt6-base-dev \
    qt6-declarative-dev \
    qt6-tools-dev \
    qml6-module-qtquick \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts \
    qml6-module-qtquick-window \
    libqt6sql6-sqlite
```

#### Option 2: Qt Online Installer (For Qt 6.5+)

1. Download the Qt Online Installer:
```bash
cd ~/Downloads
wget https://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run
chmod +x qt-unified-linux-x64-online.run
```

2. Install required dependencies:
```bash
sudo apt install -y libgl1-mesa-dev libxkbcommon-x11-0 libxcb-cursor0
```

3. Run the installer:
```bash
./qt-unified-linux-x64-online.run
```

4. In the installer:
   - Select Qt 6.7 or higher
   - Choose components: Qt Quick, Qt Network, Qt SQL, Qt Positioning
   - Installation directory: `/opt/Qt` or `~/Qt`

5. Set environment variables (add to `~/.bashrc`):
```bash
export Qt6_DIR=~/Qt/6.7.0/gcc_64
export PATH=$Qt6_DIR/bin:$PATH
export LD_LIBRARY_PATH=$Qt6_DIR/lib:$LD_LIBRARY_PATH
```

Reload shell configuration:
```bash
source ~/.bashrc
```

### Install SQLite3
```bash
sudo apt install -y sqlite3 libsqlite3-dev
```

### Install Google Test

```bash
sudo apt install -y libgtest-dev libgmock-dev
```

Alternatively, Google Test will be downloaded automatically during CMake configuration if not found.

## Clone the Repository

```bash
git clone <repository-url>
cd CSCE-120-Group-Project
```

## Verify Installation

Check Qt installation:
```bash
qmake6 --version
# or
qt-cmake --version
```

Check CMake:
```bash
cmake --version
```

Should output CMake 3.21 or higher.

Check Qt6 packages:
```bash
dpkg -l | grep qt6
pkg-config --modversion Qt6Core
```

## WSL-Specific Configuration

### X Server for GUI (if running GUI)

If you want to test the GUI on WSL, install an X server on Windows:
1. Download VcXsrv or X410
2. Start the X server
3. Set DISPLAY variable:
```bash
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0.0
```

Add to `~/.bashrc` for persistence:
```bash
echo 'export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '\''{print $2}'\''):0.0' >> ~/.bashrc
```

### Alternative: Headless Testing
For WSL, you can run tests without GUI:
```bash
export QT_QPA_PLATFORM=offscreen
```

## API Keys (Optional)

Create a `.env` file or configure environment variables:

```bash
export PIRATE_WEATHER_API_KEY="your_api_key_here"
# Add other API keys as needed
```

Add to `~/.bashrc` for persistence.

## Troubleshooting

### Qt Not Found
If CMake can't find Qt6:
```bash
export CMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6
cmake -G Ninja -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6 ..
```

### OpenGL Issues
```bash
sudo apt install -y libgl1-mesa-glx libgl1-mesa-dev
```

### Permission Issues
Ensure your user has permissions:
```bash
sudo usermod -a -G dialout $USER
# Log out and log back in
```

### SQLite Issues
```bash
sudo apt install -y libsqlite3-dev
```

### Missing QML Modules
```bash
sudo apt install -y \
    qml6-module-qtquick \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts \
    qml6-module-qtquick-window
```

## Next Steps

Proceed to [BUILD.md](BUILD.md) to build the application.

