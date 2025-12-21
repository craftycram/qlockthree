#include "../mappings/45.h"

// Time calculation function implementations
bool shouldShowBaseWords() { 
    return true; 
}

uint8_t getHourWordIndex(uint8_t hour, uint8_t minute) {
    // Adjust hour for German time display logic
    if (minute >= 25) hour = (hour + 1) % 24; // "X before next hour"
    return hour % 12; // Convert to 12-hour format for word selection
}

int8_t getMinuteWordIndex(uint8_t minute) {
    if (minute >= 5 && minute < 10) return 0;      // FÜNF
    if (minute >= 10 && minute < 15) return 1;     // ZEHN
    if (minute >= 15 && minute < 20) return 2;     // VIERTEL
    if (minute >= 20 && minute < 25) return 3;     // ZWANZIG
    if (minute >= 25 && minute < 35) return 5;     // HALB
    if (minute >= 35 && minute < 40) return 5;     // HALB (for "5 nach halb")
    if (minute >= 40 && minute < 45) return 3;     // ZWANZIG vor
    if (minute >= 45 && minute < 50) return 4;     // DREIVIERTEL
    if (minute >= 50 && minute < 55) return 1;     // ZEHN vor
    if (minute >= 55) return 0;                    // FÜNF vor
    return -1; // Exact hour, no minute word
}

int8_t getConnectorWordIndex(uint8_t minute) {
    if (minute == 0) return 2;                     // UHR (o'clock)
    if (minute >= 5 && minute < 25) return 1;      // NACH (after/past)
    if (minute >= 25 && minute < 35) return -1;    // No connector for "halb"
    if (minute >= 35 && minute < 45) return 1;     // NACH (for "X nach halb")
    if (minute >= 45 && minute < 50) return -1;    // No connector for "dreiviertel"
    if (minute >= 50) return 0;                    // VOR (before/to)
    return -1; // No connector needed
}

uint8_t getMinuteDots(uint8_t minute) {
    return minute % 5; // 0-4, where 0 means no dots
}

bool isHalfPast(uint8_t minute) {
    return minute >= 25 && minute < 35;
}

bool isDreiViertel(uint8_t minute) {
    return minute >= 45 && minute < 50; // Use DREIVIERTEL instead of VIERTEL VOR
}

bool shouldShowBirthday() {
    return false; // Normally false, enable for special occasions
}

uint8_t getWeekdayIndex(uint8_t weekday) {
    // weekday: 0 = Sunday, 1 = Monday, 2 = Tuesday, ..., 6 = Saturday
    // Convert to our mapping: 0 = Monday, 1 = Tuesday, ..., 6 = Sunday
    if (weekday == 0) return 6;  // Sunday -> S (index 6)
    return weekday - 1;          // Monday-Saturday -> 0-5
}

bool shouldShowWeekday() {
    return true; // Can be made configurable via web interface
}
