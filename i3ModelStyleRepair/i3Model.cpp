//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// IceCube LED model
// Made to use Arduino Due, 4800 NeoPixels, Adafruit 2.8" Capacative Touch Screen, and 3.3V -> 5V logic level converter
// Authors: John Evans, Elim Cheung
// Version: v1.1.0-20170612
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <Adafruit_GFX.h>     // Core graphics library
#include <SPI.h>              // this is needed for display
#include <Adafruit_ILI9341.h> // Display library
#include <Wire.h>             // this is needed for FT6206
#include <Adafruit_FT6206.h>  // Touch library
#include <SD.h>               // SD card library
#include <StandardCplusplus.h>
#include <vector>

#include <Arduino.h>
#include "LED.h"
#include "Event.h"
#include "timeBin.h"
#include "Directory.h"

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

// Set button sizes and locations
#define BUTTON_SIZE_Y 40
#define BUTTON_START_X 35
#define BUTTON_START_X2 125
#define BUTTON_START_Y 220
#define BUTTON_START_Y2 270

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

// Global variables for file management
String fileNames[255];
String descriptiveFileNames[255];
String workingDir = "/";
byte page = 1;
byte indexOfLastFile;
byte lastPage;
byte fileType[255];
byte filesOnScreen;
bool containsEvents = false;

// Global variables for event management
String prevEventFile;
String currentEventFile;
String nextEventFile;
byte indexOfCurrentEvent;
byte totalEventsInWorkingDir;
bool eventOver = false;
bool playingEvent = false;
bool replay = false;
bool playNext = false;
bool playPrev = false;
bool playFolders = true;

// Switch for pausing events or tests
bool paused = false;

// Switch for stopping events or tests
bool stopped = false;

// True if event or test is playing or paused. False if no event or test running
bool playing = false;

// False if continually pressing touchscreen
bool released = true;

// 0 = none; 1 = h_menu; 2 = file_menu
byte activeMenu = 0;

// Array of pixels
CRGB pixels[NUM_PIXELS];

//--------------------------------------------------------------------------------------
// Initial commands
//--------------------------------------------------------------------------------------
void i3MClass::setup () {
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
  parseDir();
}

//--------------------------------------------------------------------------------------
// Handles touch events
//--------------------------------------------------------------------------------------
TS_Point boop () {
  TS_Point p;
  if (ts.touched()) {
    if (released) { // The released flag prevents repreated touch events from being registered when the screen is long-pressed
      Serial.println("boop!");
      p = ts.getPoint();

      // Modifies X and Y coordinates to fit the orientation of the screen
      p.x = -(p.x - 240);
      p.y = -(p.y - 320);

      Serial.println(p.x);
      Serial.println(p.y);

      // Flip released flag now that the screen has been touched
      released = false;

      return p;
    }

  }
  else { // No touch event recorded, flip released flag
    released = true;
  }

  // If the touchscreen does not register a touch event or not released, set X, Y values to 0
  p.x = 0;
  p.y = 0;

  return p;
}

//--------------------------------------------------------------------------------------
// Runs loop() a set number of times. Much better than delay.
//--------------------------------------------------------------------------------------
void wait (int timer) {
  for (int i = 0; i < timer; i++) {
    loop();
  }
  return;
}

