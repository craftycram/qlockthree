#ifndef MAPPING_45_H
#define MAPPING_45_H

#include <stdint.h>

// Word mapping structure - shared across ALL mappings (global scope)
struct WordMapping {
    const char* word;
    uint8_t start_led;
    uint8_t length;
    bool active;
};

// Wrap everything mapping-specific in a namespace to avoid symbol conflicts
namespace Mapping45 {

// qlockthree 11x11 Layout Mapping Configuration
// This file defines the word-to-LED mappings for the 45cm qlockthree with 11x11 layout
// Layout:
// Row 0: ESKISTLFÜNF (0-10)
// Row 1: ZEHNZWANZIG (11-21)  
// Row 2: DREIVIERTEL (22-32)
// Row 3: NACHAPPYVOR (33-43)
// Row 4: HALBIRTHDAY (44-54)
// Row 5: DRZWÖLFÜNFX (55-65)
// Row 6: ZEHNEUNDREI (66-76)
// Row 7: ZWEINSIEBEN (77-87)
// Row 8: ELFVIERACHT (88-98)
// Row 9: SECHSIUHRYE (99-109)
// Row 10: MDMDFSS + 4 corner dots (110-120 + corners)

// Mapping metadata
#define MAPPING_NAME "45cm qlockthree 11x11"
#define MAPPING_ID "45cm"
#define MAPPING_LANGUAGE "DE"
#define MAPPING_TOTAL_LEDS 125
#define MAPPING_DESCRIPTION "45cm German qlockthree with 11x11 grid, weekdays, and 4 corner dots"

// Time word mappings - Base words always shown
static const WordMapping BASE_WORDS[] = {
    {"ES", 112, 2, false},       // Row 0: ES (0-1)
    {"IST", 115, 3, false},      // Row 0: IST (3-5)
};

// Hour mappings (0-23)
static const WordMapping HOUR_WORDS[] = {
    {"ZWÖLF", 61, 5, false},   // Row 5: ZWÖLF (57-60) - skipping DR
    {"EINS", 40, 4, false},    // Row 7: EINS (78-81) - inside ZWEINSIEBEN
    {"ZWEI", 42, 4, false},    // Row 7: ZWEI (77-80)
    {"DREI", 53, 4, false},    // Row 6: DREI (73-76)
    {"VIER", 27, 4, false},    // Row 8: VIER (91-94)
    {"FÜNF", 58, 4, false},    // Row 5: FÜNF (61-64)
    {"SECHS", 19, 5, false},   // Row 9: SECHS (99-103)
    {"SIEBEN", 35, 6, false},  // Row 7: SIEBEN (82-87)
    {"ACHT", 31, 4, false},    // Row 8: ACHT (95-98)
    {"NEUN", 49, 4, false},    // Row 6: NEUN (67-70) - inside ZEHNEUNDREI
    {"ZEHN", 46, 4, false},    // Row 6: ZEHN (66-69)
    {"ELF", 24, 3, false},     // Row 8: ELF (88-90)
};

// Minute mappings (5-minute intervals)
static const WordMapping MINUTE_WORDS[] = {
    {"FÜNF", 119, 4, false},     // Row 0: FÜNF (7-10)
    {"ZEHN", 108, 4, false},    // Row 1: ZEHN (11-14)
    {"VIERTEL", 94, 7, false}, // Row 2: VIERTEL (26-32)
    {"ZWANZIG", 101, 7, false}, // Row 1: ZWANZIG (15-21)
    {"DREIVIERTEL", 90, 11, false}, // Row 2: DREIVIERTEL (22-32)
    {"HALB", 68, 4, false},    // Row 4: HALB (44-47)
};

// Connector words
static const WordMapping CONNECTOR_WORDS[] = {
    {"VOR", 79, 3, false},     // Row 3: VOR (40-42)
    {"NACH", 86, 4, false},    // Row 3: NACH (33-36)
    {"UHR", 15, 3, false},    // Row 9: UHR (105-107)
};

// Minute dots for precise time (corner LEDs based on 125 LED total)
static const uint8_t MINUTE_DOTS[] = {124, 123, 12, 0}; // corner positions from image

// Status LED configuration (using constexpr instead of #define to work with namespaces)
static constexpr uint8_t STATUS_LED_WIFI = 11;      // WiFi status LED (index 11)
static constexpr uint8_t STATUS_LED_SYSTEM = 10;    // System/NTP/OTA/Update status LED (index 10)

// Startup animation sequence (0-based array indices)
static const uint8_t STARTUP_SEQUENCE[] = {
    // 1st row: indices 112-122
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
    // 2nd row: indices 111-101 (reverse order)
    111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101,
    // 3rd row: indices 90-100
    90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
    // 4th row: indices 89-79 (reverse order)
    89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79,
    // 5th row: indices 68-78
    68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
    // 6th row: indices 67-57 (reverse order)
    67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57,
    // 7th row: indices 46-56
    46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    // 8th row: indices 45-35 (reverse order)
    45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35,
    // 9th row: indices 24-34
    24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
    // 10th row: indices 23-13 (reverse order)
    23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13,
    // 11th row: indices 1-11
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

// Calculate sequence length (using constexpr instead of #define to work with namespaces)
static constexpr uint16_t STARTUP_SEQUENCE_LENGTH = sizeof(STARTUP_SEQUENCE) / sizeof(STARTUP_SEQUENCE[0]);

// Weekday mappings (Bottom row: M D M D F S S)
static const WordMapping WEEKDAY_WORDS[] = {
    {"M", 3, 1, false},        // Monday (Montag)
    {"D", 4, 1, false},        // Tuesday (Dienstag) 
    {"M", 5, 1, false},        // Wednesday (Mittwoch)
    {"D", 6, 1, false},        // Thursday (Donnerstag)
    {"F", 7, 1, false},        // Friday (Freitag)
    {"S", 8, 1, false},        // Saturday (Samstag)
    {"S", 9, 1, false},        // Sunday (Sonntag)
};

// Special words (for birthday/special occasions - not used in normal time display)
static const WordMapping SPECIAL_WORDS[] = {
    {"HAPPY", 86, 82, false},     // Row 3: HAPPY (37-41) - part of NACHAPPYVOR
    {"BIRTHDAY", 71, 78, false},  // Row 4: BIRTHDAY (48-55)
};

// Time calculation functions - inline implementations for header-only mapping
//
// German Time Display Logic:
// +---------+---------------------------+----------------------------------+
// | Minutes | Display                   | Components                       |
// +---------+---------------------------+----------------------------------+
// | 00-04   | ES IST X UHR              | hour + UHR                       |
// | 05-09   | ES IST FÜNF NACH X        | FÜNF + NACH + hour               |
// | 10-14   | ES IST ZEHN NACH X        | ZEHN + NACH + hour               |
// | 15-19   | ES IST VIERTEL NACH X     | VIERTEL + NACH + hour            |
// | 20-24   | ES IST ZWANZIG NACH X     | ZWANZIG + NACH + hour            |
// | 25-29   | ES IST FÜNF VOR HALB X+1  | prefix FÜNF + VOR + HALB + hour  |
// | 30-34   | ES IST HALB X+1           | HALB + hour                      |
// | 35-39   | ES IST FÜNF NACH HALB X+1 | prefix FÜNF + NACH + HALB + hour |
// | 40-44   | ES IST ZWANZIG VOR X+1    | ZWANZIG + VOR + hour             |
// | 45-49   | ES IST DREIVIERTEL X+1    | DREIVIERTEL + hour               |
// | 50-54   | ES IST ZEHN VOR X+1       | ZEHN + VOR + hour                |
// | 55-59   | ES IST FÜNF VOR X+1       | FÜNF + VOR + hour                |
// +---------+---------------------------+----------------------------------+
// Note: For minutes >= 25, hour is incremented (X+1 = next hour)

inline bool shouldShowBaseWords() {
    return true;
}

inline uint8_t getHourWordIndex(uint8_t hour, uint8_t minute) {
    // Adjust hour for German time display logic
    if (minute >= 25) hour = (hour + 1) % 24; // "X before next hour"
    return hour % 12; // Convert to 12-hour format for word selection
}

inline int8_t getMinuteWordIndex(uint8_t minute) {
    if (minute >= 5 && minute < 10) return 0;      // FÜNF nach
    if (minute >= 10 && minute < 15) return 1;     // ZEHN nach
    if (minute >= 15 && minute < 20) return 2;     // VIERTEL nach
    if (minute >= 20 && minute < 25) return 3;     // ZWANZIG nach
    if (minute >= 25 && minute < 40) return 5;     // HALB (with prefix for 25-29, 35-39)
    if (minute >= 40 && minute < 45) return 3;     // ZWANZIG vor
    if (minute >= 45 && minute < 50) return 2;     // VIERTEL vor
    if (minute >= 50 && minute < 55) return 1;     // ZEHN vor
    if (minute >= 55) return 0;                    // FÜNF vor
    return -1; // Exact hour, no minute word
}

// Returns prefix minute word index for "X VOR/NACH HALB" cases
// Returns the minute word to show before HALB
inline int8_t getMinutePrefixWordIndex(uint8_t minute) {
    if (minute >= 25 && minute < 30) return 0;     // FÜNF vor halb
    if (minute >= 35 && minute < 40) return 0;     // FÜNF nach halb
    return -1; // No prefix needed
}

inline int8_t getConnectorWordIndex(uint8_t minute) {
    if (minute < 5) return 2;                      // UHR (o'clock)
    if (minute >= 5 && minute < 25) return 1;      // NACH (after/past)
    if (minute >= 25 && minute < 30) return 0;     // VOR (for "fünf vor halb")
    if (minute >= 30 && minute < 35) return -1;    // No connector for "halb"
    if (minute >= 35 && minute < 40) return 1;     // NACH (for "fünf nach halb")
    if (minute >= 40 && minute < 45) return 0;     // VOR (for "zwanzig vor")
    if (minute >= 45 && minute < 50) return 0;    // No connector for "dreiviertel"
    if (minute >= 50) return 0;                    // VOR (before/to)
    return -1; // No connector needed
}

inline uint8_t getMinuteDots(uint8_t minute) {
    return minute % 5; // 0-4, where 0 means no dots
}

inline bool isHalfPast(uint8_t minute) {
    return minute >= 25 && minute < 35;
}

inline bool isDreiViertel(uint8_t minute) {
    return minute >= 45 && minute < 50; // Use DREIVIERTEL instead of VIERTEL VOR
}

inline bool shouldShowBirthday() {
    return false; // Normally false, enable for special occasions
}

inline uint8_t getWeekdayIndex(uint8_t weekday) {
    // weekday: 0 = Sunday, 1 = Monday, 2 = Tuesday, ..., 6 = Saturday
    // Convert to our mapping: 0 = Monday, 1 = Tuesday, ..., 6 = Sunday
    if (weekday == 0) return 6;  // Sunday -> S (index 6)
    return weekday - 1;          // Monday-Saturday -> 0-5
}

inline bool shouldShowWeekday() {
    return true; // Can be made configurable via web interface
}

} // namespace Mapping45

#endif // MAPPING_45_H
