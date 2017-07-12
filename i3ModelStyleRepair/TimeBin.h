#ifndef TimeBin_h
#define TimeBin_h

#include <StandardCplusplus.h>
#include <vector>
#include "LED.h"

struct TimeBin {
  TimeBin() {}
  void addLEDs(vector<LED>);
  void addLED(LED) {leds.push_back(in_LED);}
  vector<LED> leds;
};

#endif
