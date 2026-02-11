#ifndef CONFIG_H
#define CONFIG_H

// qlockthree Production Configuration
// This file is included in the repository with production defaults
// Users can modify settings via the web interface or override locally

// WiFi Configuration - Leave empty for captive portal setup
#define WIFI_SSID ""              // Leave empty to enable captive portal
#define WIFI_PASSWORD ""          // Leave empty to enable captive portal

// WiFi Manager Settings
#define AP_SSID "qlockthree" // Access point name for setup
#define AP_PASSWORD ""            // Leave empty for open AP, or set password
#define WIFI_TIMEOUT 30000        // 30 seconds timeout for WiFi connection

// OTA Configuration
#define OTA_HOSTNAME "qlockthree"   // Network hostname for device discovery
// Optional: Uncomment and set a password for OTA security
// #define OTA_PASSWORD "your_ota_password"

// Auto Update Configuration
// Note: GitHub repository URL is hardcoded in firmware for persistence after OTA updates
#define CURRENT_VERSION "1.1.0"    // Firmware version
#define UPDATE_CHECK_INTERVAL 3600000 // Check every hour (in milliseconds)

// LED Configuration for WS2812 LEDs - Configurable via web interface
#define LED_DATA_PIN 2        // GPIO pin for LED data (ESP32-C3 compatible)
#define LED_BRIGHTNESS 128    // Default brightness (0-255, 50% brightness)
#define LED_ANIMATION_SPEED 50 // Default animation speed (0-255)
// NOTE: LED count is determined by the selected mapping, not configured here

// LED Strip Type Configuration
#define LED_STRIP_TYPE_RGB  0   // WS2812 RGB (3-channel, default)
#define LED_STRIP_TYPE_RGBW 1   // SK6812 RGBW (4-channel with white)
#define DEFAULT_LED_STRIP_TYPE LED_STRIP_TYPE_RGB

// qlockthree LED Layout - Customize for your specific word grid
#define LED_REVERSE_ORDER false     // Reverse LED strip direction if needed
#define LED_CORNER_LEDS 4            // Number of corner minute indicator LEDs

// Web Server Configuration
#define WEB_SERVER_PORT 80           // HTTP port for web interface

// Cloud Configuration
#define CLOUD_API_URL "https://qlockthree.l4b.dev"

#endif // CONFIG_H
