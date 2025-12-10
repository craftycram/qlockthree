#include "auto_updater.h"
#include <Update.h>

AutoUpdater::AutoUpdater() : updateAvailable(false), lastUpdateCheck(0) {
}

void AutoUpdater::begin(const char* githubRepo, const char* currentVersion, unsigned long checkInterval) {
    githubUpdateUrl = String("https://api.github.com/repos/") + githubRepo + "/releases/latest";
    this->currentVersion = currentVersion;
    updateCheckInterval = checkInterval;
    
    Serial.println("Auto-updater initialized:");
    Serial.println("GitHub URL: " + githubUpdateUrl);
    Serial.println("Current Version: " + this->currentVersion);
}

void AutoUpdater::checkForUpdates() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, skipping update check");
        return;
    }
    
    // Check if enough time has passed
    if (millis() - lastUpdateCheck < updateCheckInterval) {
        return;
    }
    
    Serial.println("Checking for updates...");
    lastUpdateCheck = millis();
    
    WiFiClientSecure client;
    client.setInsecure(); // Skip SSL certificate verification for GitHub API
    
    HTTPClient http;
    http.begin(client, githubUpdateUrl);
    http.addHeader("User-Agent", "QlockThree-ESP32");
    http.setTimeout(15000); // 15 second timeout
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // Parse JSON response
        JsonDocument doc;
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
            Serial.println(currentVersion);
            
            // Compare versions
            String comparison = compareVersions(currentVersion, latestVersion);
            
            if (comparison == "outdated") {
                updateAvailable = true;
                
                // Find download URL for main firmware binary (not bootloader or partitions)
                JsonArray assets = doc["assets"];
                downloadUrl = "";
                for (JsonObject asset : assets) {
                    String name = asset["name"].as<String>();
                    // Look specifically for the main firmware file, not bootloader or partitions
                    if (name.startsWith("qlockthree-esp32c3-") && name.endsWith(".bin") && 
                        name.indexOf("complete") == -1 && name.indexOf("bootloader") == -1 && name.indexOf("partition") == -1) {
                        downloadUrl = asset["browser_download_url"].as<String>();
                        break;
                    }
                }
                
                if (downloadUrl.length() > 0) {
                    Serial.println("Update available! Download URL: " + downloadUrl);
                    
                    // Automatically perform update
                    performUpdate();
                } else {
                    Serial.println("Update available but no suitable firmware file found");
                    updateAvailable = false;
                }
                
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

bool AutoUpdater::performUpdate() {
    if (!updateAvailable || downloadUrl.length() == 0) {
        Serial.println("No update available");
        return false;
    }
    
    return downloadAndInstallUpdate(downloadUrl);
}

bool AutoUpdater::downloadAndInstallUpdate(String url) {
    if (url.length() == 0) {
        Serial.println("No download URL available");
        return false;
    }
    
    Serial.println("Starting firmware update from: " + url);
    
    WiFiClientSecure client;
    client.setInsecure(); // Skip SSL certificate verification for GitHub downloads
    
    HTTPClient http;
    http.begin(client, url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); // Handle GitHub redirects
    http.setUserAgent("QlockThree-ESP32");
    http.setTimeout(30000); // 30 second timeout
    
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

String AutoUpdater::compareVersions(String current, String latest) {
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
