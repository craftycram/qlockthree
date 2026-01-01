#ifndef WEB_ASSETS_H
#define WEB_ASSETS_H

#include <cstdint>

// CSS Files
extern const uint8_t common_css_start[] asm("_binary_data_css_common_css_start");
extern const uint8_t common_css_end[] asm("_binary_data_css_common_css_end");
extern const uint8_t dark_css_start[] asm("_binary_data_css_dark_css_start");
extern const uint8_t dark_css_end[] asm("_binary_data_css_dark_css_end");

// JavaScript Files
extern const uint8_t api_js_start[] asm("_binary_data_js_api_js_start");
extern const uint8_t api_js_end[] asm("_binary_data_js_api_js_end");
extern const uint8_t utils_js_start[] asm("_binary_data_js_utils_js_start");
extern const uint8_t utils_js_end[] asm("_binary_data_js_utils_js_end");

// HTML Pages
extern const uint8_t index_html_start[] asm("_binary_data_pages_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_data_pages_index_html_end");
extern const uint8_t time_html_start[] asm("_binary_data_pages_time_html_start");
extern const uint8_t time_html_end[] asm("_binary_data_pages_time_html_end");
extern const uint8_t led_html_start[] asm("_binary_data_pages_led_html_start");
extern const uint8_t led_html_end[] asm("_binary_data_pages_led_html_end");
extern const uint8_t led_mapping_html_start[] asm("_binary_data_pages_led_mapping_html_start");
extern const uint8_t led_mapping_html_end[] asm("_binary_data_pages_led_mapping_html_end");
extern const uint8_t dev_html_start[] asm("_binary_data_pages_dev_html_start");
extern const uint8_t dev_html_end[] asm("_binary_data_pages_dev_html_end");
extern const uint8_t birthdays_html_start[] asm("_binary_data_pages_birthdays_html_start");
extern const uint8_t birthdays_html_end[] asm("_binary_data_pages_birthdays_html_end");

// Helper macro to calculate asset size (subtract 1 for null terminator added by embed_txtfiles)
#define ASSET_SIZE(name) ((size_t)(name##_end - name##_start - 1))

#endif // WEB_ASSETS_H
