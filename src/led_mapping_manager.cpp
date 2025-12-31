#include "led_mapping_manager.h"
#include "../mappings/45.h"
#include "../mappings/45bw.h"

// Helper macro to reduce boilerplate when loading mappings
// Use within a scope where the mapping namespace is imported
#define LOAD_MAPPING_FROM_HEADER() \
    setMappingData(MAPPING_NAME, MAPPING_ID, MAPPING_DESCRIPTION, MAPPING_TOTAL_LEDS); \
    setMappingArrays(BASE_WORDS, sizeof(BASE_WORDS)/sizeof(BASE_WORDS[0]), \
                    HOUR_WORDS, sizeof(HOUR_WORDS)/sizeof(HOUR_WORDS[0]), \
                    MINUTE_WORDS, sizeof(MINUTE_WORDS)/sizeof(MINUTE_WORDS[0]), \
                    CONNECTOR_WORDS, sizeof(CONNECTOR_WORDS)/sizeof(CONNECTOR_WORDS[0]), \
                    MINUTE_DOTS, sizeof(MINUTE_DOTS)/sizeof(MINUTE_DOTS[0])); \
    setMappingFunctions(shouldShowBaseWords, getHourWordIndex, getMinuteWordIndex, \
                       getConnectorWordIndex, getMinuteDots, isHalfPast)

