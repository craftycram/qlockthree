#include "web_server_manager.h"
#include "wifi_manager_helper.h"
#include "auto_updater.h"
#include "led_controller.h"
#include "config.h"
#include <WiFi.h>

WebServerManager::WebServerManager(int port) : server(port), wifiManagerHelper(nullptr), autoUpdater(nullptr), ledController(nullptr) {
}

void WebServerManager::begin(WiFiManagerHelper* wifiHelper, AutoUpdater* updater, LEDController* ledCtrl) {
    wifiManagerHelper = wifiHelper;
    autoUpdater = updater;
    ledController = ledCtrl;
    
    setupRoutes();
    server.begin();
    Serial.println("Web server started");
    
    // Add service to mDNS-SD
    MDNS.addService("http", "tcp", 80);
}

void WebServerManager::handleClient() {
    server.handleClient();
}

void WebServerManager::setupRoutes() {
    // Root page with status information
    server.on("/", [this]() { handleRoot(); });
    
    // API endpoint for JSON status
    server.on("/status", [this]() { handleStatus(); });
    
    // Manual update trigger
    server.on("/update", HTTP_POST, [this]() { handleManualUpdate(); });
    
    // Check for updates endpoint
    server.on("/check-update", [this]() { handleCheckUpdate(); });
    
    // WiFi reset endpoint
    server.on("/wifi-reset", HTTP_POST, [this]() { handleWiFiReset(); });
    
    // LED configuration endpoints
    server.on("/led", [this]() { handleLEDConfig(); });
    server.on("/led/status", [this]() { handleLEDStatus(); });
    server.on("/led/test", HTTP_POST, [this]() { handleLEDTest(); });
    server.on("/led/pattern", HTTP_POST, [this]() { handleLEDPattern(); });
    server.on("/led/config", HTTP_POST, [this]() { 
        // Handle LED configuration updates
        if (!ledController) {
            server.send(500, "text/plain", "LED controller not available");
            return;
        }
        
        if (server.hasArg("brightness")) {
            int brightness = server.arg("brightness").toInt();
            if (brightness >= 0 && brightness <= 255) {
                ledController->setBrightness(brightness);
                ledController->saveSettings();
            }
        }
        
        if (server.hasArg("speed")) {
            int speed = server.arg("speed").toInt();
            if (speed >= 0 && speed <= 255) {
                ledController->setSpeed(speed);
                ledController->saveSettings();
            }
        }
        
        if (server.hasArg("num_leds")) {
            int numLeds = server.arg("num_leds").toInt();
            if (numLeds > 0 && numLeds <= 500) {
                ledController->setNumLeds(numLeds);
            }
        }
        
        server.send(200, "text/plain", "LED settings updated");
    });
}

void WebServerManager::handleRoot() {
    server.send(200, "text/html", getStatusHTML());
}

void WebServerManager::handleStatus() {
    server.send(200, "application/json", getStatusJSON());
}

void WebServerManager::handleCheckUpdate() {
    if (autoUpdater) {
        autoUpdater->checkForUpdates();
        String json = "{";
        json += "\"current_version\":\"" + String(CURRENT_VERSION) + "\",";
        json += "\"latest_version\":\"" + autoUpdater->getLatestVersion() + "\",";
        json += "\"update_available\":" + String(autoUpdater->isUpdateAvailable() ? "true" : "false") + ",";
        json += "\"download_url\":\"" + autoUpdater->getDownloadUrl() + "\"";
        json += "}";
        server.send(200, "application/json", json);
    } else {
        server.send(500, "application/json", "{\"error\":\"Auto updater not available\"}");
    }
}

void WebServerManager::handleManualUpdate() {
    if (autoUpdater && autoUpdater->isUpdateAvailable()) {
        server.send(200, "text/plain", "Starting update...");
        delay(1000);
        autoUpdater->performUpdate();
    } else {
        server.send(400, "text/plain", "No update available");
    }
}

void WebServerManager::handleWiFiReset() {
    if (wifiManagerHelper) {
        server.send(200, "text/plain", "WiFi settings cleared. Device will restart and enter configuration mode.");
        delay(1000);
        wifiManagerHelper->resetWiFi();
    } else {
        server.send(500, "text/plain", "WiFi manager not available");
    }
}

