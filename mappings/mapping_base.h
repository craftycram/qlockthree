#ifndef MAPPING_BASE_H
#define MAPPING_BASE_H

#include <stdint.h>

// Word mapping structure - used by all mapping files
struct WordMapping {
    const char* word;
    uint8_t start_led;
    uint8_t length;
    bool active;
};

#endif // MAPPING_BASE_H
