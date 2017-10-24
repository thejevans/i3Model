#include "Directory.h"
#include "Arduino.h"
#include "SD.h"
#include "Parser.h"

Directory::Directory(String inName) {
    name = inName;
    autoplayDir = "";
    for(int i = 0; i <= 512; i++) {
        fileTypes[i] = 0;
        filenames[i] = "";
        descriptiveFileNames[i] = "";
    }
    numFiles = 0;
    numEvents = 0;
    hasConfig = false;
    hasEvents = false;
    autoplay = false;
    crawl();
}

Directory::Directory() {
    name = "/";
    autoplayDir = "";
    for(int i = 0; i < 512; i++) {
        fileTypes[i] = 0;
        filenames[i] = "";
        descriptiveFileNames[i] = "";
    }
    numFiles = 0;
    numEvents = 0;
    hasConfig = false;
    hasEvents = false;
    autoplay = false;
    crawl();
}

void Directory::crawl() {
    String k = "";
    File dir = SD.open(name);
    dir.rewindDirectory();
    File entry = dir.openNextFile();
    hasConfig = false;
    hasEvents = false;

    while (entry) { // While there is a next file in the directory, check for file type
        k = entry.name();
        if ((!k.startsWith("_")) && (k != "TRASHE~1") && (k != "SPOTLI~1") && (k != "FSEVEN~1") && (k != "TEMPOR~1")) {
            // All files that start with underlines are hidden or "deleted"
            if (k == "FOLDER.TXT") { // If entry is a folder.txt file, parse the file
                hasConfig = true;
            }
            else if (entry.isDirectory()) { // If entry is a directory, flag it as such
                fileType[i] = 1;
                fileNames[i] = k;
                numFiles++;
                Serial.println(k);
            }
            else if (k.substring(k.indexOf('.')) == ".I3R") { //If the file extension is .I3R, mark as event file
                fileType[i] = 2;
                fileNames[i] = k;
                numFiles++;
                numEvents++;
                Serial.println(k);
                hasEvents = true;
            }
        }

        // Close the file and open the next one
        entry.close();
        entry = dir.openNextFile();
    }

    // Close the file and directory
    dir.close();
    entry.close();

    // If the directory has a folder.txt, parse it
    if (hasConfig) { Parser.config(this); }

    return;
}