LEDMappingManager::LEDMappingManager() :
    currentMappingType(MappingType::MAPPING_45_GERMAN),
    currentMappingName(nullptr),
    currentMappingId(nullptr),
    currentMappingDescription(nullptr),
    currentMappingLEDCount(125),
    shouldShowBaseWords(nullptr),
    getHourWordIndex(nullptr),
    getMinuteWordIndex(nullptr),
    getMinutePrefixWordIndex(nullptr),
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
    Serial.printf("MAPPING DEBUG: loadMapping called with type %d\n", (int)type);
    
    // Load mapping directly using namespace + macro - no separate functions needed!
    switch (type) {
        case MappingType::MAPPING_45_GERMAN: {
            Serial.println("MAPPING DEBUG: Loading 45cm German mapping...");
            using namespace Mapping45;
            
            // Set mapping metadata
            setMappingData(MAPPING_NAME, MAPPING_ID, MAPPING_DESCRIPTION, MAPPING_TOTAL_LEDS);
            
            // Set mapping arrays
            setMappingArrays(BASE_WORDS, sizeof(BASE_WORDS)/sizeof(BASE_WORDS[0]),
                            HOUR_WORDS, sizeof(HOUR_WORDS)/sizeof(HOUR_WORDS[0]),
                            MINUTE_WORDS, sizeof(MINUTE_WORDS)/sizeof(MINUTE_WORDS[0]),
                            CONNECTOR_WORDS, sizeof(CONNECTOR_WORDS)/sizeof(CONNECTOR_WORDS[0]),
                            MINUTE_DOTS, sizeof(MINUTE_DOTS)/sizeof(MINUTE_DOTS[0]));
            
            // Set function pointers directly (explicit namespace resolution)
            Serial.println("MAPPING DEBUG: Setting function pointers...");
            shouldShowBaseWords = Mapping45::shouldShowBaseWords;
            getHourWordIndex = Mapping45::getHourWordIndex;
            getMinuteWordIndex = Mapping45::getMinuteWordIndex;
            getMinutePrefixWordIndex = Mapping45::getMinutePrefixWordIndex;
            getConnectorWordIndex = Mapping45::getConnectorWordIndex;
            getMinuteDots = Mapping45::getMinuteDots;
            isHalfPast = Mapping45::isHalfPast;
            
            Serial.printf("MAPPING DEBUG: Function pointers set:\n");
            Serial.printf("  shouldShowBaseWords: %p\n", shouldShowBaseWords);
            Serial.printf("  getHourWordIndex: %p\n", getHourWordIndex);
            Serial.printf("  getMinuteWordIndex: %p\n", getMinuteWordIndex);
        } break;

        case MappingType::MAPPING_45BW_GERMAN: {
            Serial.println("MAPPING DEBUG: Loading 45cm Swabian (BW) German mapping...");
            using namespace Mapping45BW;

            // Set mapping metadata
            setMappingData(MAPPING_NAME, MAPPING_ID, MAPPING_DESCRIPTION, MAPPING_TOTAL_LEDS);

            // Set mapping arrays
            setMappingArrays(BASE_WORDS, sizeof(BASE_WORDS)/sizeof(BASE_WORDS[0]),
                            HOUR_WORDS, sizeof(HOUR_WORDS)/sizeof(HOUR_WORDS[0]),
                            MINUTE_WORDS, sizeof(MINUTE_WORDS)/sizeof(MINUTE_WORDS[0]),
                            CONNECTOR_WORDS, sizeof(CONNECTOR_WORDS)/sizeof(CONNECTOR_WORDS[0]),
                            MINUTE_DOTS, sizeof(MINUTE_DOTS)/sizeof(MINUTE_DOTS[0]));

            // Set function pointers
            shouldShowBaseWords = Mapping45BW::shouldShowBaseWords;
            getHourWordIndex = Mapping45BW::getHourWordIndex;
            getMinuteWordIndex = Mapping45BW::getMinuteWordIndex;
            getMinutePrefixWordIndex = Mapping45BW::getMinutePrefixWordIndex;
            getConnectorWordIndex = Mapping45BW::getConnectorWordIndex;
            getMinuteDots = Mapping45BW::getMinuteDots;
            isHalfPast = Mapping45BW::isHalfPast;
        } break;

        case MappingType::MAPPING_110_GERMAN: {
            // TODO: Create mappings/110.h, add #include, then uncomment:
            // using namespace Mapping110;
            // LOAD_MAPPING_FROM_HEADER();
            Serial.println("Warning: 110-LED mapping not yet implemented, falling back to 45cm");
            using namespace Mapping45;
            LOAD_MAPPING_FROM_HEADER();
        } break;
        
        default: {
            Serial.println("Warning: Unknown mapping type, falling back to 45cm");
            using namespace Mapping45;
            LOAD_MAPPING_FROM_HEADER();
        } break;
    }
    
    Serial.printf("Loaded mapping: %s\n", getCurrentMappingName());
    
    // Debug: Check if function pointers were set correctly
    Serial.printf("MAPPING DEBUG: After loading - function pointers:\n");
    Serial.printf("  shouldShowBaseWords: %p\n", shouldShowBaseWords);
    Serial.printf("  getHourWordIndex: %p\n", getHourWordIndex);
    Serial.printf("  getMinuteWordIndex: %p\n", getMinuteWordIndex);
    Serial.printf("  getConnectorWordIndex: %p\n", getConnectorWordIndex);
    Serial.printf("  getMinuteDots: %p\n", getMinuteDots);
    Serial.printf("  isHalfPast: %p\n", isHalfPast);
    
    // Debug: Check if word arrays were set correctly
    Serial.printf("MAPPING DEBUG: Word arrays:\n");
    Serial.printf("  baseWords: %p (count: %d)\n", baseWords, baseWordsCount);
    Serial.printf("  hourWords: %p (count: %d)\n", hourWords, hourWordsCount);
    Serial.printf("  minuteWords: %p (count: %d)\n", minuteWords, minuteWordsCount);
    Serial.printf("  connectorWords: %p (count: %d)\n", connectorWords, connectorWordsCount);
    Serial.printf("  minuteDotLEDs: %p (count: %d)\n", minuteDotLEDs, minuteDotCount);
}

void LEDMappingManager::setCustomMapping(const char* mappingId) {
    // Map string ID to MappingType enum
    if (String(mappingId) == "45") {
        loadMapping(MappingType::MAPPING_45_GERMAN);
    } else if (String(mappingId) == "45bw") {
        loadMapping(MappingType::MAPPING_45BW_GERMAN);
    } else if (String(mappingId) == "110") {
        loadMapping(MappingType::MAPPING_110_GERMAN);
    } else {
        // Unknown ID, use default
        loadMapping(MappingType::MAPPING_45_GERMAN);
    }
}