String WebServerManager::getStatusJSON() {
    String json = "{";
    json += "\"hostname\":\"" + String(OTA_HOSTNAME) + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"uptime\":" + String(millis()) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"chip_model\":\"" + String(ESP.getChipModel()) + "\",";
    json += "\"sdk_version\":\"" + String(ESP.getSdkVersion()) + "\",";
    json += "\"current_version\":\"" + String(CURRENT_VERSION) + "\",";
    
    if (autoUpdater) {
        json += "\"latest_version\":\"" + autoUpdater->getLatestVersion() + "\",";
        json += "\"update_available\":" + String(autoUpdater->isUpdateAvailable() ? "true" : "false");
    } else {
        json += "\"latest_version\":\"\",";
        json += "\"update_available\":false";
    }
    
    json += "}";
    return json;
}

String WebServerManager::getStatusHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>QlockThree Status</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:Arial,sans-serif;margin:40px;background:#f0f0f0}";
    html += ".container{background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
    html += "h1{color:#333;text-align:center}";
    html += ".info{margin:10px 0;padding:10px;background:#f8f9fa;border-left:4px solid #007bff}";
    html += ".update{margin:10px 0;padding:10px;background:#d4edda;border-left:4px solid #28a745}";
    html += ".warning{margin:10px 0;padding:10px;background:#fff3cd;border-left:4px solid #ffc107}";
    html += ".button{display:inline-block;padding:10px 20px;margin:10px 5px;background:#007bff;color:white;text-decoration:none;border-radius:4px;border:none;cursor:pointer}";
    html += ".button:hover{background:#0056b3}";
    html += ".update-btn{background:#28a745}.update-btn:hover{background:#1e7e34}";
    html += ".check-btn{background:#ffc107;color:#212529}.check-btn:hover{background:#e0a800}";
    html += ".danger-btn{background:#dc3545}.danger-btn:hover{background:#c82333}</style>";
    
    html += "<script>";
    html += "function checkUpdate() {";
    html += "  fetch('/check-update').then(r => r.json()).then(data => {";
    html += "    document.getElementById('current-version').textContent = data.current_version;";
    html += "    document.getElementById('latest-version').textContent = data.latest_version;";
    html += "    const updateDiv = document.getElementById('update-info');";
    html += "    if (data.update_available) {";
    html += "      updateDiv.innerHTML = '<strong>Update Available!</strong> Version ' + data.latest_version + ' is ready for installation.';";
    html += "      updateDiv.className = 'update';";
    html += "      document.getElementById('update-btn').style.display = 'inline-block';";
    html += "    } else {";
    html += "      updateDiv.innerHTML = '<strong>Up to Date</strong> - You are running the latest version.';";
    html += "      updateDiv.className = 'info';";
    html += "      document.getElementById('update-btn').style.display = 'none';";
    html += "    }";
    html += "  });";
    html += "}";
    html += "function performUpdate() {";
    html += "  if (confirm('Are you sure you want to update the firmware? The device will restart.')) {";
    html += "    fetch('/update', {method: 'POST'}).then(() => {";
    html += "      alert('Update started. Device will restart when complete.');";
    html += "    });";
    html += "  }";
    html += "}";
    html += "function resetWiFi() {";
    html += "  if (confirm('Are you sure you want to reset WiFi settings? The device will restart and enter configuration mode.')) {";
    html += "    fetch('/wifi-reset', {method: 'POST'}).then(() => {";
    html += "      alert('WiFi settings reset. Device will restart.');";
    html += "    });";
    html += "  }";
    html += "}";
    html += "</script>";
    
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>QlockThree Status</h1>";
    html += "<div class='info'><strong>Hostname:</strong> " + String(OTA_HOSTNAME) + "</div>";
    html += "<div class='info'><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</div>";
    html += "<div class='info'><strong>WiFi Network:</strong> " + WiFi.SSID() + "</div>";
    html += "<div class='info'><strong>WiFi RSSI:</strong> " + String(WiFi.RSSI()) + " dBm</div>";
    html += "<div class='info'><strong>Uptime:</strong> " + String(millis() / 1000) + " seconds</div>";
    html += "<div class='info'><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</div>";
    html += "<div class='info'><strong>Chip Model:</strong> " + String(ESP.getChipModel()) + "</div>";
    html += "<div class='info'><strong>SDK Version:</strong> " + String(ESP.getSdkVersion()) + "</div>";
    
    html += "<div class='info'><strong>Current Version:</strong> <span id='current-version'>" + String(CURRENT_VERSION) + "</span></div>";
    
    String latestVersion = autoUpdater ? autoUpdater->getLatestVersion() : "Unknown";
    html += "<div class='info'><strong>Latest Version:</strong> <span id='latest-version'>" + (latestVersion.length() > 0 ? latestVersion : "Checking...") + "</span></div>";
    
    bool updateAvailable = autoUpdater ? autoUpdater->isUpdateAvailable() : false;
    html += "<div id='update-info' class='" + String(updateAvailable ? "update" : "info") + "'>";
    if (updateAvailable && autoUpdater) {
        html += "<strong>Update Available!</strong> Version " + autoUpdater->getLatestVersion() + " is ready for installation.";
    } else {
        html += "<strong>Up to Date</strong> - You are running the latest version.";
    }
    html += "</div>";
    
    html += "<div class='warning'>";
    html += "<strong>WiFi Configuration:</strong> To reconfigure WiFi settings, click the 'Reset WiFi' button below. ";
    html += "The device will restart and create a 'QlockThree-Setup' access point for configuration.";
    html += "</div>";
    
    html += "<br>";
    html += "<a href='/status' class='button'>JSON Status</a>";
    html += "<a href='/led' class='button'>LED Config</a>";
    html += "<a href='javascript:location.reload()' class='button'>Refresh</a>";
    html += "<button onclick='checkUpdate()' class='button check-btn'>Check for Updates</button>";
    html += "<button id='update-btn' onclick='performUpdate()' class='button update-btn' style='display:" + String(updateAvailable ? "inline-block" : "none") + "'>Install Update</button>";
    html += "<button onclick='resetWiFi()' class='button danger-btn'>Reset WiFi</button>";
    html += "</div></body></html>";
    
    return html;
}

