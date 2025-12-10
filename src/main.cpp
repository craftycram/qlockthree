#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config.h"

// Create objects
WebServer server(WEB_SERVER_PORT);
WiFiManager wifiManager;
Preferences preferences;

// WiFi configuration variables
String savedSSID = "";
String savedPassword = "";
bool configModeActive = false;

// Update tracking variables
unsigned long lastUpdateCheck = 0;
bool updateAvailable = false;
String latestVersion = "";
String downloadUrl = "";

// Function declarations
int myFunction(int, int);
void setupWiFi();
void setupWiFiManager();
void setupOTA();
void setupWebServer();
String getStatusHTML();
String getStatusJSON();
void checkForUpdates();
bool performUpdate(String url);
String compareVersions(String current, String latest);
void handleManualUpdate();
void handleWiFiReset();
void saveWiFiCallback();
void configModeCallback(WiFiManager *myWiFiManager);
void loadWiFiConfig();
void saveWiFiConfig(String ssid, String password);

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("Starting QlockThree with WiFi Portal and OTA...");
  
  // Initialize preferences
  preferences.begin("qlockthree", false);
  
  // Load saved WiFi configuration
  loadWiFiConfig();
  
  // Setup WiFi connection with manager
  setupWiFi();
  
  // Only setup other services if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    // Setup OTA updates
    setupOTA();
    
    // Setup web server
    setupWebServer();
    
    // Initial update check
    checkForUpdates();
    
    Serial.println("Setup complete!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: ");
    Serial.println(OTA_HOSTNAME);
    Serial.print("Current Version: ");
    Serial.println(CURRENT_VERSION);
  } else {
    Serial.println("WiFi configuration mode active - connect to QlockThree-Setup");
  }
}

void loop() {
  // Handle WiFi Manager
  if (configModeActive) {
    wifiManager.process();
    return;
  }
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting reconnection...");
    setupWiFi();
    return;
  }
  
  // Handle OTA updates
  ArduinoOTA.handle();
  
  // Handle web server
  server.handleClient();
  
  // Check for updates periodically
  if (millis() - lastUpdateCheck > UPDATE_CHECK_INTERVAL) {
    checkForUpdates();
    lastUpdateCheck = millis();
  }
  
  // Add your main application code here
  
  // Small delay to prevent watchdog issues
  delay(10);
}

void loadWiFiConfig() {
  savedSSID = preferences.getString("wifi_ssid", "");
  savedPassword = preferences.getString("wifi_password", "");
  
  Serial.println("Loaded WiFi config:");
  Serial.println("SSID: " + savedSSID);
  Serial.println("Password: [" + String(savedPassword.length() > 0 ? "SAVED" : "EMPTY") + "]");
}

void saveWiFiConfig(String ssid, String password) {
  preferences.putString("wifi_ssid", ssid);
  preferences.putString("wifi_password", password);
  savedSSID = ssid;
  savedPassword = password;
  Serial.println("WiFi config saved to NVS");
}

void setupWiFi() {
  // Check if we have saved credentials and hardcoded values are empty
  String ssidToUse = (strlen(WIFI_SSID) == 0) ? savedSSID : String(WIFI_SSID);
  String passToUse = (strlen(WIFI_PASSWORD) == 0) ? savedPassword : String(WIFI_PASSWORD);
  
  if (ssidToUse.length() > 0) {
    // Try to connect with saved/configured credentials
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(OTA_HOSTNAME);
    WiFi.begin(ssidToUse.c_str(), passToUse.c_str());
    
    Serial.print("Connecting to WiFi: " + ssidToUse);
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected successfully!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      configModeActive = false;
      
      // Start mDNS
      if (!MDNS.begin(OTA_HOSTNAME)) {
        Serial.println("Error setting up MDNS responder!");
      } else {
        Serial.println("mDNS responder started");
      }
      return;
    }
  }
  
  // If we get here, WiFi connection failed or no credentials available
  Serial.println("Starting WiFi configuration portal...");
  setupWiFiManager();
}

