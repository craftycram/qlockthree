#include "time_manager.h"

// Common timezone definitions
const TimeManager::TimezoneInfo TimeManager::timezones[] = {
    {"UTC", "UTC0", "UTC (Coordinated Universal Time)"},
    {"CET", "CET-1CEST,M3.5.0,M10.5.0/3", "Central European Time (Germany, France, Italy)"},
    {"EET", "EET-2EEST,M3.5.0/3,M10.5.0/4", "Eastern European Time (Finland, Greece)"},
    {"WET", "WET0WEST,M3.5.0/1,M10.5.0", "Western European Time (Portugal, UK)"},
    {"EST", "EST5EDT,M3.2.0,M11.1.0", "Eastern Standard Time (US East Coast)"},
    {"CST", "CST6CDT,M3.2.0,M11.1.0", "Central Standard Time (US Central)"},
    {"MST", "MST7MDT,M3.2.0,M11.1.0", "Mountain Standard Time (US Mountain)"},
    {"PST", "PST8PDT,M3.2.0,M11.1.0", "Pacific Standard Time (US West Coast)"},
    {"JST", "JST-9", "Japan Standard Time"},
    {"AEST", "AEST-10AEDT,M10.1.0,M4.1.0/3", "Australian Eastern Time"},
    {"IST", "IST-5:30", "India Standard Time"},
    {"CST_CN", "CST-8", "China Standard Time"},
    {"MSK", "MSK-3", "Moscow Time"},
    {"GST", "GST-4", "Gulf Standard Time"}
};

const int TimeManager::timezoneCount = sizeof(timezones) / sizeof(timezones[0]);

TimeManager::TimeManager() : 
    timeSynced(false), 
    lastSyncTime(0), 
    syncInterval(3600000), // 1 hour
    currentTimezone("CET-1CEST,M3.5.0,M10.5.0/3"),
    ntpServer1("pool.ntp.org"),
    ntpServer2("time.nist.gov"),
    ntpServer3("de.pool.ntp.org") {
}

void TimeManager::begin() {
    preferences.begin("time_manager", false);
    loadSettings();
    
    Serial.println("Time Manager initialized");
    Serial.printf("Timezone: %s\n", currentTimezone.c_str());
    Serial.printf("NTP Servers: %s, %s, %s\n", ntpServer1.c_str(), ntpServer2.c_str(), ntpServer3.c_str());
    
    configureTimezone();
    
    // Initial time sync if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        syncTime();
    }
}

void TimeManager::configureTimezone() {
    setenv("TZ", currentTimezone.c_str(), 1);
    tzset();
    Serial.printf("Timezone configured: %s\n", currentTimezone.c_str());
}

bool TimeManager::syncTime() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, cannot sync time");
        return false;
    }
    
    Serial.println("Synchronizing time via NTP...");
    
    // First configure timezone
    configureTimezone();
    
    // Configure NTP with timezone awareness
    configTime(0, 0, ntpServer1.c_str(), ntpServer2.c_str(), ntpServer3.c_str());
    
    // Wait for time sync (up to 10 seconds)
    int attempts = 0;
    while (time(nullptr) < 1000000000L && attempts < 100) {
        delay(100);
        attempts++;
    }
    
    time_t now = time(nullptr);
    if (now > 1000000000L) {
        timeSynced = true;
        lastSyncTime = millis();
        
        // Ensure timezone is applied after sync
        configureTimezone();
        
        struct tm* timeinfo = localtime(&now);
        Serial.printf("Time synchronized: %04d-%02d-%02d %02d:%02d:%02d (DST: %s)\n", 
                     timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                     timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
                     timeinfo->tm_isdst ? "Yes" : "No");
        
        // Print UTC time for comparison
        struct tm* utc_tm = gmtime(&now);
        Serial.printf("UTC Time: %04d-%02d-%02d %02d:%02d:%02d\n", 
                     utc_tm->tm_year + 1900, utc_tm->tm_mon + 1, utc_tm->tm_mday,
                     utc_tm->tm_hour, utc_tm->tm_min, utc_tm->tm_sec);
        
        saveSettings();
        return true;
    } else {
        Serial.println("Failed to synchronize time");
        timeSynced = false; // Explicitly set to false on failure
        return false;
    }
}

bool TimeManager::isTimeSynced() {
    return timeSynced && (millis() - lastSyncTime < syncInterval);
}

unsigned long TimeManager::getLastSyncTime() {
    return lastSyncTime;
}

struct tm TimeManager::getCurrentTime() {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    return timeinfo;
}

String TimeManager::getFormattedTime(const char* format) {
    struct tm timeinfo = getCurrentTime();
    char buffer[64];
    strftime(buffer, sizeof(buffer), format, &timeinfo);
    return String(buffer);
}

String TimeManager::getFormattedDate(const char* format) {
    struct tm timeinfo = getCurrentTime();
    char buffer[64];
    strftime(buffer, sizeof(buffer), format, &timeinfo);
    return String(buffer);
}

String TimeManager::getTimezoneString() {
    return currentTimezone;
}

