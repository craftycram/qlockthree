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
    rotationDegrees(0),
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

    // Load saved rotation
    rotationDegrees = preferences.getUShort("rotation", 0);
    // Validate rotation value
    if (rotationDegrees != 0 && rotationDegrees != 90 && rotationDegrees != 180 && rotationDegrees != 270) {
        rotationDegrees = 0;
    }

    Serial.println("LED Mapping Manager initialized");
    Serial.printf("Current mapping: %s (%s)\n", getCurrentMappingName(), getCurrentMappingId());
    Serial.printf("Rotation: %d degrees\n", rotationDegrees);
}

void LEDMappingManager::loadMapping(MappingType type) {
    Serial.printf("MAPPING DEBUG: loadMapping called with type %d\n", (int)type);

    // Update the current mapping type
    currentMappingType = type;

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
        uint8_t originalIndex = startLed + i;
        uint8_t transformedIndex = transformLedIndex(originalIndex);
        if (transformedIndex < currentMappingLEDCount) {
            ledStates[transformedIndex] = true;
        }
    }
}

void LEDMappingManager::illuminateMinuteDots(bool* ledStates, uint8_t numDots) {
    if (!ledStates || !minuteDotLEDs || numDots == 0) return;

    for (uint8_t i = 0; i < numDots && i < minuteDotCount; i++) {
        uint8_t originalIndex = minuteDotLEDs[i];
        uint8_t transformedIndex = transformLedIndex(originalIndex);
        if (transformedIndex < currentMappingLEDCount) {
            ledStates[transformedIndex] = true;
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
    uint8_t led;
    switch (currentMappingType) {
        case MappingType::MAPPING_45_GERMAN:
            led = Mapping45::STATUS_LED_WIFI;
            break;
        case MappingType::MAPPING_45BW_GERMAN:
            led = Mapping45BW::STATUS_LED_WIFI;
            break;
        case MappingType::MAPPING_110_GERMAN:
            led = 11;
            break;
        default:
            led = 11;
            break;
    }
    return transformLedIndex(led);
}

uint8_t LEDMappingManager::getSystemStatusLED() const {
    uint8_t led;
    switch (currentMappingType) {
        case MappingType::MAPPING_45_GERMAN:
            led = Mapping45::STATUS_LED_SYSTEM;
            break;
        case MappingType::MAPPING_45BW_GERMAN:
            led = Mapping45BW::STATUS_LED_SYSTEM;
            break;
        case MappingType::MAPPING_110_GERMAN:
            led = 10;
            break;
        default:
            led = 10;
            break;
    }
    return transformLedIndex(led);
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

uint8_t LEDMappingManager::getTransformedStartupLED(uint16_t sequenceIndex) const {
    const uint8_t* sequence = getStartupSequence();
    uint16_t length = getStartupSequenceLength();

    if (!sequence || sequenceIndex >= length) {
        return 0;
    }

    return transformLedIndex(sequence[sequenceIndex]);
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

// Rotation functions
uint16_t LEDMappingManager::getRotationDegrees() const {
    return rotationDegrees;
}

void LEDMappingManager::setRotationDegrees(uint16_t degrees) {
    if (degrees == 0 || degrees == 90 || degrees == 180 || degrees == 270) {
        rotationDegrees = degrees;
        Serial.printf("Rotation set to %d degrees\n", rotationDegrees);
    }
}

void LEDMappingManager::saveRotation() {
    preferences.putUShort("rotation", rotationDegrees);
    Serial.printf("Saved rotation: %d degrees\n", rotationDegrees);
}

// Coordinate transformation for rotation
// The LED grid is 11x11 with serpentine wiring pattern:
// - Row 0 (top): LEDs 112-122 (left to right)
// - Row 1: LEDs 111-101 (right to left)
// - Row 2: LEDs 90-100 (left to right)
// - ... alternating ...
// - Row 10 (bottom): LEDs 1-11 (left to right)
// - Corner dots: 0 (bottom-left), 12 (bottom-right), 123 (top-right), 124 (top-left)

void LEDMappingManager::indexToCoords(uint8_t ledIndex, int8_t& row, int8_t& col) const {
    // Handle corner dots specially
    if (ledIndex == 124) { row = -1; col = -1; return; }      // Top-left corner
    if (ledIndex == 123) { row = -1; col = 11; return; }      // Top-right corner
    if (ledIndex == 12)  { row = 11; col = 11; return; }      // Bottom-right corner
    if (ledIndex == 0)   { row = 11; col = -1; return; }      // Bottom-left corner

    // Main grid (LEDs 1-122, excluding corners)
    // Row 0 starts at LED 112, each row has 11 LEDs
    // Rows are numbered 0-10 from top to bottom
    if (ledIndex >= 1 && ledIndex <= 122) {
        // Calculate which row this LED is in
        // Row 10 (bottom): LEDs 1-11
        // Row 9: LEDs 13-23
        // Row 8: LEDs 24-34
        // Row 7: LEDs 35-45
        // Row 6: LEDs 46-56
        // Row 5: LEDs 57-67
        // Row 4: LEDs 68-78
        // Row 3: LEDs 79-89
        // Row 2: LEDs 90-100
        // Row 1: LEDs 101-111
        // Row 0: LEDs 112-122

        int rowFromBottom;
        int posInRow;

        if (ledIndex >= 112) {
            rowFromBottom = 10;
            posInRow = ledIndex - 112;
        } else if (ledIndex >= 101) {
            rowFromBottom = 9;
            posInRow = 111 - ledIndex;  // Reversed
        } else if (ledIndex >= 90) {
            rowFromBottom = 8;
            posInRow = ledIndex - 90;
        } else if (ledIndex >= 79) {
            rowFromBottom = 7;
            posInRow = 89 - ledIndex;   // Reversed
        } else if (ledIndex >= 68) {
            rowFromBottom = 6;
            posInRow = ledIndex - 68;
        } else if (ledIndex >= 57) {
            rowFromBottom = 5;
            posInRow = 67 - ledIndex;   // Reversed
        } else if (ledIndex >= 46) {
            rowFromBottom = 4;
            posInRow = ledIndex - 46;
        } else if (ledIndex >= 35) {
            rowFromBottom = 3;
            posInRow = 45 - ledIndex;   // Reversed
        } else if (ledIndex >= 24) {
            rowFromBottom = 2;
            posInRow = ledIndex - 24;
        } else if (ledIndex >= 13) {
            rowFromBottom = 1;
            posInRow = 23 - ledIndex;   // Reversed
        } else {  // 1-11
            rowFromBottom = 0;
            posInRow = ledIndex - 1;
        }

        row = 10 - rowFromBottom;  // Convert to row from top
        col = posInRow;
    } else {
        // Invalid index, return center
        row = 5;
        col = 5;
    }
}

uint8_t LEDMappingManager::coordsToIndex(int8_t row, int8_t col) const {
    // Handle corner dots
    if (row == -1 && col == -1) return 124;   // Top-left
    if (row == -1 && col == 11) return 123;   // Top-right
    if (row == 11 && col == 11) return 12;    // Bottom-right
    if (row == 11 && col == -1) return 0;     // Bottom-left

    // Clamp to valid grid range
    if (row < 0) row = 0;
    if (row > 10) row = 10;
    if (col < 0) col = 0;
    if (col > 10) col = 10;

    int rowFromBottom = 10 - row;

    // Calculate LED index based on row and serpentine pattern
    switch (rowFromBottom) {
        case 10: return 112 + col;           // Row 0: 112-122 (left to right)
        case 9:  return 111 - col;           // Row 1: 111-101 (right to left)
        case 8:  return 90 + col;            // Row 2: 90-100
        case 7:  return 89 - col;            // Row 3: 89-79
        case 6:  return 68 + col;            // Row 4: 68-78
        case 5:  return 67 - col;            // Row 5: 67-57
        case 4:  return 46 + col;            // Row 6: 46-56
        case 3:  return 45 - col;            // Row 7: 45-35
        case 2:  return 24 + col;            // Row 8: 24-34
        case 1:  return 23 - col;            // Row 9: 23-13
        case 0:  return 1 + col;             // Row 10: 1-11
        default: return 1;
    }
}

void LEDMappingManager::rotateCoords(int8_t& row, int8_t& col) const {
    if (rotationDegrees == 0) return;

    int8_t newRow, newCol;

    // For corner dots, we need to handle the -1 and 11 positions
    // The grid center is at (5, 5), but corners are outside the 0-10 range

    switch (rotationDegrees) {
        case 90:  // 90 degrees clockwise
            newRow = col;
            newCol = 10 - row;
            // Adjust for corners
            if (row == -1) newCol = 11;
            if (row == 11) newCol = -1;
            if (col == -1) newRow = -1;
            if (col == 11) newRow = 11;
            break;
        case 180:
            newRow = 10 - row;
            newCol = 10 - col;
            // Adjust for corners
            if (row == -1) newRow = 11;
            if (row == 11) newRow = -1;
            if (col == -1) newCol = 11;
            if (col == 11) newCol = -1;
            break;
        case 270: // 270 degrees clockwise (= 90 counter-clockwise)
            newRow = 10 - col;
            newCol = row;
            // Adjust for corners
            if (row == -1) newCol = -1;
            if (row == 11) newCol = 11;
            if (col == -1) newRow = 11;
            if (col == 11) newRow = -1;
            break;
        default:
            return;
    }
    row = newRow;
    col = newCol;
}

uint8_t LEDMappingManager::transformLedIndex(uint8_t originalIndex) const {
    if (rotationDegrees == 0) {
        return originalIndex;  // No transformation needed
    }

    int8_t row, col;
    indexToCoords(originalIndex, row, col);
    rotateCoords(row, col);
    return coordsToIndex(row, col);
}