void setupWiFiManager() {
  configModeActive = true;
  
  // Reset WiFi settings for fresh start
  WiFi.disconnect(true);
  
  // Set up WiFiManager
  wifiManager.setSaveConfigCallback(saveWiFiCallback);
  wifiManager.setAPCallback(configModeCallback);
  
  // Configure WiFiManager
  wifiManager.setConfigPortalTimeout(0); // Never timeout
  wifiManager.setConnectTimeout(30); // 30 second timeout for connecting
  wifiManager.setDebugOutput(true);
  
  // Customize portal
  wifiManager.setTitle("QlockThree WiFi Setup");
  wifiManager.setDarkMode(true);
  
  // Add custom parameters
  WiFiManagerParameter custom_hostname("hostname", "Device Hostname", OTA_HOSTNAME, 20, "readonly");
  WiFiManagerParameter custom_version("version", "Firmware Version", CURRENT_VERSION, 20, "readonly");
  
  wifiManager.addParameter(&custom_hostname);
  wifiManager.addParameter(&custom_version);
  
  // Start configuration portal
  String apPassword = (strlen(AP_PASSWORD) > 0) ? AP_PASSWORD : "";
  
  if (!wifiManager.startConfigPortal(AP_SSID, apPassword.c_str())) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
  }
  
  // If we get here, connection was successful
  Serial.println("WiFi connected via portal!");
  configModeActive = false;
}

void saveWiFiCallback() {
  Serial.println("WiFi configuration saved via portal");
  
  // Save the credentials
  String newSSID = WiFi.SSID();
  String newPassword = WiFi.psk();
  
  if (newSSID.length() > 0) {
    saveWiFiConfig(newSSID, newPassword);
  }
}

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered WiFi config mode");
  Serial.print("AP SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Connect to this AP and go to http://192.168.4.1 to configure WiFi");
}

void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  
  // Optional: Set OTA password for security
  // ArduinoOTA.setPassword(OTA_PASSWORD);
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void setupWebServer() {
  // Root page with status information
  server.on("/", []() {
    server.send(200, "text/html", getStatusHTML());
  });
  
  // API endpoint for JSON status
  server.on("/status", []() {
    server.send(200, "application/json", getStatusJSON());
  });
  
  // Manual update trigger
  server.on("/update", HTTP_POST, handleManualUpdate);
  
  // Check for updates endpoint
  server.on("/check-update", []() {
    checkForUpdates();
    String json = "{";
    json += "\"current_version\":\"" + String(CURRENT_VERSION) + "\",";
    json += "\"latest_version\":\"" + latestVersion + "\",";
    json += "\"update_available\":" + String(updateAvailable ? "true" : "false") + ",";
    json += "\"download_url\":\"" + downloadUrl + "\"";
    json += "}";
    server.send(200, "application/json", json);
  });
  
  // WiFi reset endpoint
  server.on("/wifi-reset", HTTP_POST, handleWiFiReset);
  
  // Start server
  server.begin();
  Serial.println("Web server started");
  
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", WEB_SERVER_PORT);
}

void handleWiFiReset() {
  Serial.println("WiFi reset requested");
  
  // Clear saved WiFi credentials
  preferences.remove("wifi_ssid");
  preferences.remove("wifi_password");
  savedSSID = "";
  savedPassword = "";
  
  server.send(200, "text/plain", "WiFi settings cleared. Device will restart and enter configuration mode.");
  delay(2000);
  ESP.restart();
}

void checkForUpdates() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping update check");
    return;
  }
  
  Serial.println("Checking for updates...");
  
  HTTPClient http;
  http.begin(UPDATE_URL);
  http.addHeader("User-Agent", "QlockThree-ESP32");
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    // Parse JSON response
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      latestVersion = doc["tag_name"].as<String>();
      
      // Remove 'v' prefix if present
      if (latestVersion.startsWith("v")) {
        latestVersion = latestVersion.substring(1);
      }
      
      Serial.print("Latest version: ");
      Serial.println(latestVersion);
      Serial.print("Current version: ");
      Serial.println(CURRENT_VERSION);
      
      // Compare versions
      String comparison = compareVersions(CURRENT_VERSION, latestVersion);
      
      if (comparison == "outdated") {
        updateAvailable = true;
        
        // Find download URL for binary
        JsonArray assets = doc["assets"];
        for (JsonObject asset : assets) {
          String name = asset["name"].as<String>();
          if (name.endsWith(".bin") || name.endsWith(".firmware")) {
            downloadUrl = asset["browser_download_url"].as<String>();
            break;
          }
        }
        
        Serial.println("Update available! Download URL: " + downloadUrl);
        
        // Optionally auto-update (uncomment next line for automatic updates)
        // performUpdate(downloadUrl);
        
      } else {
        updateAvailable = false;
        Serial.println("Firmware is up to date");
      }
    } else {
      Serial.println("Failed to parse JSON response");
    }
  } else {
    Serial.printf("HTTP GET failed with code: %d\n", httpCode);
  }
  
  http.end();
}