void LEDMappingManager::calculateTimeDisplay(uint8_t hour, uint8_t minute, bool* ledStates) {
    Serial.printf("MAPPING DEBUG: calculateTimeDisplay called with hour=%d, minute=%d\n", hour, minute);
    
    // Debug: Check what's missing
    Serial.printf("MAPPING DEBUG: ledStates=%p, shouldShowBaseWords=%p, getHourWordIndex=%p, getMinuteWordIndex=%p\n", 
                 ledStates, shouldShowBaseWords, getHourWordIndex, getMinuteWordIndex);
    Serial.printf("MAPPING DEBUG: connectorWords=%p, baseWords=%p, hourWords=%p, minuteWords=%p\n",
                 connectorWords, baseWords, hourWords, minuteWords);
    
    if (!ledStates || !shouldShowBaseWords || !getHourWordIndex || !getMinuteWordIndex) {
        Serial.println("MAPPING ERROR: Missing function pointers or ledStates array");
        Serial.printf("MAPPING ERROR: ledStates null: %s\n", !ledStates ? "YES" : "NO");
        Serial.printf("MAPPING ERROR: shouldShowBaseWords null: %s\n", !shouldShowBaseWords ? "YES" : "NO");
        Serial.printf("MAPPING ERROR: getHourWordIndex null: %s\n", !getHourWordIndex ? "YES" : "NO");
        Serial.printf("MAPPING ERROR: getMinuteWordIndex null: %s\n", !getMinuteWordIndex ? "YES" : "NO");
        return;
    }
    
    // Clear all LEDs first
    clearAllLEDs(ledStates);
    Serial.println("MAPPING DEBUG: LED states cleared");
    
    // Always show base words ("ES IST")
    if (shouldShowBaseWords()) {
        Serial.printf("MAPPING DEBUG: Showing base words (count: %d)\n", baseWordsCount);
        for (uint8_t i = 0; i < baseWordsCount; i++) {
            Serial.printf("MAPPING DEBUG: Base word %d: '%s' at LED %d, length %d\n", 
                         i, baseWords[i].word, baseWords[i].start_led, baseWords[i].length);
            illuminateWord(ledStates, baseWords[i]);
        }
    }
    
    // Show hour word
    uint8_t hourIndex = getHourWordIndex(hour, minute);
    Serial.printf("MAPPING DEBUG: Hour index calculated as %d (max: %d)\n", hourIndex, hourWordsCount);
    if (hourIndex < hourWordsCount) {
        Serial.printf("MAPPING DEBUG: Hour word: '%s' at LED %d, length %d\n", 
                     hourWords[hourIndex].word, hourWords[hourIndex].start_led, hourWords[hourIndex].length);
        illuminateWord(ledStates, hourWords[hourIndex]);
    }
    
    // Show minute prefix word (for "FÜNF VOR HALB" / "FÜNF NACH HALB" cases)
    if (getMinutePrefixWordIndex) {
        int8_t prefixIndex = getMinutePrefixWordIndex(minute);
        if (prefixIndex >= 0 && prefixIndex < minuteWordsCount) {
            Serial.printf("MAPPING DEBUG: Minute prefix word: '%s' at LED %d, length %d\n",
                         minuteWords[prefixIndex].word, minuteWords[prefixIndex].start_led, minuteWords[prefixIndex].length);
            illuminateWord(ledStates, minuteWords[prefixIndex]);
        }
    }

    // Show minute word
    int8_t minuteIndex = getMinuteWordIndex(minute);
    Serial.printf("MAPPING DEBUG: Minute index calculated as %d (max: %d)\n", minuteIndex, minuteWordsCount);
    if (minuteIndex >= 0 && minuteIndex < minuteWordsCount) {
        Serial.printf("MAPPING DEBUG: Minute word: '%s' at LED %d, length %d\n",
                     minuteWords[minuteIndex].word, minuteWords[minuteIndex].start_led, minuteWords[minuteIndex].length);
        illuminateWord(ledStates, minuteWords[minuteIndex]);
    }
    
    // Show connector word
    int8_t connectorIndex = getConnectorWordIndex(minute);
    Serial.printf("MAPPING DEBUG: Connector index calculated as %d (max: %d)\n", connectorIndex, connectorWordsCount);
    if (connectorIndex >= 0 && connectorIndex < connectorWordsCount) {
        Serial.printf("MAPPING DEBUG: Connector word: '%s' at LED %d, length %d\n", 
                     connectorWords[connectorIndex].word, connectorWords[connectorIndex].start_led, connectorWords[connectorIndex].length);
        illuminateWord(ledStates, connectorWords[connectorIndex]);
    }
    
    // Show minute dots (for precise minutes 1-4)
    if (getMinuteDots) {
        uint8_t dots = getMinuteDots(minute);
        Serial.printf("MAPPING DEBUG: Minute dots: %d\n", dots);
        illuminateMinuteDots(ledStates, dots);
    }
}

