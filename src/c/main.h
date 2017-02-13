#include <pebble.h>
#pragma once


#define KEY_HOURS_MINUTES_SEPARATOR 4
#define KEY_DATE_FORMAT 5
#define KEY_BLUETOOTH_ALERT 7

  #define BLUETOOTH_ALERT_DISABLED 0  
  #define BLUETOOTH_ALERT_SILENT 1
  #define BLUETOOTH_ALERT_WEAK 2
  #define BLUETOOTH_ALERT_NORMAL 3
  #define BLUETOOTH_ALERT_STRONG 4
  #define BLUETOOTH_ALERT_DOUBLE 5

#define KEY_LANGUAGE 10
  
// bluetooth vibe patterns
const VibePattern VIBE_PATTERN_WEAK = {
	.durations = (uint32_t []) {100},
	.num_segments = 1
};

const VibePattern VIBE_PATTERN_NORMAL = {
	.durations = (uint32_t []) {300},
	.num_segments = 1
};

const VibePattern VIBE_PATTERN_STRONG = {
	.durations = (uint32_t []) {500},
	.num_segments = 1
};

const VibePattern VIBE_PATTERN_DOUBLE = {
	.durations = (uint32_t []) {500,100,500},
	.num_segments = 3
};


//converts string to uppercase (even multibyte one)
// s - pointer to the string
// limit (optionl) - limit in characters at which to cut the string

uint8_t utf8_str_to_upper(char* s, uint8_t limit) {
  
    uint8_t char_no = 0;
    uint8_t* p;

    for (p = (uint8_t*)s; *p; ++p) {

        // (<128) ascii character
        // U+00000000 – U+0000007F: 0xxxxxxx
        if (*p < 0b10000000) {
            if (*p >= 0x61 && *p <= 0x7A) {
                *p = *p - 0x20; // a~z -> A~Z
            }

        // (<192) unexpected continuation byte
        } else if (*p < 0b11000000) {

        // (<224) 2 byte sequence
        // U+00000080 – U+000007FF: 110xxxxx 10xxxxxx
        } else if (*p < 0b11100000) {
            uint16_t code = ((uint16_t)(p[0] & 0b00011111) << 6) | (p[1] & 0b00111111);
            if (
                (code >= 0x00E0 && code <= 0x00F6) || // à~ö -> À~Ö
                (code >= 0x00F8 && code <= 0x00FE)    // ø~þ -> Ø~Þ
            ) {
                code -= 0x0020;
                p[0] = 0b11000000 | ((code >> 6) & 0b00011111);
                p[1] = 0b10000000 | ( code       & 0b00111111);
            }
            ++p;

        // (<240) 3 byte sequence
        // U+00000800 – U+0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx
        } else if (*p < 0b11110000) {
            p += 2;

        // (<248) 4 byte sequence
        // U+00010000 – U+001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        } else if (*p < 0b11111000) {
            p += 3;

        // (<252) 5 byte sequence
        // U+00200000 – U+03FFFFFF: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        } else if (*p < 0b11111100) {
            p += 4;

        // (<254) 6 byte sequence
        // U+04000000 – U+7FFFFFFF: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        } else if (*p < 0b11111110) {
            p += 5;
        }
      
        if (limit) { // if limit paramerer is passed
            char_no++;  // increment character count
          
            if (char_no == limit) {       // if we reached the limit
               *(p + 1) = 0;              // terminate the string
               return p-(uint8_t*)s + 1;  // return byte position
               break;
            }
            
        }
      
    }
  
    return p-(uint8_t*)s + 1; // return byte position
}