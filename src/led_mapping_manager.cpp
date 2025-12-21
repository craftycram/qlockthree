#include "led_mapping_manager.h"
#include "../mappings/45.h"

LEDMappingManager::LEDMappingManager() : 
    currentMappingType(MappingType::MAPPING_45_GERMAN),
    currentMappingName(nullptr),
    currentMappingId(nullptr),
    currentMappingDescription(nullptr),
    currentMappingLEDCount(125),
    shouldShowBaseWords(nullptr),
    getHourWordIndex(nullptr),
    getMinuteWordIndex(nullptr),
    getConnectorWordIndex(nullptr),
    getMinuteDots(nullptr),
    isHalfPast(nullptr),
    baseWords(nullptr),
    hourWords(nullptr),
    minuteWords(nullptr),
    connectorWords(nullptr),
    minuteDotLEDs(nullptr),
    baseWordsCount(0),
    hourWordsCount(0),
    minuteWordsCount(0),
    connectorWordsCount(0),
    minuteDotCount(0) {
}

void LEDMappingManager::begin() {
    preferences.begin("led_mapping", false);
    loadSavedMapping();
    
    Serial.println("LED Mapping Manager initialized");
    Serial.printf("Current mapping: %s (%s)\n", getCurrentMappingName(), getCurrentMappingId());
}

void LEDMappingManager::loadMapping(MappingType type) {
    currentMappingType = MappingType::MAPPING_45_GERMAN;
    
    // Only load the 45cm layout
    load45GermanMapping();
    
    Serial.printf("Loaded mapping: %s\n", getCurrentMappingName());
}

void LEDMappingManager::setCustomMapping(const char* mappingId) {
    loadCustomMapping(mappingId);
}

void LEDMappingManager::calculateTimeDisplay(uint8_t hour, uint8_t minute, bool* ledStates) {
    if (!ledStates || !shouldShowBaseWords || !getHourWordIndex || !getMinuteWordIndex) {
        return;
    }
    
    // Clear all LEDs first
    clearAllLEDs(ledStates);
    
    // Always show base words ("ES IST")
    if (shouldShowBaseWords()) {
        for (uint8_t i = 0; i < baseWordsCount; i++) {
            illuminateWord(ledStates, baseWords[i]);
        }
    }
    
    // Show hour word
    uint8_t hourIndex = getHourWordIndex(hour, minute);
    if (hourIndex < hourWordsCount) {
        illuminateWord(ledStates, hourWords[hourIndex]);
    }
    
    // Show minute word
    int8_t minuteIndex = getMinuteWordIndex(minute);
    if (minuteIndex >= 0 && minuteIndex < minuteWordsCount) {
        illuminateWord(ledStates, minuteWords[minuteIndex]);
    }
    
    // Show connector word
    int8_t connectorIndex = getConnectorWordIndex(minute);
    if (connectorIndex >= 0 && connectorIndex < connectorWordsCount) {
        illuminateWord(ledStates, connectorWords[connectorIndex]);
    }
    
    // Show minute dots (for precise minutes 1-4)
    if (getMinuteDots) {
        uint8_t dots = getMinuteDots(minute);
        illuminateMinuteDots(ledStates, dots);
    }
}

void LEDMappingManager::calculateTimeDisplayWithWeekday(uint8_t hour, uint8_t minute, uint8_t weekday, bool* ledStates) {
    // First calculate regular time display
    calculateTimeDisplay(hour, minute, ledStates);
    
    // Add weekday if enabled (from mapping functions)
    extern bool shouldShowWeekday();
    extern uint8_t getWeekdayIndex(uint8_t weekday);
    
    if (shouldShowWeekday()) {
        uint8_t weekdayIndex = getWeekdayIndex(weekday);
        if (weekdayIndex < 7) { // 7 weekdays (M D M D F S S)
            // Illuminate the appropriate weekday LED
            extern const WordMapping WEEKDAY_WORDS[];
            if (weekdayIndex < 7) {
                illuminateWord(ledStates, WEEKDAY_WORDS[weekdayIndex]);
            }
        }
    }
}

void LEDMappingManager::clearAllLEDs(bool* ledStates) {
    if (ledStates) {
        for (uint16_t i = 0; i < currentMappingLEDCount; i++) {
            ledStates[i] = false;
        }
    }
}

void LEDMappingManager::illuminateWord(bool* ledStates, const WordMapping& word) {
    illuminateRange(ledStates, word.start_led, word.length);
}

void LEDMappingManager::illuminateRange(bool* ledStates, uint8_t startLed, uint8_t length) {
    if (!ledStates) return;
    
    for (uint8_t i = 0; i < length; i++) {
        uint16_t ledIndex = startLed + i;
        if (ledIndex < currentMappingLEDCount) {
            ledStates[ledIndex] = true;
        }
    }
}

