#include <Adafruit_GFX.h>     // Core graphics library
#include <SPI.h>              // this is needed for display
#include <Adafruit_ILI9341.h> // Display library
#include <Wire.h>             // this is needed for FT6206
#include <Adafruit_FT6206.h>  // Touch library
#include <SD.h>               // SD card library
#include <Adafruit_NeoPixel.h>// NeoPixel library

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
#define BOXSIZE 40
#define BOXSTART_X 35
#define BOXSTART_X2 125
#define BOXSTART_Y 220
#define BOXSTART_Y2 270

// Set pin for SD card
const int chipSelect = 4;

// Global variables for file management
int page = 1;
File root;
String fileNames[255, 2];
int lastFile = 0;

// Switch for pausing events or tests
bool paused = false;

// True if event or test is playing or paused. False if no event or test running
bool playing = false;

//0 = none; 1 = h_menu; 2 = file_menu
int activeMenu = 0;

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN            12

// we have 78 strings x 60 doms/string = 4680 total doms NeoPixels are attached to the Arduino.
#define NUMPIXELS      4680

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // wait for Arduino Serial Monitor

  tft.begin();
  ts.begin();
  pixels.begin();

  // Clear NeoPixels
  pixels.show();

  // Clear TFT
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  // End program if no SD card detected
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  // Save filenames and directory flags to fileNames for printing in the file menu
  root = SD.open("/");
  File entry = root.openNextFile();
  int i = 0;
  String k = "";
  while(entry) {
    fileNames[i,0] = "";
    fileNames[i,1] = "";
    k = entry.name();
    if (!entry.isDirectory()) {
      k = k + "\t\t\t\t\t\t\t";
      k = k + entry.size();
    }
    else {
      fileNames[i,1] = "1";
    }
    fileNames[i,0] = k;
    entry = root.openNextFile();
    i = i + 1;
    lastFile = lastFile + 1;
  }
  root.rewindDirectory();

  // Display initial home menu
  make_h_menu(0);
}

// Handles touch event
TS_Point boop() {
  TS_Point p;
  if (ts.touched()) {
    Serial.println("boop!");
    p = ts.getPoint();
    p.x = -(p.x - 240);
    p.y = -(p.y - 320);
    Serial.println(p.x);
    Serial.println(p.y);
    return p;
  }
  p.x = 0;
  p.y = 0;
  return p;
}

// Main loop
void loop() {
  TS_Point p = boop(); // Checks for touch event

  // Prints buttons on display and responds to touch events
  switch (activeMenu) {
    case 1: // Main menu
    if (p.x >= BOXSTART_X && p.x <= BOXSTART_X2 - 10) {
      if (p.y >= BOXSTART_Y && p.y <= BOXSTART_Y2 - 10) {
        make_h_menu(1); // Play button selected
        paused = false; // If paused, play
      }
      else if (p.y >= BOXSTART_Y2 && p.y <= BOXSTART_Y2 + BOXSIZE) {
        make_h_menu(3); // File button selected
        if(playing) { // If playing, do nothing
          break;
        }
        make_file_menu(0, 0); // Switch to file menu (page = 0 to clear screen)
      }
    }
    else if (p.x >= BOXSTART_X2 && p.x <= BOXSTART_X2 + BOXSIZE * 2) {
      if (p.y >= BOXSTART_Y && p.y <= BOXSTART_Y2 - 10) {
        make_h_menu(2); // Pause button selected
        paused = true; // If playing, pause
      }
      else if (p.y >= BOXSTART_Y2 && p.y <= BOXSTART_Y2 + BOXSIZE) {
        make_h_menu(4); // Test button selected
        led_test(); // Run basic LED strip test
      }
    }
    break;

    case 2: // File menu
    if (p.x >= BOXSTART_X && p.x <= BOXSTART_X2 - 10) {
      if (p.y >= BOXSTART_Y2 && p.y <= BOXSTART_Y2 + BOXSIZE) { // Back button selected
        if (page >= 2) { // If not on first page, display previous page
          page = page - 1; // Move back one page
          make_file_menu(page, 1);
        }
        else { // Else, return to main menu
          tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
          tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
          make_h_menu(0);
        }
      }
    }
    else if (p.x >= BOXSTART_X2 && p.x <= BOXSTART_X2 + BOXSIZE * 2) {
      if (p.y >= BOXSTART_Y2 && p.y <= BOXSTART_Y2 + BOXSIZE) { // Next button selected
        if (page <= lastFile/25) { //If on last page, do nothing
          page = page + 1; // Move forward one page
          make_file_menu(page, 2);
        }
      }
    }
    break;

    default:
    break;
  }
}

