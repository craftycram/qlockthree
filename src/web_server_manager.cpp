#include "web_server_manager.h"
#include "wifi_manager_helper.h"
#include "auto_updater.h"
#include "led_controller.h"
#include "time_manager.h"
#include "birthday_manager.h"
#include "config.h"
#include <WiFi.h>

WebServerManager::WebServerManager(int port) : server(port), wifiManagerHelper(nullptr), autoUpdater(nullptr), ledController(nullptr), timeManager(nullptr),
    birthdayManager(nullptr), debugModeEnabled(nullptr), debugHour(nullptr), debugMinute(nullptr) {
}

void WebServerManager::begin(WiFiManagerHelper* wifiHelper, AutoUpdater* updater, LEDController* ledCtrl, TimeManager* timeMgr,
                             bool* debugEnabled, int* debugH, int* debugM) {
    wifiManagerHelper = wifiHelper;
    autoUpdater = updater;
    ledController = ledCtrl;
    timeManager = timeMgr;
    debugModeEnabled = debugEnabled;
    debugHour = debugH;
    debugMinute = debugM;
    
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
    server.on("/led/rotation/set", HTTP_POST, [this]() { handleSetRotation(); });
    
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
        
        // LED count is now determined by mapping, not manually configured
        
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

    // Debug mode endpoints (hidden at /dev)
    server.on("/dev", [this]() { handleDevPage(); });
    server.on("/dev/status", [this]() { handleDevStatus(); });
    server.on("/dev/set", HTTP_POST, [this]() { handleDevSet(); });
    server.on("/dev/toggle", HTTP_POST, [this]() { handleDevToggle(); });
    server.on("/dev/reboot", HTTP_POST, [this]() { handleReboot(); });
    server.on("/dev/factory-reset", HTTP_POST, [this]() { handleFactoryReset(); });

    // Birthday endpoints
    server.on("/birthdays", [this]() { handleBirthdayPage(); });
    server.on("/birthdays/list", [this]() { handleBirthdayList(); });
    server.on("/birthdays/add", HTTP_POST, [this]() { handleBirthdayAdd(); });
    server.on("/birthdays/remove", HTTP_POST, [this]() { handleBirthdayRemove(); });
    server.on("/birthdays/mode", HTTP_POST, [this]() { handleBirthdayMode(); });
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
    html += "<title>qlockthree Status</title>";
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
    html += "<h1>qlockthree Status</h1>";
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
    html += "The device will restart and create a 'qlockthree-Setup' access point for configuration.";
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
    html += "<title>qlockthree Time Configuration</title>";
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
    html += "<h1>🕐 qlockthree Time Configuration</h1>";
    
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
    html += "<title>qlockthree LED Configuration</title>";
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
    
    // LED count is now determined by mapping, not manually configured
    
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
    html += "<h1>🌈 qlockthree LED Configuration</h1>";
    
    if (ledController) {
        html += "<div class='control-group'>";
        html += "<h3>💡 Brightness Control</h3>";
        html += "<label for='brightness'>Brightness: <span id='brightness-value'>" + String(ledController->getBrightness()) + "</span></label>";
        html += "<input type='range' id='brightness' class='slider' min='0' max='255' value='" + String(ledController->getBrightness()) + "' oninput='updateBrightness(this.value)'>";
        html += "</div>";
        
        // ENHANCED: Color Configuration Section
        html += "<div class='control-group'>";
        html += "<h3>🎨 Color Configuration</h3>";
        html += "<p><strong>📝 Instructions:</strong> Select a color below, then click '💎 Show Selected Color' to see it on your qlockthree. The main loop shows time in clock mode, so you need to switch to solid color mode to test colors.</p>";
        
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
        html += "<h3>🔢 LED Information</h3>";
        html += "<p><strong>LED Count:</strong> " + String(ledController->getNumLeds()) + " LEDs</p>";
        html += "<p><strong>Data Pin:</strong> GPIO " + String(ledController->getDataPin()) + "</p>";
        html += "<p><small><em>Note: LED count is automatically set by the selected mapping. Use <a href='/led/mapping'>LED Mapping</a> to change the layout.</em></small></p>";
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
    html += "<title>qlockthree LED Mapping</title>";
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
    html += "function setRotation() {";
    html += "  const rotation = document.getElementById('rotation-select').value;";
    html += "  fetch('/led/rotation/set', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'degrees=' + rotation});";
    html += "  setTimeout(() => location.reload(), 500);";
    html += "}";
    html += "</script>";
    
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>🗺️ qlockthree LED Mapping</h1>";
    
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
        html += "<option value='0'" + String(mappingManager->getCurrentMappingType() == MappingType::MAPPING_45_GERMAN ? " selected" : "") + ">45cm German</option>";
        html += "<option value='1'" + String(mappingManager->getCurrentMappingType() == MappingType::MAPPING_45BW_GERMAN ? " selected" : "") + ">45cm Swabian (BW)</option>";
        html += "<option value='2'" + String(mappingManager->getCurrentMappingType() == MappingType::MAPPING_110_GERMAN ? " selected" : "") + ">110-LED German Layout</option>";
        html += "</select>";
        html += "<button onclick='setMapping()' class='button mapping-btn'>Apply Mapping</button>";
        html += "</div>";

        // Rotation selection
        uint16_t currentRotation = mappingManager->getRotationDegrees();
        html += "<div class='mapping-group'>";
        html += "<h3>Display Rotation</h3>";
        html += "<div class='info'><strong>Current Rotation:</strong> " + String(currentRotation) + "&deg;</div>";
        html += "<label for='rotation-select'>Rotate Clock Face:</label>";
        html += "<select id='rotation-select'>";
        html += "<option value='0'" + String(currentRotation == 0 ? " selected" : "") + ">0&deg; (Normal)</option>";
        html += "<option value='90'" + String(currentRotation == 90 ? " selected" : "") + ">90&deg; Clockwise</option>";
        html += "<option value='180'" + String(currentRotation == 180 ? " selected" : "") + ">180&deg; (Upside Down)</option>";
        html += "<option value='270'" + String(currentRotation == 270 ? " selected" : "") + ">270&deg; Clockwise</option>";
        html += "</select>";
        html += "<button onclick='setRotation()' class='button mapping-btn'>Apply Rotation</button>";
        html += "<p style='margin-top:10px;color:#666;font-size:0.9em'>Use this if your clock is mounted rotated from its default orientation.</p>";
        html += "</div>";

        html += "<div class='mapping-group'>";
        html += "<h3>Mapping Information</h3>";
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
                server.send(200, "text/plain", "Mapping changed to 45cm German");
                break;
            case 1:
                ledController->setMapping(MappingType::MAPPING_45BW_GERMAN);
                server.send(200, "text/plain", "Mapping changed to 45cm Swabian (BW)");
                break;
            case 2:
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

void WebServerManager::handleSetRotation() {
    if (!ledController) {
        server.send(500, "text/plain", "LED controller not available");
        return;
    }

    LEDMappingManager* mappingManager = ledController->getMappingManager();
    if (!mappingManager) {
        server.send(500, "text/plain", "Mapping manager not available");
        return;
    }

    if (server.hasArg("degrees")) {
        int degrees = server.arg("degrees").toInt();

        if (degrees == 0 || degrees == 90 || degrees == 180 || degrees == 270) {
            mappingManager->setRotationDegrees(degrees);
            mappingManager->saveRotation();
            server.send(200, "text/plain", "Rotation set to " + String(degrees) + " degrees");
            Serial.printf("Rotation changed via web interface to %d degrees\n", degrees);
        } else {
            server.send(400, "text/plain", "Invalid rotation value. Use 0, 90, 180, or 270.");
        }
    } else {
        server.send(400, "text/plain", "Missing degrees parameter");
    }
}

// Debug mode handlers
void WebServerManager::handleDevPage() {
    server.send(200, "text/html", getDevPageHTML());
}

void WebServerManager::handleDevStatus() {
    server.send(200, "application/json", getDevStatusJSON());
}

void WebServerManager::handleDevSet() {
    if (!debugModeEnabled || !debugHour || !debugMinute) {
        server.send(500, "text/plain", "Debug mode not available");
        return;
    }

    if (server.hasArg("hour") && server.hasArg("minute")) {
        int hour = server.arg("hour").toInt();
        int minute = server.arg("minute").toInt();

        if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59) {
            *debugHour = hour;
            *debugMinute = minute;
            Serial.printf("Debug time set to %02d:%02d\n", hour, minute);
            server.send(200, "text/plain", "Time set");
        } else {
            server.send(400, "text/plain", "Invalid time values");
        }
    } else {
        server.send(400, "text/plain", "Missing hour or minute parameter");
    }
}

void WebServerManager::handleDevToggle() {
    if (!debugModeEnabled) {
        server.send(500, "text/plain", "Debug mode not available");
        return;
    }

    *debugModeEnabled = !(*debugModeEnabled);
    Serial.printf("Debug mode %s\n", *debugModeEnabled ? "enabled" : "disabled");
    server.send(200, "text/plain", *debugModeEnabled ? "enabled" : "disabled");
}

void WebServerManager::handleReboot() {
    Serial.println("Reboot requested via web interface");
    server.send(200, "text/plain", "Rebooting...");
    delay(500);
    ESP.restart();
}

void WebServerManager::handleFactoryReset() {
    Serial.println("Factory reset requested via web interface");

    // Clear all preference namespaces
    Preferences prefs;

    prefs.begin("led_mapping", false);
    prefs.clear();
    prefs.end();

    prefs.begin("led_config", false);
    prefs.clear();
    prefs.end();

    prefs.begin("time_manager", false);
    prefs.clear();
    prefs.end();

    prefs.begin("qlockthree", false);
    prefs.clear();
    prefs.end();

    Serial.println("All settings cleared");
    server.send(200, "text/plain", "Factory reset complete. Rebooting...");
    delay(500);
    ESP.restart();
}

String WebServerManager::getDevStatusJSON() {
    String json = "{";
    json += "\"enabled\":" + String(debugModeEnabled && *debugModeEnabled ? "true" : "false") + ",";
    json += "\"hour\":" + String(debugHour ? *debugHour : 0) + ",";
    json += "\"minute\":" + String(debugMinute ? *debugMinute : 0) + ",";

    // Get real time
    if (timeManager) {
        struct tm currentTime = timeManager->getCurrentTime();
        json += "\"realHour\":" + String(currentTime.tm_hour) + ",";
        json += "\"realMinute\":" + String(currentTime.tm_min);
    } else {
        json += "\"realHour\":0,\"realMinute\":0";
    }

    json += "}";
    return json;
}

String WebServerManager::getDevPageHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Debug Mode</title>";
    html += "<style>";
    html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; ";
    html += "background: #1a1a2e; color: #eee; margin: 0; padding: 20px; }";
    html += ".container { max-width: 400px; margin: 0 auto; }";
    html += "h1 { color: #ff6b6b; font-size: 1.5em; }";
    html += ".status { padding: 15px; border-radius: 8px; margin: 15px 0; font-size: 1.2em; text-align: center; }";
    html += ".status.enabled { background: #2d5a27; border: 2px solid #4ade80; }";
    html += ".status.disabled { background: #5a2727; border: 2px solid #f87171; }";
    html += ".group { background: #16213e; padding: 15px; border-radius: 8px; margin: 15px 0; }";
    html += "label { display: block; margin: 10px 0 5px; color: #a0a0a0; }";
    html += "input[type='number'] { width: 80px; padding: 10px; font-size: 1.2em; border: 1px solid #444; ";
    html += "border-radius: 4px; background: #0f0f23; color: #fff; text-align: center; }";
    html += ".time-input { display: flex; align-items: center; gap: 10px; justify-content: center; }";
    html += ".time-input span { font-size: 1.5em; color: #888; }";
    html += ".button { display: inline-block; padding: 12px 24px; margin: 5px; border: none; ";
    html += "border-radius: 6px; font-size: 1em; cursor: pointer; text-decoration: none; }";
    html += ".button.primary { background: #4361ee; color: white; }";
    html += ".button.toggle { background: #f72585; color: white; }";
    html += ".button.back { background: #444; color: white; }";
    html += ".button.warning { background: #f59e0b; color: white; }";
    html += ".button.danger { background: #dc2626; color: white; }";
    html += ".buttons { text-align: center; margin-top: 20px; }";
    html += ".info { margin-top: 15px; padding: 10px; background: #0f0f23; border-radius: 4px; text-align: center; }";
    html += ".info .label { color: #888; font-size: 0.9em; }";
    html += ".info .value { font-size: 1.3em; font-family: monospace; }";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>Debug Mode</h1>";

    // Status indicator
    bool enabled = debugModeEnabled && *debugModeEnabled;
    html += "<div id='status' class='status " + String(enabled ? "enabled" : "disabled") + "'>";
    html += enabled ? "DEBUG ENABLED" : "DEBUG DISABLED";
    html += "</div>";

    // Time input
    html += "<div class='group'>";
    html += "<label>Set Debug Time:</label>";
    html += "<div class='time-input'>";
    html += "<input type='number' id='hour' min='0' max='23' value='" + String(debugHour ? *debugHour : 12) + "'>";
    html += "<span>:</span>";
    html += "<input type='number' id='minute' min='0' max='59' value='" + String(debugMinute ? *debugMinute : 0) + "'>";
    html += "</div>";
    html += "</div>";

    // Info displays
    html += "<div class='group'>";
    html += "<div class='info'>";
    html += "<div class='label'>Debug Display</div>";
    html += "<div class='value' id='debugTime'>--:--</div>";
    html += "</div>";
    html += "<div class='info'>";
    html += "<div class='label'>Real Time</div>";
    html += "<div class='value' id='realTime'>--:--</div>";
    html += "</div>";
    html += "</div>";

    // Buttons
    html += "<div class='buttons'>";
    html += "<button class='button primary' onclick='setTime()'>Set Time</button>";
    html += "<button class='button toggle' onclick='toggle()' id='toggleBtn'>" + String(enabled ? "Disable" : "Enable") + "</button>";
    html += "</div>";

    // System controls
    html += "<div class='group' style='margin-top:30px'>";
    html += "<label style='text-align:center'>System Controls:</label>";
    html += "<div class='buttons'>";
    html += "<button class='button warning' onclick='reboot()'>Reboot</button>";
    html += "<button class='button danger' onclick='factoryReset()'>Factory Reset</button>";
    html += "</div>";
    html += "</div>";

    html += "<div class='buttons'>";
    html += "<a href='/' class='button back'>Back to Home</a>";
    html += "</div>";

    html += "</div>";

    // JavaScript
    html += "<script>";
    html += "function updateStatus() {";
    html += "  fetch('/dev/status').then(r=>r.json()).then(d=>{";
    html += "    document.getElementById('debugTime').textContent = ";
    html += "      String(d.hour).padStart(2,'0')+':'+String(d.minute).padStart(2,'0');";
    html += "    document.getElementById('realTime').textContent = ";
    html += "      String(d.realHour).padStart(2,'0')+':'+String(d.realMinute).padStart(2,'0');";
    html += "    var s=document.getElementById('status');";
    html += "    s.className='status '+(d.enabled?'enabled':'disabled');";
    html += "    s.textContent=d.enabled?'DEBUG ENABLED':'DEBUG DISABLED';";
    html += "    document.getElementById('toggleBtn').textContent=d.enabled?'Disable':'Enable';";
    html += "  });";
    html += "}";
    html += "function setTime() {";
    html += "  var h=document.getElementById('hour').value;";
    html += "  var m=document.getElementById('minute').value;";
    html += "  fetch('/dev/set',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},";
    html += "    body:'hour='+h+'&minute='+m}).then(()=>updateStatus());";
    html += "}";
    html += "function toggle() {";
    html += "  fetch('/dev/toggle',{method:'POST'}).then(()=>updateStatus());";
    html += "}";
    html += "function reboot() {";
    html += "  if(confirm('Reboot the clock?')) {";
    html += "    fetch('/dev/reboot',{method:'POST'});";
    html += "    document.body.innerHTML='<div style=\"text-align:center;padding:50px;color:#fff\"><h2>Rebooting...</h2><p>Please wait</p></div>';";
    html += "  }";
    html += "}";
    html += "function factoryReset() {";
    html += "  if(confirm('WARNING: This will delete ALL settings including WiFi credentials.\\n\\nAre you sure you want to factory reset?')) {";
    html += "    fetch('/dev/factory-reset',{method:'POST'});";
    html += "    document.body.innerHTML='<div style=\"text-align:center;padding:50px;color:#fff\"><h2>Factory Reset...</h2><p>All settings cleared. Rebooting...</p></div>';";
    html += "  }";
    html += "}";
    html += "updateStatus();";
    html += "setInterval(updateStatus,1000);";
    html += "</script>";

    html += "</body></html>";
    return html;
}

