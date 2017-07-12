#ifndef LED_h
#define LED_h

#include <FastLED.h>

struct LED {
  LED() : index(0), color(CRGB(0,0,0)) {}
  LED(int in_index, CRGB in_color) : index(in_index), color(in_color) {}
  CRGB color;
  int index;
};

#endif
