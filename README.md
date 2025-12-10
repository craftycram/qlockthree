# QlockThree - WiFi and OTA Enabled

This PlatformIO project for ESP32-S3 includes WiFi connectivity and Over-The-Air (OTA) update capabilities.

## Features

- **WiFi Configuration Portal**: Easy WiFi setup through captive portal - no code changes needed
- **WiFi Connectivity**: Connects to your home WiFi network with persistent storage
- **OTA Updates**: Update firmware wirelessly without USB cable
- **Auto Updates**: Automatic firmware updates from GitHub releases or custom web server
- **Web Interface**: Monitor device status and manage updates via web browser
- **JSON API**: Programmatic access to device information and update status

## Setup Instructions

### 1. WiFi Configuration Options

You have two options for configuring WiFi credentials:

#### Option A: WiFi Configuration Portal (Recommended)

Leave WiFi credentials empty in the config file to enable the captive portal:

```bash
cp include/config.h.template include/config.h
# The template has empty WiFi credentials by default
```

When you flash the firmware:
1. Device will create a WiFi Access Point named `QlockThree-Setup`
2. Connect to this AP with password: `qlockthree123`
3. A captive portal will open automatically (or go to `http://192.168.4.1`)
4. Select your WiFi network and enter the password
5. WiFi credentials are saved to device memory permanently
6. Device restarts and connects to your WiFi network

#### Option B: Hard-coded Credentials

Edit `include/config.h` and set your WiFi details directly:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";        // Replace with your WiFi network name
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD"; // Replace with your WiFi password
```

**Note**: The `config.h` file is excluded from git to keep your WiFi credentials secure.

### 2. Optional: Configure OTA Security

For added security, you can uncomment and set an OTA password in `include/config.h`:

```cpp
const char* OTA_PASSWORD = "your_ota_password";
```

Then uncomment the corresponding line in `src/main.cpp` in the `setupOTA()` function:

```cpp
ArduinoOTA.setPassword(OTA_PASSWORD);
```

### 3. Build and Upload

1. First upload via USB cable:
   ```bash
   pio run --target upload
   ```

2. Open Serial Monitor to see the device IP address:
   ```bash
   pio device monitor
   ```

## Usage

### Web Interface

Once connected to WiFi, you can access the device via web browser:

- **Status Page**: `http://qlockthree.local/` or `http://[IP_ADDRESS]/`
- **JSON API**: `http://qlockthree.local/status` or `http://[IP_ADDRESS]/status`

The status page shows:
- Device hostname and IP address
- WiFi signal strength (RSSI)
- Device uptime
- Available memory
- Chip model and SDK version

### OTA Updates

After the initial USB upload, you can update the firmware wirelessly:

#### Using PlatformIO CLI:
```bash
pio run --target upload --upload-port qlockthree.local
```

#### Using Arduino IDE:
1. Go to Tools > Port
2. Select "qlockthree at [IP_ADDRESS]" (Network Port)
3. Upload normally

### Automatic Updates

The device can automatically check for and install firmware updates from GitHub releases or a custom web server.

#### Configuration

In `include/config.h`, configure the update settings:

```cpp
// GitHub releases (recommended)
const char* UPDATE_URL = "https://api.github.com/repos/your-username/your-repo/releases/latest";

// Or use a direct URL to firmware binary
// const char* FIRMWARE_URL = "https://your-server.com/firmware/latest.bin";

const char* CURRENT_VERSION = "1.0.0";  // Update this with each release
const unsigned long UPDATE_CHECK_INTERVAL = 3600000; // Check every hour
```

#### How It Works

1. **Automatic Checking**: Device checks for updates every hour (configurable)
2. **GitHub Integration**: Queries GitHub releases API for the latest version
3. **Version Comparison**: Uses semantic versioning (x.y.z) comparison
4. **Manual Updates**: Web interface provides buttons to check and install updates
5. **Safety**: Updates require manual confirmation by default

#### Web Interface Features

- **Check for Updates**: Manual update checking via web button
- **Install Update**: One-click update installation
- **Version Display**: Shows current and latest available versions
- **Update Status**: Visual indicators for available updates

