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
    {"SECHS", 20, 5, false},   // Row 9: SECHS (99-103)
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

// Function declarations - implementations in mapping_45.cpp
bool shouldShowBaseWords();
uint8_t getHourWordIndex(uint8_t hour, uint8_t minute);
int8_t getMinuteWordIndex(uint8_t minute);
int8_t getConnectorWordIndex(uint8_t minute);
uint8_t getMinuteDots(uint8_t minute);
bool isHalfPast(uint8_t minute);
bool isDreiViertel(uint8_t minute);
bool shouldShowBirthday();
uint8_t getWeekdayIndex(uint8_t weekday);
bool shouldShowWeekday();

#endif // MAPPING_45_H
