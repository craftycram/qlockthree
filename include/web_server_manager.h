#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <WebServer.h>
#include <ESPmDNS.h>

// Forward declarations
class WiFiManagerHelper;
class AutoUpdater;
class LEDController;
class TimeManager;

class WebServerManager {
public:
    WebServerManager(int port = 80);
    void begin(WiFiManagerHelper* wifiHelper, AutoUpdater* updater, LEDController* ledController = nullptr, TimeManager* timeManager = nullptr);
    void handleClient();
    
private:
    WebServer server;
    WiFiManagerHelper* wifiManagerHelper;
    AutoUpdater* autoUpdater;
    LEDController* ledController;
    TimeManager* timeManager;
    
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
    
    // HTML/JSON generators
    String getStatusHTML();
    String getStatusJSON();
    String getLEDConfigHTML();
    String getLEDStatusJSON();
    String getTimeConfigHTML();
    String getTimeStatusJSON();
};

#endif // WEB_SERVER_MANAGER_H