//--------------------------------------------------------------------------------------
// Main loop
//--------------------------------------------------------------------------------------
void loop () {
  TS_Point p = boop(); // Checks for touch event
  // Prints buttons on display and responds to touch events
  switch (activeMenu) {
    case 1: // Main menu
      if (p.y >= BUTTON_START_Y2 && p.y <= BUTTON_START_Y2 + BUTTON_SIZE_Y) {
        if (p.x >= BUTTON_START_X && p.x <= BUTTON_START_X2 - 10) { // Play or Replay or Pause or File button selected
          if (playing) {
            if (paused) {
              if ((playingEvent) && (eventOver)) { // If replay button is selected, replay
                replay = true;
                eventOver = false;
              }
              paused = false; // If paused, play
              makeHomeMenu(1);
              break;
            }
            paused = true; // Else if playing, pause
            makeHomeMenu(1);
            break;
          }
          makeHomeMenu(1); // Else file button selected
          page = 0; // page = 0 to clear screen
          workingDir = "/";
          makeFileMenu(0, true); // Switch to file menu
        }
        else if (p.x >= BUTTON_START_X2 && p.x <= BUTTON_START_X2 + BUTTON_SIZE_Y * 2) {
          if (playing) { // If paused, stop button selected
            if (!paused) {
              break;
            }
            makeHomeMenu(2); // Stop button selected
            paused = false;
            stopped = true;
            break;
          }
          makeHomeMenu(2); // Test button selected
          play(1, ""); // Run basic LED strip test
          // Clear screen above buttons
          tft.fillRect(0, 0, 240, 218, ILI9341_BLACK);
          makeHomeMenu(0);
        }
      }
      else if (p.y >= BUTTON_START_Y && p.y <= BUTTON_START_Y + BUTTON_SIZE_Y) {
        if (p.x >= BUTTON_START_X && p.x <= BUTTON_START_X2 - 10) { // Prev. button selected
          if ((playing) && (paused) && (playingEvent) && (indexOfCurrentEvent > 0)) { // If paused while playing an event and there are prev. events available, play prev. event
            playPrev = true;
            paused = false;
            stopped = true;
          }
        }
        else if (p.x >= BUTTON_START_X2 && p.x <= BUTTON_START_X2 + BUTTON_SIZE_Y * 2) { // Next button selected
          if ((playing) && (paused) && (playingEvent) && (indexOfCurrentEvent < totalEventsInWorkingDir)) { // If paused while playing an event and there are next events available, play next event
            playNext = true;
            paused = false;
            stopped = true;
          }
        }
      }
      break;

    case 2: // File menu
      if (p.y >= BUTTON_START_Y2 && p.y <= BUTTON_START_Y2 + BUTTON_SIZE_Y) {
        if (p.x >= BUTTON_START_X && p.x <= BUTTON_START_X2 - 10) { // Back button selected
          if (page >= 2) { // If not on first page, display previous page
            page -= 1; // Move back one page
            makeFileMenu(1, false);
          }
          else if (workingDir.lastIndexOf('/') == 0) { // Else if in root directory, return to main menu

            // File menu never called, so highlight button here
            tft.drawRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_WHITE);
            tft.drawRect(BUTTON_START_X2, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);

            tft.fillScreen(ILI9341_BLACK);
            makeHomeMenu(0);
          }
          else { // Else move to parent directory
            workingDir = workingDir.substring(0,workingDir.lastIndexOf('/'));
            workingDir = workingDir.substring(0,workingDir.lastIndexOf('/') + 1);
            Serial.println("moved to new working directory");
            Serial.println(workingDir);
            page = 1;
            makeFileMenu(0, true);
          }
        }
        else if (p.x >= BUTTON_START_X2 && p.x <= BUTTON_START_X2 + BUTTON_SIZE_Y * 2) { // Next button selected
          if (page < lastPage) { // If on last page, do nothing
            page += 1; // Move forward one page
            makeFileMenu(2, false);
          }
        }
        else if (p.x >= 5 && p.x <= 5 + BUTTON_SIZE_Y / 2 + 5) {
          if (playFolders) {
            playFolders = false;
          }
          else {
            playFolders = true;
          }
          makeFileMenu(3, false);
        }
      }
      else { // Checks to see if an event file is touched
        int xstart;
        int ystart;
        int j;
        for (int i = 0; i < filesOnScreen; i++) { // Check each file/folder button for selection

          // Set top-left position of each button
          xstart = BUTTON_START_X - 16 + (i % 2) * (BUTTON_START_X2 + 16 - BUTTON_START_X);
          ystart = 10 + (int(i / 2) % 6) * (BUTTON_SIZE_Y + 10);

          if ((p.y >= ystart && p.y <= ystart + BUTTON_SIZE_Y) && (p.x >= xstart && p.x <= xstart + BUTTON_SIZE_Y * 2 + 16)) { // If this button is selected check filetype
            j = i + (page - 1) * FILES_PER_PAGE;
            switch (fileType[j]) {
              case 1: // If directory, move to that directory
                workingDir += fileNames[j] + "/";
                page = 0;
                Serial.println("moved to new working directory");
                Serial.println(workingDir);
                makeFileMenu(0, true);
                break;

              case 2: // If event, play that event
                play(2, fileNames[j]);
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
                break;
            }
          }
        }
      }
      break;
  }
}