bool performUpdate(String url) {
  if (url.length() == 0) {
    Serial.println("No download URL available");
    return false;
  }
  
  Serial.println("Starting firmware update from: " + url);
  
  HTTPClient http;
  http.begin(url);
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    
    if (contentLength > 0) {
      bool canBegin = Update.begin(contentLength);
      
      if (canBegin) {
        WiFiClient* client = http.getStreamPtr();
        
        Serial.println("Starting update...");
        size_t written = Update.writeStream(*client);
        
        if (written == contentLength) {
          Serial.println("Update successful!");
        } else {
          Serial.printf("Update failed. Expected %d bytes, got %d bytes\n", contentLength, written);
        }
        
        if (Update.end()) {
          if (Update.isFinished()) {
            Serial.println("Update finished. Restarting...");
            delay(1000);
            ESP.restart();
            return true;
          } else {
            Serial.println("Update not finished");
          }
        } else {
          Serial.printf("Update error: %s\n", Update.errorString());
        }
      } else {
        Serial.println("Not enough space for update");
      }
    } else {
      Serial.println("Content length is 0");
    }
  } else {
    Serial.printf("HTTP GET failed with code: %d\n", httpCode);
  }
  
  http.end();
  return false;
}

String compareVersions(String current, String latest) {
  // Simple version comparison (assumes semantic versioning: x.y.z)
  // Returns "outdated", "current", or "newer"
  
  if (current == latest) return "current";
  
  // Parse version numbers
  int currentParts[3] = {0, 0, 0};
  int latestParts[3] = {0, 0, 0};
  
  // Parse current version
  int partIndex = 0;
  String temp = "";
  for (int i = 0; i <= current.length() && partIndex < 3; i++) {
    if (i == current.length() || current.charAt(i) == '.') {
      currentParts[partIndex++] = temp.toInt();
      temp = "";
    } else {
      temp += current.charAt(i);
    }
  }
  
  // Parse latest version
  partIndex = 0;
  temp = "";
  for (int i = 0; i <= latest.length() && partIndex < 3; i++) {
    if (i == latest.length() || latest.charAt(i) == '.') {
      latestParts[partIndex++] = temp.toInt();
      temp = "";
    } else {
      temp += latest.charAt(i);
    }
  }
  
  // Compare versions
  for (int i = 0; i < 3; i++) {
    if (latestParts[i] > currentParts[i]) return "outdated";
    if (latestParts[i] < currentParts[i]) return "newer";
  }
  
  return "current";
}

void handleManualUpdate() {
  if (updateAvailable && downloadUrl.length() > 0) {
    server.send(200, "text/plain", "Starting update...");
    delay(1000);
    performUpdate(downloadUrl);
  } else {
    server.send(400, "text/plain", "No update available");
  }
}

String getStatusJSON() {
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
  json += "\"latest_version\":\"" + latestVersion + "\",";
  json += "\"update_available\":" + String(updateAvailable ? "true" : "false");
  json += "}";
  return json;
}

String getStatusHTML() {
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
  html += "<div class='info'><strong>Latest Version:</strong> <span id='latest-version'>" + (latestVersion.length() > 0 ? latestVersion : "Checking...") + "</span></div>";
  
  html += "<div id='update-info' class='" + String(updateAvailable ? "update" : "info") + "'>";
  if (updateAvailable) {
    html += "<strong>Update Available!</strong> Version " + latestVersion + " is ready for installation.";
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

int myFunction(int x, int y) {
  // Your custom function implementation
  return x + y;
}
