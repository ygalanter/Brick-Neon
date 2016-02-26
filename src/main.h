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