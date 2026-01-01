#include "web_server_manager.h"
#include "wifi_manager_helper.h"
#include "auto_updater.h"
#include "led_controller.h"
#include "time_manager.h"
#include "birthday_manager.h"
#include "cloud_manager.h"
#include "config.h"
#include "web/web_assets.h"
#include <WiFi.h>

WebServerManager::WebServerManager(int port) : server(port), wifiManagerHelper(nullptr), autoUpdater(nullptr), ledController(nullptr), timeManager(nullptr),
    birthdayManager(nullptr), cloudManager(nullptr), debugModeEnabled(nullptr), debugHour(nullptr), debugMinute(nullptr) {
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
    // Static asset routes
    server.on("/css/common.css", [this]() {
        server.send_P(200, "text/css", (const char*)common_css_start, ASSET_SIZE(common_css));
    });
    server.on("/css/dark.css", [this]() {
        server.send_P(200, "text/css", (const char*)dark_css_start, ASSET_SIZE(dark_css));
    });
    server.on("/js/api.js", [this]() {
        server.send_P(200, "application/javascript", (const char*)api_js_start, ASSET_SIZE(api_js));
    });
    server.on("/js/utils.js", [this]() {
        server.send_P(200, "application/javascript", (const char*)utils_js_start, ASSET_SIZE(utils_js));
    });

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
                CRGB color = CRGB(r, g, b);
                ledController->setSolidColor(color);
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

    // Cloud endpoints
    server.on("/cloud", [this]() { handleCloudPage(); });
    server.on("/cloud/status", [this]() { handleCloudStatus(); });
    server.on("/cloud/pair/start", HTTP_POST, [this]() { handleCloudPairStart(); });
    server.on("/cloud/pair/stop", HTTP_POST, [this]() { handleCloudPairStop(); });
    server.on("/cloud/disconnect", HTTP_POST, [this]() { handleCloudDisconnect(); });
}

void WebServerManager::handleRoot() {
    server.send_P(200, "text/html", (const char*)index_html_start, ASSET_SIZE(index_html));
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

// Time configuration handlers
void WebServerManager::handleTimeConfig() {
    server.send_P(200, "text/html", (const char*)time_html_start, ASSET_SIZE(time_html));
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
    server.send_P(200, "text/html", (const char*)led_html_start, ASSET_SIZE(led_html));
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

        // Add mapping type and rotation
        LEDMappingManager* mappingManager = ledController->getMappingManager();
        if (mappingManager) {
            json += "\"mapping_type\":" + String((int)mappingManager->getCurrentMappingType()) + ",";
            json += "\"rotation\":" + String(mappingManager->getRotationDegrees()) + ",";
        }

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

// LED mapping handlers
void WebServerManager::handleLEDMapping() {
    server.send_P(200, "text/html", (const char*)led_mapping_html_start, ASSET_SIZE(led_mapping_html));
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
    server.send_P(200, "text/html", (const char*)dev_html_start, ASSET_SIZE(dev_html));
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

// Birthday handlers
void WebServerManager::handleBirthdayPage() {
    server.send_P(200, "text/html", (const char*)birthdays_html_start, ASSET_SIZE(birthdays_html));
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

// Cloud handlers
void WebServerManager::handleCloudPage() {
    server.send_P(200, "text/html", (const char*)cloud_html_start, ASSET_SIZE(cloud_html));
}

void WebServerManager::handleCloudStatus() {
    if (!cloudManager) {
        server.send(500, "application/json", "{\"error\":\"Cloud manager not available\"}");
        return;
    }
    server.send(200, "application/json", cloudManager->getStatusJSON());
}

void WebServerManager::handleCloudPairStart() {
    if (!cloudManager) {
        server.send(500, "application/json", "{\"success\":false,\"error\":\"Cloud manager not available\"}");
        return;
    }

    if (cloudManager->startPairing(CLOUD_API_URL)) {
        String response = "{\"success\":true,\"code\":\"" + cloudManager->getPairingCode() + "\"}";
        server.send(200, "application/json", response);
    } else {
        server.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to start pairing\"}");
    }
}

void WebServerManager::handleCloudPairStop() {
    if (!cloudManager) {
        server.send(500, "application/json", "{\"success\":false,\"error\":\"Cloud manager not available\"}");
        return;
    }

    cloudManager->stopPairing();
    server.send(200, "application/json", "{\"success\":true}");
}

void WebServerManager::handleCloudDisconnect() {
    if (!cloudManager) {
        server.send(500, "application/json", "{\"success\":false,\"error\":\"Cloud manager not available\"}");
        return;
    }

    cloudManager->disconnect();

    // Clear cloud settings
    CloudConfig config;
    config.begin();
    config.clear();

    server.send(200, "application/json", "{\"success\":true}");
}
