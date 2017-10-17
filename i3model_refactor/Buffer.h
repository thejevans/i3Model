#ifndef Buffer_h
#define Buffer_h

#include "Arduino.h"

struct Buffer {
  String readBuffer(File file) {
      byte i = 0;
      char temp = '\0';
      char val[255];
      memset(val, '\0', sizeof(val));
      while ((temp != '\n') && (temp >= 0)) { // Read entire line of file as a char array, ignoring spaces
          if ((i > 0)){ val[i - 1] = temp; }
          if (!file.available()) { break; }
          temp = file.read();
          i++;
          if (temp == ' ') { i--; }0
      }
      return String(val);
  }
};

#endif
