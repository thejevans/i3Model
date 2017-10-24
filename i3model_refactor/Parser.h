#ifndef Parser_h
#define Parser_h

#include "Arduino.h"
#include "Display.h"
#include "Directory.h"
#include <SD.h>

class Parser {
    String readBuffer(char *val, File *file);
public:
    void eventFile(int *ledList, int *timeBinIndicies, CRGB *color, Display *workingDisplay);
    void config(Directory *workingDirectory);
};

#endif