// LED configuration handlers
void WebServerManager::handleLEDStatus() {
    server.send(200, "application/json", getLEDStatusJSON());
}

void WebServerManager::handleLEDConfig() {
    server.send(200, "text/html", getLEDConfigHTML());
}

void WebServerManager::handleLEDTest() {
    if (!ledController) {
        server.send(500, "text/plain", "LED controller not available");
        return;
    }
    
    String pattern = server.arg("pattern");
    if (pattern == "rainbow") {
        ledController->setPattern(LEDPattern::RAINBOW);
    } else if (pattern == "breathing") {
        ledController->setPattern(LEDPattern::BREATHING);
    } else if (pattern == "solid") {
        ledController->setPattern(LEDPattern::SOLID_COLOR);
    } else if (pattern == "off") {
        ledController->setPattern(LEDPattern::OFF);
    } else {
        ledController->setPattern(LEDPattern::CLOCK_DISPLAY);
    }
    
    server.send(200, "text/plain", "LED pattern changed to " + pattern);
}

void WebServerManager::handleLEDPattern() {
    if (!ledController) {
        server.send(500, "text/plain", "LED controller not available");
        return;
    }
    
    String action = server.arg("action");
    if (action == "clock") {
        ledController->setPattern(LEDPattern::CLOCK_DISPLAY);
    } else if (action == "rainbow") {
        ledController->setPattern(LEDPattern::RAINBOW);
    } else if (action == "breathing") {
        ledController->setPattern(LEDPattern::BREATHING);
    } else if (action == "off") {
        ledController->setPattern(LEDPattern::OFF);
    }
    
    server.send(200, "text/plain", "Pattern set to " + action);
}

String WebServerManager::getLEDStatusJSON() {
    String json = "{";
    
    if (ledController) {
        json += "\"num_leds\":" + String(ledController->getNumLeds()) + ",";
        json += "\"brightness\":" + String(ledController->getBrightness()) + ",";
        json += "\"speed\":" + String(ledController->getSpeed()) + ",";
        json += "\"data_pin\":" + String(ledController->getDataPin()) + ",";
        
        CRGB color = ledController->getSolidColor();
        json += "\"color\":{";
        json += "\"r\":" + String(color.r) + ",";
        json += "\"g\":" + String(color.g) + ",";
        json += "\"b\":" + String(color.b);
        json += "},";
        
        int pattern = (int)ledController->getCurrentPattern();
        json += "\"pattern\":" + String(pattern);
    } else {
        json += "\"error\":\"LED controller not available\"";
    }
    
    json += "}";
    return json;
}