//--------------------------------------------------------------------------------------
// Parse directory
//--------------------------------------------------------------------------------------
bool parseDir () {
  File dir = SD.open(workingDir);

  // Returns to the beginning of the working directory
  dir.rewindDirectory();

  File entry = dir.openNextFile();
  int i = 0;
  String k = "";
  bool hasEvents = false;
  bool hasFolderText = false;

  indexOfLastFile = 0;
  lastPage = 0;
  totalEventsInWorkingDir = -1;

  for (int j = 0; j < 255; j++) {
    descriptiveFileNames[j] = "";
  }

  while (entry) { // While there is a next file in the directory, check for file type
    fileNames[i] = "";
    k = entry.name();
    if ((!k.startsWith("_")) && (k != "TRASHE~1") && (k != "SPOTLI~1") && (k != "FSEVEN~1") && (k != "TEMPOR~1")) { // All files that start with underlines are hidden or "deleted"
      if (k == "FOLDER.TXT") { // If entry is a folder.txt file, parse the file
        hasFolderText = true;
      }
      else if (entry.isDirectory()) { // If entry is a directory, flag it as such
        fileType[i] = 1;
        fileNames[i] = k;
        i++;
        indexOfLastFile++;
        Serial.println(k);
      }
      else if (k.substring(k.indexOf('.')) == ".I3R") { //If the file extension is .I3R, mark as event file
        fileType[i] = 2;
        fileNames[i] = k;
        i++;
        indexOfLastFile++;
        totalEventsInWorkingDir++;
        Serial.println(k);
        hasEvents = true;
      }
    }

    // Close the file and open the next one
    entry.close();
    entry = dir.openNextFile();
  }

  // Update number of pages based on number of files
  lastPage = (indexOfLastFile - (indexOfLastFile % FILES_PER_PAGE)) / FILES_PER_PAGE;
  if (indexOfLastFile % FILES_PER_PAGE > 0) {
    lastPage++;
  }

  // Close the file and directory
  dir.close();
  entry.close();

  // If the directory has a folder.txt, parse it
  if (hasFolderText) {
    Serial.println("parsing folder.txt");
    parseDirText();
  }

  Serial.print("hasEvents: ");
  Serial.println(hasEvents);

  // If folder playing is turned on, play the first file in the folder
  if ((playFolders) && (hasEvents) && (workingDir != "/")) {
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
    return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------
// Parse folder.txt
//--------------------------------------------------------------------------------------
bool parseDirText () {
  bool hasEvents = false;
  bool hasEventsProperty = false;
  bool mapsProperty = false;
  bool nextMap = false;
  bool autoplayProperty = false;

  byte i = 0;

  char temp = '\0';
  char val[255];

  int pos = 0;

  File file = SD.open(workingDir + "FOLDER.TXT"); // Open folder.txt in working directory

  while (file.available()) {

    // Reset variables
    i = 0;
    temp = '\0';

    // Reset array
    memset(val, '\0', sizeof(val));
    while ((temp != '\n') && (temp >= 0)) { // Read entire line of file as a char array, ignoring spaces
      if ((i > 0)){
        val[i - 1] = temp;
      }
      if (!file.available()) {
        break;
      }
      temp = file.read();
      i++;
      if (temp == ' ') {
        i--;
      }
    }

    if (nextMap) { // If the previous line was a filename, this line is the map
      if (String(val) != "") {
        descriptiveFileNames[pos] = String(val);
        nextMap = false;
      }
    }

    else if (autoplayProperty) { // If the previous line was the property for autoplay, this line is the value
      if (String(val) != "") {
        autoplayProperty = false;
        workingDir = workingDir + String(val);
        parseDir();
      }
    }

    else if (hasEventsProperty) { // If the previous line was the property for having events, this line is the value
      if (String(val) != "") {
        hasEventsProperty = false;
        if (String(val) == "true") {
          hasEvents = true;
        }
      }
    }

    else if (mapsProperty) { // If the previous line was the property for maps, all following lines are maps
      if (String(val) != "") {
        for (int j = 0; j < indexOfLastFile; j++) {
          if (String(val) == fileNames[j].substring(0,min(fileNames[j].length(), 6))) {
            pos = j;
            nextMap = true;
            break;
          }
        }
      }
    }

    else if (String(val) == "autoplay:" && workingDir == "/") { // Is this line the autoplay: property?
      autoplayProperty = true;
    }

    else if (String(val) == "contains events:") { // Is this line the contains events: property?
      hasEventsProperty = true;
    }

    else if (String(val) == "maps:") { // Is this line the maps: property?
      mapsProperty = true;
    }
  }

  // Close the event file
  file.close();
  return hasEvents;
}

//--------------------------------------------------------------------------------------
// Plays an event or test: type = (1 = ledTest(), 2 = displayEvents()), arg = passed to displayEvents()
//--------------------------------------------------------------------------------------
void play (int type, String arg) {
  // if something is already playing, exit play()
  while (playing) { return; }

  // Set flags
  paused = false;
  playing = true;

  // Return to main menu and clear screen
  tft.fillScreen(ILI9341_BLACK);
  makeHomeMenu(0);

  // Set cursor position and font size
  tft.setCursor(10, 10);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);

  switch (type) {
    case 1: // ledTest() case
      ledTest();
      break;

    case 2: // displayEvents() case
      playingEvent = true;
      eventOver = false;
      replay = false;
      displayEvents(arg);
      break;
  }

  // No longer playing
  playing = false;

  // Clear all LEDs
  clearPixels();

  return;
}

//--------------------------------------------------------------------------------------
// Checks for pause and stop: timer = (< 0 = no timer, > 0 = number of loops)
//--------------------------------------------------------------------------------------
bool stopCheck (int timer) {
  // Run loop to check for touch event
  loop();

  while (paused) { // If paused, wait until unpaused or until timer runs out
    loop();
    if (timer > 0) {
      timer--;
    }
    else if (timer == 0) {
      break;
    }
  }
  if (stopped) { // If stopped, clear pixels, clear screen
    stopped = false;
    playing = false;
    playingEvent = false;

    return true;
  }
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  return false;
}

//--------------------------------------------------------------------------------------
// Tests each string with all 3 solid colors and a gradient to make sure that all leds are working and to help with mapping
//--------------------------------------------------------------------------------------
void ledTest () {
  // Text Display
  tft.println("Running LED Test 1");
  tft.setCursor(10, 10+8);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);

  // Initialize base values for cursor position
  int ybase = 10;
  int xbase;

  for (int i = 0; i < NUM_PIXELS / 60; i++) {
    // Check for pause
    if (stopCheck(-1)) { return; }

    // Move cursor to next line and begin text display
    tft.setCursor(10, ybase + 8 * (i + 1));
    tft.print("String ");
    tft.print(i);
    if (i < 10) {
      xbase = 10 + 8 * 6;
    }
    else { // If diplaying 2 digits, move to the right one character
      xbase = 10 + 9 * 6;
    }

    // Sets string to red
    tft.print(": RED ");
    xbase = xbase + 6 * 6;
    setStringColor(i, MAX_BRIGHTNESS, 0, 0);

    // Check for pause
    if (stopCheck(-1)) { return; }

    // Sets string to green
    tft.setCursor(xbase, ybase + 8 * (i + 1));
    tft.print("GREEN ");
    xbase = xbase + 6 * 6;
    setStringColor(i, 0, MAX_BRIGHTNESS, 0);

    // Check for pause
    if (stopCheck(-1)) { return; }

    // Sets string to blue
    tft.setCursor(xbase, ybase + 8 * (i + 1));
    tft.print("BLUE ");
    xbase = xbase + 5 * 6;
    setStringColor(i, 0, 0, MAX_BRIGHTNESS);

    // Check for pause
    if (stopCheck(-1)) { return; }

    // Displays a gradient from red to blue
    tft.setCursor(xbase, ybase + 8 * (i + 1));
    tft.print("GRAD");
    setStringGrad(i, MAX_BRIGHTNESS, 0, 0, 0, 0, MAX_BRIGHTNESS);

    // Delay to see gradient
    wait(1000);

    // Check for pause
    if (stopCheck(-1)) { return; }

    // Clear string
    setStringColor(i, 0, 0, 0);

    if (i % 25 == 24) { // If output fills screen, clear screen and set cursor to top again
      tft.fillRect(10, 18, 230, 200, ILI9341_BLACK);
      ybase = ybase - 200;
    }
  }

  // Print "Done!" at the end
  tft.setCursor(10, ybase + 8 * (NUM_PIXELS / 60) + 8);
  tft.print("Done!");
}

