#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <WebServer.h>
#include <ESPmDNS.h>

// Forward declarations
class WiFiManagerHelper;
class AutoUpdater;
class LEDController;

class WebServerManager {
public:
    WebServerManager(int port = 80);
    void begin(WiFiManagerHelper* wifiHelper, AutoUpdater* updater, LEDController* ledController = nullptr);
    void handleClient();
    
private:
    WebServer server;
    WiFiManagerHelper* wifiManagerHelper;
    AutoUpdater* autoUpdater;
    LEDController* ledController;
    
    void setupRoutes();
    
    // Route handlers
    void handleRoot();
    void handleStatus();
    void handleCheckUpdate();
    void handleManualUpdate();
    void handleWiFiReset();
    
    // LED configuration handlers
    void handleLEDStatus();
    void handleLEDConfig();
    void handleLEDTest();
    void handleLEDPattern();
    
    // HTML/JSON generators
    String getStatusHTML();
    String getStatusJSON();
    String getLEDConfigHTML();
    String getLEDStatusJSON();
};

#endif // WEB_SERVER_MANAGER_H