// Birthday handlers
void WebServerManager::handleBirthdayPage() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Birthday Settings</title>";
    html += "<style>";
    html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; ";
    html += "background: #f0f0f0; margin: 0; padding: 20px; }";
    html += ".container { max-width: 500px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
    html += "h1 { color: #333; font-size: 1.5em; text-align: center; }";
    html += ".group { background: #f8f9fa; padding: 15px; border-radius: 8px; margin: 15px 0; }";
    html += "h3 { margin-top: 0; color: #555; }";
    html += ".radio-group { margin: 10px 0; }";
    html += ".radio-group label { display: block; padding: 8px; margin: 5px 0; background: #fff; border: 1px solid #ddd; border-radius: 4px; cursor: pointer; }";
    html += ".radio-group label:hover { background: #f0f0f0; }";
    html += ".radio-group input[type='radio'] { margin-right: 10px; }";
    html += ".birthday-list { list-style: none; padding: 0; margin: 10px 0; }";
    html += ".birthday-item { display: flex; justify-content: space-between; align-items: center; padding: 10px; background: #fff; border: 1px solid #ddd; border-radius: 4px; margin: 5px 0; }";
    html += ".delete-btn { background: #dc3545; color: white; border: none; padding: 5px 10px; border-radius: 4px; cursor: pointer; }";
    html += ".delete-btn:hover { background: #c82333; }";
    html += ".add-form { display: flex; gap: 10px; align-items: center; flex-wrap: wrap; }";
    html += "select, .button { padding: 10px; border: 1px solid #ddd; border-radius: 4px; font-size: 1em; }";
    html += ".button { background: #28a745; color: white; border: none; cursor: pointer; }";
    html += ".button:hover { background: #218838; }";
    html += ".button.primary { background: #007bff; }";
    html += ".button.primary:hover { background: #0056b3; }";
    html += ".button.back { background: #6c757d; }";
    html += ".buttons { text-align: center; margin-top: 20px; }";
    html += ".empty-msg { color: #888; font-style: italic; text-align: center; padding: 20px; }";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>Birthday Settings</h1>";

    // Display mode selection
    html += "<div class='group'>";
    html += "<h3>Display Mode</h3>";
    html += "<div class='radio-group' id='modeGroup'>";

    int currentMode = birthdayManager ? birthdayManager->getDisplayMode() : 2;
    html += "<label><input type='radio' name='mode' value='0'" + String(currentMode == 0 ? " checked" : "") + "> Replace - Show only HAPPY BIRTHDAY</label>";
    html += "<label><input type='radio' name='mode' value='1'" + String(currentMode == 1 ? " checked" : "") + "> Alternate - Switch between time and HAPPY BIRTHDAY</label>";
    html += "<label><input type='radio' name='mode' value='2'" + String(currentMode == 2 ? " checked" : "") + "> Overlay - Show HAPPY BIRTHDAY with time</label>";
    html += "</div>";
    html += "<button class='button primary' onclick='saveMode()'>Save Mode</button>";
    html += "</div>";

    // Birthday list
    html += "<div class='group'>";
    html += "<h3>Birthday Dates</h3>";
    html += "<ul class='birthday-list' id='birthdayList'>";
    html += "<li class='empty-msg'>Loading...</li>";
    html += "</ul>";
    html += "</div>";

    // Add birthday form
    html += "<div class='group'>";
    html += "<h3>Add Birthday</h3>";
    html += "<div class='add-form'>";
    html += "<select id='month'>";
    const char* months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
    for (int i = 0; i < 12; i++) {
        html += "<option value='" + String(i + 1) + "'>" + months[i] + "</option>";
    }
    html += "</select>";
    html += "<select id='day'>";
    for (int i = 1; i <= 31; i++) {
        html += "<option value='" + String(i) + "'>" + String(i) + "</option>";
    }
    html += "</select>";
    html += "<button class='button' onclick='addBirthday()'>Add</button>";
    html += "</div>";
    html += "</div>";

    html += "<div class='buttons'>";
    html += "<a href='/' class='button back'>Back to Home</a>";
    html += "</div>";

    html += "</div>";

    // JavaScript
    html += "<script>";
    html += "const months = ['January','February','March','April','May','June','July','August','September','October','November','December'];";
    html += "function loadBirthdays() {";
    html += "  fetch('/birthdays/list').then(r=>r.json()).then(data => {";
    html += "    const list = document.getElementById('birthdayList');";
    html += "    if (data.dates.length === 0) {";
    html += "      list.innerHTML = '<li class=\"empty-msg\">No birthdays configured</li>';";
    html += "    } else {";
    html += "      list.innerHTML = '';";
    html += "      data.dates.forEach(d => {";
    html += "        const li = document.createElement('li');";
    html += "        li.className = 'birthday-item';";
    html += "        li.innerHTML = '<span>' + months[d.month-1] + ' ' + d.day + '</span>' +";
    html += "          '<button class=\"delete-btn\" onclick=\"removeBirthday(' + d.month + ',' + d.day + ')\">Delete</button>';";
    html += "        list.appendChild(li);";
    html += "      });";
    html += "    }";
    html += "    document.querySelectorAll('input[name=\"mode\"]').forEach(r => {";
    html += "      r.checked = (parseInt(r.value) === data.mode);";
    html += "    });";
    html += "  });";
    html += "}";
    html += "function saveMode() {";
    html += "  const mode = document.querySelector('input[name=\"mode\"]:checked').value;";
    html += "  fetch('/birthdays/mode', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'mode='+mode})";
    html += "    .then(() => { alert('Mode saved!'); });";
    html += "}";
    html += "function addBirthday() {";
    html += "  const month = document.getElementById('month').value;";
    html += "  const day = document.getElementById('day').value;";
    html += "  fetch('/birthdays/add', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'month='+month+'&day='+day})";
    html += "    .then(r => r.text()).then(msg => { alert(msg); loadBirthdays(); });";
    html += "}";
    html += "function removeBirthday(month, day) {";
    html += "  if (confirm('Remove this birthday?')) {";
    html += "    fetch('/birthdays/remove', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'month='+month+'&day='+day})";
    html += "      .then(() => loadBirthdays());";
    html += "  }";
    html += "}";
    html += "loadBirthdays();";
    html += "</script>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}

