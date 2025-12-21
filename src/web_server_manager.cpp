#include "web_server_manager.h"
#include "wifi_manager_helper.h"
#include "auto_updater.h"
#include "led_controller.h"
#include "time_manager.h"
#include "config.h"
#include <WiFi.h>

WebServerManager::WebServerManager(int port) : server(port), wifiManagerHelper(nullptr), autoUpdater(nullptr), ledController(nullptr), timeManager(nullptr) {
}

void WebServerManager::begin(WiFiManagerHelper* wifiHelper, AutoUpdater* updater, LEDController* ledCtrl, TimeManager* timeMgr) {
    wifiManagerHelper = wifiHelper;
    autoUpdater = updater;
    ledController = ledCtrl;
    timeManager = timeMgr;
    
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
    
    // Time configuration endpoints
    server.on("/time", [this]() { handleTimeConfig(); });
    server.on("/time/status", [this]() { handleTimeStatus(); });
    server.on("/time/sync", HTTP_POST, [this]() { handleTimeSync(); });
    server.on("/time/timezone", HTTP_POST, [this]() { handleSetTimezone(); });
    server.on("/time/ntp", HTTP_POST, [this]() { handleSetNTP(); });
    
    // LED configuration endpoints
    server.on("/led", [this]() { handleLEDConfig(); });
    server.on("/led/status", [this]() { handleLEDStatus(); });
    server.on("/led/test", HTTP_POST, [this]() { handleLEDTest(); });
    server.on("/led/pattern", HTTP_POST, [this]() { handleLEDPattern(); });
    server.on("/led/mapping", [this]() { handleLEDMapping(); });
    server.on("/led/mapping/set", HTTP_POST, [this]() { handleSetLEDMapping(); });
    
    // ENHANCED: Add color configuration support
    server.on("/led/config", HTTP_POST, [this]() { 
        // Handle LED configuration updates with color support
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
        
        // ENHANCED: Color configuration support
        if (server.hasArg("color_r") && server.hasArg("color_g") && server.hasArg("color_b")) {
            Serial.println("DEBUG: Web server received color_r, color_g, color_b parameters");
            String rStr = server.arg("color_r");
            String gStr = server.arg("color_g");
            String bStr = server.arg("color_b");
            Serial.printf("DEBUG: Raw color values - R:'%s', G:'%s', B:'%s'\n", rStr.c_str(), gStr.c_str(), bStr.c_str());
            
            int r = rStr.toInt();
            int g = gStr.toInt();
            int b = bStr.toInt();
            Serial.printf("DEBUG: Parsed color values - R:%d, G:%d, B:%d\n", r, g, b);
            
            if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
                Serial.println("DEBUG: Color values are valid, creating CRGB object");
                CRGB color = CRGB(r, g, b);
                Serial.printf("DEBUG: Calling setSolidColor with RGB(%d, %d, %d)\n", r, g, b);
                ledController->setSolidColor(color);
                // Set pattern to solid color to show the new color immediately
                Serial.println("DEBUG: Calling setPattern(SOLID_COLOR)");
                ledController->setPattern(LEDPattern::SOLID_COLOR);
                Serial.println("DEBUG: Calling saveSettings()");
                ledController->saveSettings();
                Serial.printf("Color changed to RGB(%d, %d, %d)\n", r, g, b);
            } else {
                Serial.printf("DEBUG: Invalid color values - R:%d, G:%d, B:%d\n", r, g, b);
            }
        } else {
            Serial.println("DEBUG: Web server color config - missing some color parameters");
            if (server.hasArg("color_r")) Serial.printf("DEBUG: Has color_r: %s\n", server.arg("color_r").c_str());
            if (server.hasArg("color_g")) Serial.printf("DEBUG: Has color_g: %s\n", server.arg("color_g").c_str());
            if (server.hasArg("color_b")) Serial.printf("DEBUG: Has color_b: %s\n", server.arg("color_b").c_str());
        }
        
        // Save settings after any change
        if (server.hasArg("save")) {
            ledController->saveSettings();
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
    html += "<a href='/led' class='button'>🌈 LED Config</a>";
    html += "<a href='/time' class='button'>Time Config</a>";
    html += "<a href='javascript:location.reload()' class='button'>Refresh</a>";
    html += "<button onclick='checkUpdate()' class='button check-btn'>Check for Updates</button>";
    html += "<button id='update-btn' onclick='performUpdate()' class='button update-btn' style='display:" + String(updateAvailable ? "inline-block" : "none") + "'>Install Update</button>";
    html += "<button onclick='resetWiFi()' class='button danger-btn'>Reset WiFi</button>";
    html += "</div></body></html>";
    
    return html;
}

// Time configuration handlers
void WebServerManager::handleTimeConfig() {
    server.send(200, "text/html", getTimeConfigHTML());
}

void WebServerManager::handleTimeStatus() {
    if (timeManager) {
        server.send(200, "application/json", timeManager->getStatusJSON());
    } else {
        server.send(500, "application/json", "{\"error\":\"Time manager not available\"}");
    }
}

void WebServerManager::handleTimeSync() {
    if (!timeManager) {
        server.send(500, "text/plain", "Time manager not available");
        return;
    }
    
    if (timeManager->syncTime()) {
        server.send(200, "text/plain", "Time synchronized successfully");
    } else {
        server.send(500, "text/plain", "Failed to synchronize time");
    }
}

void WebServerManager::handleSetTimezone() {
    if (!timeManager) {
        server.send(500, "text/plain", "Time manager not available");
        return;
    }
    
    if (server.hasArg("timezone")) {
        String timezone = server.arg("timezone");
        if (timeManager->setTimezoneByName(timezone.c_str())) {
            server.send(200, "text/plain", "Timezone set to " + timezone);
        } else {
            server.send(400, "text/plain", "Invalid timezone: " + timezone);
        }
    } else if (server.hasArg("posix")) {
        String posix = server.arg("posix");
        timeManager->setTimezone(posix.c_str());
        server.send(200, "text/plain", "Timezone set to " + posix);
    } else {
        server.send(400, "text/plain", "Missing timezone parameter");
    }
}

void WebServerManager::handleSetNTP() {
    if (!timeManager) {
        server.send(500, "text/plain", "Time manager not available");
        return;
    }
    
    String ntp1 = server.arg("ntp1");
    String ntp2 = server.arg("ntp2");
    String ntp3 = server.arg("ntp3");
    
    if (ntp1.length() > 0) {
        const char* ntp2_ptr = ntp2.length() > 0 ? ntp2.c_str() : nullptr;
        const char* ntp3_ptr = ntp3.length() > 0 ? ntp3.c_str() : nullptr;
        
        timeManager->setNTPServers(ntp1.c_str(), ntp2_ptr, ntp3_ptr);
        server.send(200, "text/plain", "NTP servers updated");
    } else {
        server.send(400, "text/plain", "At least one NTP server is required");
    }
}

String WebServerManager::getTimeConfigHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>QlockThree Time Configuration</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:Arial,sans-serif;margin:40px;background:#f0f0f0}";
    html += ".container{background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
    html += "h1{color:#333;text-align:center}";
    html += ".control-group{margin:20px 0;padding:15px;background:#f8f9fa;border-radius:5px}";
    html += ".current-time{background:#d4edda;border-left:4px solid #28a745}";
    html += "label{display:block;margin-bottom:5px;font-weight:bold}";
    html += "input,select{width:100%;padding:8px;margin-bottom:10px;border:1px solid #ddd;border-radius:4px}";
    html += ".button{display:inline-block;padding:10px 20px;margin:5px;background:#007bff;color:white;text-decoration:none;border-radius:4px;border:none;cursor:pointer}";
    html += ".button:hover{background:#0056b3}";
    html += ".sync-btn{background:#28a745}.sync-btn:hover{background:#1e7e34}";
    html += ".info{margin:10px 0;padding:10px;background:#f8f9fa;border-left:4px solid #007bff}</style>";
    
    html += "<script>";
    html += "function syncTime() {";
    html += "  fetch('/time/sync', {method: 'POST'}).then(r => r.text()).then(data => {";
    html += "    alert(data);";
    html += "    setTimeout(() => location.reload(), 1000);";
    html += "  });";
    html += "}";
    html += "function setTimezone() {";
    html += "  const timezone = document.getElementById('timezone-select').value;";
    html += "  fetch('/time/timezone', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'timezone=' + timezone}).then(r => r.text()).then(data => {";
    html += "    alert(data);";
    html += "    setTimeout(() => location.reload(), 1000);";
    html += "  });";
    html += "}";
    html += "function setNTP() {";
    html += "  const ntp1 = document.getElementById('ntp1').value;";
    html += "  const ntp2 = document.getElementById('ntp2').value;";
    html += "  const ntp3 = document.getElementById('ntp3').value;";
    html += "  const data = 'ntp1=' + encodeURIComponent(ntp1) + '&ntp2=' + encodeURIComponent(ntp2) + '&ntp3=' + encodeURIComponent(ntp3);";
    html += "  fetch('/time/ntp', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: data}).then(r => r.text()).then(data => {";
    html += "    alert(data);";
    html += "  });";
    html += "}";
    html += "</script>";
    
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>🕐 QlockThree Time Configuration</h1>";
    
    if (timeManager) {
        html += "<div class='control-group current-time'>";
        html += "<h3>Current Time & Status</h3>";
        html += "<div class='info'><strong>Current Time:</strong> " + timeManager->getFormattedTime("%H:%M:%S") + "</div>";
        html += "<div class='info'><strong>Current Date:</strong> " + timeManager->getFormattedDate("%Y-%m-%d") + "</div>";
        html += "<div class='info'><strong>Timezone:</strong> " + timeManager->getTimezoneString() + "</div>";
        html += "<div class='info'><strong>Time Synced:</strong> " + String(timeManager->isTimeSynced() ? "Yes" : "No") + "</div>";
        html += "<div class='info'><strong>DST Active:</strong> " + String(timeManager->isDST() ? "Yes" : "No") + "</div>";
        html += "<div class='info'><strong>Timezone Offset:</strong> UTC" + String(timeManager->getTimezoneOffset() >= 0 ? "+" : "") + String(timeManager->getTimezoneOffset()) + "</div>";
        html += "</div>";
        
        html += "<div class='control-group'>";
        html += "<h3>🌍 Timezone Configuration</h3>";
        html += "<label for='timezone-select'>Select Timezone:</label>";
        html += "<select id='timezone-select'>";
        html += "<option value='UTC'>UTC (Coordinated Universal Time)</option>";
        html += "<option value='CET' selected>CET (Central European Time)</option>";
        html += "<option value='EET'>EET (Eastern European Time)</option>";
        html += "<option value='WET'>WET (Western European Time)</option>";
        html += "<option value='EST'>EST (Eastern Standard Time)</option>";
        html += "<option value='CST'>CST (Central Standard Time)</option>";
        html += "<option value='MST'>MST (Mountain Standard Time)</option>";
        html += "<option value='PST'>PST (Pacific Standard Time)</option>";
        html += "<option value='JST'>JST (Japan Standard Time)</option>";
        html += "<option value='AEST'>AEST (Australian Eastern Time)</option>";
        html += "<option value='IST'>IST (India Standard Time)</option>";
        html += "<option value='CST_CN'>CST (China Standard Time)</option>";
        html += "<option value='MSK'>MSK (Moscow Time)</option>";
        html += "<option value='GST'>GST (Gulf Standard Time)</option>";
        html += "</select>";
        html += "<button onclick='setTimezone()' class='button'>Set Timezone</button>";
        html += "</div>";
        
        html += "<div class='control-group'>";
        html += "<h3>🌐 NTP Server Configuration</h3>";
        html += "<label for='ntp1'>Primary NTP Server:</label>";
        html += "<input type='text' id='ntp1' value='pool.ntp.org' placeholder='pool.ntp.org'>";
        html += "<label for='ntp2'>Secondary NTP Server:</label>";
        html += "<input type='text' id='ntp2' value='time.nist.gov' placeholder='time.nist.gov'>";
        html += "<label for='ntp3'>Tertiary NTP Server:</label>";
        html += "<input type='text' id='ntp3' value='de.pool.ntp.org' placeholder='de.pool.ntp.org'>";
        html += "<button onclick='setNTP()' class='button'>Update NTP Servers</button>";
        html += "</div>";
        
        html += "<div class='control-group'>";
        html += "<h3>🔄 Time Synchronization</h3>";
        html += "<p>Force synchronization with NTP servers to ensure accurate time.</p>";
        html += "<button onclick='syncTime()' class='button sync-btn'>Sync Time Now</button>";
        html += "</div>";
    } else {
        html += "<div class='control-group'>";
        html += "<h3>❌ Time Manager Not Available</h3>";
        html += "<p>Time manager is not initialized. Check system configuration.</p>";
        html += "</div>";
    }
    
    html += "<br>";
    html += "<a href='/' class='button'>← Back to Status</a>";
    html += "<a href='/time/status' class='button'>📊 JSON Status</a>";
    html += "</div></body></html>";
    
    return html;
}

String WebServerManager::getTimeStatusJSON() {
    if (timeManager) {
        return timeManager->getStatusJSON();
    } else {
        return "{\"error\":\"Time manager not available\"}";
    }
}

// ENHANCED LED configuration handlers
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

// ENHANCED LED Configuration HTML with Color Picker
String WebServerManager::getLEDConfigHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>QlockThree LED Configuration</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0;}";
    html += ".container{background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1);max-width:800px;margin:0 auto;}";
    html += "h1{color:#333;text-align:center;margin-bottom:30px;}";
    html += ".control-group{margin:20px 0;padding:20px;background:#f8f9fa;border-radius:8px;border-left:4px solid #007bff;}";
    html += ".control-group h3{margin-top:0;color:#495057;}";
    html += "label{display:block;margin:10px 0 5px 0;font-weight:bold;color:#495057;}";
    html += "input,select{width:100%;padding:10px;margin-bottom:10px;border:1px solid #ddd;border-radius:4px;box-sizing:border-box;}";
    html += ".slider{width:100%;}";
    html += ".color-input{width:60px;height:40px;padding:0;border:2px solid #ddd;cursor:pointer;}";
    html += ".button{display:inline-block;padding:12px 24px;margin:8px;background:#007bff;color:white;text-decoration:none;border-radius:4px;border:none;cursor:pointer;font-size:14px;transition:background 0.3s;}";
    html += ".button:hover{background:#0056b3;}";
    html += ".pattern-btn{background:#28a745;}.pattern-btn:hover{background:#1e7e34;}";
    html += ".test-btn{background:#ffc107;color:#212529;}.test-btn:hover{background:#e0a800;}";
    html += ".danger-btn{background:#dc3545;}.danger-btn:hover{background:#c82333;}";
    html += ".value-display{font-weight:bold;color:#007bff;margin-left:10px;}";
    html += ".color-preview{display:inline-block;width:30px;height:30px;border:2px solid #333;border-radius:4px;margin-left:10px;vertical-align:middle;}";
    html += ".preset-colors{display:flex;gap:10px;flex-wrap:wrap;margin-top:10px;}";
    html += ".preset-color{width:40px;height:40px;border:2px solid #333;border-radius:4px;cursor:pointer;transition:transform 0.2s;}";
    html += ".preset-color:hover{transform:scale(1.1);}";
    html += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px;}";
    html += "#brightness-value,#speed-value{font-weight:bold;color:#007bff;}";
    html += "</style>";
    
    html += "<script>";
    html += "document.addEventListener('DOMContentLoaded',function(){loadCurrentSettings();});";
    
    html += "function updateBrightness(val){";
    html += "document.getElementById('brightness-value').textContent=val;";
    html += "sendConfig('brightness',val);";
    html += "}";
    
    html += "function updateSpeed(val){";
    html += "document.getElementById('speed-value').textContent=val;";
    html += "sendConfig('speed',val);";
    html += "}";
    
    html += "function updateClockColor(color){";
    html += "document.getElementById('clock-color-preview').style.backgroundColor=color;";
    html += "const rgb=hexToRgb(color);";
    html += "sendColorConfig(rgb.r,rgb.g,rgb.b);";
    html += "}";
    
    html += "function setPresetColor(color){";
    html += "document.getElementById('clock-color').value=color;";
    html += "updateClockColor(color);";
    html += "}";
    
    html += "function updateNumLeds(){";
    html += "const val=document.getElementById('num-leds').value;";
    html += "if(confirm('Changing LED count requires restart. Continue?')){";
    html += "sendConfig('num_leds',val);";
    html += "}";
    html += "}";
    
    html += "function testPattern(pattern){";
    html += "fetch('/led/test',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'pattern='+pattern}).then(r=>r.text()).then(data=>{";
    html += "updateStatus('Pattern changed to '+pattern);";
    html += "});";
    html += "}";
    
    html += "function sendConfig(param,value){";
    html += "fetch('/led/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:param+'='+encodeURIComponent(value)}).then(r=>r.text()).then(data=>{";
    html += "updateStatus(data);";
    html += "}).catch(err=>{";
    html += "updateStatus('Error: '+err.message);";
    html += "});";
    html += "}";
    
    html += "function sendColorConfig(r,g,b){";
    html += "const data='color_r='+encodeURIComponent(r)+'&color_g='+encodeURIComponent(g)+'&color_b='+encodeURIComponent(b);";
    html += "fetch('/led/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:data}).then(r=>r.text()).then(data=>{";
    html += "updateStatus('Color updated: '+data);";
    html += "}).catch(err=>{";
    html += "updateStatus('Color error: '+err.message);";
    html += "});";
    html += "}";
    
    html += "function loadCurrentSettings(){";
    html += "fetch('/led/status').then(r=>r.json()).then(data=>{";
    html += "if(data.brightness!==undefined){";
    html += "document.getElementById('brightness').value=data.brightness;";
    html += "document.getElementById('brightness-value').textContent=data.brightness;";
    html += "}";
    html += "if(data.speed!==undefined){";
    html += "document.getElementById('speed').value=data.speed;";
    html += "document.getElementById('speed-value').textContent=data.speed;";
    html += "}";
    html += "if(data.num_leds!==undefined){";
    html += "document.getElementById('num-leds').value=data.num_leds;";
    html += "}";
    html += "if(data.color){";
    html += "const hex=rgbToHex(data.color.r,data.color.g,data.color.b);";
    html += "document.getElementById('clock-color').value=hex;";
    html += "document.getElementById('clock-color-preview').style.backgroundColor=hex;";
    html += "}";
    html += "updateStatus('Settings loaded');";
    html += "}).catch(err=>{";
    html += "updateStatus('Failed to load settings');";
    html += "});";
    html += "}";
    
    html += "function saveSettings(){";
    html += "sendConfig('save','1');";
    html += "updateStatus('Settings saved successfully!');";
    html += "}";
    
    html += "function updateStatus(message){";
    html += "console.log('Status:',message);";
    html += "}";
    
    html += "function hexToRgb(hex){";
    html += "const r=parseInt(hex.slice(1,3),16);";
    html += "const g=parseInt(hex.slice(3,5),16);";
    html += "const b=parseInt(hex.slice(5,7),16);";
    html += "return{r,g,b};";
    html += "}";
    
    html += "function rgbToHex(r,g,b){";
    html += "return'#'+((1<<24)+(r<<16)+(g<<8)+b).toString(16).slice(1);";
    html += "}";
    
    html += "</script>";
    
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>🌈 QlockThree LED Configuration</h1>";
    
    if (ledController) {
        html += "<div class='control-group'>";
        html += "<h3>💡 Brightness Control</h3>";
        html += "<label for='brightness'>Brightness: <span id='brightness-value'>" + String(ledController->getBrightness()) + "</span></label>";
        html += "<input type='range' id='brightness' class='slider' min='0' max='255' value='" + String(ledController->getBrightness()) + "' oninput='updateBrightness(this.value)'>";
        html += "</div>";
        
        // ENHANCED: Color Configuration Section
        html += "<div class='control-group'>";
        html += "<h3>🎨 Color Configuration</h3>";
        html += "<p><strong>📝 Instructions:</strong> Select a color below, then click '💎 Show Selected Color' to see it on your QlockThree. The main loop shows time in clock mode, so you need to switch to solid color mode to test colors.</p>";
        
        CRGB currentColor = ledController->getSolidColor();
        String currentHex = "#" + String(currentColor.r < 16 ? "0" : "") + String(currentColor.r, HEX) + 
                           String(currentColor.g < 16 ? "0" : "") + String(currentColor.g, HEX) + 
                           String(currentColor.b < 16 ? "0" : "") + String(currentColor.b, HEX);
        
        html += "<label for='clock-color'>Clock Display Color:</label>";
        html += "<input type='color' id='clock-color' class='color-input' value='" + currentHex + "' onchange='updateClockColor(this.value)'>";
        html += "<span id='clock-color-preview' class='color-preview' style='background-color:" + currentHex + ";'></span>";
        
        html += "<label>Preset Colors:</label>";
        html += "<div class='preset-colors'>";
        html += "<div class='preset-color' style='background-color:#ffffff' onclick=\"setPresetColor('#ffffff')\" title='White'></div>";
        html += "<div class='preset-color' style='background-color:#ff0000' onclick=\"setPresetColor('#ff0000')\" title='Red'></div>";
        html += "<div class='preset-color' style='background-color:#00ff00' onclick=\"setPresetColor('#00ff00')\" title='Green'></div>";
        html += "<div class='preset-color' style='background-color:#0000ff' onclick=\"setPresetColor('#0000ff')\" title='Blue'></div>";
        html += "<div class='preset-color' style='background-color:#ffff00' onclick=\"setPresetColor('#ffff00')\" title='Yellow'></div>";
        html += "<div class='preset-color' style='background-color:#ff00ff' onclick=\"setPresetColor('#ff00ff')\" title='Magenta'></div>";
        html += "<div class='preset-color' style='background-color:#00ffff' onclick=\"setPresetColor('#00ffff')\" title='Cyan'></div>";
        html += "<div class='preset-color' style='background-color:#ffa500' onclick=\"setPresetColor('#ffa500')\" title='Orange'></div>";
        html += "</div>";
        html += "</div>";
        
        html += "<div class='control-group'>";
        html += "<h3>⚡ Animation Speed</h3>";
        html += "<label for='speed'>Speed: <span id='speed-value'>" + String(ledController->getSpeed()) + "</span></label>";
        html += "<input type='range' id='speed' class='slider' min='0' max='255' value='" + String(ledController->getSpeed()) + "' oninput='updateSpeed(this.value)'>";
        html += "</div>";
        
        html += "<div class='control-group'>";
        html += "<h3>🔢 LED Configuration</h3>";
        html += "<label for='num-leds'>Number of LEDs:</label>";
        html += "<input type='number' id='num-leds' min='1' max='500' value='" + String(ledController->getNumLeds()) + "'>";
        html += "<button onclick='updateNumLeds()' class='button'>Update LED Count</button>";
        html += "<p><small>Current: " + String(ledController->getNumLeds()) + " LEDs on GPIO pin " + String(ledController->getDataPin()) + "</small></p>";
        html += "</div>";
        
        html += "<div class='control-group'>";
        html += "<h3>🎮 Pattern Tests & Control</h3>";
        html += "<div class='grid'>";
        html += "<button onclick=\"testPattern('solid')\" class='button pattern-btn'>💎 Show Selected Color</button>";
        html += "<button onclick=\"testPattern('clock')\" class='button test-btn'>🕐 Clock Display</button>";
        html += "<button onclick=\"testPattern('rainbow')\" class='button test-btn'>🌈 Rainbow</button>";
        html += "<button onclick=\"testPattern('breathing')\" class='button test-btn'>💨 Breathing</button>";
        html += "<button onclick=\"testPattern('off')\" class='button'>⚫ Turn Off</button>";
        html += "</div>";
        html += "<div style='margin-top:15px;'>";
        html += "<button onclick='saveSettings()' class='button pattern-btn'>💾 Save Settings</button>";
        html += "</div>";
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
    html += "<a href='/led/mapping' class='button'>🗺️ LED Mapping</a>";
    html += "</div></body></html>";
    
    return html;
}