//--------------------------------------------------------------------------------------
// Display a selected event file: filename = name of event file in working directory to be displayed
//--------------------------------------------------------------------------------------
void displayEvents (String filename) {
  // Initialize variables
  char temp;
  char val[MAX_LINE_LENGTH];

  byte headerIndex = 0;
  byte r;
  byte g;
  byte b;
  byte pos = 0;
  byte i = 0;

  int timeBin = 0;
  int led;
  int j = 0;
  int prevLed;
  int numLeds = 0;
  int ledList[NUM_PIXELS];
  int timeBinIndicies[NUM_PIXELS];

  CRGB color[NUM_PIXELS];

  bool header = false;

  // Open event file to be displayed
  File file = SD.open(workingDir + filename);

  // Reset arrays (needed for all events after first)
  memset(ledList, 0, sizeof(ledList));
  memset(color, CRGB(0,0,0), sizeof(color));
  memset(timeBinIndicies, 0, sizeof(timeBinIndicies));

  // Reset global variables
  indexOfCurrentEvent = 0;
  nextEventFile = "";
  prevEventFile = "";
  currentEventFile = "";
  playingEvent = true;

  // Get values for global variables
  for(int k = 0; k < indexOfLastFile; k++) {
    if (fileType[k] == 2) {
      if ((currentEventFile != "") && (nextEventFile == "")){
        nextEventFile = fileNames[k];
        break;
      }
      else if (fileNames[k] == filename) {
        indexOfCurrentEvent = j;
        currentEventFile = fileNames[k];
      }
      else if (indexOfCurrentEvent == 0) {
        prevEventFile = fileNames[k];
      }
      j++;
    }
  }

  // Initial text display
  if (descriptiveFileNames[indexOfCurrentEvent] != "") {
    tft.print(descriptiveFileNames[indexOfCurrentEvent]);
  }
  else {
    tft.print(filename);
  }

  tft.setCursor(10, 10+8);

  while ((file.available()) && (timeBin < NUM_PIXELS)) {

    // Check for pause
    if (stopCheck(-1)) { return; }

    // Reset variables
    i = 0;
    temp = '\0';

    // Reset array
    memset(val, '\0', sizeof(val));

    while ((temp != '\n') && (temp >= 0)) { // Read entire line of file as a char array, ignoring spaces
      if ((i > 0)){
        val[i - 1] = temp;
      }
      if (!file.available()) {
        break;
      }
      temp = file.read();
      i++;
      if (temp == ' ') {
        i--;
      }
    }

    if (header) { // If the event file has a header, print the header
      switch (headerIndex) {
        case 0:
          tft.setCursor(10, 10+8*2);
          tft.print("Date: ");
          tft.print(val);
          headerIndex++;
          break;
        case 1:
          tft.setCursor(10, 10+8*3);
          tft.print("ID: ");
          tft.print(val);
          headerIndex++;
          break;
        case 2:
          tft.setCursor(10, 10+8*4);
          tft.print("Energy: ");
          tft.print(val);
          tft.print(" TeV");
          headerIndex++;
          break;
        case 3:
          tft.setCursor(10, 10+8*5);
          tft.print("Zenith: ");
          tft.print(val);
          tft.print(" degrees");
          headerIndex++;
          break;
        case 4:
          tft.setCursor(10, 10+8*6);
          tft.print("PID: ");
          if (val[0] == '1') {
            tft.print("Track");
          }
          else if (val[0] == '0') {
            tft.print("Cascade");
          }
          else {
            tft.print("Undetermined");
          }
          headerIndex = 0;
          header = false;
          break;
      }
    }

    else if (val[0] == 'q') { // If the file starts with 'q', it has a header
      header = true;
    }

    else if (val[0] == 'n') { // If the first index of val is equal to the end time bin escape character, mark end time bin position
      Serial.println("END OF TIME BIN");

      // Set index for end of current time bin
      timeBinIndicies[timeBin] = numLeds;

      // Increment time bin number
      timeBin++;

      // Reset position
      pos = 0;
    }

    else if (val[0] == 'x') { // If the file has an 'x', then it has more than one event (and is in old format). only play the first event
      break;
    }

    else if (isDigit(val[0])){ // If the line is parsable as a number, then it is led information
      switch (pos) {
        case 0: // Parse LED index and add index to array
          led = atoi(val);
          ledList[numLeds] = led;
          Serial.print(led);
          Serial.print(" ");
          pos++;
          break;

        case 1: // Parse LED red value
          r = atoi(val);
          Serial.print(r);
          Serial.print(" ");
          pos++;
          break;

        case 2: // Parse LED green value
          g = atoi(val);
          Serial.print(g);
          Serial.print(" ");
          pos++;
          break;

        case 3: // Parse LED blue value and add LED color to array
          b = atoi(val);
          Serial.println(b);

          // Make sure LEDs are not going over the max allowed brightness
          int ir = min(r, MAX_BRIGHTNESS);
          int ig = min(g, MAX_BRIGHTNESS);
          int ib = min(b, MAX_BRIGHTNESS);

          color[numLeds] = CRGB(ir,ig,ib);
          pos = 0;
          numLeds++;
          break;
      }
    }
  }

  // Close the event file
  file.close();

  // Display the event
  do {
    clearPixels();

    // Reset previous LED value
    prevLed = 0;

    for (j = 0; j <= timeBin; j++) { // Loop through each time bin
      for (int k = prevLed; k < timeBinIndicies[j]; k++) { // Loop through each LED in each time bin
        // Set LED
        pixels[ledList[k]] = color[k];
      }
      // Check for pause
      if (stopCheck(-1)) { return; }

      //Display LEDs
      FastLED.show();

      // Set new starting position for inner for loop
      prevLed = timeBinIndicies[j];
    }

    paused = true;
    eventOver = true;

    makeHomeMenu(0);

    Serial.println("END OF EVENT");

    // Check for pause and only wait for 10000 loops
    if (stopCheck(10000)) { return; }

    // If on last event, next event = first event in directory
    if (indexOfCurrentEvent == indexOfLastFile - 1) {
      nextEventFile = fileNames[0];
    }

    // If no button pressed, play next event
    playNext = true;

  } while(replay); // If replay was selected, replay the event
  return;
}