void WebServerManager::handleBirthdayList() {
    if (!birthdayManager) {
        server.send(500, "application/json", "{\"error\":\"Birthday manager not available\"}");
        return;
    }
    server.send(200, "application/json", birthdayManager->getBirthdaysJSON());
}

void WebServerManager::handleBirthdayAdd() {
    if (!birthdayManager) {
        server.send(500, "text/plain", "Birthday manager not available");
        return;
    }

    if (server.hasArg("month") && server.hasArg("day")) {
        int month = server.arg("month").toInt();
        int day = server.arg("day").toInt();

        if (birthdayManager->addBirthday(month, day)) {
            birthdayManager->save();
            server.send(200, "text/plain", "Birthday added!");
        } else {
            server.send(400, "text/plain", "Could not add birthday (may already exist or limit reached)");
        }
    } else {
        server.send(400, "text/plain", "Missing month or day parameter");
    }
}

void WebServerManager::handleBirthdayRemove() {
    if (!birthdayManager) {
        server.send(500, "text/plain", "Birthday manager not available");
        return;
    }

    if (server.hasArg("month") && server.hasArg("day")) {
        int month = server.arg("month").toInt();
        int day = server.arg("day").toInt();

        if (birthdayManager->removeBirthday(month, day)) {
            birthdayManager->save();
            server.send(200, "text/plain", "Birthday removed");
        } else {
            server.send(400, "text/plain", "Birthday not found");
        }
    } else {
        server.send(400, "text/plain", "Missing month or day parameter");
    }
}

void WebServerManager::handleBirthdayMode() {
    if (!birthdayManager) {
        server.send(500, "text/plain", "Birthday manager not available");
        return;
    }

    if (server.hasArg("mode")) {
        int mode = server.arg("mode").toInt();
        if (mode >= 0 && mode <= 2) {
            birthdayManager->setDisplayMode(static_cast<BirthdayManager::DisplayMode>(mode));
            birthdayManager->save();
            server.send(200, "text/plain", "Mode saved");
        } else {
            server.send(400, "text/plain", "Invalid mode");
        }
    } else {
        server.send(400, "text/plain", "Missing mode parameter");
    }
}
