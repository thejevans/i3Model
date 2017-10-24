#include "Display.h"
#include "Parser.h"
#include "Directory.h"
#include <Adafruit_GFX.h>     // Core graphics library
#include <SPI.h>              // this is needed for display
#include <Adafruit_ILI9341.h> // Display library
#include <Wire.h>             // this is needed for FT6206
#include <Adafruit_FT6206.h>  // Touch library
#include <SD.h>               // SD card library


Display::Display () {
    released = true;
    playFolders = true;
    activeMenu = 0;
    for(int i = 0; i < MAX_BUTTONS; i++) {
        for(int j = 0; j < 5; j++) {
            buttonMap[i][j] = 0;
        }
    }
}

bool Display::openDir (Directory workingDir) {
    // If folder playing is turned on, play the first file in the folder
    if ((playMode) && (workingDir.hasEvents) && (workingDir.name != "/")) {
        playFolder(workingDir);
        return true;
    }
    return false;
}

void Display::playFolder () {
    makeHomeMenu(0);
    play(2, fileNames[0]);
    while ((playNext) || (playPrev)) { // If prev. or next selected, play that event
        clearPixels();
        paused = false;

        if (playNext){ // Play next event
            playNext = false;
            playPrev = false;
            makeHomeMenu(4);
            Serial.println("NEXT");
            play(2, nextEventFile);
        }

        else { // Play prev. event
            playNext = false;
            playPrev = false;
            makeHomeMenu(3);
            Serial.println("PREV");
            play(2, prevEventFile);
        }
    }
    clearPixels();
    // Clear screen above buttons
    tft.fillRect(0, 0, 240, 218, ILI9341_BLACK);
    makeHomeMenu(0);
    return;
}

// Update number of pages based on number of files
lastPage = (numFiles - (numFiles % FILES_PER_PAGE)) / FILES_PER_PAGE;
if (numFiles % FILES_PER_PAGE > 0) {
    lastPage++;
}

switch (fileType[fileIndex]) {
    case 1: // If directory, move to that directory
    workingDir += fileNames[fileIndex] + "/";
    page = 0;
    Serial.println("moved to new working directory");
    Serial.println(workingDir);
    makeFileMenu(true);
    break;

    case 2: // If event, play that event
    play(2, fileNames[fileIndex]);
    while ((playNext) || (playPrev)) { // If prev. or next selected, play that event
        clearPixels();
        paused = false;

        if (playNext){ // Play next event
            playNext = false;
            playPrev = false;
            makeHomeMenu();
            Serial.println("NEXT");
            play(2, nextEventFile);
        }

        else { // Play prev. event
            playNext = false;
            playPrev = false;
            makeHomeMenu();
            Serial.println("PREV");
            play(2, prevEventFile);
        }
    }
    clearPixels();
    // Clear screen above buttons
    tft.fillRect(0, 0, 240, 218, ILI9341_BLACK);
    makeHomeMenu();
    break;
