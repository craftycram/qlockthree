#ifndef LED_MAPPING_MANAGER_H
#define LED_MAPPING_MANAGER_H

#include <stdint.h>
#include <Preferences.h>

// Forward declarations
struct WordMapping;

// Mapping types
enum class MappingType {
    MAPPING_45_GERMAN,
    MAPPING_110_GERMAN,
    MAPPING_CUSTOM,
    MAPPING_COUNT
};

class LEDMappingManager {
public:
    LEDMappingManager();
    
    // Configuration
    void begin();
    void loadMapping(MappingType type);
    void setCustomMapping(const char* mappingId);
    
    // Time display logic
    void calculateTimeDisplay(uint8_t hour, uint8_t minute, bool* ledStates);
    void calculateTimeDisplayWithWeekday(uint8_t hour, uint8_t minute, uint8_t weekday, bool* ledStates);
    void clearAllLEDs(bool* ledStates);
    
    // Mapping information
    const char* getCurrentMappingName() const;
    const char* getCurrentMappingId() const;
    const char* getCurrentMappingDescription() const;
    uint16_t getCurrentMappingLEDCount() const;
    MappingType getCurrentMappingType() const;
    
    // Mapping management
    void saveCurrentMapping();
    void loadSavedMapping();
    bool isValidMapping(MappingType type) const;
    
    // Word illumination
    void illuminateWord(bool* ledStates, const WordMapping& word);
    void illuminateRange(bool* ledStates, uint8_t startLed, uint8_t length);
    void illuminateMinuteDots(bool* ledStates, uint8_t numDots);
    
    // Getters for web interface
    String getMappingInfoJSON() const;
    String getAvailableMappingsJSON() const;
    
    // Status LED and startup sequence configuration
    uint8_t getWiFiStatusLED() const;
    uint8_t getSystemStatusLED() const;
    const uint8_t* getStartupSequence() const;
    uint16_t getStartupSequenceLength() const;

private:
    Preferences preferences;
    MappingType currentMappingType;
    
    // Current mapping data pointers
    const char* currentMappingName;
    const char* currentMappingId;
    const char* currentMappingDescription;
    uint16_t currentMappingLEDCount;
    
    // Current mapping function pointers
    bool (*shouldShowBaseWords)();
    uint8_t (*getHourWordIndex)(uint8_t hour, uint8_t minute);
    int8_t (*getMinuteWordIndex)(uint8_t minute);
    int8_t (*getConnectorWordIndex)(uint8_t minute);
    uint8_t (*getMinuteDots)(uint8_t minute);
    bool (*isHalfPast)(uint8_t minute);
    
    // Current mapping data arrays
    const WordMapping* baseWords;
    const WordMapping* hourWords;
    const WordMapping* minuteWords;
    const WordMapping* connectorWords;
    const uint8_t* minuteDotLEDs;
    
    uint8_t baseWordsCount;
    uint8_t hourWordsCount;
    uint8_t minuteWordsCount;
    uint8_t connectorWordsCount;
    uint8_t minuteDotCount;
    
    // Mapping loading functions
    void load45GermanMapping();
    void load110GermanMapping();
    void loadCustomMapping(const char* mappingId);
    
    // Helper functions
    void setMappingData(const char* name, const char* id, const char* description, uint16_t ledCount);
    void setMappingArrays(const WordMapping* base, uint8_t baseCount,
                         const WordMapping* hours, uint8_t hourCount,
                         const WordMapping* minutes, uint8_t minuteCount,
                         const WordMapping* connectors, uint8_t connectorCount,
                         const uint8_t* dots, uint8_t dotCount);
    void setMappingFunctions(bool (*showBase)(),
                           uint8_t (*hourIndex)(uint8_t, uint8_t),
                           int8_t (*minuteIndex)(uint8_t),
                           int8_t (*connectorIndex)(uint8_t),
                           uint8_t (*minuteDots)(uint8_t),
                           bool (*halfPast)(uint8_t));
};

#endif // LED_MAPPING_MANAGER_H
