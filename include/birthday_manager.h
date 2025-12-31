#ifndef BIRTHDAY_MANAGER_H
#define BIRTHDAY_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>

class BirthdayManager {
public:
    enum DisplayMode {
        REPLACE = 0,    // Show only HAPPY BIRTHDAY instead of time
        ALTERNATE = 1,  // Alternate between time and HAPPY BIRTHDAY
        OVERLAY = 2     // Show HAPPY BIRTHDAY overlaid on time
    };

    static const uint8_t MAX_BIRTHDAYS = 10;

    BirthdayManager();

    void begin();

    // Check if a given date is a birthday
    bool isBirthday(uint8_t month, uint8_t day) const;

    // Display mode
    DisplayMode getDisplayMode() const;
    void setDisplayMode(DisplayMode mode);

    // Date management
    bool addBirthday(uint8_t month, uint8_t day);
    bool removeBirthday(uint8_t month, uint8_t day);
    void clearAllBirthdays();
    uint8_t getBirthdayCount() const;

    // For web UI
    String getBirthdaysJSON() const;

    // Save settings
    void save();

private:
    Preferences preferences;
    std::vector<uint16_t> birthdays;  // Stored as MMDD (e.g., 115 for Jan 15, 1225 for Dec 25)
    DisplayMode displayMode;

    void loadBirthdays();
    void saveBirthdays();

    // Helper to convert month/day to storage format
    static uint16_t toStorageFormat(uint8_t month, uint8_t day);
    static void fromStorageFormat(uint16_t value, uint8_t& month, uint8_t& day);
};

#endif // BIRTHDAY_MANAGER_H
