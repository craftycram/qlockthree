#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <WiFi.h>
#include <time.h>
#include <Preferences.h>
#include <ArduinoJson.h>

class TimeManager {
public:
    TimeManager();
    
    // Initialization and configuration
    void begin();
    void setTimezone(const char* timezone);
    void setNTPServers(const char* primary, const char* secondary = nullptr, const char* tertiary = nullptr);
    
    // Time synchronization
    bool syncTime();
    bool isTimeSynced();
    unsigned long getLastSyncTime();
    
    // Time retrieval
    struct tm getCurrentTime();
    String getFormattedTime(const char* format = "%H:%M:%S");
    String getFormattedDate(const char* format = "%Y-%m-%d");
    String getTimezoneString();
    
    // Timezone management
    bool setTimezoneByName(const char* timezoneName);
    String getAvailableTimezones();
    
    // Configuration persistence
    void saveSettings();
    void loadSettings();
    
    // Status information
    String getStatusJSON();
    bool isDST();
    int getTimezoneOffset();
    
private:
    Preferences preferences;
    String currentTimezone;
    String ntpServer1;
    String ntpServer2;
    String ntpServer3;
    bool timeSynced;
    unsigned long lastSyncTime;
    unsigned long syncInterval;
    
    // Internal methods
    void configureTimezone();
    bool performNTPSync();
    void updateSyncStatus();
    
    // Timezone data
    struct TimezoneInfo {
        const char* name;
        const char* posixString;
        const char* displayName;
    };
    
    static const TimezoneInfo timezones[];
    static const int timezoneCount;
};

#endif // TIME_MANAGER_H
