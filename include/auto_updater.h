#ifndef AUTO_UPDATER_H
#define AUTO_UPDATER_H

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Forward declaration to avoid circular dependency
class LEDController;

class AutoUpdater {
public:
    AutoUpdater();
    void begin(const char* githubRepo, const char* currentVersion, unsigned long checkInterval, LEDController* ledController = nullptr);
    void checkForUpdates();
    void checkForUpdates(bool force);
    bool isUpdateAvailable() const { return updateAvailable; }
    String getLatestVersion() const { return latestVersion; }
    String getDownloadUrl() const { return downloadUrl; }
    bool performUpdate();

private:
    String githubUpdateUrl;
    String currentVersion;
    unsigned long updateCheckInterval;
    unsigned long lastUpdateCheck;
    LEDController* ledController;
    
    // Update status
    bool updateAvailable;
    String latestVersion;
    String downloadUrl;
    
    String compareVersions(String current, String latest);
    bool downloadAndInstallUpdate(String url);
    void showUpdateSuccessFeedback();
};

#endif // AUTO_UPDATER_H
