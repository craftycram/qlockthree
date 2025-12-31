# qlockthree LED Mappings

This directory contains **header-only** LED mapping files for different qlockthree layouts.

## 📁 File Structure

```
mappings/
├── mapping_base.h    <- Shared struct (include this in your mapping)
├── 45.h              <- 45cm German layout
├── 45bw.h            <- 45cm Swabian (BW) layout
└── README.md         <- This file
```

## 📝 Adding a New Mapping

To add a new mapping, you only need to create **ONE** header file (`.h`) in this folder. No `.cpp` files needed!

### Quick Start

1. **Copy the template**: Start by copying `45.h` as a template
2. **Rename**: Name it descriptively (e.g., `110.h`, `custom-layout.h`)
3. **Customize**: Update the mapping data
4. **Register**: Add the mapping to `led_mapping_manager.cpp` (see below)

### Example: Creating a 110-LED Mapping

```bash
# Copy the template
cp mappings/45.h mappings/110.h

# Edit the new file with your layout
# Update metadata, word positions, etc.
```

## 🏗️ Mapping Structure

Each mapping header must contain:

### 0. **Include Base** (Required)
```cpp
#include "mapping_base.h"
```

### 1. **Metadata** (Required)
```cpp
// Use constexpr to avoid macro conflicts between mappings
static constexpr const char* MAPPING_NAME = "Your Layout Name";
static constexpr const char* MAPPING_ID = "unique-id";
static constexpr const char* MAPPING_LANGUAGE = "DE";  // or "EN", "FR", etc.
static constexpr uint16_t MAPPING_TOTAL_LEDS = 125;
static constexpr const char* MAPPING_DESCRIPTION = "Detailed description";
```

### 2. **Word Mappings** (Required)
```cpp
// Base words (ES IST, IT IS, etc.)
static const WordMapping BASE_WORDS[] = {
    {"ES", 112, 2, false},  // word, start_led, length, active
};

// Hour words (0-11 or 0-23)
static const WordMapping HOUR_WORDS[] = {
    {"ZWÖLF", 61, 5, false},
    // ... more hours
};

// Minute words
static const WordMapping MINUTE_WORDS[] = {
    {"FÜNF", 119, 4, false},
    // ... more minutes
};

// Connector words (NACH, VOR, UHR, etc.)
static const WordMapping CONNECTOR_WORDS[] = {
    {"NACH", 86, 4, false},
    // ... more connectors
};
```

### 3. **Time Calculation Functions** (Required - Inline)
```cpp
// All functions must be inline for header-only implementation
inline bool shouldShowBaseWords() { 
    return true; 
}

inline uint8_t getHourWordIndex(uint8_t hour, uint8_t minute) {
    // Your logic here
    return hour % 12;
}

inline int8_t getMinuteWordIndex(uint8_t minute) {
    // Your logic here
    if (minute >= 5 && minute < 10) return 0;
    // ... more logic
    return -1;
}

inline int8_t getConnectorWordIndex(uint8_t minute) {
    // Your logic here
    return -1;
}

inline uint8_t getMinuteDots(uint8_t minute) {
    return minute % 5;
}
```

### 4. **Status LEDs** (Required)
```cpp
static constexpr uint8_t STATUS_LED_WIFI = 11;     // WiFi status indicator
static constexpr uint8_t STATUS_LED_SYSTEM = 10;   // System status indicator
```

### 5. **Startup Sequence** (Required)
```cpp
// Animation sequence - order LEDs light up during startup
static const uint8_t STARTUP_SEQUENCE[] = {
    112, 113, 114, 115, // ... all LEDs in desired order
};

static constexpr uint16_t STARTUP_SEQUENCE_LENGTH = sizeof(STARTUP_SEQUENCE) / sizeof(STARTUP_SEQUENCE[0]);
```

### 6. **Optional Features**

#### Minute Dots (Corner LEDs)
```cpp
static const uint8_t MINUTE_DOTS[] = {124, 123, 12, 0};
```

#### Weekday Display
```cpp
static const WordMapping WEEKDAY_WORDS[] = {
    {"M", 3, 1, false},  // Monday
    // ... more weekdays
};

inline uint8_t getWeekdayIndex(uint8_t weekday) {
    // Convert weekday number to mapping index
    if (weekday == 0) return 6;  // Sunday
    return weekday - 1;
}

inline bool shouldShowWeekday() {
    return true;
}
```

