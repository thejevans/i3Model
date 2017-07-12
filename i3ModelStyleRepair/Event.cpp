#include "Event.h"

#include <StandardCplusplus.h>
#include <vector>
#include "TimeBin.h"

void Event::addTimeBins(vector<TimeBin> in_TimeBins) {
  for(int i = 0; i < in_TimeBins.size(); i++) {
    Event::timeBins.push_back(in_TimeBins[i]);
  }
}