//--------------------------------------------------------------------------------------
// Displays a solid color on a given string: stringNum = string number (0 - 79), r,g,b = color values
//--------------------------------------------------------------------------------------
void setStringColor (int stringNum, byte r, byte g, byte b) {
  // sets start at beginning of string
  int startPos = stringNum * 60;

  for (int i = 0; i < 60; i++) { // Loop through string and set each LED to color
    pixels[i+startPos] = CRGB(r,g,b);
  }

  //Display LEDs
  FastLED.show();

  Serial.print("string ");
  Serial.print(stringNum);
  Serial.println(": solid");
}

//--------------------------------------------------------------------------------------
// Displays a gradient on a given string: stringNum = string number (0 - 79), ir,ig,ib = initial color values, fr,fg,fb = final color values
//--------------------------------------------------------------------------------------
void setStringGrad (int stringNum, byte ir, byte ig, byte ib, byte fr, byte fg, byte fb) {
  // sets start at beginning of string
  int startPos = stringNum * 60;

  // Initialize r,g,b with starting values
  byte r = ir;
  byte g = ig;
  byte b = ib;

  for(int i = 0; i < 60; i++) { // Loop through string and set each LED to color
    // Set LED
    pixels[i+startPos] = CRGB(r,g,b);

    // steps from initial red to final red
    r = min(MAX_BRIGHTNESS, r + (int)(((float)fr - (float)ir) / 60.0));

    // steps from initial green to final green
    g = min(MAX_BRIGHTNESS, g + (int)(((float)fg - (float)ig) / 60.0));

    // steps from initial blue to final blue
    b = min(MAX_BRIGHTNESS, b + (int)(((float)fb - (float)ib) / 60.0));
  }

  // Display LEDs
  FastLED.show();

  return;
}

