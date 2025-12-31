#include "birthday_manager.h"

BirthdayManager::BirthdayManager() : displayMode(ALTERNATE) {
}

void BirthdayManager::begin() {
    preferences.begin("birthdays", false);

    // Load display mode (default to ALTERNATE if not set)
    displayMode = static_cast<DisplayMode>(preferences.getUChar("mode", ALTERNATE));

    // Load birthdays
    loadBirthdays();

    Serial.println("Birthday Manager initialized");
    Serial.printf("Display mode: %d, Birthdays: %d\n", displayMode, birthdays.size());
}

void BirthdayManager::loadBirthdays() {
    birthdays.clear();

    String datesStr = preferences.getString("dates", "");
    if (datesStr.length() == 0) {
        return;
    }

    // Parse comma-separated dates (format: "0115,0322,1225")
    int start = 0;
    int commaIndex;
    while ((commaIndex = datesStr.indexOf(',', start)) != -1) {
        String dateStr = datesStr.substring(start, commaIndex);
        uint16_t value = dateStr.toInt();
        if (value > 0 && value <= 1231) {
            birthdays.push_back(value);
        }
        start = commaIndex + 1;
    }
    // Last entry (or only entry if no comma)
    if (start < datesStr.length()) {
        String dateStr = datesStr.substring(start);
        uint16_t value = dateStr.toInt();
        if (value > 0 && value <= 1231) {
            birthdays.push_back(value);
        }
    }

    Serial.printf("Loaded %d birthdays\n", birthdays.size());
}

void BirthdayManager::saveBirthdays() {
    String datesStr = "";
    for (size_t i = 0; i < birthdays.size(); i++) {
        if (i > 0) datesStr += ",";
        // Format as 4-digit string with leading zeros
        char buf[5];
        snprintf(buf, sizeof(buf), "%04d", birthdays[i]);
        datesStr += buf;
    }
    preferences.putString("dates", datesStr);
    Serial.printf("Saved birthdays: %s\n", datesStr.c_str());
}

bool BirthdayManager::isBirthday(uint8_t month, uint8_t day) const {
    uint16_t target = toStorageFormat(month, day);
    for (uint16_t bd : birthdays) {
        if (bd == target) {
            return true;
        }
    }
    return false;
}

BirthdayManager::DisplayMode BirthdayManager::getDisplayMode() const {
    return displayMode;
}

void BirthdayManager::setDisplayMode(DisplayMode mode) {
    displayMode = mode;
    Serial.printf("Birthday display mode set to %d\n", mode);
}

bool BirthdayManager::addBirthday(uint8_t month, uint8_t day) {
    // Validate
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        return false;
    }

    // Check limit
    if (birthdays.size() >= MAX_BIRTHDAYS) {
        Serial.println("Birthday limit reached");
        return false;
    }

    uint16_t value = toStorageFormat(month, day);

    // Check if already exists
    for (uint16_t bd : birthdays) {
        if (bd == value) {
            Serial.printf("Birthday %02d-%02d already exists\n", month, day);
            return false;
        }
    }

    birthdays.push_back(value);
    Serial.printf("Added birthday: %02d-%02d\n", month, day);
    return true;
}

bool BirthdayManager::removeBirthday(uint8_t month, uint8_t day) {
    uint16_t value = toStorageFormat(month, day);

    for (auto it = birthdays.begin(); it != birthdays.end(); ++it) {
        if (*it == value) {
            birthdays.erase(it);
            Serial.printf("Removed birthday: %02d-%02d\n", month, day);
            return true;
        }
    }

    return false;
}

void BirthdayManager::clearAllBirthdays() {
    birthdays.clear();
    Serial.println("Cleared all birthdays");
}

uint8_t BirthdayManager::getBirthdayCount() const {
    return birthdays.size();
}

String BirthdayManager::getBirthdaysJSON() const {
    String json = "{\"mode\":" + String(displayMode) + ",\"dates\":[";

    for (size_t i = 0; i < birthdays.size(); i++) {
        if (i > 0) json += ",";
        uint8_t month, day;
        fromStorageFormat(birthdays[i], month, day);
        json += "{\"month\":" + String(month) + ",\"day\":" + String(day) + "}";
    }

    json += "]}";
    return json;
}

void BirthdayManager::save() {
    preferences.putUChar("mode", static_cast<uint8_t>(displayMode));
    saveBirthdays();
}

uint16_t BirthdayManager::toStorageFormat(uint8_t month, uint8_t day) {
    return month * 100 + day;  // e.g., January 15 = 115, December 25 = 1225
}

void BirthdayManager::fromStorageFormat(uint16_t value, uint8_t& month, uint8_t& day) {
    month = value / 100;
    day = value % 100;
}
