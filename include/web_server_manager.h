#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <WebServer.h>
#include <ESPmDNS.h>

// Forward declarations
class WiFiManagerHelper;
class AutoUpdater;

class WebServerManager {
public:
    WebServerManager(int port = 80);
    void begin(WiFiManagerHelper* wifiHelper, AutoUpdater* updater);
    void handleClient();
    
private:
    WebServer server;
    WiFiManagerHelper* wifiManagerHelper;
    AutoUpdater* autoUpdater;
    
    void setupRoutes();
    
    // Route handlers
    void handleRoot();
    void handleStatus();
    void handleCheckUpdate();
    void handleManualUpdate();
    void handleWiFiReset();
    
    // HTML/JSON generators
    String getStatusHTML();
    String getStatusJSON();
};

#endif // WEB_SERVER_MANAGER_H
