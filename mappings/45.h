#ifndef MAPPING_45_H
#define MAPPING_45_H

// QlockThree 11x11 Layout Mapping Configuration
// This file defines the word-to-LED mappings for the 45cm QlockThree with 11x11 layout
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

#include <stdint.h>

// Mapping metadata
#define MAPPING_NAME "45cm QlockThree 11x11"
#define MAPPING_ID "45cm"
#define MAPPING_LANGUAGE "DE"
#define MAPPING_TOTAL_LEDS 125
#define MAPPING_DESCRIPTION "45cm German QlockThree with 11x11 grid, weekdays, and 4 corner dots"

// Word mapping structure
struct WordMapping {
    const char* word;
    uint8_t start_led;
    uint8_t length;
    bool active;
};

// Time word mappings - Base words always shown
static const WordMapping BASE_WORDS[] = {
    {"ES", 112, 113, false},       // Row 0: ES (0-1)
    {"IST", 115, 117, false},      // Row 0: IST (3-5)
};

// Hour mappings (0-23)
static const WordMapping HOUR_WORDS[] = {
    {"ZWÖLF", 65, 61, false},   // Row 5: ZWÖLF (57-60) - skipping DR
    {"EINS", 43, 40, false},    // Row 7: EINS (78-81) - inside ZWEINSIEBEN
    {"ZWEI", 45, 42, false},    // Row 7: ZWEI (77-80)
    {"DREI", 90, 93, false},    // Row 6: DREI (73-76)
    {"VIER", 27, 30, false},    // Row 8: VIER (91-94)
    {"FÜNF", 61, 58, false},    // Row 5: FÜNF (61-64)
    {"SECHS", 23, 20, false},   // Row 9: SECHS (99-103)
    {"SIEBEN", 40, 35, false},  // Row 7: SIEBEN (82-87)
    {"ACHT", 31, 34, false},    // Row 8: ACHT (95-98)
    {"NEUN", 49, 52, false},    // Row 6: NEUN (67-70) - inside ZEHNEUNDREI
    {"ZEHN", 46, 49, false},    // Row 6: ZEHN (66-69)
    {"ELF", 24, 26, false},     // Row 8: ELF (88-90)
};

// Minute mappings (5-minute intervals)
static const WordMapping MINUTE_WORDS[] = {
    {"FÜNF", 119, 122, false},     // Row 0: FÜNF (7-10)
    {"ZEHN", 111, 108, false},    // Row 1: ZEHN (11-14)
    {"VIERTEL", 94, 100, false}, // Row 2: VIERTEL (26-32)
    {"ZWANZIG", 107, 101, false}, // Row 1: ZWANZIG (15-21)
    {"DREIVIERTEL", 90, 100, false}, // Row 2: DREIVIERTEL (22-32)
    {"HALB", 68, 71, false},    // Row 4: HALB (44-47)
};

// Connector words
static const WordMapping CONNECTOR_WORDS[] = {
    {"VOR", 81, 79, false},     // Row 3: VOR (40-42)
    {"NACH", 89, 86, false},    // Row 3: NACH (33-36)
    {"UHR", 17, 15, false},    // Row 9: UHR (105-107)
};

// Minute dots for precise time (corner LEDs based on 125 LED total)
static const uint8_t MINUTE_DOTS[] = {124, 123, 12, 0}; // corner positions from image

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

// Time calculation functions
inline bool shouldShowBaseWords() { return true; } // Always show "ES IST"

// Hour word selection (0-23 hour format)
inline uint8_t getHourWordIndex(uint8_t hour, uint8_t minute) {
    // Adjust hour for German time display logic
    if (minute >= 25) hour = (hour + 1) % 24; // "X before next hour"
    return hour % 12; // Convert to 12-hour format for word selection
}

// Minute word selection and connector logic
inline int8_t getMinuteWordIndex(uint8_t minute) {
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

// Connector word logic
inline int8_t getConnectorWordIndex(uint8_t minute) {
    if (minute == 0) return 2;                     // UHR (o'clock)
    if (minute >= 5 && minute < 25) return 1;      // NACH (after/past)
    if (minute >= 25 && minute < 35) return -1;    // No connector for "halb"
    if (minute >= 35 && minute < 45) return 1;     // NACH (for "X nach halb")
    if (minute >= 45 && minute < 50) return -1;    // No connector for "dreiviertel"
    if (minute >= 50) return 0;                    // VOR (before/to)
    return -1; // No connector needed
}

// Minute dots calculation (1-4 dots for minutes 1-4)
inline uint8_t getMinuteDots(uint8_t minute) {
    return minute % 5; // 0-4, where 0 means no dots
}

// Special case handling for German time expressions
inline bool isHalfPast(uint8_t minute) {
    return minute >= 25 && minute < 35;
}

inline bool isDreiViertel(uint8_t minute) {
    return minute >= 45 && minute < 50; // Use DREIVIERTEL instead of VIERTEL VOR
}

// Special occasion handling (for birthday mode)
inline bool shouldShowBirthday() {
    // This could be controlled via web interface or special dates
    return false; // Normally false, enable for special occasions
}

// Weekday display logic
inline uint8_t getWeekdayIndex(uint8_t weekday) {
    // weekday: 0 = Sunday, 1 = Monday, 2 = Tuesday, ..., 6 = Saturday
    // Convert to our mapping: 0 = Monday, 1 = Tuesday, ..., 6 = Sunday
    if (weekday == 0) return 6;  // Sunday -> S (index 6)
    return weekday - 1;          // Monday-Saturday -> 0-5
}

// Weekday display enabling (configurable)
inline bool shouldShowWeekday() {
    return true; // Can be made configurable via web interface
}

#endif // MAPPING_45_H