//--------------------------------------------------------------------------------------
// Clears all LEDs
//--------------------------------------------------------------------------------------
void clearPixels () {
  for (int i = 0; i < NUM_PIXELS; i++) { // Loop through all LEDs and set to black
    pixels[i] = CRGB::Black;
  }

  // Display LEDs
  FastLED.show();
}

//--------------------------------------------------------------------------------------
// Make home menu. selection = (0 = no selection, 1 = top left, 2 = top right, 3 = bottom left, 4 = bottom right)
//--------------------------------------------------------------------------------------
void makeHomeMenu (int selection) {
  activeMenu = 1;
  switch (selection) {
    case 1: // highlight top left box
      tft.drawRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_WHITE);
      tft.drawRect(BUTTON_START_X2, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      tft.drawRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      tft.drawRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      makeHomeMenu(0);
      break;

    case 2: // highlight top right box
      tft.drawRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      tft.drawRect(BUTTON_START_X2, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_WHITE);
      tft.drawRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      tft.drawRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      makeHomeMenu(0);
      break;

    case 3: // highlight bottom left box
      tft.drawRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      tft.drawRect(BUTTON_START_X2, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      tft.drawRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_WHITE);
      tft.drawRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      makeHomeMenu(0);
      break;

    case 4: // highlight bottom right box
      tft.drawRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      tft.drawRect(BUTTON_START_X2, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      tft.drawRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      tft.drawRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_WHITE);
      makeHomeMenu(0);
      break;

    default:
      // Clear boxes
      tft.fillRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 5, BUTTON_SIZE_Y * 2 + 10, ILI9341_BLACK);

      // Set text presets
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(2);

      if (paused) {

        // Make stop button
        tft.fillRect(BUTTON_START_X2, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_RED);
        tft.setCursor(BUTTON_START_X2 + 33, BUTTON_START_Y2 + 13);
        tft.println("X");
        if (playingEvent) {
          if (eventOver) {
            // Make replay button
            tft.fillRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_GREEN);
            tft.setCursor(BUTTON_START_X + 4, BUTTON_START_Y2 + 13);
            tft.println("REPLAY");
          }

          else {
            // Make play button
            tft.fillRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_GREEN);
            tft.setCursor(BUTTON_START_X + 35, BUTTON_START_Y2 + 13);
            tft.println(">");
          }

          // Make previous button
          if (indexOfCurrentEvent > 0) {
            tft.fillRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLUE);
            tft.setCursor(BUTTON_START_X + 16, BUTTON_START_Y + 1);
            tft.println("PREV");
            tft.setCursor(BUTTON_START_X + 4, BUTTON_START_Y + 24);
            tft.setTextSize(1);
            if (descriptiveFileNames[indexOfCurrentEvent-1] != "") {
              tft.setCursor(BUTTON_START_X + 2, BUTTON_START_Y + 24);
              tft.println(descriptiveFileNames[indexOfCurrentEvent-1].substring(0,min(13,descriptiveFileNames[indexOfCurrentEvent-1].length())));
            }
            else {
              tft.println(prevEventFile);
            }
            tft.setTextSize(2);
          }

          // Make next button
          if (indexOfCurrentEvent < totalEventsInWorkingDir) {
            tft.fillRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLUE);
            tft.setCursor(BUTTON_START_X2 + 16, BUTTON_START_Y + 1);
            tft.println("NEXT");
            tft.setCursor(BUTTON_START_X2 + 4, BUTTON_START_Y + 24);
            tft.setTextSize(1);

            // If the file has a descriptive file name, display that instead
            if (descriptiveFileNames[indexOfCurrentEvent+1] != "") {
              tft.setCursor(BUTTON_START_X2 + 2, BUTTON_START_Y + 24);
              tft.println(descriptiveFileNames[indexOfCurrentEvent+1].substring(0,min(13,descriptiveFileNames[indexOfCurrentEvent+1].length())));
            }
            else {
              tft.println(nextEventFile);
            }
            tft.setTextSize(2);
          }
        }
        else {
          // Make play button
          tft.fillRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_GREEN);
          tft.setCursor(BUTTON_START_X + 35, BUTTON_START_Y2 + 13);
          tft.println(">");
        }
      }

      else if (playing) {
        // Make pause button
        tft.fillRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_YELLOW);
        tft.setCursor(BUTTON_START_X + 29, BUTTON_START_Y2 + 13);
        tft.println("||");
      }

      else {
        // Make files button
        tft.fillRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_YELLOW);
        tft.setCursor(BUTTON_START_X + 10, BUTTON_START_Y2 + 13);
        tft.println("FILES");

        // Make test button
        tft.fillRect(BUTTON_START_X2, BUTTON_START_Y2, BUTTON_SIZE_Y * 2/3, BUTTON_SIZE_Y, ILI9341_RED);
        tft.fillRect(BUTTON_START_X2 + BUTTON_SIZE_Y * 2/3 , BUTTON_START_Y2, BUTTON_SIZE_Y * 2/3 + 2, BUTTON_SIZE_Y, ILI9341_GREEN);
        tft.fillRect(BUTTON_START_X2 + BUTTON_SIZE_Y * 4/3 , BUTTON_START_Y2, BUTTON_SIZE_Y * 2/3, BUTTON_SIZE_Y, ILI9341_BLUE);
        tft.setCursor(BUTTON_START_X2 + 16, BUTTON_START_Y2 + 13);
        tft.println("TEST");
      }
      break;
  }
  return;
}

