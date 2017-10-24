#ifndef Display_h
#define Display_h

#include "Directory.h"
#include "Parser.h"
#include "Arduino.h"
#include <Adafruit_GFX.h>     // Core graphics library
#include <SPI.h>              // this is needed for display
#include <Adafruit_ILI9341.h> // Display library
#include <Wire.h>             // this is needed for FT6206
#include <Adafruit_FT6206.h>  // Touch library

template <int MAX_BUTTONS, int MAX_LINE_LENGTH, int FILES_PER_PAGE>

class Display {
    int activeMenu; // 0 = Home menu, 1 = File menu
    int buttonMap[MAX_BUTTONS][5];
    // Buttons:
    //  1 = Replay
    //  2 = Play
    //  3 = Pause
    //  4 = Files
    //  5 = Stop
    //  6 = Play Previous
    //  7 = Play Next
    //  8 = Back (Files)
    //  9 = Next (Files)
    // 10 = Play Mode (Files)
    //11+ = Items in File Menu (Files)
public:
    bool released;
    bool playFolders;
    bool playTrigger;
    bool replay;
    bool paused;
    bool stopped;
    int fileMenuPage;
    Display(Adafruit_ILI9341 *tft, Adafruit_FT6206 *ts);
    toHomeScreen();
    toFileMenu();
    toPlayer();
    openDir(String dir);
    buttonPressed();
    playerResetEvent();
    playerPlay();
    playerPause();
    playerPrevious();
    playerNext();
    playerEndOfEvent();
    fileMenuBack();
    fileMenuNext();
    fileMenuToggleMode();
    fileMenuItemSelected(int index);
};

#endif
