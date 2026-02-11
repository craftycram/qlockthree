#ifndef MAPPING_69_H
#define MAPPING_69_H

#include "mapping_base.h"

namespace Mapping69
{

    // Mapping metadata (using constexpr to avoid macro conflicts with other mappings)
    static constexpr const char *MAPPING_NAME = "69cm qlockthree 11x11";
    static constexpr const char *MAPPING_ID = "69cm";
    static constexpr const char *MAPPING_LANGUAGE = "DE";
    static constexpr uint16_t MAPPING_TOTAL_LEDS = 125;
    static constexpr const char *MAPPING_DESCRIPTION = "69cm German qlockthree with 11x11 grid, weekdays, and 4 corner dots";

    static const WordMapping BASE_WORDS[] = {
        {"ES", 0, 4, false},  // Zeile 0 (Normal)
        {"IST", 6, 6, false}, // Zeile 0 (Normal)
    };

    static const WordMapping HOUR_WORDS[] = {
        {"ZWÖLF", 188, 10, false},  // Zeile 8 (Normal)
        {"EINS", 124, 8, true},     // Zeile 5 (Reversed)
        {"ZWEI", 110, 8, true},     // Zeile 5 (Reversed)
        {"DREI", 132, 8, false},    // Zeile 6 (Normal)
        {"VIER", 146, 8, false},    // Zeile 6 (Normal)
        {"FÜNF", 102, 8, false},    // Zeile 4 (Normal) - Teilt 'F' mit ELF
        {"SECHS", 166, 10, true},   // Zeile 7 (Reversed)
        {"SIEBEN", 176, 12, false}, // Zeile 8 (Normal)
        {"ACHT", 154, 8, true},     // Zeile 7 (Reversed)
        {"NEUN", 206, 8, true},     // Zeile 9 (Reversed) - Teilt 'N' mit ZEHN
        {"ZEHN", 212, 8, true},     // Zeile 9 (Reversed)
        {"ELF", 98, 6, false},      // Zeile 4 (Normal)
    };

    static const WordMapping MINUTE_WORDS[] = {
        {"FÜNF", 14, 8, false},         // Zeile 0 (Normal)
        {"ZEHN", 36, 8, true},          // Zeile 1 (Reversed)
        {"VIERTEL", 52, 14, false},     // Zeile 2 (Normal)
        {"ZWANZIG", 22, 14, true},      // Zeile 1 (Reversed)
        {"DREIVIERTEL", 44, 22, false}, // Zeile 2 (Normal)
        {"HALB", 88, 8, false},         // Zeile 4 (Normal)
    };

    static const WordMapping CONNECTOR_WORDS[] = {
        {"VOR", 82, 6, true},  // Zeile 3 (Reversed)
        {"NACH", 66, 8, true}, // Zeile 3 (Reversed)
        {"UHR", 198, 6, true}, // Zeile 9 (Reversed)
    };
    static const uint8_t MINUTE_DOTS[] = {124, 123, 12, 0};

    static constexpr uint8_t STATUS_LED_WIFI = 11;
    static constexpr uint8_t STATUS_LED_SYSTEM = 10;

    static const uint8_t STARTUP_SEQUENCE[] = {
        // 1st row: indices 112-122
        112,
        113,
        114,
        115,
        116,
        117,
        118,
        119,
        120,
        121,
        122,
        // 2nd row: indices 111-101 (reverse order)
        111,
        110,
        109,
        108,
        107,
        106,
        105,
        104,
        103,
        102,
        101,
        // 3rd row: indices 90-100
        90,
        91,
        92,
        93,
        94,
        95,
        96,
        97,
        98,
        99,
        100,
        // 4th row: indices 89-79 (reverse order)
        89,
        88,
        87,
        86,
        85,
        84,
        83,
        82,
        81,
        80,
        79,
        // 5th row: indices 68-78
        68,
        69,
        70,
        71,
        72,
        73,
        74,
        75,
        76,
        77,
        78,
        // 6th row: indices 67-57 (reverse order)
        67,
        66,
        65,
        64,
        63,
        62,
        61,
        60,
        59,
        58,
        57,
        // 7th row: indices 46-56
        46,
        47,
        48,
        49,
        50,
        51,
        52,
        53,
        54,
        55,
        56,
        // 8th row: indices 45-35 (reverse order)
        45,
        44,
        43,
        42,
        41,
        40,
        39,
        38,
        37,
        36,
        35,
        // 9th row: indices 24-34
        24,
        25,
        26,
        27,
        28,
        29,
        30,
        31,
        32,
        33,
        34,
        // 10th row: indices 23-13 (reverse order)
        23,
        22,
        21,
        20,
        19,
        18,
        17,
        16,
        15,
        14,
        13,
        // 11th row: indices 1-11
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
    };

    // Calculate sequence length (using constexpr instead of #define to work with namespaces)
    static constexpr uint16_t STARTUP_SEQUENCE_LENGTH = sizeof(STARTUP_SEQUENCE) / sizeof(STARTUP_SEQUENCE[0]);