void LEDMappingManager::illuminateMinuteDots(bool* ledStates, uint8_t numDots) {
    if (!ledStates || !minuteDotLEDs || numDots == 0) return;
    
    for (uint8_t i = 0; i < numDots && i < minuteDotCount; i++) {
        uint16_t ledIndex = minuteDotLEDs[i];
        if (ledIndex < currentMappingLEDCount) {
            ledStates[ledIndex] = true;
        }
    }
}

// Getters
const char* LEDMappingManager::getCurrentMappingName() const {
    return currentMappingName ? currentMappingName : "Unknown";
}

const char* LEDMappingManager::getCurrentMappingId() const {
    return currentMappingId ? currentMappingId : "unknown";
}

const char* LEDMappingManager::getCurrentMappingDescription() const {
    return currentMappingDescription ? currentMappingDescription : "No description";
}

uint16_t LEDMappingManager::getCurrentMappingLEDCount() const {
    return currentMappingLEDCount;
}

MappingType LEDMappingManager::getCurrentMappingType() const {
    return currentMappingType;
}

// Mapping management
void LEDMappingManager::saveCurrentMapping() {
    preferences.putUChar("mapping_type", (uint8_t)currentMappingType);
    if (currentMappingId) {
        preferences.putString("mapping_id", currentMappingId);
    }
    Serial.printf("Saved mapping: %s\n", getCurrentMappingName());
}

void LEDMappingManager::loadSavedMapping() {
    uint8_t savedType = preferences.getUChar("mapping_type", (uint8_t)MappingType::MAPPING_110_GERMAN);
    MappingType type = (MappingType)savedType;
    
    if (isValidMapping(type)) {
        loadMapping(type);
    } else {
        // Fallback to default
        loadMapping(MappingType::MAPPING_110_GERMAN);
    }
    
    Serial.printf("Loaded saved mapping: %s\n", getCurrentMappingName());
}

bool LEDMappingManager::isValidMapping(MappingType type) const {
    return type >= MappingType::MAPPING_45_GERMAN && type < MappingType::MAPPING_COUNT;
}

// JSON generators for web interface
String LEDMappingManager::getMappingInfoJSON() const {
    String json = "{";
    json += "\"name\":\"" + String(getCurrentMappingName()) + "\",";
    json += "\"id\":\"" + String(getCurrentMappingId()) + "\",";
    json += "\"description\":\"" + String(getCurrentMappingDescription()) + "\",";
    json += "\"led_count\":" + String(getCurrentMappingLEDCount()) + ",";
    json += "\"type\":" + String((int)getCurrentMappingType());
    json += "}";
    return json;
}

String LEDMappingManager::getAvailableMappingsJSON() const {
    String json = "[";
    json += "{\"name\":\"45-LED German\",\"id\":\"45\",\"type\":0,\"led_count\":45},";
    json += "{\"name\":\"110-LED German\",\"id\":\"110\",\"type\":1,\"led_count\":110}";
    json += "]";
    return json;
}

// Status LED and startup sequence configuration
uint8_t LEDMappingManager::getWiFiStatusLED() const {
    // Return mapping-specific WiFi status LED index
    switch (currentMappingType) {
        case MappingType::MAPPING_45_GERMAN:
            return STATUS_LED_WIFI; // Defined in mappings/45.h
        case MappingType::MAPPING_110_GERMAN:
            return 11; // Default for 110-LED mapping
        default:
            return 11; // Default fallback
    }
}

uint8_t LEDMappingManager::getSystemStatusLED() const {
    // Return mapping-specific system status LED index
    switch (currentMappingType) {
        case MappingType::MAPPING_45_GERMAN:
            return STATUS_LED_SYSTEM; // Defined in mappings/45.h
        case MappingType::MAPPING_110_GERMAN:
            return 10; // Default for 110-LED mapping
        default:
            return 10; // Default fallback
    }
}

const uint8_t* LEDMappingManager::getStartupSequence() const {
    // Return mapping-specific startup sequence
    switch (currentMappingType) {
        case MappingType::MAPPING_45_GERMAN:
            return STARTUP_SEQUENCE; // Defined in mappings/45.h
        case MappingType::MAPPING_110_GERMAN:
            // Could define a different sequence for 110-LED mapping
            return STARTUP_SEQUENCE; // Use same sequence for now
        default:
            return STARTUP_SEQUENCE; // Default fallback
    }
}

uint16_t LEDMappingManager::getStartupSequenceLength() const {
    // Return mapping-specific startup sequence length
    switch (currentMappingType) {
        case MappingType::MAPPING_45_GERMAN:
            return STARTUP_SEQUENCE_LENGTH; // Defined in mappings/45.h
        case MappingType::MAPPING_110_GERMAN:
            return STARTUP_SEQUENCE_LENGTH; // Use same length for now
        default:
            return STARTUP_SEQUENCE_LENGTH; // Default fallback
    }
}

