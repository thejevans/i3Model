#include "Directory.h"
#include "Arduino.h"
#include "SD.h"
#include "Buffer.h"

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
    hasFolderText = false;
    hasEvents = false;
    autoplay = false;
    Directory::parse();
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
    hasFolderText = false;
    hasEvents = false;
    autoplay = false;
    Directory::parse();
}
void Directory::parse() {
    String k = "";
    File dir = SD.open(name);
    dir.rewindDirectory();
    File entry = dir.openNextFile();
    hasFolderText = false;
    hasEvents = false;

    while (entry) { // While there is a next file in the directory, check for file type
        k = entry.name();
        if ((!k.startsWith("_")) && (k != "TRASHE~1") && (k != "SPOTLI~1") && (k != "FSEVEN~1") && (k != "TEMPOR~1")) {
            // All files that start with underlines are hidden or "deleted"
            if (k == "FOLDER.TXT") { // If entry is a folder.txt file, parse the file
                hasFolderText = true;
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

    // Update number of pages based on number of files
    lastPage = (numFiles - (numFiles % FILES_PER_PAGE)) / FILES_PER_PAGE;
    if (numFiles % FILES_PER_PAGE > 0) {
        lastPage++;
    }

    // Close the file and directory
    dir.close();
    entry.close();

    // If the directory has a folder.txt, parse it
    if (hasFolderText) { Directory::parseConfig(); }

    return;
}
void Directory::parseConfig() {
    bool mapsProperty = false;
    bool nextMap = false;
    bool autoplayProperty = false;

    int pos = 0;

    File file = SD.open(name + "FOLDER.TXT"); // Open folder.txt in working directory

    while (file.available()) {
        String val = "";
        while (val == "") { val = Buffer.readBuffer(file); }

        if (nextMap) { // If the previous line was a filename, this line is the map
            descriptiveFileNames[pos] = val;
            nextMap = false;
        }

        else if (autoplayProperty) { // If the previous line was the property for autoplay, this line is the value
            autoplayProperty = false;
            autoplay = true;
            autoplayDir = val;
        }

        else if (mapsProperty) { // If the previous line was the property for maps or a map, this is line is a filename
            // Very inefficient. Replace with better searching method.
            for (int j = 0; j < numFiles; j++) {
                int length = min(fileNames[j].length(), 6);
                if (val == fileNames[j].substring(0,length)) {
                    pos = j;
                    nextMap = true;
                    break;
                }
            }
        }

        else if (val == "autoplay:" && name == "/") { autoplayProperty = true; }
        else if (val == "maps:") { mapsProperty = true; }
    }

    // Close the folder.txt file
    file.close();
    return;
}
