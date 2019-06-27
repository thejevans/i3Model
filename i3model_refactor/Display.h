#ifndef Display_h
#define Display_h

#include "Directory.h"
#include "Parser.h"
#include "Arduino.h"
#include <Adafruit_GFX.h>     // Core graphics library
#include <SPI.h>              // this is needed for display
#include <Adafruit_ILI9341.h> // Display library

template <int MAX_BUTTONS, int MAX_LINE_LENGTH, int FILES_PER_PAGE>

class Display {
private:
    class Player;
    class FileMenu;
    Adafruit_ILI9341 *tft
    int activeScreen; // 0 = Home screen, 1 = File menu, 2 = Player
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
    Player player;
    FileMenu fileMenu;
    bool released;
    Display(Adafruit_ILI9341 *tftIn);
    void toHomeScreen();
    void toFileMenu();
    void toPlayer();
    void openDir(String dir);
    int buttonPressed();
};

class Display::Player {
private:
public:
    bool replay;
    bool paused;
    bool stopped;
    bool trigger();
    void resetEvent();
    void play();
    void pause();
    void previous();
    void next();
    void endOfEvent();
};

class Display::FileMenu {
private:
    bool playFolders;
public:
    int page;
    void back();
    void next();
    void toggleMode();
    bool itemSelected(int index);
};

#endif
