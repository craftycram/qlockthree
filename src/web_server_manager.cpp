#include "web_server_manager.h"
#include "wifi_manager_helper.h"
#include "auto_updater.h"
#include "config.h"
#include <WiFi.h>

WebServerManager::WebServerManager(int port) : server(port), wifiManagerHelper(nullptr), autoUpdater(nullptr) {
}

void WebServerManager::begin(WiFiManagerHelper* wifiHelper, AutoUpdater* updater) {
    wifiManagerHelper = wifiHelper;
    autoUpdater = updater;
    
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
    html += "<a href='javascript:location.reload()' class='button'>Refresh</a>";
    html += "<button onclick='checkUpdate()' class='button check-btn'>Check for Updates</button>";
    html += "<button id='update-btn' onclick='performUpdate()' class='button update-btn' style='display:" + String(updateAvailable ? "inline-block" : "none") + "'>Install Update</button>";
    html += "<button onclick='resetWiFi()' class='button danger-btn'>Reset WiFi</button>";
    html += "</div></body></html>";
    
    return html;
}
