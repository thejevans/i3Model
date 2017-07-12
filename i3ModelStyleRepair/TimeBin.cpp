#include "TimeBin.h"

#include <StandardCplusplus.h>
#include <vector>
#include "LED.h"

void TimeBin::addLEDs (vector<LED> in_LEDs) {
  for(int i = 0; i < in_LEDs.size(); i++) {
    TimeBin::leds.push_back(in_LEDs[i]);
  }
}