#### For Automatic Updates

To enable fully automatic updates (without manual confirmation), uncomment this line in `checkForUpdates()`:

```cpp
// performUpdate(downloadUrl);  // Remove // to enable auto-updates
```

⚠️ **Warning**: Automatic updates can potentially brick your device if a bad firmware is released. Use with caution.

### WiFi Management

#### Changing WiFi Networks

You can change WiFi networks without reflashing firmware:

1. **Via Web Interface**: Visit the device's web page and click the "Reset WiFi" button
2. **Via Serial Commands**: Send a WiFi reset command (if implemented)
3. **Manual Reset**: The device will enter configuration mode if it can't connect to the saved network

After WiFi reset:
- Device restarts and creates the `QlockThree-Setup` access point
- Connect to this AP and configure new WiFi credentials
- New credentials are saved and device connects to the new network

#### WiFi Configuration Portal Features

- **Network Scanning**: Automatically scans and lists available WiFi networks
- **Signal Strength**: Shows signal strength for each network
- **Security Info**: Displays security type for each network
- **Custom Parameters**: Shows device hostname and firmware version
- **Dark Mode**: Modern, responsive interface
- **Persistent Storage**: Credentials saved to ESP32 NVS (non-volatile storage)

### Monitoring

View serial output over the network:
```bash
pio device monitor --port socket://qlockthree.local:23
```

## Network Discovery

The device will be discoverable on your network as:
- **Hostname**: `qlockthree.local` (mDNS)
- **IP Address**: Check serial monitor or your router's DHCP table

## Troubleshooting

### WiFi Connection Issues
- Verify SSID and password are correct
- Check that your WiFi network uses 2.4GHz (ESP32 doesn't support 5GHz)
- Ensure the device is within WiFi range

### OTA Update Issues
- Make sure the device is connected to WiFi
- Verify the hostname/IP address is correct
- Check that no firewall is blocking the connection
- Ensure sufficient free memory for the update

### Can't Find Device on Network
- Check your router's connected devices list
- Use network scanner tools to find the IP address
- Try accessing via IP address instead of hostname

## GitHub Actions & Automated Builds

This project includes GitHub Actions workflows for automated building and releasing:

### 🔄 Automated Builds (`.github/workflows/build.yml`)

**Triggers:**
- Every push to `main`/`master` branch
- Pull requests
- Manual workflow dispatch

**What it does:**
- Builds firmware for ESP32-C3
- Creates build artifacts
- Caches PlatformIO dependencies for faster builds
- Uploads firmware binaries as GitHub artifacts

### 🚀 Release Workflow (`.github/workflows/release.yml`)

**How to create a release:**
1. Go to GitHub → Actions → "Create Release"
2. Click "Run workflow"
3. Enter version number (e.g., `1.0.1`)
4. Choose if it's a pre-release
5. Click "Run workflow"

**What happens:**
- Updates version in config files
- Builds firmware
- Creates complete firmware with bootloader
- Generates checksums and build info
- Creates GitHub release with all files
- Commits version bump back to repository
- Tags the release

**Release files include:**
- `qlockthree-esp32c3-X.X.X.bin` - Main firmware (for OTA)
- `qlockthree-esp32c3-X.X.X-complete.bin` - Complete firmware with bootloader
- `checksums.txt` - SHA256 verification
- `build-info.json` - Build metadata

### 📦 Using Releases for Auto-Updates

Update your `config.h` with your GitHub repository:

```cpp
const char* UPDATE_URL = "https://api.github.com/repos/YOUR-USERNAME/YOUR-REPO/releases/latest";
```

Devices will automatically check this URL and download the `.bin` file from the latest release.

## Dependencies

The following libraries are automatically installed via `platformio.ini`:
- WiFi (ESP32 built-in)
- ArduinoOTA
- WebServer
- ESPmDNS
- HTTPClient
- Update
- ArduinoJson
- WiFiManager
- Preferences

## Development

You can now add your QlockThree-specific code to the `loop()` function or create additional functions. The WiFi and OTA functionality will continue running in the background.

Remember to call `ArduinoOTA.handle()` regularly in your main loop to ensure OTA updates remain available.