void led_test() {
  while(playing) {
    return;
  }
  paused = false;
  playing = true;
  tft.fillRect(0, 0, 240, 218, ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Running LED Test 1");
  tft.setCursor(10, 10+8);
  int base = 10;
  for(int i = 0; i < 78; i++) {
    loop();
    while (paused) {
      loop();
    }
    tft.setCursor(10, base + 8 * (i + 1));
    tft.print("String ");
    tft.print(i);
    tft.print(" RED ");
    setStringColor(i, 255, 0, 0);
    tft.print("GREEN ");
    setStringColor(i, 0, 255, 0);
    tft.print("BLUE");
    setStringColor(i, 0, 0, 255);
    setStringColor(i, 0, 0, 0);
    if(i % 25 == 24) {
      tft.fillRect(10, 18, 230, 200, ILI9341_BLACK);
      base = base - 200;
    }
  }
  tft.setCursor(10, base + 8 * 79);
  tft.print("Done!");
  playing = false;
}

void setStringColor(int stringNum, byte r, byte g, byte b) {
  int startPos = stringNum * 60;
  for(int i = startPos; i < startPos + 60; i++) {
    pixels.setPixelColor(i, r, g, b);
  }
  Serial.println("lights");
  pixels.show();
}

void make_h_menu(int selection) {
  switch (selection) {
    case 1:
    tft.drawRect(BOXSTART_X, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    break;

    case 2:
    tft.drawRect(BOXSTART_X, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
    tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    break;

    case 3:
    tft.drawRect(BOXSTART_X, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    break;

    case 4:
    tft.drawRect(BOXSTART_X, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
    break;

    default:
    //do the pause/play buttons
    tft.fillScreen(ILI9341_BLACK);
    tft.fillRect(BOXSTART_X, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_GREEN);
    tft.setCursor(BOXSTART_X + 13, BOXSTART_Y + 13);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.println("  >");
    tft.fillRect(BOXSTART_X2, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_RED);
    tft.setCursor(BOXSTART_X2 + 15, BOXSTART_Y + 13);
    tft.println(" ||");

    //do the button for submenu on files
    tft.fillRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_YELLOW);
    tft.setCursor(BOXSTART_X + 10, BOXSTART_Y2 + 13);
    tft.println("FILES");

    //do the test button
    tft.fillRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2/3, BOXSIZE, ILI9341_RED);
    tft.fillRect(BOXSTART_X2 + BOXSIZE * 2/3 , BOXSTART_Y2, BOXSIZE * 2/3 + 2, BOXSIZE, ILI9341_GREEN);
    tft.fillRect(BOXSTART_X2 + BOXSIZE * 4/3 , BOXSTART_Y2, BOXSIZE * 2/3, BOXSIZE, ILI9341_BLUE);
    tft.setCursor(BOXSTART_X2 + 16, BOXSTART_Y2 + 13);
    tft.println("TEST");

    tft.setCursor(30, 280);

    break;
  }

  activeMenu = 1;
  return;
}

void make_file_menu(int page, int selection) {
  activeMenu = 2;
  switch (selection) {
    case 1:
    if (page >= 1) {
      tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
      tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
      make_file_menu(page, 0);
    }
    break;

    case 2:
    if (page <= lastFile/25) {
      tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
      tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
      make_file_menu(page, 0);
    }
    break;

    default:
    if (page < 1) {
      tft.fillScreen(ILI9341_BLACK);
      page = 1;
    }
    tft.fillRect(0, 0, 240, 270, ILI9341_BLACK);
    tft.fillRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_YELLOW);
    tft.setCursor(BOXSTART_X + 16, BOXSTART_Y2 + 13);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.println("BACK");
    if (page <= lastFile/25) {
      tft.fillRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_YELLOW);
      tft.setCursor(BOXSTART_X2 + 16, BOXSTART_Y2 + 13);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(2);
      tft.println("MORE");
    }
    else {
      tft.fillRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    }
    tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    displayFiles(page);
    break;
  }
}

void displayFiles(int page) {
  uint8_t i = 25 * (page - 1);
  uint8_t j = 0;

  tft.setCursor(180, 240);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("page ");
  tft.print(page);

  tft.setCursor(10, 10);

  while (i <= 25 * page) {
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(10, 10 + 8 * j);
    j = j + 1;

    if (i >= lastFile) {
      // no more files
      break;
    }

    if (filenames[i,0].subString(filenames[i,0].indexOf('.')) == "i3rgb") {
      tft.setTextColor(ILI9341_GREEN);
    }

    else if (fileNames[i,1] == "1") {
      tft.setTextColor(ILI9341_BLUE);
    }

    tft.print(fileNames[i,0]);

    i = i + 1;
  }
}
