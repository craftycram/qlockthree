# qlockthree LED Mappings

This directory contains **header-only** LED mapping files for different qlockthree layouts.

## 📝 Adding a New Mapping

To add a new mapping, you only need to create **ONE** header file (`.h`) in this folder. No `.cpp` files needed!

### Quick Start

1. **Copy the template**: Start by copying `45.h` as a template
2. **Rename**: Name it descriptively (e.g., `110.h`, `custom-layout.h`)
3. **Customize**: Update the mapping data
4. **Done!** The system will automatically discover it

### Example: Creating a 110-LED Mapping

```bash
# Copy the template
cp mappings/45.h mappings/110.h

# Edit the new file with your layout
# Update metadata, word positions, etc.
```

## 🏗️ Mapping Structure

Each mapping header must contain:

### 1. **Metadata** (Required)
```cpp
#define MAPPING_NAME "Your Layout Name"
#define MAPPING_ID "unique-id"
#define MAPPING_LANGUAGE "DE"  // or "EN", "FR", etc.
#define MAPPING_TOTAL_LEDS 125
#define MAPPING_DESCRIPTION "Detailed description"
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
#define STATUS_LED_WIFI 11     // WiFi status indicator
#define STATUS_LED_SYSTEM 10   // System status indicator
```

### 5. **Startup Sequence** (Required)
```cpp
// Animation sequence - order LEDs light up during startup
static const uint8_t STARTUP_SEQUENCE[] = {
    112, 113, 114, 115, // ... all LEDs in desired order
};

#define STARTUP_SEQUENCE_LENGTH (sizeof(STARTUP_SEQUENCE) / sizeof(STARTUP_SEQUENCE[0]))
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

### 45cm Layout (`45.h`)
- **LEDs**: 125 total (11×11 grid + 4 corner dots)
- **Language**: German
- **Features**: Weekday display, minute dots, special words

### Adding Your Own

Want to create a mapping for:
- Different language (English, French, etc.)?
- Different grid size (10×10, 11×11, etc.)?
- Custom word layout?

Just copy `45.h` as a template and customize!

## 🎨 Layout Planning Tips

1. **Sketch your layout**: Draw the letter grid on paper
2. **Number your LEDs**: Start from 0, follow your LED strip routing
3. **Map words to positions**: Note start position and length
4. **Test incrementally**: Test each word mapping individually
5. **Verify status LEDs**: Ensure they're accessible and visible

## 🚀 Automatic Integration

Once your mapping header is complete:

1. The LED Mapping Manager will **automatically discover** it
2. Available in the **web interface** mapping selection
3. No code changes needed in other files
4. LED count updates automatically
5. Startup sequence adapts to your layout

## 📚 Reference

- See `45.h` for a complete, working example
- Check `led_mapping_manager.cpp` for how mappings are loaded
- Review `led_controller.cpp` for how mappings are used

Happy mapping! 🎉