// Private mapping loading functions
void LEDMappingManager::load45GermanMapping() {
    // Include the 45cm mapping functions and data
    // Note: This uses the inline functions from mappings/45.h
    
    setMappingData(MAPPING_NAME, MAPPING_ID, MAPPING_DESCRIPTION, MAPPING_TOTAL_LEDS);
    
    // Set up arrays - these are from the 45.h mapping file
    setMappingArrays(BASE_WORDS, 2,      // ES, IST
                    HOUR_WORDS, 12,     // 12 hour words
                    MINUTE_WORDS, 6,    // 6 minute intervals (including DREIVIERTEL)
                    CONNECTOR_WORDS, 3, // VOR, NACH, UHR
                    MINUTE_DOTS, 4);    // 4 corner dots
    
    // Set up function pointers - these are the inline functions from 45.h
    setMappingFunctions(::shouldShowBaseWords, ::getHourWordIndex, ::getMinuteWordIndex,
                       ::getConnectorWordIndex, ::getMinuteDots, ::isHalfPast);
}

void LEDMappingManager::load110GermanMapping() {
    // Include the 110-LED mapping functions and data
    // Note: This uses the inline functions from mappings/110.h
    
    setMappingData("110-LED German Layout", "110", "Standard German qlockthree 11x10 grid", 110);
    
    // Set up arrays - these are from the 110.h mapping file
    extern const WordMapping BASE_WORDS[];
    extern const WordMapping HOUR_WORDS[];
    extern const WordMapping MINUTE_WORDS[];
    extern const WordMapping CONNECTOR_WORDS[];
    extern const uint8_t MINUTE_DOTS[];
    
    setMappingArrays(BASE_WORDS, 2,      // ES, IST
                    HOUR_WORDS, 12,     // 12 hour words
                    MINUTE_WORDS, 6,    // 6 minute intervals (including DREIVIERTEL)
                    CONNECTOR_WORDS, 3, // VOR, NACH, UHR
                    MINUTE_DOTS, 4);    // 4 corner dots
    
    // Set up function pointers - these are the inline functions from 110.h
    extern bool shouldShowBaseWords();
    extern uint8_t getHourWordIndex(uint8_t hour, uint8_t minute);
    extern int8_t getMinuteWordIndex(uint8_t minute);
    extern int8_t getConnectorWordIndex(uint8_t minute);
    extern uint8_t getMinuteDots(uint8_t minute);
    extern bool isHalfPast(uint8_t minute);
    
    setMappingFunctions(shouldShowBaseWords, getHourWordIndex, getMinuteWordIndex,
                       getConnectorWordIndex, getMinuteDots, isHalfPast);
}

void LEDMappingManager::loadCustomMapping(const char* mappingId) {
    // For now, fall back to a default mapping
    // In the future, this could load custom mappings from files or preferences
    if (String(mappingId) == "45") {
        load45GermanMapping();
    } else {
        load110GermanMapping();
    }
}

// Helper functions
void LEDMappingManager::setMappingData(const char* name, const char* id, const char* description, uint16_t ledCount) {
    currentMappingName = name;
    currentMappingId = id;
    currentMappingDescription = description;
    currentMappingLEDCount = ledCount;
}

void LEDMappingManager::setMappingArrays(const WordMapping* base, uint8_t baseCount,
                                       const WordMapping* hours, uint8_t hourCount,
                                       const WordMapping* minutes, uint8_t minuteCount,
                                       const WordMapping* connectors, uint8_t connectorCount,
                                       const uint8_t* dots, uint8_t dotCount) {
    baseWords = base;
    hourWords = hours;
    minuteWords = minutes;
    connectorWords = connectors;
    minuteDotLEDs = dots;
    
    baseWordsCount = baseCount;
    hourWordsCount = hourCount;
    minuteWordsCount = minuteCount;
    connectorWordsCount = connectorCount;
    minuteDotCount = dotCount;
}

void LEDMappingManager::setMappingFunctions(bool (*showBase)(),
                                          uint8_t (*hourIndex)(uint8_t, uint8_t),
                                          int8_t (*minuteIndex)(uint8_t),
                                          int8_t (*connectorIndex)(uint8_t),
                                          uint8_t (*minuteDots)(uint8_t),
                                          bool (*halfPast)(uint8_t)) {
    shouldShowBaseWords = showBase;
    getHourWordIndex = hourIndex;
    getMinuteWordIndex = minuteIndex;
    getConnectorWordIndex = connectorIndex;
    getMinuteDots = minuteDots;
    isHalfPast = halfPast;
}