//--------------------------------------------------------------------------------------
// Make file menu. selection = (0 = no selection, 1 = back, 2 = next), changedDir = passed to displayFiles()
//--------------------------------------------------------------------------------------
void makeFileMenu (int selection, bool changedDir) {
  activeMenu = 2;
  switch (selection) {
    case 1: // Back button selected
      if (page >= 1) {
        tft.drawRect(5, BUTTON_START_Y2, BUTTON_SIZE_Y / 2 + 5, BUTTON_SIZE_Y, ILI9341_BLACK);
        tft.drawRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_WHITE);
        tft.drawRect(BUTTON_START_X2, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
        makeFileMenu(0, changedDir);
      }
      break;

    case 2: // Next button selected
      if (page <= lastPage) {
        tft.drawRect(5, BUTTON_START_Y2, BUTTON_SIZE_Y / 2 + 5, BUTTON_SIZE_Y, ILI9341_BLACK);
        tft.drawRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
        tft.drawRect(BUTTON_START_X2, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_WHITE);
        makeFileMenu(0, changedDir);
      }
      break;
    case 3: // Play type button selected
      tft.drawRect(5, BUTTON_START_Y2, BUTTON_SIZE_Y / 2 + 5, BUTTON_SIZE_Y, ILI9341_WHITE);
      tft.drawRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      tft.drawRect(BUTTON_START_X2, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      makeFileMenu(0, changedDir);
      break;

    default:
      if (page < 1) { // If page is < 1, clear screen
        tft.fillScreen(ILI9341_BLACK);
        page = 1;
      }

      tft.fillRect(0, 0, 240, 270, ILI9341_BLACK);

      if ((displayFiles(changedDir))&& (playFolders)) { return; }

      // Make play type button
      tft.setCursor(10 + 3, BUTTON_START_Y2 + 13);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(2);
      if (playFolders) {
        tft.fillRect(5, BUTTON_START_Y2, BUTTON_SIZE_Y / 2 + 5, BUTTON_SIZE_Y, ILI9341_BLUE);
        tft.println("D");
      }
      else {
        tft.fillRect(5, BUTTON_START_Y2, BUTTON_SIZE_Y / 2 + 5, BUTTON_SIZE_Y, ILI9341_GREEN);
        tft.println("E");
      }

      // Make back button
      tft.fillRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_YELLOW);
      tft.setCursor(BUTTON_START_X + 16, BUTTON_START_Y2 + 13);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(2);
      tft.println("BACK");

      if (page < lastPage) { // If not on last page, make next button
        tft.fillRect(BUTTON_START_X2, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_YELLOW);
        tft.setCursor(BUTTON_START_X2 + 16, BUTTON_START_Y2 + 13);
        tft.setTextColor(ILI9341_BLACK);
        tft.setTextSize(2);
        tft.println("MORE");
      }

      else { // Clear next button in case there were multiple pages
        tft.fillRect(BUTTON_START_X2, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      }

      // Clear selection
      tft.drawRect(BUTTON_START_X, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      tft.drawRect(BUTTON_START_X2, BUTTON_START_Y2, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      tft.drawRect(5, BUTTON_START_Y2, BUTTON_SIZE_Y / 2 + 5, BUTTON_SIZE_Y, ILI9341_BLACK);
      break;
  }
}

//--------------------------------------------------------------------------------------
// Displays file and directory buttons: changedDir = True if the working directory has changed
//--------------------------------------------------------------------------------------
bool displayFiles (bool changedDir) {
  // If the working directory has been changed, parse the new directory
  if (changedDir) {
    tft.setCursor(10, 10);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.println("Loading Files...");

    if ((parseDir()) && (playFolders)) { return true; }

    tft.fillRect(10, 10, 10+6*16, 10+8, ILI9341_BLACK);
  }

  // Initialize variables
  int xstart;
  int ystart;
  int j;
  String filename;

  // Reset global variables
  filesOnScreen = 0;

  for (int i = FILES_PER_PAGE * (page - 1); i < FILES_PER_PAGE * page; i++) { // Loop through files in the current page
    if (i + 1 > indexOfLastFile) {
      // no more files
      break;
    }

    // Get file button top-left position on screen
    j = i - (page - 1) * FILES_PER_PAGE;
    xstart = BUTTON_START_X - 16 + (j % 2) * (BUTTON_START_X2 + 16 - BUTTON_START_X);
    ystart = 10 + (int(j / 2) % 6) * (BUTTON_SIZE_Y + 10);

    switch (fileType[i]) {
      case 1: // If a directory, make a blue button
        tft.fillRect(xstart, ystart, BUTTON_SIZE_Y * 2 + 16, BUTTON_SIZE_Y, ILI9341_BLUE);
        filename = fileNames[i];
        break;

      case 2: // If an event file, make a green button and drop the '.I3R'
        tft.fillRect(xstart, ystart, BUTTON_SIZE_Y * 2 + 16, BUTTON_SIZE_Y, ILI9341_GREEN);
        filename = fileNames[i].substring(0, fileNames[i].indexOf('.'));
        break;
    }

    // Print the filename or descriptive file name on the button
    tft.setCursor(xstart + 1, ystart + 13);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(2);

    // This formatting can handle descriptive file names up to 45 characters long, anything more will be trimmed.
    if (descriptiveFileNames[i] != "") {
      filename = descriptiveFileNames[i];
      if (filename.length() > 8) {
        tft.setTextSize(1);
        if (filename.length() > 15) {
          if (filename.length() > 30) {
            tft.setCursor(xstart + 3, ystart + 7);
            tft.println(filename.substring(0,15));
            tft.setCursor(xstart + 3, ystart + 16);
            tft.println(filename.substring(15,30));
            tft.setCursor(xstart + 3, ystart + 25);
            tft.println(filename.substring(30,filename.length()));
          }
          else {
            tft.setCursor(xstart + 3, ystart + 12);
            tft.println(filename.substring(0,15));
            tft.setCursor(xstart + 3, ystart + 21);
            tft.println(filename.substring(15,min(filename.length(), 45)));
          }
        }
        else {
          tft.setCursor(xstart + 3, ystart + 16);
          tft.println(filename);
        }
      }
      else {
        tft.println(filename);
      }
    }
    else {
      tft.println(filename);
    }

    // Count file buttons on the screen
    filesOnScreen++;
  }

  // Print the page number at the bottom of the screen
  tft.setCursor(180, 240);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("page ");
  tft.print(page);
  tft.setCursor(10, 10);

  return false;
}

//--------------------------------------------------------------------------------------
// Pull a file from the Serial port
//--------------------------------------------------------------------------------------
void pullFile(String filename) {
  if (SD.exists(filename)) {
    SD.remove(filename);
  }

  File myFile = SD.open(filename, FILE_WRITE);
  delay(100);

  String input;
  char ch;
  String led;
  String r;
  String g;
  String b;
  int pos = 0;
  bool done = false;

  while (!done){
    nextPulse();
    if (Serial.available() > 0) {
      char ch = Serial.read();
      switch (ch) {
        case 'e': // end of sending
          myFile.close();
          done = true;
          break;

        case 'x': //start a new event
          myFile.println('x');
          nextPulse();
          break;

        case 'n': //end of this time bin
          myFile.println('n');
          nextPulse();
          break;

        case '(': //indicater of pulse info
          pos = 0 ;
          input = "";
          break;

        case ',':
        case ')':
          switch (pos) {
            case 0:
              //led = input.toInt();
              led = input;
              //myFile.write(led);
              myFile.println(led);
              break;

            case 1:
              //r = input.toInt();
              r = input;
              //myFile.write(r);
              myFile.println(r);
              break;

            case 2:
              //g = input.toInt();
              g = input;
              //myFile.write(g);
              myFile.println(g);
              break;

            case 3:
              //b = input.toInt();
              b = input;
              //myFile.write(b);
              myFile.println(b);
              break;
          }
          pos += 1;
          input = "";
          if (ch == ')') {
            nextPulse();
          }
          break;

        default:
          if (isDigit(ch)) {
            input += ch;
          }
          break;
      }
    }
  }
}

//--------------------------------------------------------------------------------------
// signal python: next pulse
//--------------------------------------------------------------------------------------
void nextPulse() {
  Serial.write("n");
  delay(2);
  Serial.flush();
}