    static const WordMapping WEEKDAY_WORDS[] = {
        {"M", 3, 1, false}, // Monday (Montag)
        {"D", 4, 1, false}, // Tuesday (Dienstag)
        {"M", 5, 1, false}, // Wednesday (Mittwoch)
        {"D", 6, 1, false}, // Thursday (Donnerstag)
        {"F", 7, 1, false}, // Friday (Freitag)
        {"S", 8, 1, false}, // Saturday (Samstag)
        {"S", 9, 1, false}, // Sunday (Sonntag)
    };

    // Special words (for birthday/special occasions - not used in normal time display)
    static const WordMapping SPECIAL_WORDS[] = {
        {"HAPPY", 82, 5, false},
        {"BIRTHDAY", 71, 8, false},
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
    // | 45-49   | ES IST VIERTEL VOR X+1    | VIERTEL + VOR + hour             |
    // | 50-54   | ES IST ZEHN VOR X+1       | ZEHN + VOR + hour                |
    // | 55-59   | ES IST FÜNF VOR X+1       | FÜNF + VOR + hour                |
    // +---------+---------------------------+----------------------------------+
    // Note: For minutes >= 25, hour is incremented (X+1 = next hour)

    inline bool shouldShowBaseWords()
    {
        return true;
    }

    inline uint8_t getHourWordIndex(uint8_t hour, uint8_t minute)
    {
        // Adjust hour for German time display logic
        if (minute >= 25)
            hour = (hour + 1) % 24; // "X before next hour"
        return hour % 12;           // Convert to 12-hour format for word selection
    }

    inline int8_t getMinuteWordIndex(uint8_t minute)
    {
        if (minute >= 5 && minute < 10)
            return 0; // FÜNF nach
        if (minute >= 10 && minute < 15)
            return 1; // ZEHN nach
        if (minute >= 15 && minute < 20)
            return 2; // VIERTEL nach
        if (minute >= 20 && minute < 25)
            return 3; // ZWANZIG nach
        if (minute >= 25 && minute < 40)
            return 5; // HALB (with prefix for 25-29, 35-39)
        if (minute >= 40 && minute < 45)
            return 3; // ZWANZIG vor
        if (minute >= 45 && minute < 50)
            return 2; // VIERTEL vor
        if (minute >= 50 && minute < 55)
            return 1; // ZEHN vor
        if (minute >= 55)
            return 0; // FÜNF vor
        return -1;    // Exact hour, no minute word
    }

    // Returns prefix minute word index for "X VOR/NACH HALB" cases
    // Returns the minute word to show before HALB
    inline int8_t getMinutePrefixWordIndex(uint8_t minute)
    {
        if (minute >= 25 && minute < 30)
            return 0; // FÜNF vor halb
        if (minute >= 35 && minute < 40)
            return 0; // FÜNF nach halb
        return -1;    // No prefix needed
    }

    inline int8_t getConnectorWordIndex(uint8_t minute)
    {
        if (minute < 5)
            return 2; // UHR (o'clock)
        if (minute >= 5 && minute < 25)
            return 1; // NACH (after/past)
        if (minute >= 25 && minute < 30)
            return 0; // VOR (for "fünf vor halb")
        if (minute >= 30 && minute < 35)
            return -1; // No connector for "halb"
        if (minute >= 35 && minute < 40)
            return 1; // NACH (for "fünf nach halb")
        if (minute >= 40 && minute < 45)
            return 0; // VOR (for "zwanzig vor")
        if (minute >= 45 && minute < 50)
            return 0; // No connector for "dreiviertel"
        if (minute >= 50)
            return 0; // VOR (before/to)
        return -1;    // No connector needed
    }

    inline uint8_t getMinuteDots(uint8_t minute)
    {
        return minute % 5; // 0-4, where 0 means no dots
    }

    inline bool isHalfPast(uint8_t minute)
    {
        return minute >= 25 && minute < 35;
    }

    inline bool isDreiViertel(uint8_t minute)
    {
        return minute >= 45 && minute < 50; // Use DREIVIERTEL instead of VIERTEL VOR
    }

    inline bool shouldShowBirthday()
    {
        return false; // Normally false, enable for special occasions
    }

    inline uint8_t getWeekdayIndex(uint8_t weekday)
    {
        // weekday: 0 = Sunday, 1 = Monday, 2 = Tuesday, ..., 6 = Saturday
        // Convert to our mapping: 0 = Monday, 1 = Tuesday, ..., 6 = Sunday
        if (weekday == 0)
            return 6;       // Sunday -> S (index 6)
        return weekday - 1; // Monday-Saturday -> 0-5
    }

    inline bool shouldShowWeekday()
    {
        return true; // Can be made configurable via web interface
    }

} // namespace Mapping69

#endif // MAPPING_69_H
