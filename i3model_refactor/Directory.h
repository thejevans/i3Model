#ifndef Directory_h
#define Directory_h

#include "Arduino.h"

struct Directory {
    String name;
    String autoplayDir;
    String filenames[512];
    String descriptiveFileNames[512];
    int fileTypes[512];
    int numFiles;
    int numEvents;
    bool hasConfig;
    bool hasEvents;
    bool autoplay;
    Directory(String inName);
    Directory();
    void crawl();
};

#endif