String WebServerManager::getLEDConfigHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>QlockThree LED Configuration</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:Arial,sans-serif;margin:40px;background:#f0f0f0}";
    html += ".container{background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
    html += "h1{color:#333;text-align:center}";
    html += ".control-group{margin:20px 0;padding:15px;background:#f8f9fa;border-radius:5px}";
    html += "label{display:block;margin-bottom:5px;font-weight:bold}";
    html += "input,select{width:100%;padding:8px;margin-bottom:10px;border:1px solid #ddd;border-radius:4px}";
    html += ".slider{width:100%}";
    html += ".button{display:inline-block;padding:10px 20px;margin:5px;background:#007bff;color:white;text-decoration:none;border-radius:4px;border:none;cursor:pointer}";
    html += ".button:hover{background:#0056b3}";
    html += ".pattern-btn{background:#28a745}.pattern-btn:hover{background:#1e7e34}";
    html += ".test-btn{background:#ffc107;color:#212529}.test-btn:hover{background:#e0a800}";
    html += "#brightness-value,#speed-value{font-weight:bold;color:#007bff}</style>";
    
    html += "<script>";
    html += "function updateBrightness(val) {";
    html += "  document.getElementById('brightness-value').textContent = val;";
    html += "  fetch('/led/config', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'brightness=' + val});";
    html += "}";
    html += "function updateSpeed(val) {";
    html += "  document.getElementById('speed-value').textContent = val;";
    html += "  fetch('/led/config', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'speed=' + val});";
    html += "}";
    html += "function updateNumLeds() {";
    html += "  const val = document.getElementById('num-leds').value;";
    html += "  if (confirm('Changing LED count requires restart. Continue?')) {";
    html += "    fetch('/led/config', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'num_leds=' + val});";
    html += "  }";
    html += "}";
    html += "function testPattern(pattern) {";
    html += "  fetch('/led/test', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'pattern=' + pattern});";
    html += "}";
    html += "</script>";
    
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>🌈 QlockThree LED Configuration</h1>";
    
    if (ledController) {
        html += "<div class='control-group'>";
        html += "<h3>💡 Brightness Control</h3>";
        html += "<label for='brightness'>Brightness: <span id='brightness-value'>" + String(ledController->getBrightness()) + "</span></label>";
        html += "<input type='range' id='brightness' class='slider' min='0' max='255' value='" + String(ledController->getBrightness()) + "' onchange='updateBrightness(this.value)'>";
        html += "</div>";
        
        html += "<div class='control-group'>";
        html += "<h3>⚡ Animation Speed</h3>";
        html += "<label for='speed'>Speed: <span id='speed-value'>" + String(ledController->getSpeed()) + "</span></label>";
        html += "<input type='range' id='speed' class='slider' min='0' max='255' value='" + String(ledController->getSpeed()) + "' onchange='updateSpeed(this.value)'>";
        html += "</div>";
        
        html += "<div class='control-group'>";
        html += "<h3>🔢 LED Configuration</h3>";
        html += "<label for='num-leds'>Number of LEDs:</label>";
        html += "<input type='number' id='num-leds' min='1' max='500' value='" + String(ledController->getNumLeds()) + "'>";
        html += "<button onclick='updateNumLeds()' class='button'>Update LED Count</button>";
        html += "<p><small>Current: " + String(ledController->getNumLeds()) + " LEDs on GPIO pin " + String(ledController->getDataPin()) + "</small></p>";
        html += "</div>";
        
        html += "<div class='control-group'>";
        html += "<h3>🎨 Pattern Tests</h3>";
        html += "<button onclick=\"testPattern('rainbow')\" class='button test-btn'>🌈 Rainbow</button>";
        html += "<button onclick=\"testPattern('breathing')\" class='button test-btn'>💨 Breathing</button>";
        html += "<button onclick=\"testPattern('solid')\" class='button test-btn'>💎 Solid Color</button>";
        html += "<button onclick=\"testPattern('clock')\" class='button pattern-btn'>🕐 Clock Display</button>";
        html += "<button onclick=\"testPattern('off')\" class='button'>⚫ Turn Off</button>";
        html += "</div>";
    } else {
        html += "<div class='control-group'>";
        html += "<h3>❌ LED Controller Not Available</h3>";
        html += "<p>LED controller is not initialized. Check hardware connections.</p>";
        html += "</div>";
    }
    
    html += "<br>";
    html += "<a href='/' class='button'>← Back to Status</a>";
    html += "<a href='/led/status' class='button'>📊 JSON Status</a>";
    html += "</div></body></html>";
    
    return html;
}
