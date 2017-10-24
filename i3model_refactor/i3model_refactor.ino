#include <Adafruit_GFX.h>     // Core graphics library
#include <SPI.h>              // this is needed for display
#include <Adafruit_ILI9341.h> // Display library
#include <Wire.h>             // this is needed for FT6206
#include <Adafruit_FT6206.h>  // Touch library
#include <SD.h>               // SD card library
#include <FastLED.h>          // FastLED library
#include "Display.h"
#include "Buffer.h"

#ifdef __AVR__
#include <avr/power.h>
#endif

// SPI pins
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10

// Set maximum brightness
#define MAX_BRIGHTNESS 100

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN 12

// We have 80 strings x 60 doms/string = 4800 total doms NeoPixels are attached to the Arduino.
#define NUM_PIXELS 4800

// Set pin for SD card
#define CHIP_SELECT 4

// Maximum buttons on the screen
#define MAX_BUTTONS 20

// Maximum number of characters per line in event files
#define MAX_LINE_LENGTH 255

// Maximum number of files per page of the file menu
#define FILES_PER_PAGE 8

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_FT6206 ts = Adafruit_FT6206();

Display globalDisplay = Display<MAX_BUTTONS, MAX_LINE_LENGTH, FILES_PER_PAGE>(&tft, &ts);

// Array of pixels
CRGB pixels[NUM_PIXELS];

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
    clearPixels(&pixels);

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
    globalDisplay.toHomeScreen();

    // Parse the root directory
    globalDisplay.openDir("/");
}

void loop () {
    if (globalDisplay.playTrigger) { playEvent(&globalDisplay, &pixels); }

    TS_Point p = boop(&globalDisplay, &ts);
    int button = globalDisplay.buttonPressed(p);

    switch (button) {
        case 0: //None
        break;

        case 1: //Replay
        globalDisplay.playerResetEvent();
        //Continue to case 2 (Play)

        case 2: //Play
        globalDisplay.playerPlay();
        break;

        case 3: //Pause
        globalDisplay.playerPause();
        break;

        case 4: //Files
        globalDisplay.toFileMenu();
        break;

        case 5: //Stop
        globalDisplay.toHomeScreen();
        break;

        case 6: //Play Previous
        globalDisplay.playerPrevious();
        break;

        case 7: //Play Next
        globalDisplay.playerNext();
        break;

        case 8: //Back (Files)
        if (globalDisplay.fileMenuPage == 1) { globalDisplay.toHomeScreen(); }
        else { globalDisplay.fileMenuBack(); }
        break;

        case 9: //Next (Files)
        globalDisplay.fileMenuNext();
        break;

        case 10: //Play Mode (Files)
        globalDisplay.fileMenuToggleMode();
        break;

        default: //Item in File Menu
        int fileIndex = button - 11;
        if (globalDisplay.fileMenuItemSelected(fileIndex)) { globalDisplay.toPlayer(); }
        break;
    }
}

TS_Point boop (Display *workingDisplay, Adafruit_FT6206 *touchscreen) {
    TS_Point p;
    if (*touchscreen.touched()) {
        if (*workingDisplay.released) {
            p = *touchscreen.getPoint();

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

void wait (int timer) {
    for (int i = 0; i < timer; i++) { loop(); }
    return;
}

void clearPixels (CRGB *leds) {
    for (int i = 0; i < NUM_PIXELS; i++) {  *leds[i] = CRGB::Black; }
    FastLED.show();
}

void playEvent (Display *workingDisplay, CRGB *leds) {

    int ledList[NUM_PIXELS];
    int timeBinIndicies[NUM_PIXELS];
    CRGB color[NUM_PIXELS];

    Parser.eventFile<MAX_LINE_LENGTH>(&ledList, &timeBinIndicies, &color, *workingDisplay);

    // Display the event
    do {
        clearPixels(*leds);

        prevLed = 0;

        for (j = 0; j <= timeBin; j++) { // Loop through each time bin
            for (int k = prevLed; k < timeBinIndicies[j]; k++) { *leds[ledList[k]] = color[k]; }
            if (stopCheck(*workingDisplay)) { return; }
            FastLED.show();
            prevLed = timeBinIndicies[j];
        }

        Serial.println("END OF EVENT");
        *workingDisplay.playerEndOfEvent();

        // Check for pause and only wait for 10000 loops
        if (stopCheck(10000, *workingDisplay)) { return; }

    } while(*workingDisplay.replay); // If replay was selected, replay the event
    clearPixels(*leds);
    return;
}

bool stopCheck (Display *workingDisplay) {
    wait(3);
    while (*workingDisplay.paused) {
        wait(3);
        if (*workingDisplay.stopped) { return true; }
    }
    return false;
}

bool stopCheck (int iterations, Display *workingDisplay) {
    for (int i = 0; i < iterations; i++) {
        wait(3);
        if (*workingDisplay.stopped) { return true; }
        if (!*workingDisplay.paused) { break; }
    }
    return false;
}