void TimeManager::setTimezone(const char* timezone) {
    currentTimezone = String(timezone);
    configureTimezone();
    saveSettings();
    Serial.printf("Timezone changed to: %s\n", timezone);
    
    // Force re-sync to apply new timezone immediately
    if (WiFi.status() == WL_CONNECTED) {
        syncTime();
    }
}

bool TimeManager::setTimezoneByName(const char* timezoneName) {
    for (int i = 0; i < timezoneCount; i++) {
        if (strcmp(timezones[i].name, timezoneName) == 0) {
            setTimezone(timezones[i].posixString);
            return true;
        }
    }
    Serial.printf("Unknown timezone: %s\n", timezoneName);
    return false;
}

String TimeManager::getAvailableTimezones() {
    String json = "[";
    for (int i = 0; i < timezoneCount; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"name\":\"" + String(timezones[i].name) + "\",";
        json += "\"display\":\"" + String(timezones[i].displayName) + "\",";
        json += "\"posix\":\"" + String(timezones[i].posixString) + "\"";
        json += "}";
    }
    json += "]";
    return json;
}

void TimeManager::setNTPServers(const char* primary, const char* secondary, const char* tertiary) {
    ntpServer1 = String(primary);
    if (secondary) ntpServer2 = String(secondary);
    if (tertiary) ntpServer3 = String(tertiary);
    
    saveSettings();
    Serial.printf("NTP servers updated: %s, %s, %s\n", ntpServer1.c_str(), ntpServer2.c_str(), ntpServer3.c_str());
}

void TimeManager::saveSettings() {
    preferences.putString("timezone", currentTimezone);
    preferences.putString("ntp_server1", ntpServer1);
    preferences.putString("ntp_server2", ntpServer2);
    preferences.putString("ntp_server3", ntpServer3);
    preferences.putULong("last_sync", lastSyncTime);
    preferences.putBool("time_synced", timeSynced);
    
    Serial.println("Time settings saved");
}

void TimeManager::loadSettings() {
    currentTimezone = preferences.getString("timezone", "CET-1CEST,M3.5.0,M10.5.0/3");
    ntpServer1 = preferences.getString("ntp_server1", "pool.ntp.org");
    ntpServer2 = preferences.getString("ntp_server2", "time.nist.gov");
    ntpServer3 = preferences.getString("ntp_server3", "de.pool.ntp.org");
    lastSyncTime = preferences.getULong("last_sync", 0);
    timeSynced = preferences.getBool("time_synced", false);
    
    Serial.println("Time settings loaded");
}

String TimeManager::getStatusJSON() {
    struct tm timeinfo = getCurrentTime();
    
    String json = "{";
    json += "\"current_time\":\"" + getFormattedTime("%H:%M:%S") + "\",";
    json += "\"current_date\":\"" + getFormattedDate("%Y-%m-%d") + "\",";
    json += "\"timezone\":\"" + currentTimezone + "\",";
    json += "\"ntp_servers\":[";
    json += "\"" + ntpServer1 + "\",";
    json += "\"" + ntpServer2 + "\",";
    json += "\"" + ntpServer3 + "\"";
    json += "],";
    json += "\"ntp_server\":\"" + ntpServer1 + "\",";
    json += "\"day\":" + String(timeinfo.tm_mday) + ",";
    json += "\"month\":" + String(timeinfo.tm_mon + 1) + ",";
    json += "\"year\":" + String(timeinfo.tm_year + 1900) + ",";
    json += "\"synced\":" + String(timeSynced ? "true" : "false") + ",";
    json += "\"time_synced\":" + String(timeSynced ? "true" : "false") + ",";
    json += "\"last_sync\":" + String(lastSyncTime) + ",";
    json += "\"sync_age\":" + String(millis() - lastSyncTime) + ",";
    json += "\"is_dst\":" + String(isDST() ? "true" : "false") + ",";
    json += "\"timezone_offset\":" + String(getTimezoneOffset()) + ",";
    json += "\"weekday\":" + String(timeinfo.tm_wday) + ",";
    json += "\"hour\":" + String(timeinfo.tm_hour) + ",";
    json += "\"minute\":" + String(timeinfo.tm_min) + ",";
    json += "\"second\":" + String(timeinfo.tm_sec);
    json += "}";
    
    return json;
}

bool TimeManager::isDST() {
    struct tm timeinfo = getCurrentTime();
    return timeinfo.tm_isdst > 0;
}

int TimeManager::getTimezoneOffset() {
    time_t now = time(nullptr);
    struct tm* utc_tm = gmtime(&now);
    struct tm* local_tm = localtime(&now);
    
    // Calculate difference in hours
    int utc_hours = utc_tm->tm_hour;
    int local_hours = local_tm->tm_hour;
    
    // Handle day boundary crossings
    if (utc_tm->tm_mday != local_tm->tm_mday) {
        if (local_tm->tm_mday > utc_tm->tm_mday) {
            local_hours += 24;
        } else {
            utc_hours += 24;
        }
    }
    
    return local_hours - utc_hours;
}
