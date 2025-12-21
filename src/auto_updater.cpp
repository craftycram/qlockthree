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
    checkForUpdates(false); // Call with force=false by default
}

void AutoUpdater::checkForUpdates(bool force) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("AUTO UPDATE DEBUG: WiFi not connected, skipping update check");
        return;
    }
    
    // Check if enough time has passed (unless forced)
    if (!force && millis() - lastUpdateCheck < updateCheckInterval) {
        Serial.printf("AUTO UPDATE DEBUG: Update check interval not reached (last check %lu ms ago), skipping\n", 
                     millis() - lastUpdateCheck);
        return;
    }
    
    Serial.println("AUTO UPDATE DEBUG: Starting update check...");
    Serial.printf("AUTO UPDATE DEBUG: GitHub URL: %s\n", githubUpdateUrl.c_str());
    lastUpdateCheck = millis();
    
    WiFiClientSecure client;
    client.setInsecure(); // Skip SSL certificate verification for GitHub API
    
    HTTPClient http;
    bool beginSuccess = http.begin(client, githubUpdateUrl);
    if (!beginSuccess) {
        Serial.println("AUTO UPDATE DEBUG: Failed to begin HTTP client");
        return;
    }
    
    http.addHeader("User-Agent", "qlockthree-ESP32");
    http.setTimeout(15000); // 15 second timeout
    
    Serial.println("AUTO UPDATE DEBUG: Sending HTTP GET request...");
    int httpCode = http.GET();
    Serial.printf("AUTO UPDATE DEBUG: HTTP response code: %d\n", httpCode);
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.printf("AUTO UPDATE DEBUG: Payload length: %d bytes\n", payload.length());
        
        if (payload.length() > 100) {
            Serial.printf("AUTO UPDATE DEBUG: Payload preview: %s...\n", payload.substring(0, 100).c_str());
        }
        
        // Parse JSON response
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            Serial.println("AUTO UPDATE DEBUG: JSON parsed successfully");
            
            if (doc.containsKey("tag_name")) {
                latestVersion = doc["tag_name"].as<String>();
                Serial.printf("AUTO UPDATE DEBUG: Found tag_name: %s\n", latestVersion.c_str());
                
                // Remove 'v' prefix if present
                if (latestVersion.startsWith("v")) {
                    latestVersion = latestVersion.substring(1);
                    Serial.printf("AUTO UPDATE DEBUG: Removed 'v' prefix, version now: %s\n", latestVersion.c_str());
                }
                
                Serial.print("Latest version: ");
                Serial.println(latestVersion);
                Serial.print("Current version: ");
                Serial.println(currentVersion);
                
                // Compare versions
                String comparison = compareVersions(currentVersion, latestVersion);
                Serial.printf("AUTO UPDATE DEBUG: Version comparison result: %s\n", comparison.c_str());
                
                if (comparison == "outdated") {
                    updateAvailable = true;
                    Serial.println("AUTO UPDATE DEBUG: Update available, looking for assets...");
                    
                    // Find download URL for main firmware binary (not bootloader or partitions)
                    JsonArray assets = doc["assets"];
                    Serial.printf("AUTO UPDATE DEBUG: Found %d assets\n", assets.size());
                    
                    downloadUrl = "";
                    for (JsonObject asset : assets) {
                        String name = asset["name"].as<String>();
                        Serial.printf("AUTO UPDATE DEBUG: Asset: %s\n", name.c_str());
                        
                        // Look specifically for the main firmware file, not bootloader or partitions
                        if (name.startsWith("qlockthree-esp32c3-") && name.endsWith(".bin") && 
                            name.indexOf("complete") == -1 && name.indexOf("bootloader") == -1 && name.indexOf("partition") == -1) {
                            downloadUrl = asset["browser_download_url"].as<String>();
                            Serial.printf("AUTO UPDATE DEBUG: Found firmware binary: %s\n", downloadUrl.c_str());
                            break;
                        }
                    }
                    
                    if (downloadUrl.length() > 0) {
                        Serial.println("Update available! Download URL: " + downloadUrl);
                        
                        // Automatically perform update
                        performUpdate();
                    } else {
                        Serial.println("AUTO UPDATE DEBUG: No suitable firmware file found in assets");
                        updateAvailable = false;
                    }
                    
                } else {
                    updateAvailable = false;
                    Serial.println("Firmware is up to date");
                }
            } else {
                Serial.println("AUTO UPDATE DEBUG: No 'tag_name' field found in JSON response");
            }
        } else {
            Serial.printf("AUTO UPDATE DEBUG: Failed to parse JSON response: %s\n", error.c_str());
            Serial.printf("AUTO UPDATE DEBUG: Raw payload: %s\n", payload.c_str());
        }
    } else if (httpCode > 0) {
        Serial.printf("AUTO UPDATE DEBUG: HTTP GET failed with code: %d\n", httpCode);
        String errorPayload = http.getString();
        if (errorPayload.length() > 0) {
            Serial.printf("AUTO UPDATE DEBUG: Error response: %s\n", errorPayload.c_str());
        }
    } else {
        Serial.printf("AUTO UPDATE DEBUG: HTTP request failed: %s\n", http.errorToString(httpCode).c_str());
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
    http.setUserAgent("qlockthree-ESP32");
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
