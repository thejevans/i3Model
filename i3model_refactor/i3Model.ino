#include <Adafruit_GFX.h>     // Core graphics library
#include <SPI.h>              // this is needed for display
#include <Adafruit_ILI9341.h> // Display library
#include <Wire.h>             // this is needed for FT6206
#include <Adafruit_FT6206.h>  // Touch library
#include <SD.h>               // SD card library
#include <FastLED.h>          // FastLED library
#include "Directory.h"
#include "Display.h"

#ifdef __AVR__
#include <avr/power.h>
#endif

// SPI pins
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_FT6206 ts = Adafruit_FT6206();

// Set maximum brightness
#define MAX_BRIGHTNESS 100

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN 12

// We have 80 strings x 60 doms/string = 4800 total doms NeoPixels are attached to the Arduino.
#define NUM_PIXELS 4800

// Maximum number of characters per line in event files
#define MAX_LINE_LENGTH 20

// Set pin for SD card
#define CHIP_SELECT 4

// Maximum number of files per page of the file menu
#define FILES_PER_PAGE 8

// Maximum buttons on the screen
#define MAX_BUTTONS 20

Directory globalDirectory = Directory();
Display globalDisplay = Display<MAX_BUTTONS>();

void setup () {
  Serial.begin(250000); // Set baud rate to 250000
  while (!Serial) ; // wait for Arduino Serial Monitor

  delay(500); // needed to properly boot on first try

  // Set LED pin as output
  pinMode(PIN, OUTPUT);

  // Initialize touchscreen
  tft.begin();
  ts.begin();

  // Initialize FastLED
  FastLED.addLeds<NEOPIXEL, PIN>(pixels, NUM_PIXELS);

  // Clear NeoPixels
  clearPixels();

  // Clear TFT
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  // End program if no SD card detected
  if (!SD.begin(CHIP_SELECT)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  // Display initial home menu
  tft.fillScreen(ILI9341_BLACK);
  makeHomeMenu(0);

  // Parse the root directory
  openDir(globalDirectory);
}

TS_Point boop (Display *workingDisplay) {
    TS_Point p;
    if (ts.touched()) {
        if (*workingDisplay.released) {
            p = ts.getPoint();

            // Modifies X and Y coordinates to fit the orientation of the screen
            p.x = 240 - p.x;
            p.y = 320 - p.y;
            *workingDisplay.released = false;
            return p;
        }
    }
    else { *workingDisplay.released = true; }
    p.x = 0;
    p.y = 0;
    return p;
}

void loop () {
    Directory workingDirectory = globalDirectory;
    Display workingDisplay = globalDisplay;
    TS_Point p = boop(&workingDisplay);
    int button = buttonPressed(workingDisplay,p);

    switch (button) {
        case 0: //None
        break;

        case 1: //Replay
        replay = true;
        eventOver = false;
        //Continue to case 2 (Play)

        case 2: //Play
        paused = false;
        makeHomeMenu();
        break;

        case 3: //Pause
        paused = true;
        makeHomeMenu();
        break;

        case 4: //Files
        makeHomeMenu();
        page = 0; // page = 0 to clear screen
        workingDirectory = Directory();
        makeFileMenu(workingDirectory, true); // Switch to file menu
        break;

        case 5: //Stop
        makeHomeMenu();
        paused = false;
        stopped = true;
        break;

        case 6: //Play Previous
        playPrev = true;
        paused = false;
        stopped = true;
        break;

        case 7: //Play Next
        playNext = true;
        paused = false;
        stopped = true;
        break;

        case 8: //Back (Files)
        if (page >= 2) { // If not on first page, display previous page
            page -= 1; // Move back one page
            makeFileMenu(false);
        }
        else if (workingDir == "/") { // Else if in root directory, return to main menu
            tft.fillScreen(ILI9341_BLACK);
            makeHomeMenu();
        }
        else { // Else move to parent directory
            workingDir = workingDir.substring(0,workingDir.lastIndexOf('/'));
            workingDir = workingDir.substring(0,workingDir.lastIndexOf('/') + 1);
            Serial.println("moved to new working directory");
            Serial.println(workingDir);
            page = 1;
            makeFileMenu(true);
        }
        break;

        case 9: //Next (Files)
        page += 1;
        makeFileMenu(false);
        break;

        case 10: //Play Mode (Files)
        playFolders = !playFolders;
        makeFileMenu(false);
        break;

        default: //Item in File Menu
        int fileIndex = button - 11;
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
        }
        break;
    }
    globalDirectory = workingDirectory;
    globalDisplay = workingDisplay;
}

void wait (int timer) {
    for (int i = 0; i < timer; i++) {
        loop();
    }
    return;
}

void clearPixels () {
    for (int i = 0; i < NUM_PIXELS; i++) { // Loop through all LEDs and set to black
        pixels[i] = CRGB::Black;
    }

    // Display LEDs
    FastLED.show();
}

bool openDir (Directory workingDir) {
    // If folder playing is turned on, play the first file in the folder
    if ((playFolders) && (workingDir.hasEvents) && (workingDir.name != "/")) {
        playFolder(workingDir);
        return true;
    }
    return false;
}

void playFolder () {
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

void parseDirText () {
    bool mapsProperty = false;
    bool nextMap = false;
    bool autoplayProperty = false;

    int pos = 0;

    File file = SD.open(workingDir + "FOLDER.TXT"); // Open folder.txt in working directory

    while (file.available()) {
        String val = "";
        while (val == "") { val = readBuffer(file); }

        if (nextMap) { // If the previous line was a filename, this line is the map
            descriptiveFileNames[pos] = val;
            nextMap = false;
        }

        else if (autoplayProperty) { // If the previous line was the property for autoplay, this line is the value
            autoplayProperty = false;
            workingDir = workingDir + val;
            openDir();
        }

        else if (mapsProperty) { // If the previous line was the property for maps, all following lines are maps
            // Very inefficient. Replace with better searching method.
            for (int j = 0; j < indexOfLastFile; j++) {
                int length = min(fileNames[j].length(), 6);
                if (val == fileNames[j].substring(0,length)) {
                    pos = j;
                    nextMap = true;
                    break;
                }
            }
        }

        else if (val == "autoplay:" && workingDir == "/") { autoplayProperty = true; }
        else if (val == "maps:") { mapsProperty = true; }
    }

    // Close the event file
    file.close();
    return;
}

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
