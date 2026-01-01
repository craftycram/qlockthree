#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <WebServer.h>
#include <ESPmDNS.h>

// Forward declarations
class WiFiManagerHelper;
class AutoUpdater;
class LEDController;
class TimeManager;
class BirthdayManager;

class WebServerManager {
public:
    WebServerManager(int port = 80);
    void begin(WiFiManagerHelper* wifiHelper, AutoUpdater* updater, LEDController* ledController = nullptr, TimeManager* timeManager = nullptr,
               bool* debugModeEnabled = nullptr, int* debugHour = nullptr, int* debugMinute = nullptr);
    void handleClient();
    void setBirthdayManager(BirthdayManager* manager) { birthdayManager = manager; }

private:
    WebServer server;
    WiFiManagerHelper* wifiManagerHelper;
    AutoUpdater* autoUpdater;
    LEDController* ledController;
    TimeManager* timeManager;
    BirthdayManager* birthdayManager;

    // Debug mode state pointers
    bool* debugModeEnabled;
    int* debugHour;
    int* debugMinute;
    
    void setupRoutes();
    
    // Route handlers
    void handleRoot();
    void handleStatus();
    void handleCheckUpdate();
    void handleManualUpdate();
    void handleWiFiReset();
    
    // Time configuration handlers
    void handleTimeConfig();
    void handleTimeStatus();
    void handleTimeSync();
    void handleSetTimezone();
    void handleSetNTP();
    
    // LED configuration handlers
    void handleLEDStatus();
    void handleLEDConfig();
    void handleLEDTest();
    void handleLEDPattern();
    void handleLEDMapping();
    void handleSetLEDMapping();
    void handleSetRotation();

    // Debug mode handlers
    void handleDevPage();
    void handleDevStatus();
    void handleDevSet();
    void handleDevToggle();
    void handleReboot();
    void handleFactoryReset();

    // Birthday handlers
    void handleBirthdayPage();
    void handleBirthdayList();
    void handleBirthdayAdd();
    void handleBirthdayRemove();
    void handleBirthdayMode();

    // JSON generators
    String getStatusJSON();
    String getLEDStatusJSON();
    String getTimeStatusJSON();
    String getDevStatusJSON();
};

#endif // WEB_SERVER_MANAGER_H