#### Special Words (Birthday, etc.)
```cpp
static const WordMapping SPECIAL_WORDS[] = {
    {"HAPPY", 86, 82, false},
    {"BIRTHDAY", 71, 78, false},
};

inline bool shouldShowBirthday() {
    return false;  // Enable for special occasions
}
```

## 🎯 Important Guidelines

### ✅ DO
- Use `inline` for all function implementations
- Use `static const` for all data arrays
- Include guard macros (`#ifndef MAPPING_XXX_H`)
- Test your LED positions carefully
- Document your layout with ASCII art or comments

### ❌ DON'T
- Create separate `.cpp` files (defeats the purpose!)
- Use global variables without `static`
- Forget `inline` keyword on functions
- Hardcode values - use constants

## 🔧 Testing Your Mapping

1. **Build the firmware**:
```bash
pio run
```

2. **Upload and check serial output** for errors

3. **Test the startup animation** to verify LED sequence

4. **Test time display** at different times:
   - On the hour (12:00, 1:00, etc.)
   - Half past (12:30, 1:30, etc.)
   - Quarter hours (12:15, 12:45, etc.)
   - 5-minute intervals

## 📦 Example Mappings

### 45cm German Layout (`45.h`)
- **LEDs**: 125 total (11×11 grid + 4 corner dots)
- **Language**: German (Standard)
- **Features**: Weekday display, minute dots, special words

### 45cm Swabian Layout (`45bw.h`)
- **LEDs**: 125 total (11×11 grid + 4 corner dots)
- **Language**: German (Swabian/Baden-Württemberg dialect)
- **Features**: Weekday display, minute dots, special words

### Adding Your Own

Want to create a mapping for:
- Different language (English, French, etc.)?
- Different grid size (10×10, 11×11, etc.)?
- Regional dialect?
- Custom word layout?

Just copy `45.h` as a template and customize!

## 🎨 Layout Planning Tips

1. **Sketch your layout**: Draw the letter grid on paper
2. **Number your LEDs**: Start from 0, follow your LED strip routing
3. **Map words to positions**: Note start position and length
4. **Test incrementally**: Test each word mapping individually
5. **Verify status LEDs**: Ensure they're accessible and visible

## 🚀 Registering Your Mapping

Once your mapping header is complete, you need to register it in the system:

### 1. Add to `led_mapping_manager.h`
```cpp
enum class MappingType {
    MAPPING_45_GERMAN,
    MAPPING_45BW_GERMAN,
    MAPPING_YOUR_NEW,      // Add your mapping here
    MAPPING_110_GERMAN,
    MAPPING_CUSTOM,
    MAPPING_COUNT
};
```

### 2. Add to `led_mapping_manager.cpp`
```cpp
// Include your header
#include "../mappings/your_new.h"

// Add case in loadMapping()
case MappingType::MAPPING_YOUR_NEW: {
    using namespace MappingYourNew;
    setMappingData(MAPPING_NAME, MAPPING_ID, MAPPING_DESCRIPTION, MAPPING_TOTAL_LEDS);
    setMappingArrays(...);
    // Set function pointers...
} break;

// Add to setCustomMapping()
} else if (String(mappingId) == "your_id") {
    loadMapping(MappingType::MAPPING_YOUR_NEW);

// Add to getAvailableMappingsJSON()
json += ",{\"name\":\"Your Name\",\"id\":\"your_id\",\"type\":X,\"led_count\":125,\"status\":\"active\"}";

// Add cases to getWiFiStatusLED(), getSystemStatusLED(), getStartupSequence(), getStartupSequenceLength()
// Add case to calculateTimeDisplayWithWeekday() if you have weekdays
```

### 3. Add to `web_server_manager.cpp`
```cpp
// Add option in handleLEDMapping() dropdown
html += "<option value='X'>Your Mapping Name</option>";

// Add case in handleSetLEDMapping()
case X:
    ledController->setMapping(MappingType::MAPPING_YOUR_NEW);
    break;
```

## 📚 Reference

- See `45.h` for a complete, working example
- Check `led_mapping_manager.cpp` for how mappings are loaded
- Review `led_controller.cpp` for how mappings are used

Happy mapping! 🎉