void LEDMappingManager::calculateTimeDisplayWithWeekday(uint8_t hour, uint8_t minute, uint8_t weekday, bool* ledStates) {
    // First calculate regular time display
    calculateTimeDisplay(hour, minute, ledStates);
    
    // Add weekday display based on current mapping
    switch (currentMappingType) {
        case MappingType::MAPPING_45_GERMAN: {
            if (Mapping45::shouldShowWeekday()) {
                uint8_t weekdayIndex = Mapping45::getWeekdayIndex(weekday);
                if (weekdayIndex < 7) {
                    illuminateWord(ledStates, Mapping45::WEEKDAY_WORDS[weekdayIndex]);
                }
            }
        } break;

        case MappingType::MAPPING_45BW_GERMAN: {
            if (Mapping45BW::shouldShowWeekday()) {
                uint8_t weekdayIndex = Mapping45BW::getWeekdayIndex(weekday);
                if (weekdayIndex < 7) {
                    illuminateWord(ledStates, Mapping45BW::WEEKDAY_WORDS[weekdayIndex]);
                }
            }
        } break;
        
        case MappingType::MAPPING_110_GERMAN: {
            // TODO: Add weekday support for 110-LED mapping when implemented
        } break;
        
        default:
            break;
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
    uint8_t savedType = preferences.getUChar("mapping_type", (uint8_t)MappingType::MAPPING_45_GERMAN);
    MappingType type = (MappingType)savedType;
    
    if (isValidMapping(type)) {
        loadMapping(type);
    } else {
        // Fallback to default - use the implemented 45cm mapping
        loadMapping(MappingType::MAPPING_45_GERMAN);
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
    json += "{\"name\":\"45cm German\",\"id\":\"45\",\"type\":0,\"led_count\":125,\"status\":\"active\"}";
    json += ",{\"name\":\"45cm Swabian (BW)\",\"id\":\"45bw\",\"type\":1,\"led_count\":125,\"status\":\"active\"}";
    // Uncomment when 110-LED mapping is implemented:
    // json += ",{\"name\":\"110-LED German\",\"id\":\"110\",\"type\":2,\"led_count\":110,\"status\":\"coming_soon\"}";
    json += "]";
    return json;
}

// Status LED and startup sequence configuration
uint8_t LEDMappingManager::getWiFiStatusLED() const {
    switch (currentMappingType) {
        case MappingType::MAPPING_45_GERMAN:
            return Mapping45::STATUS_LED_WIFI;
        case MappingType::MAPPING_45BW_GERMAN:
            return Mapping45BW::STATUS_LED_WIFI;
        case MappingType::MAPPING_110_GERMAN:
            return 11;
        default:
            return 11;
    }
}

uint8_t LEDMappingManager::getSystemStatusLED() const {
    switch (currentMappingType) {
        case MappingType::MAPPING_45_GERMAN:
            return Mapping45::STATUS_LED_SYSTEM;
        case MappingType::MAPPING_45BW_GERMAN:
            return Mapping45BW::STATUS_LED_SYSTEM;
        case MappingType::MAPPING_110_GERMAN:
            return 10;
        default:
            return 10;
    }
}

const uint8_t* LEDMappingManager::getStartupSequence() const {
    switch (currentMappingType) {
        case MappingType::MAPPING_45_GERMAN:
            return Mapping45::STARTUP_SEQUENCE;
        case MappingType::MAPPING_45BW_GERMAN:
            return Mapping45BW::STARTUP_SEQUENCE;
        case MappingType::MAPPING_110_GERMAN:
            return Mapping45::STARTUP_SEQUENCE;
        default:
            return Mapping45::STARTUP_SEQUENCE;
    }
}

uint16_t LEDMappingManager::getStartupSequenceLength() const {
    switch (currentMappingType) {
        case MappingType::MAPPING_45_GERMAN:
            return Mapping45::STARTUP_SEQUENCE_LENGTH;
        case MappingType::MAPPING_45BW_GERMAN:
            return Mapping45BW::STARTUP_SEQUENCE_LENGTH;
        case MappingType::MAPPING_110_GERMAN:
            return Mapping45::STARTUP_SEQUENCE_LENGTH;
        default:
            return Mapping45::STARTUP_SEQUENCE_LENGTH;
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