// LED mapping handlers (unchanged)
void WebServerManager::handleLEDMapping() {
    if (!ledController) {
        server.send(500, "text/plain", "LED controller not available");
        return;
    }
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>QlockThree LED Mapping</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:Arial,sans-serif;margin:40px;background:#f0f0f0}";
    html += ".container{background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
    html += "h1{color:#333;text-align:center}";
    html += ".mapping-group{margin:20px 0;padding:15px;background:#f8f9fa;border-radius:5px}";
    html += ".current-mapping{background:#d4edda;border-left:4px solid #28a745}";
    html += "label{display:block;margin-bottom:5px;font-weight:bold}";
    html += "select{width:100%;padding:8px;margin-bottom:10px;border:1px solid #ddd;border-radius:4px}";
    html += ".button{display:inline-block;padding:10px 20px;margin:5px;background:#007bff;color:white;text-decoration:none;border-radius:4px;border:none;cursor:pointer}";
    html += ".button:hover{background:#0056b3}";
    html += ".mapping-btn{background:#28a745}.mapping-btn:hover{background:#1e7e34}";
    html += ".info{margin:10px 0;padding:10px;background:#f8f9fa;border-left:4px solid #007bff}</style>";
    
    html += "<script>";
    html += "function setMapping() {";
    html += "  const mappingType = document.getElementById('mapping-select').value;";
    html += "  if (confirm('Change LED mapping to ' + document.getElementById('mapping-select').selectedOptions[0].text + '?')) {";
    html += "    fetch('/led/mapping/set', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'type=' + mappingType});";
    html += "    setTimeout(() => location.reload(), 1000);";
    html += "  }";
    html += "}";
    html += "</script>";
    
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>🗺️ QlockThree LED Mapping</h1>";
    
    // Current mapping info
    LEDMappingManager* mappingManager = ledController->getMappingManager();
    if (mappingManager) {
        html += "<div class='mapping-group current-mapping'>";
        html += "<h3>Current Mapping</h3>";
        html += "<div class='info'><strong>Name:</strong> " + String(mappingManager->getCurrentMappingName()) + "</div>";
        html += "<div class='info'><strong>ID:</strong> " + String(mappingManager->getCurrentMappingId()) + "</div>";
        html += "<div class='info'><strong>Description:</strong> " + String(mappingManager->getCurrentMappingDescription()) + "</div>";
        html += "<div class='info'><strong>LED Count:</strong> " + String(mappingManager->getCurrentMappingLEDCount()) + "</div>";
        html += "</div>";
        
        // Mapping selection
        html += "<div class='mapping-group'>";
        html += "<h3>Select LED Mapping</h3>";
        html += "<label for='mapping-select'>Available Mappings:</label>";
        html += "<select id='mapping-select'>";
        html += "<option value='0'" + String(mappingManager->getCurrentMappingType() == MappingType::MAPPING_45_GERMAN ? " selected" : "") + ">45-LED German Layout</option>";
        html += "<option value='1'" + String(mappingManager->getCurrentMappingType() == MappingType::MAPPING_110_GERMAN ? " selected" : "") + ">110-LED German Layout</option>";
        html += "</select>";
        html += "<button onclick='setMapping()' class='button mapping-btn'>Apply Mapping</button>";
        html += "</div>";
        
        html += "<div class='mapping-group'>";
        html += "<h3>ℹ️ Mapping Information</h3>";
        html += "<p><strong>45-LED German Layout:</strong> Compact design with 45 LEDs arranged in a smaller grid. Perfect for space-constrained installations.</p>";
        html += "<p><strong>110-LED German Layout:</strong> Standard 11×10 grid layout providing full German word clock functionality with all time expressions.</p>";
        html += "<p><strong>Note:</strong> Changing the mapping will automatically adjust the LED count to match the selected layout.</p>";
        html += "</div>";
    } else {
        html += "<div class='mapping-group'>";
        html += "<h3>❌ Mapping Manager Not Available</h3>";
        html += "<p>LED mapping manager is not initialized.</p>";
        html += "</div>";
    }
    
    html += "<br>";
    html += "<a href='/led' class='button'>← Back to LED Config</a>";
    html += "<a href='/' class='button'>🏠 Home</a>";
    html += "</div></body></html>";
    
    server.send(200, "text/html", html);
}

void WebServerManager::handleSetLEDMapping() {
    if (!ledController) {
        server.send(500, "text/plain", "LED controller not available");
        return;
    }
    
    if (server.hasArg("type")) {
        int mappingType = server.arg("type").toInt();
        
        switch (mappingType) {
            case 0:
                ledController->setMapping(MappingType::MAPPING_45_GERMAN);
                server.send(200, "text/plain", "Mapping changed to 45-LED German Layout");
                break;
            case 1:
                ledController->setMapping(MappingType::MAPPING_110_GERMAN);
                server.send(200, "text/plain", "Mapping changed to 110-LED German Layout");
                break;
            default:
                server.send(400, "text/plain", "Invalid mapping type");
                return;
        }
        
        Serial.printf("LED mapping changed via web interface to type %d\n", mappingType);
    } else {
        server.send(400, "text/plain", "Missing mapping type parameter");
    }
}
